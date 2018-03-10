#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#pragma once
#ifndef PRODUCTS_H
#define PRODUCTS_H

#include "smfifo.h"
#include "mydata.h"

#ifdef IS_DUAL_POL 
MYDATA *product_open(char *src_fifo, char* src_fifo_dp, char *dst_fifo, bool isHorzElseVert);
#else
MYDATA *product_open(char *src_fifo,                    char *dst_fifo, bool isHorzElseVert);
#endif
DWORD WINAPI ProductThreadProc( LPVOID lpParam );
void productworker(MYDATA *pData);
void product_close(MYDATA *pData);

#ifdef IS_DUAL_POL
	void newsimplepp(float *pptr, short *aptr, PIRAQX *px, float *dpptr, MYDATA *p);
	void dualprt(float *pptr, float *aptr, PIRAQX *px, float *dpptr, MYDATA *p);
#else
	void newsimplepp(float *pptr, short *aptr, PIRAQX *px);
	void dualprt(float *pptr, float *aptr, PIRAQX *px);
#endif

#endif	// PRODUCTS_H
