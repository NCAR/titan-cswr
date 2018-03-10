//#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <io.h>
#include <math.h>
#include "pmac.h"
#include "angles.h"
#include "cbw.h"
//#include "arcLogger.h"
//#include "../include/pmac.h"

/*
using namespace log4cxx;
using namespace log4cxx::helpers;
*/
//#define USE_CPP	// enable here and in pmac.h

//static LoggerPtr logger;
unsigned short dcounter = 0;
static int initangles = 1;
static float fakeangle = 0.0;
static bool	needPmacSetup = true;	// flag to control running PMAC initialization only once.
#ifdef USE_CPP
	static CpmacApp pmac;
#endif


void setNeedPmacSetup( void )
{
	needPmacSetup = true;
}

/**
 * Read angles from the PMAC dual-port RAM using TvicHW32
 */
void pmac_angles_dpram(IQRECORD *r)
{
	float bin2deg = 0.0054932f;		// 360 degrees/(2**16)
	static unsigned short *pmacDpram = NULL;  // Avoid PCI lookup on each call
	if(pmacDpram == NULL) pmacDpram =  get_pmac_dpram();
	
	r->az  =  bin2deg*(float)pmacDpram[0]; 	
	r->el  =  bin2deg*(float)pmacDpram[1];
	r->scan_type	= (uint4)pmacDpram[2];
	r->scan_num		= (uint4)pmacDpram[3];
	r->vol_num		= (uint4)pmacDpram[4];
	r->fxd_angle	= bin2deg*(float)pmacDpram[5];
	r->transition	= (uint4)pmacDpram[6];
}

/****************************** GET_ANGLES ******************************************
obtains floating point angles via the routine specified by
p->angle_source as defined in angles.h.
*/

//Uncomment for Pulse-Wise Angle Dump, also in next function.
//FILE* bigdump = 0;
static int aoctr = 50;
static int firstwrite = 1;

void get_angles(IQRECORD *r, int angle_source, unsigned short *pChanC)
{
	//AFX_MANAGE_STATE(AfxGetStaticModuleState());	FILE *afp;
	char *aefname;
	FILE *aefp;
	int aemodulus = 100;
	
	static float fAZ1=0.0, fAZ2=0.0, fAZ3=0.0;
	static float fEL1=0.0, fEL2=0.0, fEL3=0.0;
	static int num_calls = 0;
	
	switch(angle_source)
	{
	case ANGLE_SRC_PARALLEL_BIN_ISA:
	case ANGLE_SRC_PARALLEL_BIN_PCI:
		angles_bin(r,angle_source, pChanC);	// Only grab counter data on first call	
		fAZ1 = r->az; fEL1 = r->el;
		angles_bin(r,angle_source, &dcounter);
		fAZ2 = r->az; fEL2 = r->el;		
		angles_bin(r,angle_source, &dcounter);	
		fAZ3 = r->az; fEL3 = r->el;
		break;

	case ANGLE_SRC_PARALLEL_BCD_ISA:
	case ANGLE_SRC_PARALLEL_BCD_PCI:
		angles_bcd(r,angle_source, pChanC);	// Only grab counter data on first call
		fAZ1 = r->az; fEL1 = r->el;
		angles_bcd(r,angle_source, &dcounter);
		fAZ2 = r->az; fEL2 = r->el;		
		angles_bcd(r,angle_source, &dcounter);	
		fAZ3 = r->az; fEL3 = r->el;			
		break;

	case ANGLE_SRC_PMAC_ISA:
	case ANGLE_SRC_PMAC_PCI:
#ifdef USE_CPP
		if(needPmacSetup)
		{
			pmac = CpmacApp();		// ctor handles PMAC setup.
			needPmacSetup = false;
		}
		pmac.angles_pmac(r);
		fAZ1 = r->az; fEL1 = r->el;
		pmac.angles_pmac(r);
		fAZ2 = r->az; fEL2 = r->el;		
		pmac.angles_pmac(r);
		fAZ3 = r->az; fEL3 = r->el;
#else
		pmac_angles_dpram(r);
		fAZ3 = r->az;
		fEL3 = r->el;

#endif // USE_CPP
		break;

	case ANGLE_SRC_ETHERNET_BOSCH:
		angles_enet_bosch(r, angle_source, &dcounter);
		fAZ3 = r->az; fEL3 = r->el;
		break;
	case ANGLE_SRC_COM_1:
		angles_serial(r, angle_source, pChanC);
		fAZ3 = r->az; fEL3 = r->el;
		break;

	case ANGLE_SRC_COM_2:
		angles_serial(r, angle_source, pChanC);
		fAZ3 = r->az; fEL3 = r->el;
		break;

	case ANGLE_SRC_FILE:
		angles_file(r, angle_source, pChanC);
		if(r->az > -500)	
		{
			// If file didn't exist, az < 500.
//			fAZ1 = fAZ2 = fAZ3 = r->az; 	// New angles if file exists
//			fEL1 = fEL2 = fEL3 = r->el;

			if(r->az < 0.0 && r->az > -180.0)r->az += 360;
//			if(fabs(fAZ3 - r->az) > 1.0)r->az = fAZ3;
			fAZ3 = r->az; 	// New angles if file exists

			if(r->el < 0.2)r->el = fEL3;
		}
//		printf("                    \r%f, %f\r", fAZ3, fEL3);
		break;

	case ANGLE_SRC_FAKE:
		fakeangle -= (float)ANGLE_SRC_FAKE_INCR;
		if(fakeangle <= 0.0)fakeangle = 360.0;
		fAZ1 = fAZ2 = fAZ3 = fakeangle;
		fEL1 = fEL2 = fEL3 = fakeangle;
//		r->pulsenum++;				// Simulates the 16 bit hardware counter
//		r->pulsenum &= 0xFFFF;	
		break;

	default:
		MessageBox(NULL,TEXT("Invalid .cfg file value for 'angle_source'.  Exiting."), TEXT("Invalid Angle Source"), MB_OK | MB_ICONERROR);
		exit(-999);
	}

	if(angle_source == ANGLE_SRC_FILE)return;	// Don't do the lazy median filter for file angles

	if(num_calls  == 0) { // First two times punt and use the latest angles.
		 fAZ2 = fAZ1 = fAZ3; 
		 fEL2 = fEL1 = fEL3; 
	} else if( num_calls == 1) {
		 fAZ2 = fAZ3; 
		 fEL2 = fEL3; 
	}
	num_calls ++;

	// Jon's lazy median filter
	if ((fAZ1<=fAZ2)&&(fAZ2<=fAZ3)) r->az = fAZ2;
	if ((fAZ1<=fAZ3)&&(fAZ3<=fAZ2)) r->az = fAZ3;
	if ((fAZ2<=fAZ1)&&(fAZ1<=fAZ3)) r->az = fAZ1;
	if ((fAZ2<=fAZ3)&&(fAZ3<=fAZ1)) r->az = fAZ3;
	if ((fAZ3<=fAZ1)&&(fAZ1<=fAZ2)) r->az = fAZ1;
	if ((fAZ3<=fAZ2)&&(fAZ2<=fAZ1)) r->az = fAZ2;
	if ((fEL1<=fEL2)&&(fEL2<=fEL3)) r->el = fEL2;
	if ((fEL1<=fEL3)&&(fEL3<=fEL2)) r->el = fEL3;
	if ((fEL2<=fEL1)&&(fEL1<=fEL3)) r->el = fEL1;
	if ((fEL2<=fEL3)&&(fEL3<=fEL1)) r->el = fEL3;
	if ((fEL3<=fEL1)&&(fEL1<=fEL2)) r->el = fEL1;
	if ((fEL3<=fEL2)&&(fEL2<=fEL1)) r->el = fEL2;

	fAZ1 = fAZ2; // Keep track of previous angles.
	fAZ2 = fAZ3;
	fEL1 = fEL2; // Keep track of previous angles.
	fEL2 = fEL3;

	if(r->az < 0.0)	// Convert +/-180 into 0.00-359.99.
	{
		r->az += 360.0;
	}
	
	// Write floating az el to desti
	aefname = getenv("ANGLE_DEST");
	aemodulus = atoi(getenv("AE_MODULUS"));
	
	if(firstwrite == 1)
	{
		aoctr = aemodulus/2;		// Set write counter to half modulus
		firstwrite = 0;
	}

	if(aoctr++ % aemodulus != 0)return;

	if((aefp = fopen(aefname, "w")) != NULL)
	{
		fprintf(aefp, " %6.2f  %6.2f\n", r->az, r->el);
//		printf("fAZ %6.2f fEL %6.2f\r", r->az, r->el);
		fclose(aefp);
	}
	return;
}


/******************************	ANGLES_BIN	******************************************
This is an adaptation of the DOS code written by Jon Lutz for ingesting binary data
via a Measurement Computing (MC) CIO-DIO48 card.  This version makes use of MC Universal
Library calls supplied for Windows XP.
*/
void angles_bin(IQRECORD *ang, int angle_source, unsigned short *pChanC)
{	
	/* Variable Declarations */
	int BoardNum = 0;
	int ULStat = 0;
	//int PortNum; 
	int Direction;
	int Zero = 0;
	int One = 1;
	unsigned int iAZ, iEL;
	//unsigned int iAZH, iAZL, iELH, iELL;
	int pAZH, pAZL, pELH, pELL, pPNH1, pPNL1, pPNH2, pPNL2;
	WORD DataH, DataL, DataH2, DataL2;
	float    RevLevel = (float)CURRENTREVNUM;
	float	fAZ, fEL, bin2deg = 0.0054932f;

#ifndef AZELSWAP	// This is normal
	pAZH = FIRSTPORTA;
	pAZL = FIRSTPORTB;
	pELH = SECONDPORTA;
	pELL = SECONDPORTB;
#else			// This is abnormal
	pAZH = SECONDPORTA;
	pAZL = SECONDPORTB;
	pELH = FIRSTPORTA;
	pELL = FIRSTPORTB;
#endif
	// This doesn't change
	pPNH1 = FIRSTPORTCH;
	pPNL1 = FIRSTPORTCL;
	pPNH2 = SECONDPORTCH;
	pPNL2 = SECONDPORTCL;
	// Get correct BoardNum
	if(angle_source > 10)BoardNum = 1;

	/* Declare UL Revision Level */
	ULStat = cbDeclareRevision(&RevLevel);

	if(initangles == 1)
	{
		if( ! hasParallelBoardSoftware() ) { exit(-999); }
		/* Initiate error handling
		Parameters:
		PRINTALL :all warnings and errors encountered will be printed
		DONTSTOP :program will continue even if error occurs.
		Note that STOPALL and STOPFATAL are only effective in 
		Windows applications, not Console applications. 
		*/
		cbErrHandling (PRINTALL, DONTSTOP);

		/* Configure FIRSTPORTA,B and SECONDPORTA,B for input
		Parameters:
		BoardNum    :the number used by CB.CFG to describe this board.
		PortNum     :the input port
		Direction   :sets the port for input or output 
		*/
		Direction = DIGITALIN;
		ULStat = cbDConfigPort (BoardNum, pAZL, Direction);
		ULStat = cbDConfigPort (BoardNum, pAZH, Direction);
		ULStat = cbDConfigPort (BoardNum, pELL, Direction);
		ULStat = cbDConfigPort (BoardNum, pELH, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNL1, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNH1, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNL2, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNH2, Direction);

		initangles = 0;
	}

	/* Read 4 8-bit ports and convert to floats
	Parameters:
	BoardNum    :the number used by CB.CFG to describe this board
	PortNum     :the input port
	DataValue   :the value read from the port   */
	ULStat = cbDIn(BoardNum, pAZH, &DataH);
	ULStat = cbDIn(BoardNum, pAZL, &DataL);
	iAZ  = ((DataH & 0xFF)<<8) | (DataL & 0xFF);
	ULStat = cbDIn(BoardNum, pELH, &DataH);
	ULStat = cbDIn(BoardNum, pELL, &DataL);
	iEL  = ((DataH & 0xFF)<<8) | (DataL & 0xFF);
	fAZ = bin2deg*(float)iAZ; fEL = bin2deg*(float)iEL;
#define DEBUG_P_BIN
#ifdef DEBUG_P_BIN

	printf("azHex 0x%04X, elHex 0x%04X\r", iAZ, iEL);

#endif
	ang->az = fAZ; ang->el = fEL;
	// Pick up extra channel
	ULStat = cbDIn(BoardNum, pPNH1, &DataH);
	ULStat = cbDIn(BoardNum, pPNL1, &DataL);
	ULStat = cbDIn(BoardNum, pPNH2, &DataH2);
	ULStat = cbDIn(BoardNum, pPNL2, &DataL2);

	*pChanC = (DataH2 & 0xF)<<12 | (DataL2 & 0xF)<<8 | (DataH & 0xF)<<4 | (DataL & 0xF);		// Build pulsenumber

	return;	
}



/******************************	ANGLES_BCD ******************************************
angles_bcd is the same as angles_bin except that the input format expected is BCD.
*************************************************************************************/
void angles_bcd(IQRECORD *ang, int angle_source, unsigned short *pChanC)
{	
	/* Variable Declarations */
	int BoardNum;
	int ULStat = 0;
	//int PortNum;
	int Direction;
	int Zero = 0;
	int One = 1;
	int pAZH, pAZL, pELH, pELL, pPNH1, pPNL1, pPNH2, pPNL2;
	WORD DataH, DataL, DataH2, DataL2;
	float    RevLevel = (float)CURRENTREVNUM;
	float	fAZ, fEL, bin2deg = 0.0054932f;

	pAZH = FIRSTPORTA;
	pAZL = FIRSTPORTB;
	pELH = SECONDPORTA;
	pELL = SECONDPORTB;
	pPNH1 = FIRSTPORTCH;
	pPNL1 = FIRSTPORTCL;
	pPNH2 = SECONDPORTCH;
	pPNL2 = SECONDPORTCL;

	// Get correct BoardNum
	if(angle_source > 10)
	{
		BoardNum = 1;
	}


	/* Declare UL Revision Level */
	ULStat = cbDeclareRevision(&RevLevel);
	if(initangles == 1)
	{
		if( ! hasParallelBoardSoftware() ) { exit(-999); }
		/* Initiate error handling
		Parameters:
		PRINTALL :all warnings and errors encountered will be printed
		DONTSTOP :program will continue even if error occurs.
		Note that STOPALL and STOPFATAL are only effective in 
		Windows applications, not Console applications. 
		*/
		cbErrHandling (PRINTALL, DONTSTOP);

		/* Configure FIRSTPORTA,B and SECONDPORTA,B for input
		Parameters:
		BoardNum    :the number used by CB.CFG to describe this board.
		PortNum     :the input port
		Direction   :sets the port for input or output 
		*/
		Direction = DIGITALIN;
		ULStat = cbDConfigPort (BoardNum, pAZL, Direction);
		ULStat = cbDConfigPort (BoardNum, pAZH, Direction);
		ULStat = cbDConfigPort (BoardNum, pELL, Direction);
		ULStat = cbDConfigPort (BoardNum, pELH, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNL1, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNH1, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNL2, Direction);
		ULStat = cbDConfigPort (BoardNum, pPNH2, Direction);
		initangles = 0;
	}

	/* Read 6 8-bit ports 
	Parameters:
	BoardNum    :the number used by CB.CFG to describe this board
	PortNum     :the input port
	DataValue   :the value read from the port   
	*/
	ULStat = cbDIn(BoardNum, pAZH, &DataH);
	ULStat = cbDIn(BoardNum, pAZL, &DataL);
	fAZ  = (100.0f*(float)(DataH & 0x30)/16.0f) +
		(10.0f*(float)(DataH & 0x0F)) +
		(1.0f*(float)(DataL & 0xF0)/16.0f) + 
		(0.1f*(float)(DataL & 0x0F));

	ULStat = cbDIn(BoardNum, pELH, &DataH);
	ULStat = cbDIn(BoardNum, pELL, &DataL);
	fEL  = (100.0f*(float)(DataH & 0x30)/16.0f) +
		(10.0f*(float)(DataH & 0x0F)) +
		(1.0f*(float)(DataL & 0xF0)/16.0f) + 
		(0.1f*(float)(DataL & 0x0F));

	// Pick up extra channel
	ULStat = cbDIn(BoardNum, pPNH1, &DataH);
	ULStat = cbDIn(BoardNum, pPNL1, &DataL);
	ULStat = cbDIn(BoardNum, pPNH2, &DataH2);
	ULStat = cbDIn(BoardNum, pPNL2, &DataL2);

	*pChanC = (DataH2 & 0xF)<<12 | (DataL2 & 0xF)<<8 | (DataH & 0xF)<<4 | (DataL & 0xF);		// Build pulsenumber

	ang->az = fAZ; ang->el = fEL;
	//	printf("AZ = %10.2f, EL = %10.2f\r", fAZ, fEL);
	//	printf ("\n");
	return;	
}
/***********************************************************************
void angles_serial(IQRECORD *r, int angle_source, unsigned short *port);
	receives angles via serial port 1 or 2.  
	angle_source = 5=> com1.
	angle_source = 6=> com2.
***********************************************************************/

/*****WARNING: This routine has never been fully implemented or debugged*****************/

#define DATA_READY 0x100
#define SETTINGS ( 0x80 | 0x02 | 0x00 | 0x00)
#define COM1	5
/* COM1 0x3F8 */
/* COM2 0x2F8 */

static char dbuf[20];
static int	cbindex = 0;

void angles_serial(IQRECORD *r, int angle_source, unsigned short *port)
{
	int in, out, status;  
	char c = 0x0;
//	c = in(0x3f8);
	dbuf[cbindex++%20] = c;
	printf("%c",c);
	
	return;
}
/***************************************************************
angles_file reads angles from file azel.dat
***************************************************************/
static int aictr = 1;
static float laz = 0.0, lel = 0.0;
static int firsttime = 1;

void angles_file(IQRECORD *ang, int angle_source, unsigned short *other)
{
	FILE *afp;
	char angbuf[40], caz[6], cel[6], *fname;
	int	aemodulus = 100;
	float diffa, diffe, angles[2];

	fname = getenv("ANGLE_SRC");
	aemodulus = atoi(getenv("AE_MODULUS"));

	if(aictr++ % aemodulus != 0)
	{
		ang->az = laz; ang->el = lel;
		return;
	}
	
	if(((afp = fopen(fname, "r")) == NULL))
	{
		ang->az = laz; ang->el = lel;
		return;
	}
	
	fread(angbuf, 1, 20, afp);
	strip_cnr_angles(angbuf, angles);
	ang->az = angles[0]; ang->el = angles[1];
	
	if(firsttime)
	{
		laz = ang->az; lel = ang->az;
		firsttime = 0;
		diffa = 0; diffe = 0;
		return;
	}
	else
	{
		diffa = ang->az - laz; diffe = ang->el - lel;
	}

	if((fabs(ang->az) < .0001 && fabs(ang->el) < .0001))// || fabs(diffa) > 2.0 || fabs(diffe) > 2.0)
	{
		ang->az = laz;
		ang->el = lel;
	}
	else
	{
		laz = ang->az;		// Save the last angles read
		lel = ang->el;
	}
//	printf("                                             \rAZ %6.2f, dAZ %6.2f, EL %6.2f, dEL %6.2f\r",ang->az, diffa, ang->el, diffe);
	fclose(afp);				// Close the file
	unlink(fname);				// Delete the file after reading
	return;
}


void strip_cnr_angles(char *abuf, float angles[])
{
	char ca[6] = {0}, ce[6] = {0};
	int i = 1, j = 0, k = 0;

	for(j = 0; j < 6; j++)
	{
		ca[j] = abuf[j+1];
		ce[j] = abuf[j+9];
	}
	angles[0] = atof(ca);
	angles[1] = atof(ce);

//	printf("%6.2f %6.2f\n", angles[0], angles[1]);
	return;
}

/***************************************************************
angles_enet_bosch gets angles via Named Shared memory segment
***************************************************************/

#include "BoschAngleData.h"

void angles_enet_bosch(IQRECORD *r, int angle_source, unsigned short *other)
{
   HANDLE hMapFile;
   static LPCTSTR pBuf = NULL;
   Bosch_ACU_data_t *Acu;
   int wait_count = 0;
   long last_line_number = 0;
   if(pBuf == NULL) {  // first time through
	   hMapFile = OpenFileMapping(
					   FILE_MAP_ALL_ACCESS,   // read/write access
					   FALSE,                 // do not inherit the name
					   SHM_Name);             // name of mapping object 
	 
	   if (hMapFile == NULL) { 
		   printf("Angle Data Shared Memory not present. Error:%d\n", GetLastError());
		 return;
	   } 
	 
	   pBuf = (LPTSTR) MapViewOfFile(hMapFile, // handle to map object
				   FILE_MAP_ALL_ACCESS,  // read/write permission
				   0,                    
				   0,                    
				   MBUF_SIZE);
	}
    Acu = (Bosch_ACU_data_t *) pBuf;

	r->az  = Acu->az_position; 	
	r->el  =  Acu->el_position;
	
	r->scan_num		= Acu->tilt_number;
	r->vol_num		= Acu->volume_number;
	switch(Acu->scan_type) {
		case PPI_SCAN:
			r->scan_type = 8;
			r->fxd_angle =Acu->el_target;
		break;

		case PPI_SECTOR_SCAN:
			r->scan_type = 1;
			r->fxd_angle =Acu->el_target;
		break;

		case PPI_VOLUME_SCAN:
			r->scan_type = 8;
			r->fxd_angle =Acu->el_target;
		break;

		case RHI_SCAN:
			r->scan_type = 3;
			r->fxd_angle =Acu->az_target;
		break;

		default:
			r->scan_type = 7;  // Idle
			r->fxd_angle =Acu->az_target;
		break;

	}
	r->transition = Acu->transition_flag;
	return;
}
bool hasParallelBoardSoftware( void )
{
	struct _finddata_t c_file;
	intptr_t hFile;

	// if the software install directory exists, 
	char	*cfgFile = "C:/Program Files/Measurement Computing/DAQ/CB.CFG";
	char	*msg = "The Measurement Computing software has not been installed in this system.  A parallel BINARY or BCD 'angle_source' setting in the .cfg file is invalid.";
	if( (hFile = _findfirst( cfgFile, &c_file )) == -1L )
	{
		MessageBox(NULL, TEXT(msg), TEXT("Bad .cfg file Angle Source?"), MB_OK | MB_ICONSTOP);
//		LOG4CXX_FATAL(logger, TEXT(msg));
		printf( "%s\nPress any key to exit...", msg );
		do{} while( ! _kbhit() );
		return false;
	}
	else
	{
		return true;
	}
}


static  float  AZangles[256],ELangles[256];
static  float  AZrate=0.0,ELrate=0.0,AZold=-1.0,ELold=-1.0,AZbeam=0,ELbeam=0;
static  int    AZcount=0,ELcount=0;
static  unsigned char   AngleIndex = 0;
static  int     Firstangle=0;

/******************************	interp_angles ******************************************
interp_angles interpolates az and el in case the angle is asyncronously time sampled 
*/
/*
void interp_angles(IQRECORD *pa)
{
	double       az,el,delta;

	az = (double)pa->az;             // Retrieve angles from system
	el = (double)pa->el;

	if(!Firstangle)      // initialize the array and various variables
	{
		for(; Firstangle<256; Firstangle++) 
			AZangles[Firstangle] = ELangles[Firstangle] = 21.0;
		AZold = az;  
		ELold = el; 
		AZbeam = ELbeam = pa->beam_num;  
	}

	// Interpolation takes place in the following code
	if(AZold != az)      // get AZ rate if possible
	{
		delta = az - AZold;
		if(delta < -180.0) delta += 360;
		else if(delta > 180.0) delta -= 360;
		AZrate = 0.5 * AZrate + 0.5 * delta / (pa->beam_num - AZbeam);
		AZbeam = pa->beam_num;
		AZcount = 0;
	}

	if(AZold != az)      // get EL rate if possible
	{
		delta = az - ELold;
		if(delta < -180.0) delta += 360;
		else if(delta > 180.0) delta -= 360;
		ELrate = 0.5 * ELrate + 0.5 * delta / (pa->beam_num - ELbeam);
		ELbeam = pa->beam_num;
		ELcount = 0;
	}

	AZangles[AngleIndex] = az + AZrate * AZcount++;
	if(AZangles[AngleIndex] > 360.0)     AZangles[AngleIndex] -= 360.0;
	else if(AZangles[AngleIndex] < 0.0)  AZangles[AngleIndex] += 360.0;
	ELangles[AngleIndex] = el + ELrate * ELcount++;
	if(ELangles[AngleIndex] > 360.0)     ELangles[AngleIndex] -= 360.0;
	else if(ELangles[AngleIndex] < 0.0)  ELangles[AngleIndex] += 360.0;

	// use angle that corresponds to data comming out of DSP FIFO
	//   pa->az = AZangles[(AngleIndex - (char)Beamdelay) & 0xFF];
	//   pa->el = ELangles[(AngleIndex - (char)Beamdelay) & 0xFF];

	AngleIndex++;        // will roll over at 256.
}
*/
