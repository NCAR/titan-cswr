//#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#pragma once

/****************************************************/
/* Structure for each IQDATA record                 */
/****************************************************/
typedef	float	float4;
typedef	unsigned __int32	uint4;
typedef	unsigned __int64	uint8;

// Care must be taken with invisible byte padding
// for compatibility with other systems

#pragma pack(4) // set 4-byte alignment of structures for compatibility w/destination computers
typedef struct {
   uint8	pulsenum;
   uint4	gates;
   uint4	pulsewidth;  /* tells system how many unit4's of g0data preceed IQ data (bit 0 indicates gate0mode) */
   uint4	eofstatus;
   uint4	configid;

   float4	az;	// 0.0 to 359.99 degrees.
   float4	el;	// 0.0 to 359.99 degrees.
   uint4	scan_type;	// scan type.
   uint4	scan_num;	// scan number.
   uint4	vol_num;	// volume number.
   float4	fxd_angle;	// fixed angle.
   uint4	transition;	// transition flag.

   uint8	secs;		// UTC time.
   uint8	nanosecs;	// UTC time.

   int		gatezero; /* this will be the *start* of iqdata.  MUST BE THE LAST ELEMENT OF THE STRUCTURE */
   } IQRECORD;
#pragma pack(8) // return to default 8-byte alignment of structures 

/* this is an index into the location of the EOF for each IQRECORD */
//#define	EOFVALUE(a)	(a)->iqdata[2*(a)->gates+(a)->g0size]


