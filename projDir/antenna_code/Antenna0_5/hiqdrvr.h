#pragma once

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the HIQDRVR_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// HIQDRVR_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef HIQDRVR_EXPORTS
#define HIQDRVR_API __declspec(dllexport)
#else
#define HIQDRVR_API __declspec(dllimport)
#endif

#include "smfifo.h"
#include "hiqop.h"
#include "piraqx.h"
#include "iqdata.h"
#include "drvwrapper.h"
#include "hiq_lib.h"

// exposed methods
HIQDRVR_API int fnhiqdrvr(void);
HIQDRVR_API FIFO *HIQ_open(int whichhiq, char* aFifoName, int gates, int records, PIRAQX *px, PHIQ_DEV_CTX pHiqDevCtx);
HIQDRVR_API void HIQ_close(int whichhiq, FIFO *hiq);
HIQDRVR_API void HIQ_set(FIFO *hiq, HIQOP *op, PHIQ_DEV_CTX pHiqDevCtx);
HIQDRVR_API HIQOP *hiqvalid(FIFO *hiq);
HIQDRVR_API int HIQ_start(FIFO *hiq, unsigned long long pulsenum);
HIQDRVR_API void HIQ_stop(FIFO *hiq);
HIQDRVR_API int HIQ_do_afc(int whichhiq, PIRAQX *p, bool slaveMode);
HIQDRVR_API int HIQ_get_Int_Cnt(int whichhiq);
HIQDRVR_API int HIQ_get_Int_Err_Cnt( int whichhiq );
HIQDRVR_API bool HIQ_is_In_Sync( int whichhiq );
HIQDRVR_API int HIQ_pll_adjust(int whichhiq, double freqStep, bool increaseFreq);
HIQDRVR_API void HIQ_timer(int timernum,int timermode,int count,int whichhiq); 
HIQDRVR_API	int HIQ_timerset(FIFO *hiq);
HIQDRVR_API int HIQ_timerset(FIFO *hiq, bool slaveMode);
HIQDRVR_API	void HIQ_start_internal(int whichhiq);
HIQDRVR_API	void HIQ_halt(int whichhiq);
HIQDRVR_API void HIQ_ReprogramDelay(float4 aDelay, FIFO *srcfifo);
HIQDRVR_API void HIQ_setAngleSource(int whichhiq, int anAngleSource);
HIQDRVR_API int HIQ_getAngleSource(int whichhiq);
HIQDRVR_API void HIQ_get_angles(IQRECORD *r, int angle_source, unsigned short *ctr);
HIQDRVR_API HIQ_INT_DATA *getHiqIntData(int whichhiq);

// Utility function not directly called in the WinDriver HiQ driver DLL interface.
void HIQ_init_op(FIFO *hiq, int whichhiq);
int HIQ_rxfreq(FIFO *hiq, double freq);
