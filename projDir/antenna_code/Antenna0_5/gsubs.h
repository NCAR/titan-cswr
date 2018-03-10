#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#pragma once

#include "display.h"

//#include <windows.h>
#define WIN32_LEAN_AND_MEAN

extern HDC g_hdc;
extern HDC plane[];	// PPI display planes.
extern DWORD *g_dibBits[PLANE_TOTAL];
extern int DataYtop;		// PPI polar display top.
extern int DataYbot;		// PPI polar display bottom.
extern int DataXleft;		// left side of PPI display (radraw).
extern int DataXright;		// right side of PPI display (radraw).
extern int Xscreen;
extern int Yscreen;

// Define FAST_DRAWING to enable drawing pixels directly to memory. They must be converted manually
// to BGR format, since that is what the DIB uses behind the scenes. 
#define FAST_DRAWING
#ifdef FAST_DRAWING
	#define BGR(color)(RGB(GetBValue(color),GetGValue(color),GetRValue(color)))
	#define FAST_PLOT(plane,x,y,c) if(x >= 0 && y >= 0 && x < Xscreen && y < Yscreen){g_dibBits[plane][y * Xscreen + x] = BGR(c);}
	#define DATA_PLOT(plane,x,y,c) if(x >= DataXleft && y >= DataYtop && x < DataXright && y < DataYbot){g_dibBits[plane][y * Xscreen + x] = (DWORD)BGR(c);}
	#define GET_PIXEL(plane,x,y) ((x >= 0 && y >= 0 && x < Xscreen && y < Yscreen) ? BGR((g_dibBits[plane][y * Xscreen + x])) : 0)
#else
	#define FAST_PLOT(plane,x,y,c) (plot(plane,x,y,c))
	#define DATA_PLOT(plane,x,y,c) (plot(plane,x,y,c))
	#define GET_PIXEL(plane,x,y) (unplot(plane,x,y))
#endif


int getDataXc(void);
int getDataYc(void);
HDC getPpiHdc(void);
void setupScreen(void);
void gmode(HWND hwnd);
void renderPpi(int which);
void renderoverlay(int num);
void killglib();
void tmode(void);
void plot(int which, int x,int y,DWORD c);
int unplot(int which, int x,int y);
void gclearPlane(int which);
void gclearPlaneColor(int which, DWORD color);
void setclip(int which, int x,int y,int x2,int y2);
void setColorInPlane(int which, DWORD color);
DWORD getcolor(void);
void glibinit(HWND hwnd);
void move(int which, int x,int y);
void draw(int which, int x,int y);
void grputs(int which, int x,int y,const char *string);
void grprintf(int which, int x,int y,char *fmt,...);
void grputs_center(int which, int x,int y,const char *string);
void grprintf_center(int which, int x,int y,char *fmt,...);
void line(int which, int x0, int y0, int x1, int y1, int color);
void rectfill(int which, int x0, int y0, int x1, int y1, int color);
void textout(int which, char *string, int x, int y, int color);
void circle(int which, int x, int y, int r, int color);
bool isMouseOverPpiDisplay( void );
void ellipse(int which, int x, int y, int halfwidth, int halfheight, int color);

