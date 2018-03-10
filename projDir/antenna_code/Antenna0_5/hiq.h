#pragma once

#ifndef _HIQ_H_
#define _HIQ_H_

#include "C:\WinDriver\include\wdc_defs.h" 
/***********************************************************************/
/*  HIQ hardware specific defines                                      */
/*  Ideally, only the device driver and the HIQ library need this file */
/***********************************************************************/

#define BITMASK 0xFFFFC000
#define BITSHIFT ((double)((~BITMASK)+1))
#define ADFULLSCALE ((double)0x2000)	// is half of the full range (+/-2^N/2)

#define	LOFREQ	95E6
#define	HIFREQ	120E6	

/* Structure containing HIQ addresses and info used at the hardware */
/* and interrupt level */
/* The board has hardware functions that are fixed, but in addition the board */
/* has some software implemented functions that are equally well fixed (like a pulsenum counter) */
/* The decision to group certain software functions with the native hardware capabilities */
/* involves determining if the function is fundamental and universal */
/* Functionality at a higher level, or likely to change with applicaton can be included */
/* in another structure */
/* The software variables such as hits should be thought of as a hardware register that */
/* controls the functionality of the board. Not as the location where that information is held. */
/* This means that there will be another structure containing 'hits' that higher level */
/* functions will use as the definition. Configuring the board involves updating this 'hits' */
/* with the values from the definition in higher level structures. */
/* In short, when writing higher level routines, just pretend everything in this structure */
/* refers to hardware. And also assume that the output of the HIQ is a FIFO structure */
/* filled with raw data. */
/* This philosophy is key to successfully organizing the architecture! */
/* Also, this structure can only be seen by the driver library. */
/* Each swfifo record will contain eofstatus,hitcount,beamnum,pulsenum, followed by IQDATA. */
typedef struct
	{
	// Hardware addresses
	int					*fifo;				/* address of the HIQ hardware fifo */
	int					cfgbase;			/* address of the configuration registers in the PCI chip */

	// PC interrupt handling related variables
	int					imask;				/* istat mask */
	int					intnum,irqmask;		/* PC hardware IRQ, PC PIC mask */
	
	// Low level software functionality associated with the HIQ
	int					gates,gate0mode,pulsewidth;	/* to know how many reads for the int routine */
	int					eofstatus;			/* keep a variable representing eofstatus read from hwfifo */
	int					configid;			/* configuration id associates each hit with the HIQOP struct */
	unsigned long long	pulsenum;			/* keep track of pulsenum at int level */

	// afc related variables
	float				numdiscrim,dendiscrim,g0invmag,err,avgerr;	/* afc feedback loop parameters */
    double				afcfreq,freq;		/* afcfreq is the integrator value, freq is current value (actual on-board pll freq) */
	int					afc,firstpll;		/* afc on/off, flag indicating if the pll chip has not been intialized */
	float				afcgain;
	
	// pll variables
	int					n,r;
	double				ref,cmpfreq;

	// generic pointers that will be specifically defined elsewhere
	void				*dma;		// pointer to DMA structure containing DMA related pointers, variables
	WDC_DEVICE_HANDLE	hDev;		// handle for this card, which is the address of the 'slot' struct.
	} HIQ;

#endif
