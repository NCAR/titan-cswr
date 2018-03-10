#include "stdafx.h"
//#include	<windows.h>
#include	<math.h>
#include	<string.h>
#include	<stdio.h>   // this is just for the printf call

#include "code_controls.h"
#include "port_types.h"
#include "piraqx.h"
#include "smfifo.h"
#include "mydata.h"
#include "iqdata.h"
#include "readpiraqx.h"
#include "dpproducts.h"
#include "productdef.h"
#include "swversion.h"
#include "iwrf_data.h"
#include "iwrf_functions.h"
#include "piraqx2Iwrf.h"
#include "dpProducts.h"

// Log4cxx
#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <apr_general.h>
#include <apr_time.h>
#include <iostream>
#include <log4cxx/stream.h>
#include <exception>
#include <stdlib.h>

#include "arcLogger.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

// CONSOLE display
#define CONSOLE false

#define	ADFULLSCALE	8192.0
#define	BITSHIFT	16384.0
#define	TWOPI		6.283185307
#define	MAXNAME		80
#define	STARTGATE	20
#define	SMALL		0.0183156389 // TODO: Find out where in the world Mitch got this number! From the DOS version.
									// Turns out that ln(SMALL) = -4.  Again, where did it come from?

#define GAUSSIAN	4.4*2.5	// Correction to noise power for dual PRT

static LoggerPtr logger;
static int Abort = 0;	// TODO: might be ok to leave this as a GLOBAL but it could be included in MYDATA

// Send the true packets.	Normally	true	false.			
#define	SEND_SYNC_PKT					true
#define	SEND_RADAR_INFO_PKT				true
#define SEND_SCAN_SEGMENT_PKT			true
#define	SEND_ANTENNA_CORRECTION_PKT				false
#define	SEND_TS_PROCESSING_PKT			true
#define	SEND_XMIT_POWER_PKT			true
#define	SEND_XMIT_SAMPLE_PKT					false
#define	SEND_CALIBRATION_PKT			false
#define SEND_EVENT_NOTICE_PKT			false
#define SEND_PHASECODE_PKT						false
#define	SEND_XMIT_INFO_PKT						false
// PULSE_HEADER_PKT with each beam of data.

#define MAX_TS_FIFO_GATES 2048 // limited by 16KB  HIQ Hardware FIFO - (I+Q)*4096gates/pulse * 4 pulses = 16KB
#define TS_BUF_SIZE 24576  // 24k 
#define TS_FIFO_RECORDS 4096  // 96Mb Buffer  - Approx 12 seconds of data 


#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE		// write packets out when encountered.  WARNING: File size is NOT limited!
FILE *p_f_pkt_hdrs_dp;	// for writing headers to a file.
#endif
#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_PULSE_AND_SEQ_TO_FILE	// write packets out when encountered.  WARNING: File size is NOT limited!
FILE *p_f_seq_dp;	// for writing seq#s to a file.
#endif
/*
*/
MYDATA *dpProduct_open(char *src_fifo_ch0, char *src_fifo_ch1, char *dp_prod_dst_fifo, char *iq_dst_fifo)
{
	char	*self = "dpProduct_open";
	MYDATA *pData;
#ifdef IS_DUAL_POL
	DWORD dwThreadId;

	Abort = 0;	// global flag.

	// allocate storage for structure returned from this thread for the close operation.
	pData = (MYDATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,sizeof(MYDATA));
	if( pData == NULL )
	{
		LOG4CXX_FATAL(logger, "MYDATA allocate failed.");
		ExitProcess(2); 
	}

	// Open the dual pol Products destination fifo.
	pData->dstfifo = fifo_alloc(dp_prod_dst_fifo, sizeof(PIRAQX), sizeof(PIRAQX)+MAXGATES*24, 100);
	if(!pData->dstfifo)	
	{
		printf("%s:Bad destination dual pol products fifo, pData->dstfifo! Exiting.\n",self); 
		LOG4CXX_FATAL(logger, "Bad destination fifo, pData->dstfifo.");
		exit(0);
	}

	// Calculate maximum header size
	int maxSizeOfPacket = 0;
	if( SEND_SYNC_PKT )
		maxSizeOfPacket = (sizeof(iwrf_sync_t)				> maxSizeOfPacket) ? sizeof(iwrf_sync_t)          : maxSizeOfPacket;	// Synchronization.
	if( SEND_RADAR_INFO_PKT )
		maxSizeOfPacket = (sizeof(iwrf_radar_info_t)		> maxSizeOfPacket) ? sizeof(iwrf_radar_info_t)    : maxSizeOfPacket;	// Radar info.
	if( SEND_SCAN_SEGMENT_PKT )
		maxSizeOfPacket = (sizeof(iwrf_scan_segment_t)		> maxSizeOfPacket) ? sizeof(iwrf_scan_segment_t)  : maxSizeOfPacket;	// Scan Segment.
	if( SEND_ANTENNA_CORRECTION_PKT )
		maxSizeOfPacket = (sizeof(iwrf_antenna_correction_t)> maxSizeOfPacket) ? sizeof(iwrf_antenna_correction_t) : maxSizeOfPacket;// Antenna Correction.
	if( SEND_TS_PROCESSING_PKT )
		maxSizeOfPacket = (sizeof(iwrf_ts_processing_t)		> maxSizeOfPacket) ? sizeof(iwrf_ts_processing_t) : maxSizeOfPacket;	// TS processing.
	if( SEND_XMIT_POWER_PKT )
		maxSizeOfPacket = (sizeof(iwrf_xmit_power_t)		> maxSizeOfPacket) ? sizeof(iwrf_xmit_power_t)    : maxSizeOfPacket;	// Xmit Power.
	if( SEND_XMIT_SAMPLE_PKT )
		maxSizeOfPacket = (sizeof(iwrf_xmit_sample_t)		> maxSizeOfPacket) ? sizeof(iwrf_xmit_sample_t)   : maxSizeOfPacket;	// Xmit sample.
	if( SEND_CALIBRATION_PKT )
		maxSizeOfPacket = (sizeof(iwrf_calibration_t)		> maxSizeOfPacket) ? sizeof(iwrf_calibration_t)   : maxSizeOfPacket;	// Calibration.
	if( SEND_EVENT_NOTICE_PKT )
		maxSizeOfPacket = (sizeof(iwrf_event_notice_t)		> maxSizeOfPacket) ? sizeof(iwrf_event_notice_t)  : maxSizeOfPacket;	// Event Notice.
	if( SEND_PHASECODE_PKT )
		maxSizeOfPacket = (sizeof(iwrf_phasecode_t)			> maxSizeOfPacket) ? sizeof(iwrf_phasecode_t)     : maxSizeOfPacket;	// Phase Code.
	if( SEND_XMIT_INFO_PKT )
		maxSizeOfPacket = (sizeof(iwrf_xmit_info_t)			> maxSizeOfPacket) ? sizeof(iwrf_xmit_info_t)     : maxSizeOfPacket;	// Xmit Info.
	
	// Pulse Header packet with IQ payload.
	maxSizeOfPacket =	(
							sizeof(iwrf_pulse_header_t) + (IQ_CHANNELS * MAX_TS_FIFO_GATES * SAMPLES_PER_GATE * sizeof(float) )
							> maxSizeOfPacket
						) ? 
							sizeof(iwrf_pulse_header_t) + (IQ_CHANNELS * MAX_TS_FIFO_GATES * SAMPLES_PER_GATE * sizeof(float) )
						: maxSizeOfPacket;	// Data Packet hdr + payload.

	// Open the MGEN destination fifo.
	pData->dstfifoIq = fifo_alloc(	iq_dst_fifo,	// Fifo name.
					16,	// This fifo does not carry info in the user header field  - 16 bytes puts the internal FIFO record offset at 128 bytes
					maxSizeOfPacket, TS_FIFO_RECORDS);	
	if(!pData->dstfifoIq)	
	{
		printf("%s:Bad destination fifo, pData->dstfifoIq! Exiting.\n",self); 
		exit(0);
	}

	// Generate unique data for each thread.
	pData->src_fifo_ch0 = src_fifo_ch0;			// ptr to source fifo name used in worker thread
	pData->src_fifo_ch1 = src_fifo_ch1;			// ptr to source fifo name used in worker thread
	pData->isHorzElseVert = false;				// polarization, unused.
	pData->thread = CreateThread( 
								NULL,			// default security attributes
								0,				// use default stack size  
								DualPolProductThreadProc,	// thread function 
								pData,			// argument to thread function 
								0,				// use default creation flags 
								&dwThreadId		// returns the thread identifier
								);

	// Check the return value for success. 
	if (pData->thread == NULL) 
	{
		LOG4CXX_FATAL(logger, "CreateThread failed.");
		ExitProcess(0);
	}
	else
	{
		// We want this only to be higher than other tasks, like PpiScope(ABOVE_NORMAL and everything non-radar(NORMAL).
		// Note that the actual priority is a combination of process(class) priority AND thread priority. 
		SetThreadPriority(pData->thread,THREAD_PRIORITY_ABOVE_NORMAL);
		//SetThreadPriority(pData->thread,THREAD_PRIORITY_LOWEST);
		//SetThreadPriority(pData->thread,THREAD_PRIORITY_BELOW_NORMAL);
		//SetThreadPriority(pData->thread,THREAD_PRIORITY_HIGHEST);
		//SetThreadPriority(pData->thread,THREAD_PRIORITY_TIME_CRITICAL);
	}

#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE		// write packets out when encountered.  WARNING: File size is NOT limited!
		// Write packet to file.
		//p_f_pkt_hdrs_dp = fopen("c:\\data\\pkt.txt","w+");		// for one packets set per pulse.
		p_f_pkt_hdrs_dp = fopen("c:\\data\\pkt.txt","a+");		// for all packets.
#endif
#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_PULSE_AND_SEQ_TO_FILE	// write packets out when encountered.  WARNING: File size is NOT limited!
		// Write packet to file.
		//p_f_seq_dp = fopen("c:\\data\\seq_dp.txt","w+");		// for one packets set per pulse.
		p_f_seq_dp = fopen("c:\\data\\seq_dp.txt","a+");		// for all packets.
#endif
#endif	// IS_DUAL_POL
	return(pData);		// for the close operation.
}


void dpProduct_close(MYDATA *pData)
{
#ifdef IS_DUAL_POL
	
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
	fclose(p_f_pkt_hdrs_dp);	// Close print packet output file.
#endif
#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_PULSE_AND_SEQ_TO_FILE
	fclose(p_f_seq_dp);	// Close print packet seq # to output file.
#endif
	Abort = 1;	// global flag.
	WaitForSingleObject(pData->thread,10000);	// wait 10 seconds for thread to stop
	CloseHandle(pData->thread);
	fifo_close(pData->dstfifo);
	// IQ data fifo
	fifo_close(pData->dstfifoIq);
	LOG4CXX_INFO(logger, "Closed Dual Polarization product thread.");
#endif
}


DWORD WINAPI DualPolProductThreadProc( LPVOID lpParam ) 
{ 
	HANDLE hStdout;
	MYDATA *pData;

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	if( hStdout == INVALID_HANDLE_VALUE )
	{
        LOG4CXX_FATAL(logger, "Invalid stdout handle.");
		return 1;
	}

	// Cast the parameter to the correct data type.
	pData = (MYDATA*)lpParam;
	dpProductWorker(pData);
	return(0);
}

// IWFR Packet send frequencies.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
	#define	PULSES_PER_SYNC_PKT					1
	#define	PULSES_PER_RADAR_INFO_PKT			2
	#define PULSES_PER_SCAN_SEGMENT_PKT			3
	#define	PULSES_PER_ANTENNA_CORRECTION_PKT	4
	#define	PULSES_PER_TS_PROCESSING_PKT		5
	#define	PULSES_PER_XMIT_POWER_PKT			6
	#define	PULSES_PER_XMIT_SAMPLE_PKT			7
	#define	PULSES_PER_CALIBRATION_PKT			8
	#define PULSES_PER_EVENT_NOTICE_PKT			9
	#define PULSES_PER_PHASECODE_PKT			10
	#define	PULSES_PER_XMIT_INFO_PKT			11
	#define MAX_PACKET_TYPE						11
	// PULSE_HEADER_PKT with each beam of data.

#else	// DEBUG_SEND_ALL_IWRF_PKTS
	// This is the actual frequency of packet transmission, by packets.
	#define	PULSES_PER_SYNC_PKT					1000
	#define	PULSES_PER_RADAR_INFO_PKT			1001
	// SCAN_SEGMENT _PKT per volume, as passed via DPR from AntCtlr.
	#define	PULSES_PER_ANTENNA_CORRECTION_PKT	1002
	#define	PULSES_PER_TS_PROCESSING_PKT		1003
	#define	PULSES_PER_XMIT_POWER_PKT			1004
	#define	PULSES_PER_XMIT_SAMPLE_PKT			1005
	#define	PULSES_PER_CALIBRATION_PKT			1006
	#define PULSES_PER_EVENT_NOTICE_PKT			1007	// Normally as required, for end of volume, errors, restart etc.
	#define PULSES_PER_PHASECODE_PKT			1008
	#define	PULSES_PER_XMIT_INFO_PKT			1009
	// PULSE_HEADER_PKT always with each beam of data.
#endif	// DEBUG_SEND_ALL_IWRF_PKTS


// This function takes integer ABP's and processes them into scientific products
void dpProductWorker(MYDATA *pData)
{
#ifdef IS_DUAL_POL
	char	*self = "dpProductWorker";
	FIFO	*srcfifo_ch0 = NULL;
	FIFO	*srcfifo_ch1 = NULL;
	int		tail_ch0 = 0;
	int		tail_ch1 = 0;
	int		test_ch0, test_ch1;
	int		recordSizeProd = -1;	// Dual Pol products record size (incl. hdr)
	int		recordSizeIq = -1;	// IQ ch 0 & 1 (H&V) record size (incl. hdr(s)).
	int		dataLengthIqPerChannel = -1;	// number of data bytes in each channel, excluding header.
	PIRAQX	*px;
	PIRAQX	*px0, *px1;
	PIRAQX	*px_0,*px_1;	// Pulses read out of IQ fifos.
	char buf[256];
	unsigned int	prev_vol_num = -1;
	bool	needToSendInfoPacket = false;
	si64	seqNumber = 0;
	si64	prevSeqNumber = -1;
	int		pulseNumberDiff = -1;	// Difference between H & V channel pulsenums.
	int		vertPulseOffset = 0;	// Number of beams to shift.


#ifdef DEBUG_SEND_ALL_IWRF_PKTS
	int		packetType = 0;	// Packet type index for determining which to send with each data chunk.
#endif

	// IQ output fifo variables
	char			*writeIqRecord;

	// IWRF packets 
	iwrf_sync_t					*iwrfSyncPkt;			// Synchronization packet
	iwrf_radar_info_t			*iwrfRadarInfoPkt;		// Radar info
	iwrf_scan_segment_t			*iwrfScanSegmentPkt;	// Scan Segment
	iwrf_antenna_correction_t	*iwrfAntennaCorrectionPkt;
	iwrf_ts_processing_t		*iwrfTsProcessingPkt;
	iwrf_xmit_power_t			*iwrfXmitPowerPkt;
	iwrf_xmit_sample_t			*iwrfXmitSamplePkt;
	iwrf_calibration_t			*iwrfCalibrationPkt;	// Occasional operation header, info and calibration.
	iwrf_event_notice_t			*iwrfEventNoticePkt;
	iwrf_phasecode_t			*iwrfPhaseCodePkt;
	iwrf_xmit_info_t			*iwrfXmitInfoPkt;
	iwrf_pulse_header_t			*iwrfPulsePkt;		// Every data header.

	float			*IqOut;					// Pointer to IQ data in output FIFO to MGEN. 

	// Informational (non data) packet send frequency.

	int		pulsesPerClibrationPkt	= 100;		// How often to send an calibration header.
	int		infoPacketByteLength = 0;

	si64	packetCount = 0;
	// Packet count after which this packet should be sent. 
	si64	nextSyncPacketCount					= 0;	// Synchronization packet
	si64	nextRadarInfoPacketCount			= 0;	// Radar info
	si64	nextScanSegmentPacketCount			= 0;	// Scan Segment
	si64	nextAntennaCorrectionPacketCount	= 0;
	si64	nextTsProcessingPacketCount			= 0;
	si64	nextXmitPowerPacketCount			= 0;
	si64	nextXmitSamplePacketCount			= 0;
	si64	nextCalibrationPacketCount			= 0;	// Calibration.
	si64	nextEventNoticePacketCount			= 0;
	si64	nextPhaseSamplePacketCount			= 0;
	si64	nextPhaseCodePacketCount			= 0;
	si64	nextXmitInfoPacketCount				= 0;

	si64	prevPulseUtcSec = 0;	// To force info packets to track previous pulse time.
	si32	prevPulseUtcNanosec = 0;
	si64	prevPx0Secs = 0;		// Backups.
	si32	prevPx0Nanosecs = 0;

	// DEBUG: print out how many records are in the incoming fifos.
#ifdef  PRINT_DPWORKER_IQ_FIFO_FULLNESS
	int		head_ch0_now = -1;	// Incoming IQ products FIFO head right now.
	int		head_ch1_now = -1;	// Incoming IQ products FIFO head right now.
	char fifoFullnessBuf[120] = "INITIAL_VALUE";
	// FIFO fullness tracing variables
	int iqDpFifoFullnessH = 0;
	int iqDpFifoFullnessV = 0;
#endif

	// Logging
	logger = setupLogger("dpProductWorker", LOGGING_CONFIG_FILE_NAME_DP_PRODUCTS);
	if( logger == NULL) { 
		printf("Failed to setup %s logger",self);
	}
	LOG4CXX_DEBUG(logger,  "=========== "<< self << "() =========");
	LOG4CXX_DEBUG(logger,  self <<": HiQ receiver program version:" << HIQ_SW_VERSION);
	testLevels(logger,self);

	memset(buf,0,sizeof(buf));	// Clear print buffer.
	sprintf(buf,"%s:reading ch0(H) IQ fifo '%s' and ch1(V) IQ fifo '%s', and writing to Dual Pol Products fifo '%s'.\n",
							self, pData->src_fifo_ch0, pData->src_fifo_ch1, pData->dstfifo->name);
	LOG4CXX_INFO(logger, buf);

	// Initialize the PIRAQX structure in the fixed header area of the output FIFO
	px = (PIRAQX *)fifo_get_header_address(pData->dstfifo);	// Get convenient pointer to fixed header

	readpiraqx("",px);			// Initialize it

#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE		// write packets out when encountered.  WARNING: File size is NOT limited!
	system("del c:\\data\\pkt.txt");	// Erase previous MGEN IQ run output.
#endif
#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_PULSE_AND_SEQ_TO_FILE		// write seq#s out to a file.  WARNING: File size is NOT limited!
	system("del c:\\data\\seq_dp.txt");	// Erase previous MGEN IQ run output.
#endif

	getUtcTime(&prevPulseUtcSec,&prevPulseUtcNanosec);		// First packet with a reasonable time.

top:
	// channel 0
	memset(buf,0,sizeof(buf));	// Clear print buffer.
	sprintf(buf,"%s:Opening ch0 fifo '%s', Abort='%d'.\n",self,pData->src_fifo_ch0,Abort);
	LOG4CXX_DEBUG(logger, buf);
	// Open up a FIFO buffer, wait forever but with a way out via Abort flag.
	while(!(srcfifo_ch0 = fifo_check_usable(pData->src_fifo_ch0, &tail_ch0)) && !Abort)  
	{
		LOG4CXX_DEBUG(logger, "Waiting to open up the channel 0 source FIFO.");
		Sleep(300); 
	}
	printf("%s:srcfifo_ch0 address = 0x%08p\n",self,(void *)srcfifo_ch0);

	test_ch0 = fifo_safe_wait(srcfifo_ch0, pData->src_fifo_ch0, &tail_ch0, 1, 10, &Abort);

	if(test_ch0 == FIFO_WAITABORT)	goto bottom;
	if(test_ch0 == FIFO_GONE)		goto top;

	px0 = (PIRAQX *)fifo_get_address(srcfifo_ch0,tail_ch0,0);	// get the address of the first valid record

	// channel 1
	memset(buf,0,sizeof(buf));	// Clear print buffer.
	sprintf(buf,"%s:Opening ch1 fifo '%s', Abort='%d'.\n",self,pData->src_fifo_ch1,Abort);
	LOG4CXX_DEBUG(logger, buf);
	// Open up a FIFO buffer, wait forever but with a way out via Abort flag.
	while(!(srcfifo_ch1 = fifo_check_usable(pData->src_fifo_ch1, &tail_ch1)) && !Abort)  
	{
		LOG4CXX_DEBUG(logger, "Waiting to open up the channel 1 source FIFO.");
		Sleep(300); 
	}
	LOG4CXX_DEBUG(logger, buf);
	printf("%s:srcfifo_ch1 address = 0x%08p\n",self,(void *)srcfifo_ch1);

	test_ch1 = fifo_safe_wait(srcfifo_ch1, pData->src_fifo_ch1, &tail_ch1, 1, 10, &Abort);

	if(test_ch1 == FIFO_WAITABORT)	goto bottom;
	if(test_ch1 == FIFO_GONE)		goto top;
	
	px1 = (PIRAQX *)fifo_get_address(srcfifo_ch1,tail_ch1,0);	// get the address of the first valid record


	if(Abort)
	{
		memset(buf,0,sizeof(buf));	// Clear print buffer.
		sprintf(buf,"%s:Aborted Wait loop to open up the source FIFO.");
		LOG4CXX_INFO(logger, buf);
		printf("%s\n", buf);
		goto bottom;
	}

	recordSizeProd = sizeof(PIRAQX) + (px0->gates * DP_PRODS_ELEMENTS * sizeof(float));

	while(!Abort)
	{
		// Wait for a new beam of data in the IQ source fifos.
		test_ch0 = fifo_safe_wait(srcfifo_ch0, pData->src_fifo_ch0, &tail_ch0, px0->hits, 10, &Abort);
	#ifdef  PRINT_DPWORKER_IQ_FIFO_FULLNESS
		head_ch0_now = srcfifo_ch0->head;	// so it does not get updated by INT handler.
	#endif
		test_ch1 = fifo_safe_wait(srcfifo_ch1, pData->src_fifo_ch1, &tail_ch1, px1->hits, 10, &Abort);
	#ifdef  PRINT_DPWORKER_IQ_FIFO_FULLNESS
		head_ch1_now = srcfifo_ch1->head;	// so it does not get updated by INT handler.
	#endif

		if(test_ch0 == FIFO_WAITABORT	|| test_ch1 == FIFO_WAITABORT)	{goto bottom;}
		if(test_ch0 == FIFO_GONE		|| test_ch1 == FIFO_GONE)		{goto top;}

		// Visually indicate reception of a new record (beam) of APB data.
		if(CONSOLE) { printf("%c",pData->isHorzElseVert ? '-' : '|'); fflush(stdout); }

		// Get pointer to the newly available record from the source FIFO (containing a piraqx followed by IQ data)
		px0 = (PIRAQX *)fifo_get_address(srcfifo_ch0,tail_ch0,0);	// get the address of the first valid channel 0 record.
		px1 = (PIRAQX *)fifo_get_address(srcfifo_ch1,tail_ch1,0);	// get the address of the first valid channel 1 record.

		pulseNumberDiff = (int)(px0->pulse_num - px1->pulse_num);
		px0->spare[0] = px1->spare[0] = pulseNumberDiff;
	#ifdef PRINT_DPWORKER_PULSE_AND_DIFF
		printf("DPWKR:IQ pulse: H=%I64u, V=%I64u, diff(H-V)=%d\n", px0->pulse_num, px1->pulse_num, pulseNumberDiff);
	#endif

		// DEBUG: print out how many records are in the fifos.
	#ifdef  PRINT_DPWORKER_IQ_FIFO_FULLNESS
		iqDpFifoFullnessH = prtFifoFullness(&fifoFullnessBuf[0], srcfifo_ch0, head_ch0_now ,tail_ch0);
		iqDpFifoFullnessV = prtFifoFullness(&fifoFullnessBuf[0], srcfifo_ch1, head_ch1_now, tail_ch1);
		printf("                DPWKR:IQ H=%3d V=%3d\n", iqDpFifoFullnessH, iqDpFifoFullnessV);
		//Sleep(100);	// TESTCASE: cause a FATAL FIFO overrun. 'Different number of gates' (1st of 3 compares) FATAL error.
	#endif
		// END DEBUG: print out how many records are in the fifos.

		// Check that the pulse/beam is the same for the two channels.
		// TODO: adjust sychronization of records. 
		// We could skip a beam to attain synchronization.  It seems to happen naturally. :-)
/*		if(   gateCountCompare(px0, px1, px) != 0		
		   ||  pulsenumCompare(px0, px1, px) != 0
		   ||   beamnumCompare(px0, px1, px) != 0
		   )
		{
			memset(buf,0,sizeof(buf));	// Clear print buffer.
			sprintf(buf,"%s:IQ headers are not synchronized.  See ARC_DP_PRODUCTS.log for cause.  Exiting...", self);
			LOG4CXX_FATAL(logger, buf);
			MessageBox(NULL,TEXT(buf),TEXT("FATAL"),MB_ICONERROR | MB_OK);
			exit(-900);
		}
*/

		// ******************************** START **********************************
		// ************************** IWRF OUTPUT IQ DATA **************************
#ifdef ENABLE_TITAN_OUTPUT_GEN
		// Put the MGEN header with IQ data from the IQh and IQv fifos  
		// into the IQ_MGEN fifo to RcvrServer.
		// NOTE:MGEN is expecting SAMPLES_PER_GATE*gates of floats for each of two channels.  See IWRF spec.
		writeIqRecord = (char *)fifo_get_write_address(pData->dstfifoIq);	// Get fifo write adx.
		infoPacketByteLength = 0;	// bytes
		recordSizeIq = 0;	// bytes

		// Send ONE of the following INFO PKTs if it is time for sending.
		needToSendInfoPacket = true;	// Assume true.
		memset(buf,0,sizeof(buf));	// Clear packet print buffer.

		// Back up present time, and force info packets to track previous pulse's time.
		prevPx0Secs = px0->secs;
		prevPx0Nanosecs = px0->nanosecs;
		px0->secs = prevPulseUtcSec;
		px0->nanosecs = prevPulseUtcNanosec;

		// Put SYNC_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		if( SEND_SYNC_PKT && (PULSES_PER_SYNC_PKT == packetType))
#else
		if( SEND_SYNC_PKT && (packetCount > nextSyncPacketCount))
#endif
		{
			//printf("%d	SYNC_PKT\n",packetCount);
			saveInfoPktNumberAndType(buf, packetCount, "SYNC");
			infoPacketByteLength = sizeof(iwrf_sync_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfSyncPkt = (iwrf_sync_t *)writeIqRecord;					// Get pointer to pkt.
			convertPiraqx2IwrfSyncPkt(px0,iwrfSyncPkt);						// Build with seq_num.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextSyncPacketCount = packetCount + PULSES_PER_SYNC_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_sync_print(p_f_pkt_hdrs_dp, *iwrfSyncPkt);
#endif
		}

		// Put RADAR_INFO_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_RADAR_INFO_PKT && (PULSES_PER_RADAR_INFO_PKT == packetType))
#else
		else if( SEND_RADAR_INFO_PKT && (packetCount > nextRadarInfoPacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "RADAR_INFO");
			infoPacketByteLength = sizeof(iwrf_radar_info_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfRadarInfoPkt = (iwrf_radar_info_t *)writeIqRecord;		// Get pointer to pkt.
			convertPiraqx2IwrfRadarInfoPkt(px0, iwrfRadarInfoPkt);		// Populate packet from PIRAQX.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextRadarInfoPacketCount = packetCount + PULSES_PER_RADAR_INFO_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_radar_info_print(p_f_pkt_hdrs_dp, *iwrfRadarInfoPkt);
#endif
		}

		// Put SCAN_SEGMENT_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_SCAN_SEGMENT_PKT && (PULSES_PER_SCAN_SEGMENT_PKT == packetType) )
#else
		else if( SEND_SCAN_SEGMENT_PKT && (px0->vol_num != prev_vol_num ) )
#endif	 
		{
			prev_vol_num = px0->vol_num;	// Remember to detect next volume.
			saveInfoPktNumberAndType(buf, packetCount, "SCAN_SEGMENT");
			infoPacketByteLength = sizeof(iwrf_scan_segment_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfScanSegmentPkt = (iwrf_scan_segment_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfScanSegmentPkt(px0, iwrfScanSegmentPkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_scan_segment_print(p_f_pkt_hdrs_dp, *iwrfScanSegmentPkt);
#endif
		}

		// Put ANTENNA CORRECTION_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_ANTENNA_CORRECTION_PKT && (PULSES_PER_ANTENNA_CORRECTION_PKT == packetType))
#else
		else if( SEND_ANTENNA_CORRECTION_PKT && (packetCount > nextAntennaCorrectionPacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "ANTENNA_CORRECTION");
			infoPacketByteLength = sizeof(iwrf_antenna_correction_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfAntennaCorrectionPkt = (iwrf_antenna_correction_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfAntennaCorrectionPkt(px0, iwrfAntennaCorrectionPkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextAntennaCorrectionPacketCount = packetCount + PULSES_PER_ANTENNA_CORRECTION_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_antenna_correction_print(p_f_pkt_hdrs_dp, *iwrfAntennaCorrectionPkt);
#endif
		}

		// Put TIME SERIES PROCESSING_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_TS_PROCESSING_PKT && (PULSES_PER_TS_PROCESSING_PKT == packetType))
#else
		else if( SEND_TS_PROCESSING_PKT && (packetCount > nextTsProcessingPacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "TS_PROCESSING");
			infoPacketByteLength = sizeof(iwrf_ts_processing_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfTsProcessingPkt = (iwrf_ts_processing_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfTSProcessingPkt(px0, iwrfTsProcessingPkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextTsProcessingPacketCount = packetCount + PULSES_PER_TS_PROCESSING_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_ts_processing_print(p_f_pkt_hdrs_dp, *iwrfTsProcessingPkt);
#endif
		}

		// Put XMIT_POWER_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_XMIT_POWER_PKT && (PULSES_PER_XMIT_POWER_PKT == packetType))
#else
		else if( SEND_XMIT_POWER_PKT && (packetCount > nextXmitPowerPacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "XMIT_POWER");
			infoPacketByteLength = sizeof(iwrf_xmit_power_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfXmitPowerPkt = (iwrf_xmit_power_t *)writeIqRecord;	// Get pointer to pkt.
                        px0->v_xmit_power = px1->v_xmit_power; // Power from both channels is used in IWRF.
                                                               // Copy Vert pwr to header for Horiz 
			convertPiraqx2IwrfXmitPowerPkt(px0, iwrfXmitPowerPkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextXmitPowerPacketCount = packetCount + PULSES_PER_XMIT_POWER_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_xmit_power_print(p_f_pkt_hdrs_dp, *iwrfXmitPowerPkt);
#endif
		}

		// Put XMIT_SAMPLE_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_XMIT_SAMPLE_PKT && (PULSES_PER_XMIT_SAMPLE_PKT == packetType))
#else
		else if( SEND_XMIT_SAMPLE_PKT && (packetCount > nextXmitSamplePacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "XMIT_SAMPLE");
			infoPacketByteLength = sizeof(iwrf_xmit_sample_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfXmitSamplePkt = (iwrf_xmit_sample_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfXmitSamplePkt(px0, iwrfXmitSamplePkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextXmitSamplePacketCount = packetCount + PULSES_PER_XMIT_SAMPLE_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_xmit_sample_print(p_f_pkt_hdrs_dp, *iwrfXmitSamplePkt);
#endif
		}

		// Put CALIBRATION_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_CALIBRATION_PKT && (PULSES_PER_CALIBRATION_PKT == packetType))
#else
		else if( SEND_CALIBRATION_PKT && (packetCount > nextCalibrationPacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "CALIBRATION");
			infoPacketByteLength = sizeof(iwrf_calibration_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfCalibrationPkt = (iwrf_calibration_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfCalibrationPkt(px0, iwrfCalibrationPkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextCalibrationPacketCount = packetCount + PULSES_PER_CALIBRATION_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_calibration_print(p_f_pkt_hdrs_dp, *iwrfCalibrationPkt);
#endif
		}

		// Put in EVENT_NOTICE_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_EVENT_NOTICE_PKT && (PULSES_PER_EVENT_NOTICE_PKT == packetType))
#else
		//else if( SEND_EVENT_NOTICE_PKT && px0->event == true)	//TODO: need to add an event to PIRAQX.  PLL Lock started & completed, Loss of Sync, etc.
		else if( SEND_EVENT_NOTICE_PKT && (packetCount > nextEventNoticePacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "EVENT_NOTICE");
			infoPacketByteLength = sizeof(iwrf_event_notice_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfEventNoticePkt = (iwrf_event_notice_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfEventNoticePkt(px0, iwrfEventNoticePkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextEventNoticePacketCount = packetCount + PULSES_PER_EVENT_NOTICE_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_event_notice_print(p_f_pkt_hdrs_dp, *iwrfEventNoticePkt);
#endif
		}

		// Put PHASECODE_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_PHASECODE_PKT && (PULSES_PER_PHASECODE_PKT == packetType))
#else
		else if( SEND_PHASECODE_PKT && (packetCount > nextPhaseCodePacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "PHASECODE");
			infoPacketByteLength = sizeof(iwrf_phasecode_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfPhaseCodePkt = (iwrf_phasecode_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfPhaseCodePkt(px0, iwrfPhaseCodePkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextPhaseCodePacketCount = packetCount + PULSES_PER_PHASECODE_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_phasecode_print(p_f_pkt_hdrs_dp, *iwrfPhaseCodePkt);
#endif
		}

		// Put XMIT_INFO_PKT in IQ output fifo.
#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		else if( SEND_XMIT_INFO_PKT && (PULSES_PER_XMIT_INFO_PKT == packetType))
#else
		else if( SEND_XMIT_INFO_PKT && (packetCount > nextXmitInfoPacketCount))
#endif
		{
			saveInfoPktNumberAndType(buf, packetCount, "XMIT_INFO");
			infoPacketByteLength = sizeof(iwrf_xmit_info_t);
			recordSizeIq = recordSizeIq + infoPacketByteLength;		
			iwrfXmitInfoPkt = (iwrf_xmit_info_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfXmitInfoPkt(px0, iwrfXmitInfoPkt);					// Initialize packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
			nextXmitInfoPacketCount = packetCount + PULSES_PER_XMIT_INFO_PKT;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_xmit_info_print(p_f_pkt_hdrs_dp, *iwrfXmitInfoPkt);
#endif
		}

		// Add any more new info packets here.
		
		else
		{
			needToSendInfoPacket = false;
		}

		if(needToSendInfoPacket)
		{
			fifo_set_rec_size(pData->dstfifoIq, recordSizeIq);
			fifo_increment_head(pData->dstfifoIq);	// increment the output fifo head

			// Test for continuous packet sequence numbers.
			seqNumber = ((iwrf_packet_info *)(writeIqRecord - infoPacketByteLength))->seq_num;
			checkSequenceNumberContinutity( seqNumber, (si64 *)(&prevSeqNumber) );

#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_TYPE_IN_CONSOLE
			printf("%s",buf);
#endif
#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_PULSE_AND_SEQ_TO_FILE		// write seq#s out to a file.  WARNING: File size is NOT limited!
			fprintf(p_f_seq_dp,"%s",buf);
#endif
		}
		
		// Restore original time;
		px0->secs = prevPx0Secs;		
		px0->nanosecs = prevPx0Nanosecs;

		// DONE WITH INFO PACKETS


		// START WITH PULSE_HEADER_PKT in IQ output fifo in front of the data.
		// TODO: fill in the size of the data part in the pulse packet.
		dataLengthIqPerChannel = px0->gates * SAMPLES_PER_GATE * sizeof(float);	// bytes

		//  Shift the vertical data in order to line up with Horiz, based pulse number offset.
		vertPulseOffset = pulseNumberDiff;
		int localdiff = 0;	// for DEBUG printout in loop.

		for(uint4 hctr = 0; hctr < px0->hits; hctr++)		// Loop over all appropriate fifo records
		{
			// Get new record's write adx for the coming pulse pkt.
			writeIqRecord = (char *)fifo_get_write_address(pData->dstfifoIq);

			px_0 = (PIRAQX *)fifo_get_address(srcfifo_ch0, tail_ch0, hctr - px0->hits + 1);	// get the address of the H record
			//  Shift the vertical data in order to line up with Horiz, based pulse number offset.
			px_1 = (PIRAQX *)fifo_get_address(srcfifo_ch1, tail_ch1, hctr - px1->hits + 1 + vertPulseOffset);	// get the address of the V record				
		#ifdef PRINT_DPWORKER_2TITAN_PULSE_AND_DIFF
			localdiff = (int)(px_0->pulse_num - px_1->pulse_num);
			if(localdiff)
			{
				// Should only get following print if the correction fails!
				printf("DPWKR:2TITAN: IQ pulse: hctr=%d, H=%I64u, V=%I64u, adj diff(H-V)=%d\n", hctr, px_0->pulse_num, px_1->pulse_num, localdiff);
			}
		#endif

			//printf("%d				PKT\n",packetCount);
			savePulsePktNumberAndType(buf, packetCount, "\t\tPULSE", px_0->pulse_num, px_1->pulse_num);
			infoPacketByteLength = sizeof(iwrf_pulse_header_t);
			iwrfPulsePkt = (iwrf_pulse_header_t *)writeIqRecord;	// Get pointer to pkt.
			convertPiraqx2IwrfPulsePkt(px_0,iwrfPulsePkt);		// Convert PIRAQX packet to IWRF pulse_header_packet.
			writeIqRecord += infoPacketByteLength;
			packetCount++;
#ifdef DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE
			iwrf_pulse_header_print(p_f_pkt_hdrs_dp, *iwrfPulsePkt);
#endif
			// Test for continuous packet sequence numbers.
			seqNumber = ((iwrf_packet_info *)(writeIqRecord - infoPacketByteLength))->seq_num;
			checkSequenceNumberContinutity( seqNumber, (si64 *)(&prevSeqNumber) );
			
			// Save time last pulse packet for the next info packets
			prevPulseUtcSec = px_0->secs;
			prevPulseUtcNanosec = px_0->nanosecs;

			// Copy a count of bytes of each channel's IQ data from the source record to the destination record.
			// Move I's & Q's from incoming FIFOs to output FIFO after PULSE_HEADER.
			IqOut = (float *)(writeIqRecord);	// Set up char(byte) pointer to data start (end of header packets).
#ifdef	DEBUG_IQ_DATA_FROM_DP_PRODUCTS
			createIqTestPulse( 100 , 200, px_0->gates, IqOut);
#else
#ifdef DEBUG_DP_CH0_BEAM_PULSE_TRACE
			printf("DP: CH0:beam#=%I64u, pulse#=%I64u\n", px_0->beam_num, px_0->pulse_num);
#endif
			memcpy((void *)IqOut,(void *)( ((char *)px_0)+sizeof(PIRAQX) ), dataLengthIqPerChannel);	// ch 0
			//scaleIqToTitanInput( IqOut, px_0->gates * SAMPLES_PER_GATE);
#endif	// DEBUG_IQ_DATA_FROM_DP_PRODUCTS
			writeIqRecord += dataLengthIqPerChannel;

			// Move I's & Q's from incoming FIFOs to output FIFO following first channels data.
			IqOut = (float *)(writeIqRecord);	// Set up char(byte) pointer to data start (end of header packets).
#ifdef	DEBUG_IQ_DATA_FROM_DP_PRODUCTS
			createIqTestPulse( 100 , 200, px_0->gates, IqOut);
#else
#ifdef DEBUG_DP_CH1_BEAM_PULSE_TRACE
			printf("DP: CH1:beam#=%I64u, pulse#=%I64u\n", px_1->beam_num, px_1->pulse_num);
#endif
			memcpy((void *)IqOut,(void *)( ((char *)px_1)+sizeof(PIRAQX) ), dataLengthIqPerChannel);	// ch 1
			//scaleIqToTitanInput( IqOut, px_1->gates * SAMPLES_PER_GATE);
#endif	// DEBUG_IQ_DATA_FROM_DP_PRODUCTS

			// Finish with this pulse packet.
			recordSizeIq = infoPacketByteLength + IQ_CHANNELS * dataLengthIqPerChannel;	// in bytes, add the data channels.		
			fifo_set_rec_size(pData->dstfifoIq, recordSizeIq);
			fifo_increment_head(pData->dstfifoIq);	// increment the output fifo head
#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_TYPE_IN_CONSOLE
			printf("%s",buf);
#endif
#ifdef DEBUG_PRINT_EACH_IWRF_PACKET_PULSE_AND_SEQ_TO_FILE		// write seq#s out to a file.  WARNING: File size is NOT limited!
			fprintf(p_f_seq_dp,"%s",buf);
#endif
		}

#ifdef DEBUG_SEND_ALL_IWRF_PKTS
		if(packetType==MAX_PACKET_TYPE)
		{
			packetType=0;
		} else {
			packetType++;
		}
#endif

#endif	// ENABLE_TITAN_OUTPUT_GEN	This cuts out packets to Titan for debugging purposes
		// ************************** IWRF OUTPUT IQ DATA **************************
		// ********************************* END ***********************************

		//***************  DUAL POL ENGINEERING DISPLAY PRODUCTS*********************
		// Calculate the dual pol products (Zdr & PHIdp) from the IQh and IQv data.
		// Put the PIRAQX header with dual pol products into the DP_PRODUCTS fifo to PpiScope.  

		// Choose product generation routine on basis of data format.

		switch(px0->dataformat)
		{
			case DATA_DUALPP:
			case DATA_SIMPLEPP:
			case DATA_SIMPLE16:
			case DATA_HVSIMUL:
#ifdef ENABLE_DP_PRODUCTS_CALC
				dpHVSimul(pData, srcfifo_ch0, tail_ch0, srcfifo_ch1, tail_ch1, px_0, px_1, vertPulseOffset); // Simultaneous HV computations
#endif ENABLE_DP_PRODUCTS_CALC
				break;
			default:
				// Just ignore if not one of the dual-pol formats.
				MessageBox(NULL, TEXT("Bad dataformat"), TEXT("Bad px0->dataformat in dpProducts.cpp"), MB_ICONERROR | MB_OK );
		}

		// set the record size so that it can be put on disk or over ethernet
		fifo_set_rec_size(pData->dstfifo, recordSizeProd);

		// increment the output fifo head
		fifo_increment_head(pData->dstfifo);

	}

bottom:
	if(srcfifo_ch0)	// if you were using it...
	{
		LOG4CXX_DEBUG(logger, "Disconnect from the channel 0 source fifo: " << pData->src_fifo_ch0);
		fifo_disconnect(srcfifo_ch0);		
	}
	if(srcfifo_ch1)	// if you were using it...
	{
		LOG4CXX_DEBUG(logger, "Disconnect from the channel 1 source fifo: " << pData->src_fifo_ch1);
		fifo_disconnect(srcfifo_ch1);		
	}
#endif	// IS_DUAL_POL
}


int gateCountCompare(PIRAQX *aPx0, PIRAQX *aPx1, PIRAQX *aFixedpx)
{	// ASSUMPTION: H & V IQ fifos contain the same header, same number of gates.
	char *self = "gateCountCompare:";
	char buf[256];
	int difference = 0;
	if(aPx0->gates == aPx1->gates)
	{
		aFixedpx->gates = aPx0->gates;
	}
	else
	{
		difference = aPx0->gates - aPx1->gates;
		memset(buf,0,sizeof(buf));	// Clear print buffer.
		sprintf(buf,"%s:IQ headers have different numbers of gates!  ch0=%d, ch1=%d", self, aPx0->gates, aPx1->gates);
		LOG4CXX_ERROR(logger, buf);
		printf("ERROR:%s:%s\n", self, buf);
		Abort = 1;
	}
	return difference;
}


int pulsenumCompare(PIRAQX *aPx0, PIRAQX *aPx1, PIRAQX *aFixedpx)
{	// ASSUMPTION: H & V IQ fifos contain the same header, same number of gates.
	char *self = "pulsenumCompare:";
	char buf[256];
	int difference = 0;
	if(aPx0->pulse_num == aPx1->pulse_num)
	{
		aFixedpx->pulse_num = aPx0->pulse_num;
	}
	else
	{
		difference = (int)(aPx0->pulse_num - aPx1->pulse_num);
		memset(buf,0,sizeof(buf));	// Clear print buffer.
		sprintf(buf,"%s:IQ headers have different numbers of pulse_num!  ch0=%d, ch1=%d", self, aPx0->pulse_num, aPx1->pulse_num);
		LOG4CXX_ERROR(logger, buf);
		printf("ERROR:%s:%s\n", self, buf);
		Abort = 1;
	}
	return difference;
}


int beamnumCompare(PIRAQX *aPx0, PIRAQX *aPx1, PIRAQX *aFixedpx)
{	// ASSUMPTION: H & V IQ fifos contain the same header, same number of gates.
	char *self = "beamnumCompare:";
	char buf[256];
	int difference = 0;
	if(aPx0->beam_num == aPx1->beam_num)
	{
		aFixedpx->beam_num = aPx0->beam_num;
	}
	else
	{
		difference = (int)(aPx0->beam_num - aPx1->beam_num);
		memset(buf,0,sizeof(buf));	// Clear print buffer.
		sprintf(buf,"%s:IQ headers have different beam_num values!  ch0=%d, ch1=%d", self, aPx0->beam_num, aPx1->beam_num);
		LOG4CXX_ERROR(logger, buf);
		printf("ERROR:%s:%s\n", self, buf);
		Abort = 1;
	}
	return difference;
}
// Test condition switches
//#define DP_TEST_PP		// Generate phoney data from dpHVSimulPP


// Local definition of DP output products.  Shut off ALL switches for normal operation
#ifndef M_PI
#define	M_PI	3.1415926536
#endif

#define ZDR		0
#define	PHIDP	1
#define RHOHV	2
#define RAD2DEG	(180.0/M_PI)

#define NUMBER_OF_DP_SUMS	8		// Ah,Bh,Ph,Av,Bv,Pv,ReXc,ImXc

static long testcounter = 0;		// For debugging
void dpHVSimul(MYDATA *pdat, FIFO *srch, int tailh, FIFO *srcv, int tailv, PIRAQX *pxh, PIRAQX *pxv, int vertPulseOffset)
{
	char	*self = "dpHVSimul";
	char	*writerecord;					// Pointer to beginning of output fifo
	uint4	igate, hctr;					// Indices
	float	rconst_corr = 0;
	float	*fptrp, *fptrh, *fptrv, *prods;	// Pointers to in/out arrays
	float	Vh, Vv, theta, dp;
	float	Ph_noise, Pv_noise;			// Noise power values
	float	hvsums[MAXGATES*NUMBER_OF_DP_SUMS], *phvsums;	// Summing array and temp pointer to the array
	float	Ah, Bh, Ph, Av, Bv, Pv, ReXc, ImXc;		// Temporary variables for auto/cross correlation
	float	testh, testv;				// Debugging variables
	int		recordSizeProd = -1;		// Dual Pol products record size (incl. hdr)
	PIRAQX	*pxrech, *pxrecv;			// Pointers to H and V FIFO records

	for(hctr = 0; hctr < pxh->hits; hctr++)		// Loop over all appropriate fifo records
	{
		pxrech = (PIRAQX *)fifo_get_address(srch,tailh,hctr - pxh->hits + 1);	// get the address of the H record
		pxrecv = (PIRAQX *)fifo_get_address(srcv,tailv,hctr - pxh->hits + 1 + vertPulseOffset);	// get the address of the V record	
		
		// Compute pointers to H and V IQ data (floats)

		fptrh = (float *)((char *)pxrech + sizeof(PIRAQX));
		fptrv = (float *)((char *)pxrecv + sizeof(PIRAQX));
	
		phvsums = hvsums;	// Pointer to beginning of summing array
		testh = *fptrh;		// For debugging
		testv = *fptrv;

		dpHVSimulPP(fptrh, fptrv, phvsums, pxrech, hctr);		// Process a pulse worth of IQ data.
	}
	
	// Averaging cycle is complete: pxh->hits samples have been averaged. Now write to the output fifo
	// Get pointer to the current record of the destination FIFO (will contain a piraqx struct and scientific products)
	writerecord = (char *)fifo_get_write_address(pdat->dstfifo);	// Get pointer to beginning of DP products fifo  
	recordSizeProd = sizeof(PIRAQX) + (pxh->gates * DP_PRODS_ELEMENTS * sizeof(float)); // Compute record size
	prods = (float *)(writerecord + sizeof(PIRAQX));	// get start address for data area of fifo record	
	memcpy(writerecord,((char *)pxh),sizeof(PIRAQX));	// Copy the piraqx struct from the source record to the destination record.

	fptrp = prods;										// Copy the pointer to output fifo
	phvsums = hvsums;									// Pointer to start of summing array
	rconst_corr = pxh->zdr_accumulated_error_correction -
				  (pxh->h_rconst + pxh->h_measured_xmit_power - pxh->h_xmit_power) +
				  (pxv->v_rconst + pxv->v_measured_xmit_power - pxv->v_xmit_power);

	Ph_noise = (pxh->h_noise_power < -10.0)? pow(10.0,0.1*pxh->h_noise_power): 0;		
	Pv_noise = (pxh->v_noise_power < -10.0)? pow(10.0,0.1*pxh->v_noise_power): 0;	


/*
printf("                                                       \rh,v meas %f, %f, rccorr %f\r", 
	   pxh->h_xmit_power, pxv->v_xmit_power,
	   rconst_corr);
*/
	for(igate = 0; igate< pxh->gates; igate++)			// Compute final DP products for all gates
	{
		// Retrieve the auto & cross correlation values in the order they were summed in dpHVSimulPP
		// DEBUG: Need to subtract noise linearly from Ph and Pv
		Ah = *phvsums++;
		Bh = *phvsums++;
		Ph = *phvsums++ - Ph_noise;
		Av = *phvsums++;
		Bv = *phvsums++;
		Pv = *phvsums++ - Pv_noise;
		ReXc = *phvsums++;
		ImXc = *phvsums++;

		// Calculate ZDR (differential reflectivity) in dB
		*(fptrp + ZDR) = (float)(10.0*(log10(Ph) - log10(Pv)) - rconst_corr);

		// Calculate PHIDP (differential phase) in radians
		theta = atan2(ImXc,ReXc);

		// Correct for phase offset and limit magnitude to <= pi
		dp = (theta * 0.5) + pxh->phaseoffset;
		if(dp >  M_PI) dp -= M_PI;	// Limit dp to +/- M_PI
		if(dp < -M_PI) dp += M_PI;

		// Convert differential phase (dp) from radians to degrees
		*(fptrp + PHIDP) = (float)(dp * RAD2DEG);

		// Calculate RHOHV (cross-polar correlation) (unitless)
#ifndef DP_TEST_PP
		*(fptrp + RHOHV) = sqrt((ReXc*ReXc + ImXc*ImXc)/(Ph*Pv));	
#else // DP_TEST_PP
		*(fptrp + RHOHV) = sqrt((ReXc*ReXc + ImXc*ImXc)/(Ph*Pv)) - 0.25;
#endif // DP_TEST_PP

		fptrp += DP_PRODS_ELEMENTS;	// Bump the output pointer by the number of DP products
	}		

	return;
}

// Dual Pol pulse pair additional products algorithms
static float lagbufh[2*MAXGATES], lagbufv[2*MAXGATES];	// Arrays to hold IQ data from last pulse (last lag)

void dpHVSimulPP(float *fph, float *fpv, float *abpxhv, PIRAQX *px, int nsample)
{
	char *self = "dpHVSimulPP";
	float       Ihold, Qhold, Ih, Qh, *lagph1, *lagph2, *cfph;	// Pointers for h channel
	float       Ivold, Qvold, Iv, Qv, *lagpv1, *lagpv2, *cfpv;	// Pointers for v channel
	float		*labpxhv;					// Local pointer to sum array
	int			lagsim = 1, csim = 1;		// Data simulators for DP_TEST_PP

	labpxhv = abpxhv;				// Copy incoming pointer to sum array
	lagph1 = lagph2 = lagbufh;		// Start both h buffer pointers the same
	lagpv1 = lagpv2 = lagbufv;		// Start both v buffer pointers the same
	cfph = fph;						// Local copy of current h I/Q data pointer
	cfpv = fpv;						// Local copy of current v I/Q data pointer

//	printf("%s: nsample %d ", self, nsample);

	for(uint4 i = 0; i < px->gates; i++)
	{
		// Get the new and old horizontal data
		Ihold = *lagph1++;			// Grab Ih at one lag
		Qhold = *lagph1++;			// Grab Qh at one lag
		Ih    = *cfph++;			// Grab current Ih
		Qh    = *cfph++;			// Grab current Qh

		// Get the new and old vertical data
		Ivold = *lagpv1++;			// Grab Iv at one lag
		Qvold = *lagpv1++;			// Grab Qv at one lag
		Iv    = *cfpv++;			// Grab current Iv
		Qv    = *cfpv++;			// Grab current Qv

#ifdef DP_TEST_PP					// If DP_TEST_PP is defined, replace new and old IQ samples with fixed values
		Ihold = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab Ih at one lag (sim)
		Qhold = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab Qh at one lag (sim)
		Ih    = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab current Ih (sim)
		Qh    = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab current Qh (sim)
		
		Ivold = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab Iv at one lag (sim)
		Qvold = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab Qv at one lag (sim)
		Iv    = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab current Iv (sim)
		Qv    = 0.1*(NUMBER_OF_DP_SUMS - (lagsim++ % NUMBER_OF_DP_SUMS));			// Grab current Qv (sim)
#endif	// DP_TEST_PP

		// Start main computation loop  
		// If nsample == 0 don't add to the existing values.  This effectively zeros the averages. 
		if(nsample == 0)			
		{
			*labpxhv++ = Ih*Ihold + Qh*Qhold;		// Compute	horiz A
			*labpxhv++ = Qh*Ihold - Ih*Qhold;		//			horiz B	
			*labpxhv++ = Ih*Ih + Qh*Qh;				//			horiz P

			*labpxhv++ = Iv*Ivold + Qv*Qvold;		// Compute	vert A
			*labpxhv++ = Qv*Ivold - Iv*Qvold;		//			vert B
			*labpxhv++ = Iv*Iv + Qv*Qv;				//			vert P
		
			*labpxhv++ = Ih*Iv + Qh*Qv;				// Compute	Re(HV cross-correlation)
			*labpxhv++ = Qh*Iv - Qv*Ih;				//			Im(HV cross-correlation)
		}	
		else										// Continue summing, now adding to the averages
		{
			*labpxhv++ += Ih*Ihold + Qh*Qhold;		// Compute	horiz A
			*labpxhv++ += Qh*Ihold - Ih*Qhold;		//			horiz B	
			*labpxhv++ += Ih*Ih + Qh*Qh;			//			horiz P

			*labpxhv++ += Iv*Ivold + Qv*Qvold;		// Compute	vert A
			*labpxhv++ += Qv*Ivold - Iv*Qvold;		//			vert B
			*labpxhv++ += Iv*Iv + Qv*Qv;			//			vert P
		
			*labpxhv++ += Ih*Iv + Qh*Qv;			// Compute	Re(HV cross-correlation)
			*labpxhv++ += Qh*Iv - Qv*Ih;			//			Im(HV cross-correlation)
		}
		// Move current samples to lag buffers
		*lagph2++ = Ih;		
		*lagph2++ = Qh;
		*lagpv2++ = Iv;
		*lagpv2++ = Qv;
	}
return;
}

// Store pulse packet number and name for later printing.
void savePulsePktNumberAndType(char *aBuffer, si64 aPacketNumber, char *aPacketName, si64 aHorzPulseNumber, si64 aVertPulseNumber)
{
	memset(aBuffer,0,sizeof(aBuffer));	// Clear print buffer.
	sprintf(aBuffer,"%lld\t%s\t%lld\t%lld\n",aPacketNumber,aPacketName,aHorzPulseNumber,aVertPulseNumber);
}

// Store info packet number and name for later printing.
void saveInfoPktNumberAndType(char *aBuffer, si64 aPacketNumber, char *aPacketName)
{
	memset(aBuffer,0,sizeof(aBuffer));	// Clear print buffer.
	sprintf(aBuffer,"%lld\t%s\n",aPacketNumber,aPacketName);
}

void checkSequenceNumberContinutity( si64 aSequenceNumber, si64 *p_aPrevSequencenumber )
{
	// Check for sequence number continutity.
	if((*p_aPrevSequencenumber) != (aSequenceNumber-1) )
	{
		si64 prev = *p_aPrevSequencenumber;
		printf("dpProducts.cpp: *** Skipped sequence #'%lld', prev #='%lld'! ***\n", aSequenceNumber, prev);
	}
	(*p_aPrevSequencenumber) = aSequenceNumber;
}

// Scale I and Q data input to conform to TITAN needs.
void scaleIqToTitanInput( float *pIQ, int aNumberOfSamples)
{		
	int i = 0;
	float anI = 0.0;
	float aQ = 0.0;
	int		i2to14th = 2^14;
	float	f2to14th = (float)i2to14th;
	int		i2to18th = 2^18;
	float	f2to18th = (float)i2to18th;
//	float scaleFactor = 1.0e-6f;	// scale IQ data for IWRF.
	float scaleFactor = 1.0e-3f;	// scale IQ data for IWRF.

	for(i=0; i<aNumberOfSamples/2; i++)	// Operate on pairs of samples (I,Q).
	{
		// A/D output is in the top 18 bits - 14 bit resolution 
		// Should bring values to 0-1.0  - Then bias by -0.5 volts.
		anI = *pIQ;
		*pIQ = (float)( ((anI/f2to18th) / f2to14th) - 0.5) * scaleFactor; // I: 1V scale -.5 to +.5
		pIQ++;
		aQ = *pIQ;
		*pIQ = (float)( (( aQ/f2to18th) / f2to14th) - 0.5) * scaleFactor; // Q: 1V scale -.5 to +.5
		pIQ++;
		//printf("I=%g Q=%g\n",anI,aQ);
	}
}


// Dummy IQ data as a test pulse from aGate1 to aGate2 over aGatesCount gates.
void createIqTestPulse( int aGate1 , int aGate2, int aGatesCount, float* pIQ)
{		
	int i=0;
	int j=0;
	int k=0;
	for(i=0; i<2*aGate1; i++)
	{
		*pIQ++ = (float)1.0e-2;
	}
	for(j=2*aGate1; j<2*aGate2; j++)
	{
		*pIQ++ = (float)1.0e-1;
	}
	for(k=aGate2; k<2*aGatesCount; k++)
	{
		*pIQ++ = (float)1.0e-2;
	}
}
