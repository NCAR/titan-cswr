
//#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#pragma once
#include "iqdata.h"
/****************************************************/
// Angle source definitions
/****************************************************/
#define ANGLE_SRC_FAKE				0	// internally generated binary angles.
#define	ANGLE_SRC_PARALLEL_BIN_ISA	1	// CIO-DIO48H ISA parallel card, binary format.
#define	ANGLE_SRC_PARALLEL_BCD_ISA  2	// CIO-DIO48H ISA parallel card, BCD format.
#define	ANGLE_SRC_PMAC_ISA			3	// ISA PMAC card, binary format.
#define	ANGLE_SRC_PMAC_PCI			4	// PCI PMAC card, binary format.
#define	ANGLE_SRC_ETHERNET_BOSCH	5	// Angles from Bosch controller via Shared Mem. 

#define	ANGLE_SRC_FILE				7	// Read angles from a file a la CRN
#define	ANGLE_SRC_COM_1				8	// Serial angle data via COM1
#define	ANGLE_SRC_COM_2				9	// Serial angle data via COM2

#define	ANGLE_SRC_PARALLEL_BIN_PCI	11	// CPCI-DIO48H PCI parallel card, binary format.
#define	ANGLE_SRC_PARALLEL_BCD_PCI	12	// CPCI-DIO48H PCI parallel card, BCD format.

#define ANGLE_SRC_FAKE_INCR			0.01	// fake angle step size (degrees)


//#define CSWR		// Define this to use hardware counter for pulse number generation

//#define AZELSWAP	// Defining AZELSWAP swaps azimuth and elevation

// Methods
void get_angles(IQRECORD *r, int angle_source, unsigned short *ctr);
void angles_bin(IQRECORD *r, int angle_source, unsigned short *ctr);
void angles_bcd(IQRECORD *r, int angle_source, unsigned short *ctr);
void angles_enet_bosch(IQRECORD *r, int angle_source, unsigned short *ctr);
void angles_serial(IQRECORD *r, int angle_source, unsigned short *port);
void angles_file(IQRECORD *r, int angle_source, unsigned short *port);
void pmac_angles_dpram(IQRECORD *r);

void strip_cnr_angles(char *a, float b[]);

//void interp_angles(IQRECORD *r);
bool hasParallelBoardSoftware(void);
void setNeedPmacSetup(void);