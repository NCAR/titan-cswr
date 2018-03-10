//***************************************************************************************
// This file contains c-callable wrapper functions representing a one to one mapping
// of the WDM driver capabilities. This is a VERY limited set of functions at only
// the lowest level. This will include the interrupt on and off functions, and a function
// that returns a useable pointer to the hardware.
//
// This file gets compiled and linked together with other files in this project to
// generate a Generic HIQ DLL Library that will be used for all future HIQ applications.
//
// Other c subroutines in this project will have access to the HIQ base addresses and
// the ability to read and write registers.
//
// At one time the philosophy was that only the WDM driver would actually read and
// write registers, but I changed my mind!!
//
// For now, these functions will use Jungo routines in place of a home made WDM driver
// because there is no such driver yet. In the future if and when we do our own WDM driver
// this will be the only routine that will need to be changed for this project. That is
// the underlying purpose of this wrapper file.
//
// List of callable functionalities:
//
//   1) Get base address of hiq card
//   2) Enable interrupts (including registering the user mode interrupt routine)
//   3) Disable interrupts
//
// List of other functions the driver provides but aren't callable:
//
//   1) Kernal mode interrupt routine (sets up the DMA transfer only)
//
//***************************************************************************************

//#include "Forcelib.h"
#include <windows.h>
#include <stdlib.h>

#include "windrvr.h"
#include "wdc_lib.h"		// req'd for WDC_IntEnable
#include "windrvr_int_thread.h"
#include "windrvr_events.h"
#include "pci_regs.h"
#include "bits.h"
#include "status_strings.h"

#ifndef __KERNEL__
#include <stdio.h>
#endif

#include	<math.h>    // for sqrt in phase correction algorithm

#include "hiq_hardware.h"
#include "hiq_lib.h"
#include "smfifo.h"
#include "hiq.h"			/* HIQ struct plus status reg defines */
#include "udpsock.h"		/* IQ record structure define */
#include "iqdata.h"		/* IQ record structure define */
#include "drvwrapper.h"	/* jungo prototypes for now */
#include "hiqop.h"
#include "piraqx.h"		/* For ctrlflags */
#include "mydata.h"		// for IS_DUAL_POL #ifdef
#include "hiqutil.h"
#include "angles.h"


// CONSOLE display
#define CONSOLE true

#define	VENDORID	0x10B5
#define	DEVICEID	0x9056

// If an error occurs, this string will be set to contain a relevant error message
CHAR HIQ_ErrorString[1024];

// Customized WinDriver Debug Monitor constants
WD_DEBUG_ADD dbgMsg;


/* KPTODO: Obsolete?
// internal data structures
typedef struct
{
	WD_INTERRUPT Int;
	HANDLE hThread;
	HIQ_INT_HANDLER funcIntHandler;
} HIQ_INT_INTERRUPT;

typedef struct
{
	DWORD index;
	BOOL  fIsMemory;
	BOOL  fActive;
} HIQ_ADDR_DESC;

typedef struct HIQ_STRUCT
{
	HANDLE hWD;
	HANDLE hEvent;
	HIQ_INT_INTERRUPT Int;
	WD_PCI_SLOT pciSlot;
	HIQ_ADDR_DESC addrDesc[HIQ_ITEMS];
	WD_CARD_REGISTER cardReg;
} HIQ_STRUCT;
*/ //KPTODO: Obsolete?

static HIQ_INT_DATA Hiq_Int_Data[INT_MAX_BOARDS];	// Use in User mode interrupt service routine called by KP_HIQ_IntAtDpc().

//***************************************************************************************
// Get a contiguous chunk of locked Kernel memory
//****************************************************************************************
WD_DMA *GetContiguousLockedMem(HIQ *hiq, int size)
{
	char	*self = "GetContiguousLockedMem:";
	int		dwStatus;
	WD_DMA	*pdma;
	//WD_DMA	**ppDma = NULL;
	//PVOID	*ppBuf = NULL;
	DWORD	dwOptions = DMA_KERNEL_BUFFER_ALLOC | DMA_FROM_DEVICE;	// KPTODO: is DMA_KERNEL_BUFFER_ALLOC needed/allowed?
	PWDC_DEVICE pDev = (PWDC_DEVICE)(hiq->hDev);	// KPTODO: why do they do this?  PWDC_DEVICE == WDC_DEVICE_HANDLE?

	pdma = (WD_DMA *)malloc(sizeof(WD_DMA));	// allocate the WD_DMA struct defining the buffer
	if(!pdma)	{printf("failed to allocate WD_DMA struct to describe contiguous kernel buffer\n");   return(NULL);}

	// first, get a Kernel buffer for the DMA_STUFF structure
	memset(pdma, 0, sizeof(WD_DMA));
	pdma->pUserAddr = NULL;	// output, for contiguous case.
	pdma->dwBytes = size;
	pdma->dwOptions = DMA_KERNEL_BUFFER_ALLOC | DMA_READ_FROM_DEVICE;   // huh????
	//pdma->hCard = ((HIQ_HANDLE)hiq->jungo)->cardReg.hCard;
	pdma->hCard = pDev->cardReg.hCard;
	//dwStatus = WD_DMALock(((HIQ_HANDLE)(hiq->jungo))->hWD,pdma);
	dwStatus = WD_DMALock(WD_Open(),pdma);	// This works, but is the handle correct?
	//dwStatus = WDC_DMAContigBufLock (hiq->hDev, ppBuf, dwOptions, size, ppDma );
	if(dwStatus)
	{
		printf("%s Failed locking down contiguous kernel buffer. Error 0x%1x - %s\n",
						self,dwStatus,Stat2Str(dwStatus));
		return(NULL);
	}
	return(pdma);
	//return(*ppDma);
}

//***************************************************************************************
// Lock down user buffer (so the operation system doesn't swap it)
// Works for buffers of all sizes
//****************************************************************************************
WD_DMA *LockUserMem(HIQ *hiq, void *buf, int size)
{
	char	*self = "LockUserMem:";
	int		dwPagesNeeded,extrapages;
	int		dwStatus;
	WD_DMA	*pdma = NULL;
	//WD_DMA	**ppDma = &pdma;
	PWDC_DEVICE pDev = (PWDC_DEVICE)(hiq->hDev);	// KPTODO: why do they do this?  PWDC_DEVICE == WDC_DEVICE_HANDLE?
	//PVOID	*pBuf = NULL;

	dwPagesNeeded = (size - 1) / 4096 + 2;
	extrapages = dwPagesNeeded - WD_DMA_PAGES;	// structure already has space for WD_DMA_PAGES (256) pages
	if(extrapages < 0){ extrapages = 0; }
	pdma = (WD_DMA *)calloc(sizeof(WD_DMA) + sizeof(WD_DMA_PAGE) * extrapages, 1);
	pdma->pUserAddr = buf;	// fifo as input, so this is a Scatter/Gather request. 
	pdma->dwBytes = size;
	pdma->dwOptions = DMA_FROM_DEVICE;
	if(extrapages > 0)	pdma->dwOptions |= DMA_LARGE_BUFFER;
	pdma->dwPages = (extrapages > 0) ? dwPagesNeeded : 0;
	//pdma->hCard = ((HIQ_HANDLE)hiq->jungo)->cardReg.hCard;
	pdma->hCard = pDev->cardReg.hCard;
	//dwStatus = WD_DMALock(((HIQ_HANDLE)(hiq->jungo))->hWD,pdma);	// prev implementation: works
	//dwStatus = WD_DMALock(hiq->hDev,pdma);	// invalid WinDriver handle!
	//dwStatus = WD_DMALock((HANDLE)(hiq->hDev),pdma);	// invalid WinDriver handle!
	//dwStatus = WD_DMALock((HANDLE)(pDev),pdma);	// invalid WinDriver handle!
	dwStatus = WD_DMALock(WD_Open(),pdma);	// This works, but is the handle correct?
	if(dwStatus)
	{
		printf("%s Failed locking down Scatter-Gather buffer. Error 0x%1x - %s\n",
						self,dwStatus,Stat2Str(dwStatus));
		return(NULL);
	}
	return(pdma);
	/*  Hig Level interface way of doing this?
	dwStatus = WDC_DMASGBufLock(hiq->hDev, &buf, DMA_FROM_DEVICE|DMA_ALLOW_CACHE, size, ppDma);
	if(dwStatus)
	{
		printf("%s Failed locking down contiguous kernel buffer. Error 0x%1x - %s\n",
						self,dwStatus,Stat2Str(dwStatus));
		return(NULL);
	}
	return(*ppDma);
	*/
}

//***********************************************************************************
// This routine 
//   1) Locks the FIFO memory so it can be used within an interrupt routine.
//   2) Allocates a locked memory buffer to hold the scatter/gather tables - an array of tables.
//   3) returns the address of the Jungo memory descriptor list
//   4) returns the address of a PLX formatted memory descriptor list (not initialized)
// FifoDma is a pointer to a pointer to the WD_DMA struct for the FIFO
// ListDma is a pointer to a pointer to the WD_DMA struct for the memory descriptor list pList
// pList is a pointer to a pointer to a PLX formatted memory descriptor list
//***********************************************************************************
void LockFIFO(HIQ *hiq, FIFO *fifo)
{
	//DWORD	dwStatus;
	int		dwPagesNeeded,align,pagesperrecord;

	// allocate the DMA_STUFF structure pointed to by hiq
	hiq->dma = malloc(sizeof(DMA_STUFF));
	if(!hiq->dma)	{printf("LockFIFO: Could not allocate DMA_STUFF structure\n"); exit(-1);}

	// allocate the listaddr array pointed to by DMA_STUFF structure
	((DMA_STUFF *)hiq->dma)->listaddr = (int *)malloc(sizeof(int) * fifo->record_num);
	if(!((DMA_STUFF *)hiq->dma)->listaddr)	{printf("LockFIFO: Could not allocate listaddr array\n"); exit(-1);}

	// allocate the FifoDma structure pointed to by the DMA_STUFF structure
	// Lock the fifo and get the pointer to it's WD_DMA struct for later use
	((DMA_STUFF *)hiq->dma)->FifoDma = LockUserMem(hiq, fifo, fifo->size);

	//printf("FYI: The fifo buffer was scattered into %d pages by the operating system\n",((DMA_STUFF *)hiq->dma)->FifoDma->dwPages);

	// allocate the ListDma structure pointed to by the DMA_STUFF structure
	// Also allocate the pList array pointed to by the DMA_STUFF structure
	// Now get a locked kernal buffer (contiguous) for the memory descriptor chain list
	pagesperrecord = (fifo->record_size - 1) / 4096 + 2;
	dwPagesNeeded =  pagesperrecord * fifo->record_num;
	((DMA_STUFF *)hiq->dma)->ListDma = GetContiguousLockedMem(hiq, dwPagesNeeded * sizeof(DMA_LIST) + 0x10);

	// point the pList element of DMA_STUFF to the contiguous memory that was just allocated
	align = 0x10 - (int)((DMA_STUFF *)hiq->dma)->ListDma->pUserAddr & 0xf;
	((DMA_STUFF *)hiq->dma)->pList = (DMA_LIST *)((int)((DMA_STUFF *)hiq->dma)->ListDma->pUserAddr + align);  // get byte-aligned address

	((DMA_STUFF *)hiq->dma)->ListDma->Page[0].pPhysicalAddr += align;	// use the aligned address later for the PLX chip
	((DMA_STUFF *)hiq->dma)->ListDma->pKernelAddr += align;				// point the Kernal address there, too
	((DMA_STUFF *)hiq->dma)->ListDma->pUserAddr = (PVOID *)((int)((DMA_STUFF *)hiq->dma)->ListDma->pUserAddr + align);				// as well as the user address
	//printf("The ListDma array was allocated successfully\n");

	printf("physical address of pList = ((DMA_STUFF *)hiq->dma)->ListDma->Page[0].pPhysicalAddr = %08X\n",((DMA_STUFF *)hiq->dma)->ListDma->Page[0].pPhysicalAddr);
	printf("dma->pList (userspace) = %08X\n",(unsigned long long)((DMA_STUFF *)hiq->dma)->pList);
	printf("FIFO physical address = dma->FifoDma->Page[0].pPhysicalAddr = %08X\n",((DMA_STUFF *)hiq->dma)->FifoDma->Page[0].pPhysicalAddr);
	printf("in LockFIFO, fifo->record_num = %d\n",fifo->record_num);
}


#define	LocalFifoPhysAddr	0x30		// Eric says so

//***********************************************************************************
// This routine 
//   1) Initializes the memory descriptor list for each record of the FIFO.
//***********************************************************************************
void DMAListsetup(HIQ *hiq, FIFO *fifo, PHIQ_DEV_CTX pCtx, int whichhiq)
{
	int		i,j,transferbytes;	// use this to calculate addresses
	unsigned int k;
	unsigned int start,end;
	int pagesperrecord,addraccum,bytes;
	unsigned int m;

	//printf("gates used to program DMA: %d\n",hiq->gates);

	// note: this in conjuction with the start point gets gate[0] in the right place. The gate0mode data has two spaces before it
	// because it is a series of differences. Later in the interrupt it will be expanded to the correct number of absolute values
	transferbytes = 4 * (2 * hiq->gates + 1) + 4 * (hiq->gate0mode ? 2 * hiq->pulsewidth - 2 : 0);  // Reads EOF, too. later, add in read for gate0mode
	// have to make sure DMAListsetup is called after gate0mode is set to reflect how data is actually being taken
	// if it's wrong, it will just generate an EOF

	pagesperrecord = (fifo->record_size - 1) / 4096 + 2;

	//printf("pagesperrecord = %d\n",pagesperrecord);
	//printf("fifo->record_num = %d\n",fifo->record_num);

	for(i=0; i<fifo->record_num; i++)		// for all records
	{
		start = fifo->fifobuf_off + i * fifo->record_size + (sizeof(IQRECORD) - 4) + (hiq->gate0mode ? 4 * 2 : 0);  // the -4 puts the first word on top of iqrec.gatezero
		// if gate0mode, then leave room at the begining for a total of pulsewidth time series points
		end = start + transferbytes;
		for (j=0; j<pagesperrecord && start < end; j++)		// for all fifo pages
		{
			// find the fifo page k where start lies
			for(addraccum=0,k=0; k<((DMA_STUFF *)hiq->dma)->FifoDma->dwPages && addraccum + ((DMA_STUFF *)hiq->dma)->FifoDma->Page[k].dwBytes <= start; k++)
				addraccum += ((DMA_STUFF *)hiq->dma)->FifoDma->Page[k].dwBytes;

			// addraccum <= start < addraccum + dwBytes    for page k

			if(addraccum + ((DMA_STUFF *)hiq->dma)->FifoDma->Page[k].dwBytes > end)   // > or >= work the same
				bytes = end - start;
			else
				bytes = addraccum + ((DMA_STUFF *)hiq->dma)->FifoDma->Page[k].dwBytes - start;

			m = i * pagesperrecord + j;
			((DMA_STUFF *)hiq->dma)->pList[m].dwPADR = ((DMA_STUFF *)hiq->dma)->FifoDma->Page[k].pPhysicalAddr + start - addraccum;
			((DMA_STUFF *)hiq->dma)->pList[m].dwLADR = LocalFifoPhysAddr;
			((DMA_STUFF *)hiq->dma)->pList[m].dwSIZ = bytes;
			((DMA_STUFF *)hiq->dma)->pList[m].dwDPR = 
				((DWORD) ((DMA_STUFF *)hiq->dma)->ListDma->Page[0].pPhysicalAddr + sizeof(DMA_LIST) * (i * pagesperrecord + (j + 1)))
				| BIT0 | BIT3 | (addraccum + ((DMA_STUFF *)hiq->dma)->FifoDma->Page[k].dwBytes >= end ? BIT1 : 0);

			start = addraccum + ((DMA_STUFF *)hiq->dma)->FifoDma->Page[k].dwBytes;
		}
		((DMA_STUFF *)hiq->dma)->listaddr[i] = ((DWORD) ((DMA_STUFF *)hiq->dma)->ListDma->Page[0].pPhysicalAddr + sizeof(DMA_LIST) * (i * pagesperrecord)) | BIT0;
		pCtx->Hiq_Kp_Int_Data[whichhiq].pDmaDescriptor[i] = ((DMA_STUFF *)hiq->dma)->listaddr[i];	// Save DMA description ptrs for Kernel Plugin.
	}
}   


#define BITMASK 0xFFFFC000
#define BITSHIFT ((double)((~BITMASK)+1))

/* 
Get the HiQ Interrupt data specific to the card handle, by searching for the card handle.
Return the index of the data structure.
*/
int getWhichhiqHiqIntData(DWORD desired_hCard)
{
	for(int i=0; i<INT_MAX_BOARDS; i++)
	{
		if(desired_hCard == Hiq_Int_Data[i].hCard)
		{
			return i;
		}
	}
	// Error data not found!
	printf("ERROR:HIQ_INT_DATA for this interrupt was not found!");
	return -1;
}

//#define INTTEST0			// Turns on INT LED during entire intterupt service routine
//#define INTTEST1			// Turns on INT LED during phase correction
//#define INTTEST2			// Turns on INT LED during discriminator computations
#define INTLEDNORMAL		// Toggles LED every integration period

/* KPTODO: how much of this to use?

//static int hitctr = 0;
//static int *olddiscptr, *curdiscptr;
//static long discptrdiff;


// Debug display for interrupt handler. See Jungo FAQ for WinDbg printing, e.g. KdPrint().
void HIQ_PCIINT_debug_card_display( HIQ_HANDLE hHIQ )
{
	// Visually display on console hex card slot number on each interrupt.
	if(CONSOLE) { printf("%x",hHIQ->pciSlot.dwSlot);fflush(stdout); }
	// Visually display on console hex card handle on each interrupt.
	if(CONSOLE) { printf("%x",hHIQ->cardReg.hCard);fflush(stdout); }
}
*/

static unsigned short hwcounter = 0;	// Simulates hardware counter in CSWR version
static unsigned long long hwpulsenum = 0;
static unsigned short old_hwcounter = 0;

// Computation that should not be in kernel interrupt handler for DMADONE condition.
void HIQ_DmaDoneCompute( WDC_DEVICE_HANDLE hDev, HIQ_INT_RESULT *pIntResult )
{
	char	*self = "HIQ_DmaDoneCompute";
	FIFO	*pFifo;
	HIQ		*pHiq;
	int		i,size;
	int		whichhiq;	// card index, is = whichhiq, or Jungo style - 1.
	int		sign,*ts;
	int		*iqptr;
	float	a,b,c,d,num,den;
	int		ledCounter;
	IQRECORD	*record;
	HIQOP	*hiqop;
	bool	haveLostSync = false;	// Initial sync hifreq burst location is unknown.
	int		v_gate_offset = 0;
	uint8	tempSecs = 0;				// temporary variable.
	bool	isInSync = true;
	PWDC_DEVICE	pDev = (PWDC_DEVICE)hDev;


	// Determine which card sourced the interrupt and use that card's data.
	whichhiq = (pDev->cardReg.hCard) - 1;
		
	if(pIntResult->dwCounter % 10000 == 0 && pIntResult->dwLost>0) 
	{
		printf("%s: whichhiq=%d, interrupts=%ld, lost=%ld\n", self, whichhiq, pIntResult->dwCounter, pIntResult->dwLost );	// Print Interrupt count.
	}

	// Determine which card sourced the interrupt and use that card's data.
	PHIQ_DEV_CTX pDevCtx = (PHIQ_DEV_CTX)WDC_GetDevContext(hDev);

	pFifo = Hiq_Int_Data[whichhiq].mHIQ;	// the fifo.
	pHiq  = Hiq_Int_Data[whichhiq].mH;	// the HIQ structure.
	v_gate_offset = Hiq_Int_Data[whichhiq].v_gate_offset;
	isInSync = Hiq_Int_Data[whichhiq].isInSync;	
	ledCounter = Hiq_Int_Data[whichhiq].ledCounter;	
#define CSWR
	// Init or Update pulse number.
	if(pHiq->pulsenum == 0)	// On the first time interrupt...
	{
#ifdef CSWR		
	// For CSWR the pulse numbers will start with 1, not at the epoch
		pHiq->pulsenum = 1;
		hwpulsenum = 0;
//		hwcounter = 1;
#else
		// Generate the starting pulse number.
		pHiq->pulsenum = 	getStartPulseNumber(Hiq_Int_Data[whichhiq].prt1);		
#endif
		getUtcTime(&Hiq_Int_Data[whichhiq].secs, &Hiq_Int_Data[whichhiq].nanosecs);	// Establish a single start time.
	}
	else
	{

#ifdef CSWR2	
		// This scheme was an attempt to sync two separate HiQ boxes.  Didn't work because
		// the hardware counter was operating in realtime and this code operates in XP time. (DPC)

		if((abs(hwcounter - old_hwcounter) > 0xFFF))	// We've wrapped
		{
			hwpulsenum++;
		}
		old_hwcounter = hwcounter;

		pHiq->pulsenum = hwcounter;
		pHiq->pulsenum = (hwpulsenum << 16) | (hwcounter);
#else

		pHiq->pulsenum += 1;	// Increment pulse number since epoch.
#endif					
	}	
	old_hwcounter = hwcounter;		// Save the old value
	// Increment the head to match the record where the data was just sent to
	fifo_increment_head(pFifo);
	
	// All the I's and Q's and the EOF has just been received in this record
	// Now fill in the other data for the record
	hiqop = (HIQOP *)fifo_get_header_address(pFifo);
	record = (IQRECORD *)((int)fifo_get_write_address(pFifo));

	// First, grab the current antenna data and put them in the IQRECORD structure.
	get_angles(record, Hiq_Int_Data[whichhiq].angle_source, &hwcounter);
	
	// Get seconds and nanoseconds
	record->secs = Hiq_Int_Data[whichhiq].secs;
	record->nanosecs = Hiq_Int_Data[whichhiq].nanosecs;

	// Another pulse occurred so increment time by the pulse time.
	if(Hiq_Int_Data[whichhiq].dual_prt)	// If dual PRT we need to know if the pulse is short or long, and add that time.
	{
		if(pHiq->pulsenum & 0x0000000000000001)	// Odd numbered int64 pulse, started with long interval.
		{
			record->nanosecs += (uint4)(Hiq_Int_Data[whichhiq].prt2*1.0e9);
		}
		else
		{
			record->nanosecs += (uint4)(Hiq_Int_Data[whichhiq].prt1*1.0e9);
		}
	}
	else	// If single PRT, then all pulses take the same time.
	{
		record->nanosecs += (uint4)(Hiq_Int_Data[whichhiq].prt1*1.0e9);
	}
	// Roll over nanosecs to seconds, but can not trust the PRT to be accurate so just reset to a new time.
	if(record->nanosecs > 1000000000)
	{
		getUtcTime(&tempSecs, &record->nanosecs);
		record->secs = (uint8)tempSecs;
	}

	// always set the size of a record for anything that might eventually be recorded or sent over a socket
	size = sizeof(int) * (2 * pHiq->gates + (pHiq->gate0mode ? 2 * pHiq->pulsewidth : 0)) + (sizeof(IQRECORD) - sizeof(int));
	fifo_set_rec_size(pFifo,size);

	record->pulsenum = pHiq->pulsenum;
	//      record->beamnum = pHiq->beamnum;
	record->configid = hiqop->configid;
	// note, this is right based on the definition.  
	// 3/17/2009 Had to add a one to address to get just past the data area and pick up EOF character (0's).
	record->eofstatus = 0xFFFFC000 & (&record->gatezero)[2 * pHiq->gates + (pHiq->gate0mode ? 2 * pHiq->pulsewidth : 0) + 1]; 
//	record->eofstatus = (&record->gatezero)[2 * pHiq->gates + (pHiq->gate0mode ? 2 * pHiq->pulsewidth : 0)]; 
//	record->eofstatus = 0xAAAAAAAA;
	record->pulsewidth = pHiq->pulsewidth | (pHiq->gate0mode ? 1 : 0);		// pulsewidth with bit0 indicating gate0mode
	record->gates = pHiq->gates;

	// prepare things for the next DMA transfer
	// clear out the EOF on the next record, so that even weird errors get caught!
	int	tail;
	fifo_clear(pFifo,&tail);	// Reset SW fifo ptrs.
	IQRECORD *trecord = (IQRECORD *)((int)fifo_get_address(pFifo,tail,1));
//	(&trecord->gatezero)[2 * pHiq->gates + (pHiq->gate0mode ? 2 * pHiq->pulsewidth : 0)] = 0xAAAAAAAA; // note, this is right based on the definition

	//#define PCIINT_PRINT_BLIP
#ifdef PCIINT_PRINT_BLIP
	// print a little blip in console, mark every 10th interrupt.
	if(whichhiq==0)
	{
		if(Hiq_Int_Data[0].intCnt%10 == 1){printf("%c",'1');}else{printf("%c",'.');}
	}
	if(whichhiq==1)
	{
		if(Hiq_Int_Data[1].intCnt%10 == 1){printf("%c",'2');}else{printf("%c",':');}
	}
#endif

	// Assume in sync unless detected below as not in sync.
	if(Hiq_Int_Data[whichhiq].intCnt < 1000)
	{
		isInSync = true;	
	}
	//------------------------------------------------------------------ 
	// TESTCASE: Test loss of AFC hi freq burst of data used for sync by 
	// setting flag monitored by Runhiq.exe.
	// Count on next line determines how many interrupts before restart condition.
	//#define FORCE_OUT_OF_SYNC 100000	// TEST: Forced OUT OF SYNC condition!
#ifdef	FORCE_OUT_OF_SYNC
	if(Hiq_Int_Data[whichhiq].intCnt>FORCE_OUT_OF_SYNC && isInSync)
#else
	if((record->eofstatus != 0x00000000) && isInSync)	// TODO: Don't understand this condition
#endif
		//------------------------------------------------------------------ 
		{			
			isInSync = false;	// enter only once.
			haveLostSync = true;

#define EOF_SEARCH	10
#ifdef EOF_SEARCH
			for(int eofindex = 0; eofindex < EOF_SEARCH; eofindex++)
			{
				printf("%d: 0x%08X\n", -eofindex+5, 
				(&trecord->gatezero)[2 * pHiq->gates + (pHiq->gate0mode ? 2 * pHiq->pulsewidth : 0) - eofindex+5]);
			}
			printf("\n");
#else
			//printf("*****************************************************\n");
			printf("* Loss of Sync Occurred: %8X on card %d of {1,2}*\n",record->eofstatus, hHIQ->cardReg.hCard);
			//printf("*****************************************************\n");
			//printf("%c",(record->eofstatus == 0xAAAAAAAA) ? '!' : '#');	// print a little blip if out of sync
#endif
			fflush(stdout);
			WDC_Err("%s: Card %d: Loss of Sync int error!",self,whichhiq);
			//HIQ_DebugMsg(hHIQ, "Loss of Sync int error!", D_ERROR, S_INT);
		}	

	// This is where the gate 0 converter goes!!!!!!!!!!!!!!!
	if(pHiq->gate0mode)
	{
		ts = &record->gatezero;
		iqptr = ts + 2;
		sign = 0;
		if(*iqptr & 0x80000000) sign = 2;     // if the first value is negative.
		*ts++ = (int)(((sign?-1: 1) * (int)(iqptr[0] & BITMASK) - 0x8000000) / BITSHIFT);
		*ts++ = (int)(((sign? 1:-1) * (int)(iqptr[1] & BITMASK) - 0x8000000) / BITSHIFT);
		for(i=2; i<2*pHiq->pulsewidth; i++)
		{
			if(((i+1)&2)^sign) 
			{
				*ts++ = (int)(( (int)(iqptr[i-2] & BITMASK) - (int)(iqptr[i] & BITMASK) - 0x8000000) / BITSHIFT);
			} else {
				*ts++ = (int)((-(int)(iqptr[i-2] & BITMASK) + (int)(iqptr[i] & BITMASK) - 0x8000000) / BITSHIFT);
			}
		}
	}
#ifdef INTTEST2
	turnLedOn(hDev);		// turn on the LED.
#endif
	// This is where the discriminator goes
	// this is supposed to be XMITWIDTH!

	a = (float)((&record->gatezero)[pHiq->pulsewidth - 2 + 0]);
	b = (float)((&record->gatezero)[pHiq->pulsewidth - 2 + 1]);
	c = (float)((&record->gatezero)[pHiq->pulsewidth - 2 + 2]);
	d = (float)((&record->gatezero)[pHiq->pulsewidth - 2 + 3]);

	num = a * d - b * c;
	den = b * (b - d) + c * (c - a);  // max = 2 * ADFULLSCALE^2.

	//#define DEBUGDISC0		// debug discriminator: For grabbing interrupts with debugger
#ifdef	DEBUGDISC0
	static int *olddiscptr, *curdiscptr;
	static long discptrdiff;
	curdiscptr = &(&record->gatezero)[pHiq->pulsewidth-2];
	discptrdiff = curdiscptr - olddiscptr;
	if(discptrdiff != 0x806)
	{
		int qq = 1;
	}
	olddiscptr = curdiscptr;
	printf("DISCPTR: cur 0x%p, diff 0x%p\n",&(&record->gatezero)[pHiq->pulsewidth-2] , discptrdiff);
#endif			
	// if den less than -30 dB, set at -30 dB (make discriminator read zero for weak signals).
	if(den < 2 * ADFULLSCALE * ADFULLSCALE / 1000.0)    den = (float)(2 * ADFULLSCALE * ADFULLSCALE / 1000.0);   

	// Exponential filter on numerator and denominator 


	// Set DELTA to a value between 0 and 1, say 0.9. The higher DELTA, the longer the time constant (longer integration time).
#define	DELTA	0.99		// 0.0 shuts off exp averaging	// Was .75 GRG 8/12/10

	pHiq->numdiscrim = num;
	pHiq->dendiscrim = den;
	pHiq->err = fabs(num/den) < 7.0 ? num/den: pHiq->avgerr;	// avgerr not changed if err too large
	pHiq->avgerr = (float)((1.0 - DELTA) * pHiq->err + DELTA * pHiq->avgerr);

	//#define DEBUGDISC1	// debug discriminator: For grabbing data with debugger
#ifdef DEBUGDISC1
	static int hitctr = 0;
	float	test = pHiq->numdiscrim/pHiq->dendiscrim;
	if(hitctr++ %pHiq->hits == 0)
		printf("DISC-DRVW: num %20.3f, den %20.3f, err %8.3f\n", num*1e-6, den*1e-6, den != 0? test: 0);
#endif

#ifdef INTTEST2
	turnLedOff(hDev);		// turn off the LED. 
#endif

	if(pHiq->gate0mode)	// Only correct phase in gate0mode is on (non-coherent xmitter).
	{
		//#define INTEGER_ARITHMETIC
#ifndef INTEGER_ARITHMETIC
		float	I,Q,I0,Q0;
		// This is where the phase correction goes!
		// This is the floating point version  
		// normalize and conjugate gate zero 
		iqptr = &record->gatezero + 2 * (pHiq->pulsewidth & ~1);
		I0 = (float)iqptr[0];
		Q0 = (float)iqptr[1];
		if(I0 == 0.0 && Q0 == 0.0)  pHiq->g0invmag = 1.0; 
		else pHiq->g0invmag = (float)(1.0 / sqrt(I0 * I0 + Q0 * Q0));
		I0 *=  pHiq->g0invmag;
		Q0 *= -pHiq->g0invmag;

#ifdef INTTEST1
		turnLedOn(hDev);		// turn on the LED.
#endif

		// Adjust the vertical channel gate offset correction with config file value.
		if(whichhiq == 0) 
		{
			v_gate_offset = 0;	// Only correct Vertical channel.
		}

		// correct all but the zeroeth of the gates with m and n 
		iqptr += 2;
		for(i=1; i<pHiq->gates - v_gate_offset; i++)	// Shift everything toward radar by v_gate_offset gates.
		{
			I = (float)iqptr[0 + v_gate_offset];
			Q = (float)iqptr[1 + v_gate_offset];
			*iqptr++ = (int)(I0 * I - Q0 * Q);
			*iqptr++ = (int)(Q0 * I + I0 * Q);
		}

		int *iqptre = iqptr-2*v_gate_offset;

		if(whichhiq > 0)
		{
			for(i = 0; i < 2*v_gate_offset; i++)
				*iqptr++ = *iqptre++;				// Repeat last v_gate_offset gates at end
		}

#ifdef INTTEST1
		turnLedOff(hDev);		// turn off the LED.
#endif

#ifdef INTLEDNORMAL
		if(ledCounter++ % hiqop->hits == 0)
		{
			toggleLed(hDev);
		}
#endif
#endif		// INTEGER_ARITHMETIC
#ifdef INTEGER_ARITHMETIC

		int	I,Q,I0,Q0;
		// This is where the phase correction goes!
		// This is the integer version  5us for 260 gates benchmark on Mitch's lab machine
		// normalize and conjugate gate zero.
		iqptr = &record->gatezero + 2 * (pHiq->pulsewidth & ~1);
		I0 = iqptr[0];
		Q0 = iqptr[1];
		if(I0 == 0.0 && Q0 == 0.0)  
			pHiq->g0invmag = 16384.0; 
		else
			pHiq->g0invmag = 16383.0 / sqrt((double)(I0 * I0) + (double)(Q0 * Q0));
		I0 = (int)((double)I0 *  pHiq->g0invmag);
		Q0 = (int)((double)Q0 * -pHiq->g0invmag);

		//			turnLedOn(hDev);		// turn on the LED.

		// correct all but the zeroeth of the gates with m and n.
		iqptr += 2;
		for(i=1; i<pHiq->gates; i++)
		{
			I = iqptr[0] / 16384;
			Q = iqptr[1] / 16384;
			*iqptr++ = I0 * I - Q0 * Q;
			*iqptr++ = Q0 * I + I0 * Q;
		}

		if(ledCounter++ % hiqop->hits == 0)
		{
			toggleLed(hDev);
		}
#endif
	}	// if(pHiq->gate0mode)

	// Rollover interrupt count.
	if(Hiq_Int_Data[whichhiq].intCnt == 0xEFFFFFFF ){	// Max positive value.
		Hiq_Int_Data[whichhiq].intCnt = 1;		// Skip 0 as it means first ever interrupt.
	} else {
		Hiq_Int_Data[whichhiq].intCnt++;
	}

	// Save updated data.
	Hiq_Int_Data[whichhiq].mH = pHiq;	// the HIQ structure.
	Hiq_Int_Data[whichhiq].secs = record->secs;
	Hiq_Int_Data[whichhiq].nanosecs = record->nanosecs;
	Hiq_Int_Data[whichhiq].isInSync = isInSync;
	Hiq_Int_Data[whichhiq].ledCounter = ledCounter;

	#ifdef INTTEST0
	turnLedOff(hDev);		// turn off the LED.
	#endif
}

/* KPTODO: add lated as needed. */
// Data for monitoring health of system.
int HIQ_getIntCnt( int whichhiq )    { return Hiq_Int_Data[whichhiq].intCnt; }
int HIQ_getIntErrCnt( int whichhiq ) { return Hiq_Int_Data[whichhiq].intErrCnt; }
bool HIQ_isInSync( int whichhiq )    { return Hiq_Int_Data[whichhiq].isInSync; }



// Return a ptr to the per card interrupt data structure. 
HIQ_INT_DATA *getHiq_Int_Data(int whichhiq)
{
	return &(Hiq_Int_Data[whichhiq]);
}


// Set the KP interrupt context.  Initialize the specific HiQ card's FIFO and HIQ data structures.	
void HIQ_setDeviceContext(int whichhiq, HIQ *hiq, FIFO *pFifo, PHIQ_DEV_CTX pCtx)
{
	WD_DMA *ListDma = ((DMA_STUFF *)hiq->dma)->ListDma;		// ptr to DMA structures.
	DMA_LIST *pList = NULL; //((DMA_STUFF *)hiq->dma)->pList;		// ptr to PLX descriptor list structure.

	pCtx->Hiq_Kp_Int_Data[whichhiq].waitingForHF = TRUE;	// this flag mirrors the HF enable bit.
	pCtx->Hiq_Kp_Int_Data[whichhiq].startedDMA = FALSE;		// this flag mirrors DMASTART. 
//	pCtx->Hiq_Kp_Int_Data[whichhiq].mHIQ = fifo;			// Pointer to FIFO struct.
//	pCtx->Hiq_Kp_Int_Data[whichhiq].fifoHead = pFifo->head;	// FIFO head.
//	pCtx->Hiq_Kp_Int_Data[whichhiq].fifoRecordNumber = pFifo->record_num;	// FIFO record number.

	pCtx->Hiq_Kp_Int_Data[whichhiq].pDmaDescriptorIndex = 1;	// Initialize index into DMA descriptor ptr list.

	pCtx->funcIntHandler = HIQ_DmaDoneCompute;
}


// High level interrupt enabler, which initializes necessary data stuctures.
bool HIQ_enableint(int whichhiq, HIQ *hiq, FIFO *fifo)
{
	char	*self = "HIQ_enableint";
	DWORD	dwStatus;
	//int		cardHandle;
	WDC_DEVICE_HANDLE hDev = hiq->hDev;
	bool	result = true;
	
	PWDC_DEVICE pDev = (PWDC_DEVICE)(hiq->hDev);
 
	dwStatus = HIQ_IntEnable(hDev, HIQ_DmaDoneCompute);
	if (WD_STATUS_SUCCESS != dwStatus)
    {
		printf("%s:FAILED TO ENABLE INTERRUPTS on whichhiq %d, Bus:Slot %d:%d. Error 0x%lx - %s\n",
			self,whichhiq,pDev->slot.pciSlot.dwBus,pDev->slot.pciSlot.dwSlot,
			dwStatus,Stat2Str(dwStatus));
		result = false;
	} 
	else 
	{
		printf("%s: INTERRUPT ENABLED on whichhiq %d, Bus:Slot %d:%d.\n",
			self,whichhiq,pDev->slot.pciSlot.dwBus,pDev->slot.pciSlot.dwSlot);

		printf("HW FIFO is %s half full.\n",(HIQ_ReadISTAT(hDev)& 0x02)?"greater than":"less than");
	}
	return result;
}


void HIQ_disableint(HIQ *hiq, int whichhiq)
{
	char *self = "HIQ_disableint";
	
	if(HIQ_IntDisable(hiq->hDev) == WD_STATUS_SUCCESS)
	{
		printf("%s:Card %d (of 1-N) INTERRUPT DISABLED.\n", self, whichhiq);
	} 
	else 
	{
		printf("%s: Card %d (of 1-N) INTERRUPT DISABLE FAILED.\n", self, whichhiq);
	}	
}


// setup debug monitor input.
/* KPTODO: Use WDC_Err() instead.
void HIQ_DebugSetup(void)
{
	BZERO(dbgMsg);
	dbgMsg.dwLevel = D_ERROR;	//default
	//dbgMsg.dwLevel = D_TRACE;
	dbgMsg.dwSection = S_ALL;

	/*
	debugInfo.dwSection =
	S_IO
	| S_MEM
	| S_INT
	| S_PCI
	| S_DMA
	| S_MISC
	| S_LICENSE
	//    | S_ISAPNP
	| S_PCMCIA
	| S_PNP
	| S_CARD_REG
	| S_KER_DRV
	//    | S_US
	//    | S_KER_PLUG
	| S_EVENT
	;
	*/
//}

// Add a message at a level to the Debug Monitor.
// See windrvr.h for definitions.
/* KPTODO: Use WDC_Err() instead.
void HIQ_DebugMsg(HIQ_HANDLE hHIQ, char* pDbgMsg, DEBUG_LEVEL aLevel, DEBUG_SECTION aSection)
{
	strcpy(dbgMsg.pcBuffer, pDbgMsg);
	dbgMsg.dwLevel = aLevel;
    dbgMsg.dwSection = aSection;
	WD_DebugAdd(hHIQ->hWD, &dbgMsg);
}
*/

// Init/update the interrupt structure with the angle source (see angles.h).
void setAngleSource(int whichhiq, int anAngleSource)
{
// KPTODO: just so it will compile for now.
	Hiq_Int_Data[whichhiq].angle_source = anAngleSource;
}

// Get the interrupt structure with the angle source (see angles.h).
int getAngleSource(int whichhiq)
{
	//return 0;	// KPTODO: just so it will compile for now.
	return Hiq_Int_Data[whichhiq].angle_source;
}
