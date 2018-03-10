//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>

#include "code_controls.h"
//#include "wdc_lib.h"
//#include "hiq_device.h"
//#include "smfifo.h"
//#include "hiqop.h"
//#include "readpiraqx.h"
//#include "piraqx.h"
//#include "hiq.h"		/* HIQ struct plus status reg bits */
//#include "hiqdrvr.h"
//#include "hiqutil.h"
//#include "udpsock.h"
#include "iqdata.h"	/* IQ record definitions */
//#include "drvwrapper.h"	/* hiq driver defines */
//#include "mydata.h"		// for IS_DUAL_POL #ifdef
//#include "hiq_hardware.h"



void getUtcTime(uint8 *p_secs, uint8 *p_nanosecs)
{
	SYSTEMTIME systemTime;
	FILETIME fileTime;
	ULONGLONG qw, nanos;

	GetSystemTime(&systemTime);

	SystemTimeToFileTime( &systemTime, &fileTime );
	qw = (((ULONGLONG)fileTime.dwHighDateTime)<<32) + fileTime.dwLowDateTime;

	nanos = (qw - 116444736000000000ui64);

	(*p_secs) = (uint8)(time_t)(nanos / 10000000ui64);
	(*p_nanosecs) = (uint8)100*(nanos % 10000000ui64);	// Remainder of nanoseconds, but only accurate to 1 msec.
}


