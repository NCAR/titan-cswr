// hiqdrvr.cpp : Defines the entry point for the DLL application.
//
#include <windows.h>
#include "status_strings.h"
#include "hiqdrvr.h"
#include "hiqutil.h"
#include "drvwrapper.h"		// for accessing interrupt counts
//#include "readpiraqx.h"
//#include "piraqx.h"
#include "hiq_hardware.h"
#include "angles.h"

#include <stdio.h>
#include <time.h>
#include <conio.h>
//#include <stdlib.h>

static int IDCOUNT = 100;

/* Open a hiq board */
/* records specifies the number of records in the FIFO */
/* gates specifies the maximum number of gates for each record */
/* Returns a pointer to the FIFO associated with the numbered HIQ card */
/* The user structure in the FIFO is a HIQ_CONFIG type */
/* Returns a NULL pointer on failure */
//#define	MAXPW	20
//#define	RECORDSIZE(gates) (MAXPW)*sizeof(uint4)+sizeof(UDPHEADER)+sizeof(IQRECORD)+2*sizeof(uint4)*(gates)
//#define	FIFONAME  "/IQDATA"
FIFO *HIQ_open(int whichhiq, char *aFifoName, int maxgates, int records, PIRAQX *px, PHIQ_DEV_CTX pHiqDevCtx)
{
	char	*self = "HIQ_open";
	HIQOP	*h;
	FIFO	*hiq;

	//printf("passing gates = %d to HIQ_init()  (records = %d)\n",gates,records);
	hiq = HIQ_init(whichhiq, aFifoName, sizeof(HIQOP), maxgates, records, px, pHiqDevCtx);

	/* put some reasonable default values into the HIQOP (user) structure of the fifo */
	HIQ_init_op(hiq,whichhiq);

	/* need to transfer the default parameters from the FIFO HIQOP struct into Hiq[] struct */
	h = (HIQOP *)fifo_get_header_address(hiq);
	//HIQ	*pHiq = getHiqStruct(whichhiq);
	h->hiq = (HIQ *)getHiqStruct(whichhiq);
	HIQ_set_device_handle(whichhiq, h->hiq->hDev); 
	HIQ_set_gates(whichhiq,h->gates);
	HIQ_set_gate0mode(whichhiq,h->gate0mode);
	HIQ_set_pulsewidth(whichhiq,h->pulsewidth);
	//   HIQ_set_hits(whichhiq,h->hits);
	HIQ_set_configid(whichhiq,h->configid);
	HIQ_set_afc(whichhiq,h->afc);
	HIQ_set_afcgain(whichhiq,h->afcgain);
	HIQ_set_ref(whichhiq,h->ref);
	HIQ_set_cmpfreq(whichhiq,h->cmpfreq);
	// What about setting the stalo frequency here? (in the HIQ structure)
	// Maybe we should just make the HIQ struct match whatever actually gets programmed?

	// Test for validity.
	h = hiqvalid(hiq);
	if(!h)
	{
		return(0);
	}

	/* check that the card number is in range and that that card has been initialized */
	if(!HIQ_initvalid(h->boardnum))	return(NULL);

	printf ("%s:Initialized HIQ receiver card '%d'.\n",self,whichhiq);
	return(hiq);
}


/* this is a mess. the error checking is screwed up so far */
void HIQ_close(int whichhiq, FIFO *hiq)
{
	HIQOP	*h;

	if(!(h = hiqvalid(hiq)))	return;
	HIQ_kill(hiq, h->boardnum);	// terminate FIFO.
}


/* 
Start a data session.
Board must be stopped prior to starting or behaviour will be unpredictable.
Always returns 1.
Note: in future HIQ revisions, make sure that synchronous starting does 
not require spinning on FIRSTTRIG!  This screws up synchronous start for
multiple boards! (this can be fixed with dedicated delay timer, 
or FIRSTRIG interrupt.)  
*/
int HIQ_start(FIFO *hiq, unsigned long long pulsenum)
{
	char *self = "HIQ_start:";
	time_t	first;
	char c;
	HIQOP *h;
	unsigned long long Beamnum;

	if(!(h = hiqvalid(hiq)))	return(0);

	/* initialize the HIQ with the correct counts */
	HIQ_initcounts(h->boardnum,pulsenum);
	hiq->head = (int)(pulsenum % hiq->record_num);	/* this is where the first beam is going */
	Beamnum = pulsenum / h->hits;

	printf("%s: board %d: timingmode='%d'\n", self, h->boardnum, h->timingmode);
	if(h->timingmode == 1 || h->timingmode == 3)
	{
		HIQ_tmode_trig(h->boardnum);				/* set HIQ for external triggers */
	} 
	else 
	{
		HIQ_tmode_cont(h->boardnum);				/* set HIQ for continuous (internal) triggers */
	}
	if(h->timingmode == 2)
	{
		first = time(NULL) + 3;   /* wait up to 3 seconds */
		while(!HIQ_status1rd(h->boardnum,STAT_FIRST))
		{
			if(time(NULL) > first)
			{
				if(_kbhit())
				{
					if(toupper(c = _getch()) == 'Q') // TODO: Consider changing this to the new stop charecter, '!'
					{
						printf("Q was hit before external sync occured\n");
						//		   Sleep(500);
						break;
					}
					HIQ_timer(1, 5, h->prt2-2, h->hiq->hDev);   /* odd prt (2) */
				}
			}
		}
	}

	if(!h->timingmode)
		HIQ_start_internal(h->boardnum);			/* software trigger for continuous mode */

	//printf("number of gates in HIQOP struct in fifo = %d\n",h->gates);
	return(1);  /* everything is OK */
}


void HIQ_stop(FIFO *hiq)
{
	HIQOP *h;

	if(!(h = hiqvalid(hiq)))	return;

	HIQ_savefreq(h->boardnum);
	HIQ_halt(h->boardnum);
}


/* Initialize the HIQOP structure embedded as a user header in the FIFO structure */
/* with new data passed to this routine. Also configure the board to match */
void HIQ_set(FIFO *pFifo, HIQOP *op, PHIQ_DEV_CTX pHiqDevCtx)
{
	int		numsave;
	HIQOP	*h;
	
	if(!(h = hiqvalid(pFifo)))	return;

	/* need to also save rxfreq so that board maintains last used freq */
	/* but this needs to be thought through so that other parameters are consistent */
	numsave = h->boardnum;
	memcpy(h,op,sizeof(HIQOP));	// copy all the data from the new struct into the embedded struct
	h->boardnum = numsave;		/* don't muck with this parameter!!!! */

	HIQ_timerset(pFifo);			/* set up the timing and triggering characteristic of the board */

	h->function = 0;				/* i.e. V channel, TX sample, etc. */
	h->configid = IDCOUNT++;		/* matches the id for the records in the FIFO */

	/* need to transfer the parameters from the FIFO HIQOP struct into Hiq[] */
	HIQ_set_gates(h->boardnum,h->gates);
	HIQ_set_gate0mode(h->boardnum,h->gate0mode);
	HIQ_set_pulsewidth(h->boardnum,h->pulsewidth);
	//   HIQ_set_hits(h->boardnum,h->hits);
	HIQ_set_configid(h->boardnum,h->configid);
	HIQ_set_afc(h->boardnum,h->afc);
	HIQ_set_afcgain(h->boardnum,h->afcgain);
	HIQ_set_ref(h->boardnum,h->ref);
	HIQ_set_cmpfreq(h->boardnum,h->cmpfreq);

	// TODO: Check these frequency computations
	if(h->afc) {
		h->rxfreq = h->stalofreq + (HIQ_getstalofreq(h->boardnum) + HIQ_LAST_IF) * h->velsign;
	} else if(h->rxfreq < 0) {
		HIQ_rxfreq(pFifo,h->rxfreq = 0.5 * (h->afchi + h->afclo));	/* set up center frequency of the on system (after cmpfreq and ref) */
	} else {
		HIQ_rxfreq(pFifo,h->rxfreq);	/* set up center frequency of the on system (after cmpfreq and ref) */
	}
	// TODO: WPQ: 4/21/08: is above or below correct?
	HIQ_rxfreq(pFifo,h->rxfreq);
	//printf("setting the rx freq to %f\n",h->rxfreq);
	//printf("ref = %f    comp freq = %f\n",h->ref,h->cmpfreq);

	// prepare the DMA for transfer of the right number of gates
	HIQ_set_dma(h->boardnum,pFifo,pHiqDevCtx);	// set up the DMA for transfering the right number of bytes

	// Set up and pass in Ctx to KP.
	HIQ *pHiq = getHiqPerBoardArrayPtr(h->boardnum);
	HIQ_setDeviceContext(h->boardnum, pHiq, pFifo, pHiqDevCtx);	// Set context for KP ISR.
	WD_KERNEL_PLUGIN_CALL kpCall;
	PDWORD pdwResult = 0;
	kpCall.dwResult = 256;
	kpCall.dwMessage = KP_HIQ_MSG_GET_CTX;
	kpCall.hKernelPlugIn = (DWORD)(pHiq->hDev);
	kpCall.pData = pHiqDevCtx;
	WDC_CallKerPlug(pHiq->hDev, KP_HIQ_MSG_GET_CTX, kpCall.pData, pdwResult);
	if (WD_STATUS_SUCCESS != (DWORD)pdwResult)
    {
		// KPTODO: log this somewhere!
        printf("Failed transfering Ctx to kernel. Error 0x%lx - %s\n",
            pdwResult, Stat2Str(*pdwResult));
		MessageBox(NULL,TEXT(Stat2Str(*pdwResult)),TEXT("WDC_CallKerPlug"),MB_OK);
    }
}


/* Determine if the HIQ board associated with this structure has been initialized */
/* On success returns a pointer to the HIQOP structure embedded as the user */
/* structure in the FIFO structure */
/* On failure returns NULL */
/* This is kind of a GROUP A routine but probably should never be called by */
/* high level functions. */
HIQOP *hiqvalid(FIFO *hiq)
{
	HIQOP *h;

	if(!hiq)		/* check that the hiq structure itself is valid */
	{
		printf("Error: invalid FIFO structure representing HIQ board\n");
		return(NULL);	/* err if the hiq structure itself hasn't been initialized */
	}

	h = (HIQOP *)fifo_get_header_address(hiq);

	return(h);
}

// Interface method.  For now we are leaving the actual work in hiq utility file.
HIQDRVR_API int HIQ_do_afc(int whichhiq, PIRAQX *p, bool slaveMode)
{
	return HIQ_afc(whichhiq, p, slaveMode);
}


// Interface method.  Get the interrupt count for a specific card. 
HIQDRVR_API int HIQ_get_Int_Cnt(int whichhiq)
{
	return HIQ_getIntCnt(whichhiq);
}


// Interface method.  Get the interrupt ERROR count for a specific card. 
HIQDRVR_API int HIQ_get_Int_Err_Cnt( int whichhiq )
{
	return HIQ_getIntErrCnt(whichhiq);
}

// Interface method.  Get the sync state for a specific card. 
HIQDRVR_API bool HIQ_is_In_Sync( int whichhiq )
{
	return HIQ_isInSync(whichhiq);
}

// Interface method.  Set the angle source for all cards. 
HIQDRVR_API void HIQ_setAngleSource(int whichhiq, int anAngleSource)
{
	setAngleSource(whichhiq, anAngleSource);
}

// Interface method.  Get the angle source for all cards. 
HIQDRVR_API int HIQ_getAngleSource(int whichhiq)
{
	return getAngleSource(whichhiq);
}

// Interface method.  Get the angle source for all cards. 
HIQDRVR_API void HIQ_get_angles(IQRECORD *r, int angle_source, unsigned short *ctr)
{
	return get_angles(r, angle_source, ctr);
}

// Interface method.  Get the interrupt data for all cards. 
HIQDRVR_API HIQ_INT_DATA *getHiqIntData(int whichhiq)
{
	return getHiq_Int_Data(whichhiq);
}
