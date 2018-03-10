#include "stdafx.h"
//#include	<windows.h>
#include	<math.h>
#include	<string.h>
#include	<stdio.h>   // this is just for the printf call

#include "code_controls.h"
#include "smfifo.h"
#include "mydata.h"
#include "piraqx.h"
#include "products.h"
#include "productdef.h"
#include "swversion.h"

// Log4cxx
#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <apr_general.h>
#include <apr_time.h>
#include <iostream>
#include <log4cxx/stream.h>
#include <exception>
#include <stdlib.h>

#include "arcLogger.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

// CONSOLE display
#define CONSOLE false

#define	ADFULLSCALE	8192.0
#define	BITSHIFT		16384.0
#define	TWOPI			6.283185307
#define	MAXNAME			80
#define	STARTGATE		20
#define	SMALL			0.0183156389 // TODO: Find out where in the world Mitch got this number! From the DOS version.
#define GAUSSIAN		4.4*2.5		// Correction to noise power for dual PRT
#define DPRT_SMALL_DB	-140.0		// dBm

static LoggerPtr logger;
static int Abort = 0;	// TODO: might be ok to leave this as a GLOBAL but it could be included in MYDATA

#ifdef  DEBUG_PRINT_ABP_AND_DPPRODUCTS_FIFO_FULLNESS
char fifoFullnessBuf[120] = "INITIAL_VALUE";
static int abpFifoFullnessH = 0;	// Horizontal abp fifo contains this many records.
static int abpFifoFullnessV = 0;	// Vertical   abp fifo contains this many records.
static int prodFifoFullnessDP = 0;	// dual pol products fifo contains this many records.
#endif

#ifdef IS_DUAL_POL 
MYDATA *product_open(char *src_fifo, char* src_fifo_dp, char *dst_fifo, bool isHorzElseVert)
#else
MYDATA *product_open(char *src_fifo,                    char *dst_fifo, bool isHorzElseVert)
#endif

{
	char	*self = "product_open";
	DWORD dwThreadId;
	MYDATA *pData;

	Abort = 0;	// global flag.

	// allocate storage for structure returned from this thread for the close operation.
	pData = (MYDATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,sizeof(MYDATA));
	if( pData == NULL )
	{
		LOG4CXX_FATAL(logger, "MYDATA allocate failed.");
		ExitProcess(2); 
	}

	// open the destination fifo
	pData->dstfifo = fifo_alloc(dst_fifo, 
								sizeof(PIRAQX), 
								sizeof(PIRAQX)+ (MAXGATES*PRODS_ELEMENTS*sizeof(float) ), 
								100);  // Header, 6 bytes per gate, 1000 gates, 100 records
	if(!pData->dstfifo)	
	{
		printf("%s:Bad destination fifo, pData->dstfifo! Exiting.\n",self); 
		LOG4CXX_FATAL(logger, "Bad destination fifo, pData->dstfifo.");
		exit(0);
	}

	// Generate unique data for each thread.
#ifdef IS_DUAL_POL
	pData->src_fifo_dp_prod = src_fifo_dp;
#endif
	pData->src_fifo = src_fifo;					// ptr to source fifo name used in worker thread
	pData->isHorzElseVert = isHorzElseVert;		// polarization
	pData->thread = CreateThread( 
		NULL,			// default security attributes
		0,				// use default stack size  
		ProductThreadProc,	// thread function 
		pData,			// argument to thread function 
		0,				// use default creation flags 
		&dwThreadId		// returns the thread identifier
		);

	// Check the return value for success. 
	if (pData->thread == NULL) 
	{
		LOG4CXX_FATAL(logger, "CreateThread failed.");
		ExitProcess(0);
	}
	else
	{
		// We want this only to be higher than other tasks, like PpiScope(ABOVE_NORMAL and everything non-radar(NORMAL).
		// Note that the actual priority is a combination of process(class) priority AND thread priority. 
		//SetThreadPriority(pData->thread,THREAD_PRIORITY_ABOVE_NORMAL);
		SetThreadPriority(pData->thread,THREAD_PRIORITY_HIGHEST);
		//SetThreadPriority(pData->thread, THREAD_PRIORITY_TIME_CRITICAL);
	}
	return(pData);		// for the close operation.
}


void product_close(MYDATA *pData)
{
	Abort = 1;	// global flag.
	WaitForSingleObject(pData->thread,10000);	// wait 10 seconds for thread to stop
	CloseHandle(pData->thread);
	fifo_close(pData->dstfifo);
	if(pData->isHorzElseVert)
	{
		LOG4CXX_INFO(logger, "Closed Horizontal product thread.");
	}
	else 
	{
		LOG4CXX_INFO(logger, "Closed Vertical product thread.");
	}
}


DWORD WINAPI ProductThreadProc( LPVOID lpParam ) 
{ 
	HANDLE hStdout;
	MYDATA *pData;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if( hStdout == INVALID_HANDLE_VALUE )
	{
		LOG4CXX_FATAL(logger, "Invalid stdout handle.");
		return 1;
	}

	// Cast the parameter to the correct data type.
	pData = (MYDATA*)lpParam;
	productworker(pData);
	return(0);
}


// This function takes integer ABP's and processes them into scientific products
void productworker(MYDATA *pData)
{
	char	*self = "productworker";
#ifdef IS_DUAL_POL
	int head_now_dp = -1;	// Incoming DP products FIFO head right now.
	FIFO	*srcfifoDp = NULL;
	int		tail_dp = 0;
	int		test_dp; 
	PIRAQX	*px_dp;
	float	*dpf;
#endif
	FIFO	*srcfifo = NULL;
	int		tail_abp = 0;
	int		test_abp; 
	float	*prods, *abpf;
	short	*abps;
	char	*writerecord;
	PIRAQX	*px_abp;
	char buf[256];
#ifdef DEBUG_PRINT_ABP_AND_DPPRODUCTS_FIFO_FULLNESS
	int head_now_abp = -1;		// Incoming ABP FIFO head right now.
#endif

	// Logging
	logger = setupLogger("productworker", LOGGING_CONFIG_FILE_NAME_PRODUCTS);
	if( logger == NULL) { 
		printf("Failed to setup productworker logger");
	}
	LOG4CXX_DEBUG(logger,  "=============== productworker() ============");
	LOG4CXX_DEBUG(logger,  "productworker: HiQ receiver program version:" << HIQ_SW_VERSION);
	testLevels(logger,"productworker");
	setupSmfifoLogging();

	sprintf(buf,"%s:reading APB fifo '%s', and writing to Products fifo '%s'.\n",
		self,pData->src_fifo,pData->dstfifo->name);
	LOG4CXX_DEBUG(logger, buf);

top:
	sprintf(buf,"%s:Opening fifo '%s', Abort='%d'.\n",self,pData->src_fifo,Abort);
	LOG4CXX_DEBUG(logger, buf);

	// Open up a FIFO buffer, wait forever but with a way out via Abort flag.
	while(!(srcfifo = fifo_check_usable(pData->src_fifo, &tail_abp)) && !Abort)  
	{
		LOG4CXX_DEBUG(logger, "Waiting to open up the ABP source FIFO.");
		Sleep(300); 
	}
#ifdef IS_DUAL_POL
	// Open up the DP products FIFO buffer, wait forever but with a way out via Abort flag.
	while(!(srcfifoDp = fifo_check_usable(pData->src_fifo_dp_prod, &tail_dp)) && !Abort)  
	{
		LOG4CXX_DEBUG(logger, "Waiting to open up the Dual Pol Products source FIFO.");
		Sleep(300); 
	}
#endif
	if(Abort)
	{
		LOG4CXX_INFO(logger, "Aborted Wait loop to open up one of the source FIFOs.");
		goto bottom;
	}

	while(1)
	{
		// Wait for a new beam of data in both src fifos.
		test_abp = fifo_safe_wait(srcfifo,   pData->src_fifo,         &tail_abp, 1, 10, &Abort);
	#ifdef  DEBUG_PRINT_ABP_AND_DPPRODUCTS_FIFO_FULLNESS
		head_now_abp = srcfifo->head;	// so it does not get updated by INT handler.
	#endif

	#ifdef IS_DUAL_POL
		test_dp  = fifo_safe_wait(srcfifoDp, pData->src_fifo_dp_prod, &tail_dp,  1, 10, &Abort);
	#ifdef  DEBUG_PRINT_ABP_AND_DPPRODUCTS_FIFO_FULLNESS
		head_now_dp = srcfifoDp->head;	// so it does not get updated by INT handler.
	#endif
		if(test_abp == FIFO_WAITABORT || test_dp == FIFO_WAITABORT)	goto bottom;
		if(test_abp == FIFO_GONE      || test_dp == FIFO_GONE)		goto top;
	#else
		if(test_abp == FIFO_WAITABORT )	goto bottom;
		if(test_abp == FIFO_GONE )		goto top;
	#endif

		// Visually indicate reception of a new record (beam) of APB data.
		if(CONSOLE) { printf("%c",pData->isHorzElseVert ? '-' : '|'); fflush(stdout); }

		// DEBUG: print out how many records are in the incoming fifos.
	#ifdef  DEBUG_PRINT_ABP_AND_DPPRODUCTS_FIFO_FULLNESS
		if(strcmp(srcfifo->name, "/ABPSIM16H") == 0)
		{
			abpFifoFullnessH = prtFifoFullness(&fifoFullnessBuf[0],srcfifo,head_now_abp,tail_abp);	// Horizontal ABP
		}
		else if(strcmp(srcfifo->name, "/ABPSIM16V") == 0)
		{
			abpFifoFullnessV = prtFifoFullness(&fifoFullnessBuf[0],srcfifo,head_now_abp,tail_abp);	// Vertical ABP
			//Sleep(100);	// DEBUG: cause a FIFO overrun.  PPI should 'jump' to a different angle & continue.
		}
		#endif

		#ifdef DEBUG_PRINT_ABP_AND_DPPRODUCTS_FIFO_FULLNESS
		#ifdef IS_DUAL_POL
		prodFifoFullnessDP = prtFifoFullness(&fifoFullnessBuf[0],srcfifoDp,head_now_dp,tail_dp);	// Dual Pol products
		//      DPWKR:IQ H=xxx V=xxx
		//                      DPWKR:IQ H=xxx V=xxx
		printf("                                  PROD: H=%3d V=%3d DP=%3d\n",abpFifoFullnessH,abpFifoFullnessV,prodFifoFullnessDP); 
		#else
		printf("                      PROD=%3d\n",abpFifoFullnessH); 
		#endif
		#endif
		// END DEBUG: print out how many records are in the fifos.

		// Get pointer to the newly available record from the source FIFO (containing a piraqx followed by data)
		px_abp = (PIRAQX *)fifo_get_address(srcfifo,  tail_abp,0);	// get the address of the first valid ABP record.
#ifdef IS_DUAL_POL
		px_dp  = (PIRAQX *)fifo_get_address(srcfifoDp,tail_dp ,0);	// get the address of the first valid DP product record.
		px_abp->pulse_num = px_dp->pulse_num;	// H channel pulse number.
		px_abp->spare[0] = px_dp->spare[0];		// Pulse # delta (H-V) from dpProducts.
	#ifdef DEBUG_PRODUCTS_DP_PRINT_PULSE_NUM
		printf("PRODUCTS:pulse#=%I64u, diff=%d\n",px_dp->pulse_num,px_dp->spare[0]);
	#endif
#endif

#ifdef DEBUG_ANGLES
if(0==px_abp->channel_polarization)	
{
	printf("H %5.1f", px_dp->az);
}
else
{
	printf(" : V %5.1f\n", px_dp->az);
}
#endif

		// Get a pointer to the current record of the destination FIFO (will contain a piraqx struct and scientific products)
		writerecord = (char *)fifo_get_write_address(pData->dstfifo);

		// Pass a copy of the piraqx struct from the source record to the destination record
		// ASSUMPTION: This is the same as the header form the DP product record.  TODO: test for different pulsenum, beamnum, etc.
		memcpy(writerecord,px_abp,sizeof(PIRAQX));

		// Get a convenient pointer to the start of the product array that will be put into the output record in the destination FIFO
		prods = (float *)(writerecord + sizeof(PIRAQX));	// get the address of the next write record

		// Use the header to convert the record to an array of scientific parameters
		// Choose product generation routine on basis of data format.
	#ifdef IS_DUAL_POL
		dpf = (float *)((char *)px_dp + sizeof(PIRAQX));	
	#endif
		switch(px_abp->dataformat)
		{
		case DATA_SIMPLE16:
			abps = (short *)((char *)px_abp + sizeof(PIRAQX));
	#ifdef IS_DUAL_POL
			newsimplepp(prods, abps, px_abp, dpf, pData);
	#else
			newsimplepp(prods, abps, px_abp);
	#endif
			break;
		case DATA_DUALPP:
			abpf = (float *)((char *)px_abp + sizeof(PIRAQX));	
	#ifdef IS_DUAL_POL
			dualprt(prods, abpf, px_abp, dpf, pData);
	#else
			dualprt(prods, abpf, px_abp);
	#endif
			break;
		default:
			abps = (short *)((char *)px_abp + sizeof(PIRAQX));	// ptr to input APB data.
	#ifdef IS_DUAL_POL
			newsimplepp(prods, abps, px_abp, dpf, pData);
	#else
			newsimplepp(prods, abps, px_abp);
	#endif
		}
		// set the record size so that it can be put on disk or over ethernet
		fifo_set_rec_size(pData->dstfifo,sizeof(PIRAQX) + PRODS_ELEMENTS * sizeof(float) * px_abp->gates);

		// increment the output fifo head
		fifo_increment_head(pData->dstfifo);
	}

bottom:
	if(srcfifo)	// if you were using it...
	{
		LOG4CXX_DEBUG(logger, "Disconnect from the source fifo: " << pData->src_fifo);
		fifo_disconnect(srcfifo);		
	}
}


/* This routine stores ABP as 16bits each (LogAB, AngAB, LogP) */
/* Gate zero power is set to be the average of all the gates from STARTGATE */
/* to the end */
static double oldG0mag = 0.0, avgg0correct = 0.0, hpower = 0.0;

#ifdef IS_DUAL_POL
void newsimplepp(float *pptr, short *aptr, PIRAQX *px, float *dpptr, MYDATA *pData)
#else
void newsimplepp(float *pptr, short *aptr, PIRAQX *px)
#endif
{
	unsigned int i,avecount, hvflag = 0;
	short        pn; /* pn is noise corrected power */
	float        *gate0;
	double       average_power;
	double       cp,v,p,pcorrect,noise_power=0.0,linear = SMALL;
	double       velconst,dbm,widthconst,range,rconst,scale2db,scale2ln;
	double       sqrt();
	double		 g0correct = 0;
	
	// Initialize the horizontal/vertical flag: 0 = horiz, 1 = vert
	hvflag = pData->isHorzElseVert? 0: 1;
	/* initialize the things used for h and v power average */
	average_power = 0.0;
	avecount = 0;
	gate0 = pptr;   /* this is for integrating the power for solars */
	/* The power from most of the gates will be averaged */
	/* and inserted into the gate 0 value */

	scale2ln = 0.004 * log(10.0) / 10.0;  /* from counts to natural log */
	scale2db = 0.004 / scale2ln;         /* from natural log to 10log10() */
	velconst = px->velsign * C_CONSTANT / (2.0 * px->frequency * 2.0 * fabs(px->prt[0]) * 32768.0);
	rconst = - 20.0 * log10(px->xmit_pulsewidth / px->rcvr_pulsewidth) 
		+ (hvflag == 0? 
			px->h_rconst - px->h_xmit_power + px->h_measured_xmit_power:
			px->v_rconst - px->v_xmit_power + px->v_measured_xmit_power);
	pcorrect = px->data_sys_sat
		- 20.0 * log10((float)0x10000)
		- (hvflag == 0? px->h_receiver_gain: px->v_receiver_gain);
	/* NOTE: 0x10000 is just the standard offset for all systems */
	widthconst = (C_CONSTANT / px->frequency) / px->prt[0] / (2.0 * sqrtf((float)2.0) * M_PI);

	range = 0.0;
	if(hvflag == 0)
	{
		if(px->h_noise_power <= -10.0)
		{
			noise_power = exp((px->h_noise_power - pcorrect) / scale2db);
		}
	}
	else
	{
		if(px->v_noise_power <= -10.0)
		{
			noise_power = exp((px->v_noise_power - pcorrect) / scale2db);
		}
	}

	for(i=0; i<px->gates; i++)
	{
		cp = *aptr++;    /* 0.004 dB / bit */
		v  = *aptr++;    /* nyquist = 65536 = +/- 32768 */
		pn = p = *aptr++;    /* 0.004 dB / bit */

		/* get a linear version of power */
		linear = exp(p * scale2ln);

		/* noise correction */
		if(hvflag == 0)
		{
			if(px->h_noise_power <= -10.0)
			{
				linear -= noise_power;         /* subtract noise */
				if(linear <= 0.0) linear = SMALL;
				pn = (short)(log(linear) / scale2ln);   /* noise corrected power */
			}
		}
		else
		{
			if(px->v_noise_power <= -10.0)
			{
				linear -= noise_power;         /* subtract noise */
				if(linear <= 0.0) linear = SMALL;
				pn = (short)(log(linear) / scale2ln);   /* noise corrected power */
			}
		}
	
		/* average power for solar cal purposes */
		if(i >= STARTGATE)
		{
			avecount++;
			average_power += linear;
		}

		if(i)     range = 20.0 * log10(i * 0.0005 * C_CONSTANT * px->rcvr_pulsewidth);

		/* compute floating point, scaled, scientific products */
		// The product array is always PRODS_ELEMENTS (16) floats per gate, even if the algorithm isn't
		// Always put products in the right order, too, so the display can understand it

		pptr[PRODS_VELCTY] = (float)(velconst * v);								// Velocity in m/s
		pptr[PRODS_POWER] = (float)(dbm = 0.004 * pn + pcorrect);				// Power in dBm
		pptr[PRODS_NCP   ] = (float)(exp(scale2ln*(cp - p)));					// NCP no units
		pptr[PRODS_WIDTH ] = (float)(sqrtf((float)scale2ln * fabs(p-cp)) * widthconst);  // Spectrum Width in m/s
		pptr[PRODS_DBZ  ] = (float)(dbm + rconst + range);								// Reflectivity in dBZ
		pptr[PRODS_COHER_DBZ] = (float)(0.004 * cp + pcorrect + rconst + range);			// Coherent Reflectivity in dBZ
#ifdef IS_DUAL_POL
		pptr[PRODS_ZDR]		= *dpptr; dpptr++;									// Differential Reflectivity
		pptr[PRODS_PHIDP]	= *dpptr; dpptr++;									// Differential Phase
		pptr[PRODS_RHOHV]	= *dpptr; ++dpptr;									// RHO hv
#endif
		pptr += PRODS_ELEMENTS;
	}

	/* now insert the average h and v power into the gate 0 data */
	/* this is only necessary for the realtime system */
	if(avecount == 0) return;
	gate0[1] = (float)(log(average_power / (double)avecount) * scale2db + pcorrect);    /* average HH  power in dBm */
}

#ifdef IS_DUAL_POL
void dualprt(float *prods, float *abp, PIRAQX *px, float *dpptr, MYDATA *pData)
#else
void dualprt(float *prods, float *abp, PIRAQX *px)
#endif
{
	int			 i, hvflag;   
	float        *aptr,*bptr;
	float        *pptr;
	double       a,b,p,pn,a2,b2,cp,pcorrect,ncorrect,biga,bigb;
	double       noise,velconst,dprt_small = 0;
	double       dbm,widthconst,range,rconst,width,r12;
	double       sqrt(), prt, scale = 1.0;   

	// Initialize the horizontal/vertical flag: 0 = horiz, 1 = vert
	hvflag = pData->isHorzElseVert? 0: 1;
	prt = 0.5*(px->prt[0] + px->prt[1]);
	rconst =  - 20.0 * log10(px->xmit_pulsewidth / px->rcvr_pulsewidth)
		+ (hvflag == 0? 
			px->h_rconst - px->h_xmit_power + px->h_measured_xmit_power:
			px->v_rconst - px->v_xmit_power + px->v_measured_xmit_power);

	velconst = px->velsign * C_CONSTANT / (2.0 * px->frequency * 2.0 * M_PI * fabs(px->prt[0] - px->prt[1]));	

	ncorrect = px->data_sys_sat															// Correct
		- 20.0 * log10(((double)(ADFULLSCALE*BITSHIFT)/65536.0)*(px->rcvr_pulsewidth*1.0e7 + 0.5)) 
		- 20.0 * log10((double)0x10000)
		- (hvflag == 0? px->h_receiver_gain: px->v_receiver_gain);

	dprt_small = pow(10.0, .1*DPRT_SMALL_DB);		// Lower limit on linear power 

	pcorrect = ncorrect - 10.0*log10((double)px->hits/2.0);

	widthconst = (C_CONSTANT / px->frequency) / prt / (2.0 * sqrtf(2.0) * M_PI); //C/(f*PRT*2*sqrt(2)*pi)// Correct
	if(hvflag == 0)
	{
		noise = (px->h_noise_power <= -10.0) ?													// TBD
			pow(10.0, 0.1*(px->h_noise_power - ncorrect + GAUSSIAN)): 0.0;
	}
	else
	{
		noise = (px->v_noise_power <= -10.0) ?													// TBD
			pow(10.0, 0.1*(px->v_noise_power - ncorrect + GAUSSIAN)): 0.0;
	}
	aptr = abp;
	bptr = aptr + 3 * px->gates;
	pptr = prods;
	range = 0.0;

#ifdef DEBUGp0
	printf("DEBUGp0: aptr(dwell->abp)=%p, bptr(a+3*)=%p, pptr(prods)=%p\n", aptr, bptr, pptr);
#endif

	for(i=0; i<px->gates; i++)
	{
		a = scale*(double)(*aptr++);
		b = scale*(double)(*aptr++);
		p = scale*(double)(*aptr++);
		a2 =scale*(double)(*bptr++);
		b2 =scale*(double)(*bptr++);
		p +=scale*(double)(*bptr++);
		p /= 2.0;
		pn = p - noise;
		if(pn <= 0.0)
			pn = dprt_small;

#ifdef DEBUGp1
		printf("DEBUGp1: gate=%d, aptr=%p, bptr=%p, pptr=%p ", i, aptr, bptr, pptr);
		//      printf("a %f,b %f,p %f, a2 %f, b2%f", a,b,p,a2,b2);
		printf("\n");
#endif

		if(i)range = 20.0*log10(i*0.0005*C_CONSTANT*px->rcvr_pulsewidth);

		/* compute floating point, scaled, scientific products */
		biga = a * a2 + b * b2;
		bigb = a2 * b - a * b2;

		pptr[PRODS_VELCTY] = (float)(velconst * atan2(bigb,biga));							// Velocity in m/s
		pptr[PRODS_POWER ] = (float)(dbm = 10.0 * log10(fabs(pn)) + pcorrect);				// Power in dBm
		pptr[PRODS_NCP   ] = (cp = sqrtf(r12 = a*a+b*b))/p;							// NCP no units
		if((width = log(fabs((p-noise)/cp))) < 0.0) width = 0.0001;
		pptr[PRODS_WIDTH ] = (float)(sqrtf(width) * widthconst);								// Spectral Width in m/s
		pptr[PRODS_DBZ  ] = (float)(dbm + rconst + range);									// Reflectivity in dBZ
		pptr[PRODS_COHER_DBZ] = (float)(10.0 * log10(fabs(cp)) + pcorrect + rconst + range);	// Coherent Reflectivity in dBZ
#ifdef IS_DUAL_POL
		pptr[PRODS_ZDR]		= *dpptr; ++dpptr;										// Differential Reflectivity
		pptr[PRODS_PHIDP]	= *dpptr; ++dpptr;										// Differential Phase
		pptr[PRODS_RHOHV]	= *dpptr; ++dpptr;										// RHO hv
#endif
		pptr += PRODS_ELEMENTS;      
	}
}
