/* 
structure for passing info back from abp_open() for a abp_close()
22-Apr-2008 WP Quinlan
*/
#pragma once
//#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/

//#ifndef _MYDATA_H_
//#define _MYDATA_H_

#include "smfifo.h"
#include "piraqx.h"
#include <windows.h>

#define IS_DUAL_POL 1	// Uncomment to enable the IQ data FIFOs to the Dual Pol products  
						// generation and TCP server for MGEN dual pol processing.
#define IQ_CHANNELS			2	// Horizontal and Vertical channel.
#define SAMPLES_PER_GATE	2	// One I float + one Q float per channel.

typedef struct MYDATA
{	bool	isHorzElseVert;	// True for Horizontal polarizarion, False for Vertical.
	LPTSTR	ppiscopePipeName;	// pipe name from PPI Scope.
	LPTSTR	ascopePipeName;		// pipe name from AScope.
	HANDLE	hPpiscopePipe;	// pipe handle from PPI Scope.
	HANDLE	hAscopePipe;	// pipe handle from AScope.
	int		boardNumber;	// HiQ board number.
    char	*src_fifo;		// Source fifo name.
	float	*abp;			// storage ptr.
	HANDLE	thread;			// Handle to thread doing this work, for closing.
	FIFO	*dstfifo;		// Destination FIFO.
#ifdef IS_DUAL_POL
	// The section below is only used in the dual pol products generation thread case.
	FIFO	*dstfifoIq;	// Destination FIFO for IQ output, only used in the abplib generation of each dual pol IQ fifo case.
    char	*src_fifo_dp_prod;	// Dual Pol products source fifo name.
	FIFO	*dstfifoDp;	// Destination FIFO for Dual Pol products generated from both dual pol IQ fifos.
    char	*src_fifo_ch0;	// Channel 0 source fifo name.
    char	*src_fifo_ch1;	// Channel 1 source fifo name. (for Dual pol vert IQ data, otherwise not used.)
	CFG     *cfg;			// Handle to the config struct.
#endif
} mydata;

//#endif
