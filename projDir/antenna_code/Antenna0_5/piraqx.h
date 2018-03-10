/************************************************************************/
/*  This file defines the PIRAQX structure                              */
/*  Ideally, the display and recording routines can use this file       */
/*  and none of the headers that include more run-time specific defines */
/************************************************************************/
;	/*compiler happiness*/
#pragma once
//#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/ TODO: remove?
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
#include <time.h>

#ifndef PIRAQX_H	// TODO: pragma once could replace this
#define PIRAQX_H


typedef	float	float4;
typedef	unsigned __int32	uint4;
typedef	unsigned __int64	uint8;
typedef	signed   __int32	int4;

#define	M_PI		3.141592654
#define K2			0.93
#define C_CONSTANT	2.99792458E8
#define TWOPI		6.283185307
#define HIQ_LAST_IF	25.0e+6			// The final intermediate frequency on the HiQ board, 25 MHz.
#define MAX_V_GATE_OFFSET 100

#ifndef MAXGATES
#define	MAXGATES	1000
#endif 
/****************************************************/
/* First, define the various data formats           */
/****************************************************/
#define DATA_SIMPLEPP   0 /* simple pulse pair ABP */
#define DATA_POLYPP     1 /* poly pulse pair ABPAB */
#define DATA_DUALPP     2 /* dual prt pulse pair ABP,ABP */
#define DATA_POL1       3 /* dual polarization pulse pair ABP,ABP */
#define DATA_POL2       4 /* more complex dual polarization ??????? */
#define DATA_POL3       5 /* almost full dual polarization with log integers */
#define DATA_SIMPLE16   6 /* 16 bit magnitude/angle representation of ABP */
#define DATA_DOW        7 /* dow data format */
#define DATA_FULLPOL1   8 /* 1998 full dual polarization matrix for alternating H-V */
#define DATA_FULLPOLP   9 /* 1999 full dual polarization matrix plus for alternating H-V */
#define DATA_MAXPOL    10 /* 2000 full dual polarization matrix plus for alternating H-V */
#define DATA_HVSIMUL   11 /* 2000 copolar matrix for simultaneous H-V */
#define DATA_SHRTPUL   12 /* 2000 full dual pol matrix w/gate averaging no Clutter Filter */
#define DATA_SMHVSIM   13 /* 2000 DOW4 copolar matrix for simultaneous H-V (no iq average) */
#define	DATA_STOKES	   14 /* NEC Stokes parameters */
#define	DATA_FFT	   15 /* NEC FFT mean velocity and power */
#define	PIRAQ_ABPDATA  16 /* ABP data computed in piraq3: rapidDOW project */
#define	PIRAQ_ABPDATA_STAGGER_PRT 17 /* Staggered PRT ABP data computed in piraq3: rapidDOW project */
#define DATA_TYPE_MAX PIRAQ_ABPDATA_STAGGER_PRT /* limit of data types */ 

/* define the bits of the status flag */
#define	CTRL_CLUTTERFILTER		0x00000001
#define	CTRL_TIMESERIES			0x00000002
#define	CTRL_PHASECORRECT		0x00000004
#define	CTRL_SCAN_TRANSITION	0x00000008
#define	CTRL_TXON				0x00000010
#define	CTRL_SECONDTRIP			0x00000020
#define	CTRL_VELOCITYFOLD		0x00000040
#define	CTRL_CALMODE			0x00000080
#define	CTRL_CALSIMPLE			0x00000100
#define CTRL_OBSERVING          0x00000200
#define	CTRL_AFC_LOCK			0x00000400
#define	CTRL_AFC_FLAG			0x00000800
#define	CTRL_DISPLAY_FLAG		0x00030000	// 0=>none, 01=>Ascope, 10=>PPIscope, 11=>Both
#define CTRL_HARDWARE_PULSENUM	0x00040000	// 1 = get pulsenum lower 16 bits from counter.  For syncing multiple systems

// AFC
#define	STAT_AFC_LSB			0x00001000	
#define	STAT_AFC_MSB			0x00002000
/*												+------- STATUS_AFC_MSB
												|  +---- STATUS_AFC_LSB
												|  |           Meaning
*/
#define AFC_STATE_DISABLED		"Disabled"	//	0  0 = 0  All AFC functions are disabled.
#define AFC_STATE_ENABLED		"Enabled"	//	0  1 = 1  All AFC functions are enabled, and scan is inactive, not locked.     
#define AFC_STATE_SEARCHING		"Searching"	//	1  0 = 2  An AFC search scan is in process, but Lock has not been achieved.
#define AFC_STATE_LOCKED		"Locked"	//	1  1 = 3  Lock has been achieved.  An AFC search scan is not in process. 

// Display startup flags
#define	ENABLE_ASCOPE			0x00010000	// Enable Ascope engineering display
#define	ENABLE_PPISCOPE			0x00020000	// Enable PPIscope engineering display	

// ADD MORE FLAGS HERE.

// Incoming data is Remote / Card mode
#define	DATA_SOURCE_LSB			0x4000000
#define	DATA_SOURCE_MSB			0x8000000
/*									+------- DATA_SOURCE_MSB
									|  +---- DATA_SOURCE_LSB
									|  |           Meaning
									|  |						*/
enum {
	DATA_SOURCE_CARD = 0,		//	0  0 = 0  Data is coming from an on-board receiver card.
	DATA_SOURCE_REMOTE_IQ,		//	0  1 = 1  Data is from a UDP/TCP FIFO in IQ format.     
	DATA_SOURCE_REMOTE_ABP,		//	1  0 = 2  Data is from a UDP/TCP FIFO in ABP format.  
	DATA_SOURCE_REMOTE_PROD		//	1  1 = 3  Data is from a UDP/TCP FIFO in PRODUCTS format.  
};

// For multi-polarization systems, each channel is identified.
typedef enum channel_polarization_mode {
	CHANNEL_POLARIZATION_HORIZONTAL = 0,	// Horizontal
	CHANNEL_POLARIZATION_VERTICAL   = 1,	// Vertical
} channel_polarization_t;

// For multi-polarization systems, the system type is translated to in iwrf_data.h, iwrf_xmit_rcv_mode.
#define	SYSTEM_POLARIZATION_HORIZONTAL			"H"		// Horizontal
#define	SYSTEM_POLARIZATION_VERTICAL			"V"		// Vertical
#define	SYSTEM_POLARIZATION_P45_DUALPOL			"+45"	// +45 Dual Pol
#define	SYSTEM_POLARIZATION_M45_DUALPOL			"-45"	// -45 Dual Pol
#define	SYSTEM_POLARIZATION_RIGHTHAND_CIRCULAR	"RC"		// Right Hand Circular
#define	SYSTEM_POLARIZATION_LEFTHAND_CIRCULAR	"LC"		// Left Hand Circular

/*************************************************************/
/* completely define radar operation with a single structure */
/*************************************************************/
#define	PIRAQX_CURRENT_REVISION 3

#define	MIN_CFG_VERSION	6
#define	MAX_CFG_VERSION	10

typedef struct {
		/* all elements start on 4-byte boundaries
         * 8-byte elements start on 8-byte boundaries
         * character arrays that are a multiple of 4
         * are welcome
         */
/* 
The first int in any potential FIFO record should be a size
That way recording and ethernet functions can deal with them and all future revisions
including totally new record types in a uniform way.  MAR 2/24/05
Therefore, I switched the order of the first two 32bit entities.
*/
	uint4	recordlen;        /* total length of record - must be the second field */
    char	desc[4];			/* "DWLX" */
    uint4	channel;          /* e.g., RapidDOW range 0-5 */
    uint4	rev;		        /* format revision #-from RADAR structure */
    uint4	one;			    /* always set to the value 1 (endian flag) */
    uint4	byte_offset_to_data;
    uint4	dataformat;

    uint4	typeof_compression;
/*
Pulsenumber (pulse_num) is the number of transmitted pulses
since Jan 1970. It is a 64 bit number. It is assumed
that the first pulse (pulsenumber = 0) falls exactly
at the midnight Jan 1,1970 epoch. To get unix time,
multiply by the PRT. The PRT is a rational number a/b.
More specifically N/Fc where Fc is the counter clock (PIRAQ_CLOCK_FREQ),
and N is the divider number. So you can get unix time
without roundoff error by:
secs = pulsenumber * N / Fc. The
computations is done with 64 bit arithmetic. No
rollover will occur.

The 'nanosecs' field is derived without roundoff
error by: 100 * (pulsenumber * N % Fc).

Beamnumber is the number of beams since Jan 1,1970.
The first beam (beamnumber = 0) was completed exactly
at the epoch. beamnumber = pulsenumber / hits. 
*/
    uint8	pulse_num;   /*  keep this field on an 8 byte boundary */
    uint8	beam_num;	/*  keep this field on an 8 byte boundary */
    uint4	gates;
    uint4	start_gate;
    uint4	hits;
/* additional fields: simplify current integration */
    uint4	ctrlflags; /* equivalent to packetflag below?  */
    uint4	bytespergate; 
    float4	rcvr_pulsewidth;
#define PX_NUM_PRT 4
    float4	prt[PX_NUM_PRT];	// {prt1, prt2}
    float4	meters_to_first_gate;  

    uint4	num_segments;  /* how many segments are we using */
#define PX_MAX_SEGMENTS 8
    float4	gate_spacing_meters[PX_MAX_SEGMENTS];
    uint4	gates_in_segment[PX_MAX_SEGMENTS]; /* how many gates in this segment */

#define PX_NUM_CLUTTER_REGIONS 4
    uint4	clutter_start[PX_NUM_CLUTTER_REGIONS]; /* start gate of clutter filtered region */
    uint4	clutter_end[PX_NUM_CLUTTER_REGIONS];  /* end gate of clutter filtered region */
    uint4	clutter_type[PX_NUM_CLUTTER_REGIONS]; /* type of clutter filtering applied */

#define PIRAQ_CLOCK_FREQ 10000000  /* 10 Mhz */

/* following fields are computed from pulse_num by host */
    uint8	secs;     /* Unix standard - seconds since 1/1/1970
                       = pulse_num * N / ClockFrequency */
    uint4	nanosecs;  /* within this second */
    float4	az;   /* azimuth: referenced to 9550 MHz. possibily modified to be relative to true North. */
    float4	az_off_ref;   /* azimuth offset off reference */ 
    float4	el;		/* elevation: referenced to 9550 MHz.  */ 
    float4	el_off_ref;   /* elevation offset off reference */ 

    float4	radar_longitude;
    float4	radar_latitude;
    float4	radar_altitude;
#define PX_MAX_GPS_DATUM 8
    char	gps_datum[PX_MAX_GPS_DATUM]; /* e.g. "NAD27" */
    
    uint4	ts_start_gate;   /* starting time series gate , set to 0 for none */
    uint4	ts_end_gate;     /* ending time series gate , set to 0 for none */
    
    float4	ew_velocity;

    float4	ns_velocity;
    float4	vert_velocity;

    float4	fxd_angle;		/* in degrees instead of counts */
    float4	true_scan_rate;	/* degrees/second */
    uint4	scan_type;
    uint4	scan_num;
    uint4	vol_num;
    uint4	transition;

    float4	yaw;
    float4	pitch;
    float4	roll;
    float4	track;
    float4	h_gate0mag;  /* magnetron sample amplitude */
    float4	v_gate0mag;
    uint4	packetflag; 

    /*
    items from the deprecated radar "RHDR" header
    do not set "radar->recordlen"
    */

    uint4	year;             /* e.g. 2003 */
    uint4	julian_day;
    
#define PX_MAX_RADAR_NAME 16
    char	radar_name[PX_MAX_RADAR_NAME];
#define PX_MAX_CHANNEL_NAME 16
    char	channel_name[PX_MAX_CHANNEL_NAME];
#define PX_MAX_PROJECT_NAME 16
    char	project_name[PX_MAX_PROJECT_NAME];
#define PX_MAX_OPERATOR_NAME 12
    char	operator_name[PX_MAX_OPERATOR_NAME];
#define PX_MAX_SITE_NAME 12
    char	site_name[PX_MAX_SITE_NAME];
#define CFG_MAX_IP_ADX_LENGTH 15
#define CFG_MAX_HOST_NAME_LEN 128
 
#define PX_MAX_POLARIZATION	4
    char	system_polarization[PX_MAX_POLARIZATION];	// H, V, +45, -45, RC, LC, etc.--3 character limit. H, V are used in DOS
	float4	h_test_pulse_pwr;
	float4	v_test_pulse_pwr;
	float4	h_test_pulse_frq;
    float4	v_test_pulse_frq;
	float4	h_test_pulse_coupler;
    float4	v_test_pulse_coupler;
    float4	frequency;		// Radar transmit frequency
	float4	stalofreq;		// Radar local oscillator frequency

    float4	noise_figure;
    float4	h_noise_power;
    float4	h_receiver_gain;
	float4	v_noise_power;	// Vertical noise power
	float4	v_receiver_gain;	// Vertical receiver gain
    float4	h_E_plane_angle;	// offsets from normal pointing angle
    float4	h_H_plane_angle;
    float4	v_E_plane_angle;	// offsets from normal pointing angle
    float4	v_H_plane_angle;
 
    float4	data_sys_sat;
    float4	h_antenna_gain;
	float4	v_antenna_gain;	// Vertical antenna gain
    float4	h_beam_width;
    float4	v_beam_width;

    float4	xmit_pulsewidth;
    float4	h_rconst;		// Horizontal channel radar constant
	float4	v_rconst;		// Vertical channel radar constant
    float4	phaseoffset;

    float4	zdr_accumulated_error_correction;

    float4	h_mismatch_loss;
	float4	v_mismatch_loss;
    float4	rcvr_const;

    float4	test_pulse_rngs_km[2];
    float4	antenna_rotation_angle;   /* S-Pol 2nd frequency antenna may be 30 degrees off vertical */
    
#define PX_SZ_COMMENT 64
    char comment[PX_SZ_COMMENT];
    float4	i_norm;  /* normalization for timeseries */
    float4	q_norm;
    float4	i_compand;		// companding (compression) parameters 
    float4	q_compand;
    float4	transform_matrix[2][2][2];
    float4	stokes[4]; 
    
    float4	h_xmit_power;
	float4	v_xmit_power;	
	float4	h_measured_xmit_power; // Measured horizontal peak transmitted power
	float4	v_measured_xmit_power; // Measured vertical peak transmitted power

	uint4	angle_source;	// See Angle source definitions.
	uint4	timingmode;		// Added for compatability with config.dsp. Spare[15]->spare[14]
	float4	tpwidth;		// trigger width as in DOS code but expressed in seconds
	float4	tpdelay;		// trigger delay as in DOS code but expressed in seconds
	float4	delay;			// delay as in DOS code	but expressed in seconds	
	float4	pllfreq;		// Frequency command to LO expressed in Hz.
	float4	pllalpha;		// AFC time constant 0 < pllalpha < 1. Averaging time ~1./(1. - pllalpha)
	float4	afclo;			// AFC parameters: typical afclo = 95e6  Hz
	float4	afchi;			// typical afchi = 120e6 Hz
	float4	afcgain;		// typical afcgain = 3.0
	float4	velsign;		// +/- 1 to reverse sense of velocities, if necessary. Value depends on radar characteristics.
	float4  h_xmit_coupler;		// Calibrates G0 vs measured xmit power. Changes same sign as power reading. 
	float4  v_xmit_coupler;		// Calibrates G0 vs measured xmit power. Changes same sign as power reading. 
	uint4	channel_polarization;	// for multi-channel systems, use CHANNEL_POLARIZATION enum..
	float4  spare[1];		/* this used to be 20 spare floats */
	int4	v_gate_offset;	// gate shift for vertical channel.
/* always append new items so the alignment of legacy variables won't change */
} PIRAQX;

#define	DATASIZE(a)		((int)a->data.info.ts_start_gate < 0)?(a->data.info.gates * a->data.info.bytespergate):((a->data.info.gates * a->data.info.bytespergate)+(a->data.info.hits * 4 * sizeof(float)) * ((a->data.info.ts_end_gate - a->data.info.ts_start_gate)+1)) 
#define	RECORDLEN(a)		(sizeof(INFOHEADER) + (DATASIZE(a)))
#define	TOTALSIZE(a)		(sizeof(COMMAND) + (RECORDLEN(a)))

#endif	// PX_SZ_COMMENT 64

// Control information from the configuration file.
typedef struct {
	char	card_0[4];
	char	card_1[4];
	int		cfgVersion;
	int		data_source;
	int		remote_port;
	int		angle_source;	
	bool	restart;
	int     restart_delay_secs; // Restart if errors persist for this period.
	char	gps_source_ip[16];	
	char	angle_host_name[CFG_MAX_HOST_NAME_LEN];
	int		angle_tcpip_port;	
} CFG;

#ifdef __cplusplus
}
#endif
