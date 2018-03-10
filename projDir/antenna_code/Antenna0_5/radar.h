/****************************************************/
/* various defines for system operation             */
/* Ideally, this file does not contain parameters   */
/* that relate to hardware.                         */
/****************************************************/
#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#pragma once
//#include <windows.h>

/****************************************************/
/* universal defines                                */
/****************************************************/
#define MAXHITS			1024
#define	DATA_TIMEOUT	1
#define	IQDATA_FIFO_NUM		450 

#ifndef MAXGATES
#define	MAXGATES		2000  
#endif

typedef	float	float4;
typedef	unsigned __int32	uint4;
typedef	unsigned __int64	uint8;
//typedef struct  {float x,y;}   complex;

#define K2      0.93
#define C_CONSTANT 2.99792458E8
#define M_PI    3.141592654 
#define TWOPI   6.283185307

/****************************************************/
/* now use this infrastructure to define the records in the */
/* various types of FIFO's used for interprocess communication */
/****************************************************/

//#define	DATASIZE(a)		(a->data.info.numgates * a->data.info.bytespergate)
// rapidDOW: data block contains ABPDATA for each gate and diagnostic IQDATA for each hit: 
// tsgate >= 0 switches on IQDATA term. length is then gates*6*sizeof(float) + hits*4*sizeof(short)
// note ts_start_gate recast to signed quantity. 
//#define	DATASIZE(a)		((int)a->data.info.ts_start_gate < 0)?(a->data.info.gates * a->data.info.bytespergate):((a->data.info.gates * a->data.info.bytespergate)+(a->data.info.hits * 4 * sizeof(short)))
#define	DATASIZE(a)		((int)a->data.info.ts_start_gate < 0)?(a->data.info.gates * a->data.info.bytespergate):((a->data.info.gates * a->data.info.bytespergate)+(a->data.info.hits * 4 * sizeof(float)) * ((a->data.info.ts_end_gate - a->data.info.ts_start_gate)+1)) 
#define	RECORDLEN(a)		(sizeof(INFOHEADER) + (DATASIZE(a)))
#define	TOTALSIZE(a)		(sizeof(COMMAND) + (RECORDLEN(a)))

#define	SET(t,b)			(t |= (b))
#define	CLR(t,b)			(t &= ~(b))

#define DEG2RAD (M_PI/180.0)

#define DIO_OUT	 0
#define DIO_IN	 1

/****************************************************/
/* now use this infrastructure to define the records in the */
/* various types of FIFO's used for interprocess communication */
/****************************************************/

//#define	HEADERSIZE		sizeof(PACKETHEADER)
//#define	IQSIZE			(sizeof(short) * 4 * MAXGATES) 
//#define	ABPSIZE			(sizeof(float) * 12 * MAXGATES)
//#define DIAGNOSTICSIZE	2*(sizeof(float))*2*10*256	// 2*(sizeof(float))*2channels*10gates*256hits = 160*256 = 40960 

//#define	DATASIZE(a)		(a->data.info.numgates * a->data.info.bytespergate)
// rapidDOW: data block contains ABPDATA for each gate and diagnostic IQDATA for each hit: 
// tsgate >= 0 switches on IQDATA term. length is then gates*6*sizeof(float) + hits*4*sizeof(short)
// note ts_start_gate recast to signed quantity. 

//#define	DATASIZE(a)		((int)a->data.info.ts_start_gate < 0)?(a->data.info.gates * a->data.info.bytespergate):((a->data.info.gates * a->data.info.bytespergate)+(a->data.info.hits * 4 * sizeof(short)))
//#define	DATASIZE(a)		((int)a->data.info.ts_start_gate < 0)?(a->data.info.gates * a->data.info.bytespergate):((a->data.info.gates * a->data.info.bytespergate)+(a->data.info.hits * 4 * sizeof(float)) * ((a->data.info.ts_end_gate - a->data.info.ts_start_gate)+1)) 
//#define	RECORDLEN(a)		(sizeof(INFOHEADER) + (DATASIZE(a)))
//#define	TOTALSIZE(a)		(sizeof(COMMAND) + (RECORDLEN(a)))

#define	SET(t,b)			(t |= (b))
#define	CLR(t,b)			(t &= ~(b))

#define DEG2RAD (M_PI/180.0)


/* This structure defines overall processing for a group of HIQ's */
/* The system should be able to determine how to stop and start and otherwise */
/* control and configure the HIQ cards using this structure. */
/* The covariance processing algorithms should use this to figure out how to use */
/* the various HIQ data to generate ABP and time series data. */
/* I can see this structure being the most likely thing to change if new capability */
/* is needed later - very dependent on radar architecture. */
typedef	struct
	{
	int				dataformat,hits;
	int				gatesa, gatesb, rcvr_pulsewidth, timeseries, gate0mode, startgate;
	int				prt, timingmode, delay, sync;
	int				ts_start_gate, ts_end_gate, pcorrect, clutterfilter, xmit_pulsewidth; /* xmit_pulsewidth used for discrim only */
	int				clutter_start, clutter_end; 
	int				prt2, velsign, hitcount;
	float			afcgain, afchi, afclo, locktime;
	int				trigger, testpulse, tpwidth, tpdelay, intenable;
	float			stalofreq;
	signed char     int_phase;
	float           int_dac;
	unsigned int    int_sync, int_period;
	unsigned int    ethernet;  /* the ethernet address */
	int				boardnum;  /* which of n cards are we refering to */
	float			meters_to_first_gate; 
	float			gate_spacing_meters;
	float			xmitcoupler;
	int				configid;		/* unique identifier for this config at this time */
	} HIQPROCESS;
	

//	signed char		int_phase;
//	float           int_dac;
//	unsigned int    int_sync,int_period;


/* header for each dwell describing parameters which might change dwell by dwell */
/* this structure will appear on tape before each abp set */
#pragma pack(push,1)	// save, do not insert bytes
typedef struct  {
		char            desc[4];
		short           recordlen;
		short           gates,hits;
		float           rcvr_pulsewidth,prt,delay; /* delay to first gate */
		char            clutterfilter,timeseries;
		short			tsgate;
		time_t			time;      /* seconds since 1970 */
		unsigned short	subsec;    /* fractional seconds (.1 mS) */
		float           az,el;
		float           radar_longitude; 
		float           radar_latitude;
		float           radar_altitude;
		float           ew_velocity;
		float           ns_velocity;
		float           vert_velocity;
		char            dataformat;     /* 0 = abp, 1 = abpab (poly), 2 = abpab (dual prt) */
		float           prt2;
		float           fxd_angle;
		unsigned char   scan_type;
		unsigned char   scan_num;
		unsigned char   vol_num;
		unsigned int    ray_count;
		char            transition;
		float           hxmit_power;    /* on the fly hor power */
		float           vxmit_power;    /* on the fly ver power */
		float           yaw;            /* platform heading in deg */
		float           pitch;          /* platform pitch in deg */
		float           roll;           /* platform roll in deg */
		float           h_gate0mag;       /* magnetron h sample amplitude in dB rel to sat */
		float           v_gate0mag;       /* same for v */
		char		packetflag;
		char            spare[79];
		} HEADER; /* do not insert bytes */
#pragma pack(pop)	// default


/* Dwell structure gets recorded for each dwell (a angular interval of data)*/
#pragma pack(push,1)	// save, do not insert bytes
typedef struct  {                
		HEADER          header;
		short           abp[MAXGATES * 10]; /* a,b,p + time series */
		} DWELL;
#pragma pack(pop)	// default


// definition of several different data formats 
#define DATA_SIMPLEPP   0 // simple pulse pair ABP 
#define DATA_POLYPP     1 // poly pulse pair ABPAB 
#define DATA_DUALPP     2 // dual prt pulse pair ABP,ABP 
#define DATA_POL1       3 // dual polarization pulse pair ABP,ABP 
#define DATA_POL2       4 // more complex dual polarization ??????? 
#define DATA_POL3       5 // almost full dual polarization with log integers 
#define DATA_SIMPLE16   6 // 16 bit magnitude/angle representation of ABP 
#define DATA_DOW        7 // dow data format 
#define DATA_FULLPOL1   8 // 1998 full dual polarization matrix for alternating H-V 
#define DATA_FULLPOLP   9 // 1999 full dual polarization matrix plus for alternating H-V 
#define DATA_MAXPOL    10 // 2000 full dual polarization matrix plus for alternating H-V 
#define DATA_HVSIMUL   11 // 2000 copolar matrix for simultaneous H-V 
#define DATA_SHRTPUL   12 // 2000 full dual pol matrix w/gate averaging no Clutter Filter 
#define DATA_SMHVSIM   13 // 2000 DOW4 copolar matrix for simultaneous H-V (no iq average) 


#ifdef CRAPOLA
/* structure gets filled from a file to specify default radar parameters */
typedef struct  {
		char    desc[4];
		short   recordlen;
		short   rev;
		short   year;           /* this is also in the dwell as sec from 1970 */
		char    radar_name[PX_MAX_RADAR_NAME];
		char    polarization;   /* H or V */
		float   test_pulse_pwr; /* power of test pulse (refered to antenna flange) */
		float   test_pulse_frq; /* test pulse frequency */
		float   frequency;      /* transmit frequency */
		float   peak_power;     /* typical xmit power (at antenna flange) read from config.rdr file */
		float   noise_figure;
		float   noise_power;    /* for subtracting from data */
		float   receiver_gain;  /* hor chan gain from antenna flange to VIRAQ input */
		float   data_sys_sat;   /* VIRAQ input power required for full scale */
		float   antenna_gain;
		float   horz_beam_width;
		float   vert_beam_width;
		float   xmit_pulsewidth; /* transmitted pulse width */
		float   rconst;         /* radar constant */
		float   phaseoffset;    /* offset for phi dp */
		float   vreceiver_gain; /* ver chan gain from antenna flange to VIRAQ */
		float   vtest_pulse_pwr; /* ver test pulse power refered to antenna flange */
		float   vantenna_gain;  /* vertical antenna gain */
		float   vnoise_power;   /* for subtracting from data */
		float   zdr_accumulated_error_correction; /* what else? */
		float   missmatch_loss;   /* receiver missmatch loss (positive number) */
		float	E_plane_angle;   /* E field plane angle */ 
		float	H_plane_angle;   /* H field plane angle */ 
	    float	antenna_rotation_angle;   /* S-Pol 2nd frequency antenna may be 30 degrees off vertical */
		float	latitude; 
		float	longitude;
		float	altitude;
		float	i_offset;		/* dc offset to remove from I data; currently stored in data.info.spare[0] for use py piraq */ 
		float	q_offset;		/* dc offset to remove from Q data; currently stored in data.info.spare[0] for use py piraq */ 
		float   misc[3];        /* 3 more misc floats */
		char	channel_name[PX_MAX_CHANNEL_NAME];
	    char	project_name[PX_MAX_PROJECT_NAME];
		char	operator_name[PX_MAX_OPERATOR_NAME];
	    char	site_name[PX_MAX_SITE_NAME];
		char    text[960];
		} RADAR;

/* OLD structure to completely define the operation of the HIQ */
/* and data system at the lowest level */
typedef struct
	{
	int				gates,hits,pulsewidth,timeseries,gate0mode;
	int				prt,timingmode,delay,sync;
	int				tsgate,pcorrect,clutterfilter,xmitpulse; /* xmitpulse used for discrim only */
	int				prt2,velsign,hitcount;
	char			outfilename[80],outpath[80];
	float			afcgain,afchi,afclo,locktime;
	int				trigger,tpwidth,tpdelay,intenable;
	float			stalofreq;
	signed char     int_phase;
	float           int_dac;
	unsigned int    int_sync,int_period;
	unsigned int    ethernet;  /* the ethernet address */
	int				dataformat;
	int				boardnum;  /* which of n cards are we refering to */
	float			xmitcoupler;
	} CONFIG;




/* I don't understand the purpose of this yet MAR 2/13/05 */

/* UNIX standard file#s */ 

#ifndef STDIN_FILENO
#define STDIN_FILENO	0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO	1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO	2
#endif

#endif