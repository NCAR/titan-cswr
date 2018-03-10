#pragma once

#define C_CONSTANT 2.99792458E8

#include "hiq.h"

/* Structure to completely define the operation of a single HIQ */
/* There should be a one-to-one correspondance between the members of this structure */
/* and specific hardware functions (and virtual hardware functions implemented by software) */
/* on the HIQ card so that the HIQ can be unambiguously */
/* set up based on the information in this structure. */
/* Anything that differentiates one HIQ from another should also be here. */
typedef struct
	{
	// These parameters are specific to the hardware function of each HIQ
	int				gates,hits,pulsewidth;
	int				prt,prt2,timingmode;
	int				delay,sync;				/* think hard about these!!!!! It's always been confusing */
	int				trigger,tpwidth,tpdelay,intenable,dataformat;

	int				velsign;				/* defines upper or lower sideband operation */
	int				xmitpulse;				/* xmit pulsewidth used for discrim only */
	int				gate0mode;				/* control gate0mode on / off */
	int				afc;					/* afc on or off */
	float			afcgain,afchi,afclo,locktime;	/* this is specific to each HIQ in a multiple xmitter case */
	double			pllfreq;				/* this is the exact on-board VCO freq with the inherent PLL resolution */
	double			ref,cmpfreq;
	double			rxfreq;					/* defines current desired receiver center frequency of the system */

	// These parameters define the operation of this HIQ, and specify unique
	// characteristics about it's connection to the system.
	float			stalofreq;				/* defines the mixdown freq on this channel */
	float			xmitcoupler,rxgain;		/* I don't know where this should go yet!!!!!! */
	int				function;				/* i.e. V channel, TX sample, etc. */
	int				boardnum;				/* which of n cards are we refering to */
	int				configid;				// matches the id for the records in the FIFO
	char			name[14];				// name of the card, e.g. "Horiz pol". Size matches WD.
	char			description[100];		// description of the card, e.g. "Advanced Radar Corp. HiQ Receiver 33MHz PCI card"
	HIQ			   *hiq;					// pointer to card descriptor.
	}HIQOP;