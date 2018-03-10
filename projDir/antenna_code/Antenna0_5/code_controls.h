// This file may only contain switches for enabling code for functionality and debugging.
// COMMENT LINES TO DISABLE FUNCTION or DEBUGGING output.
// 
// FUNCTION
#define	ENABLE_TITAN_OUTPUT_GEN		// Enable IWRF IQ data output to RcvrServer FIFO, else send 0xFFFFFFAB data
#define	ENABLE_TITAN_OUTPUT_SERVER	// Enable RcvrServer which sends IWRF IQ data to TITAN.
//#define ENABLE_GPS_UPDATE			// Enable obtaining GPS data from Linux server and editing config file.
#define ENABLE_DP_PRODUCTS_CALC		// Enable Dual Pol products calculation, else they are 0.
//#define ENABLE_CORRECT_ON_PULSENUM	// Throw out H or V IQ records based on pulse number mismatch. May no longer work since pulse number is assigned in user mode.
//
// PARAMETERS
#define WAIT_SECS_ON_FIFO_RESTART	10	// Number of seconds to wait before a Restart due to a FIFO error.
										// Log G0 being out of range.
//#define	LOG_G0_OUT_OF_RANGE_FREQUENCY	100		// Log every Nth set of hits.
#define	LOG_G0_OUT_OF_RANGE_FREQUENCY	1000000		// Log every Nth set of hits.
#define	LOG_G0_OUT_OF_RANGE_LOW_LIMIT	-15.0	// dBm, log if G0 is lower than this value.
#define	LOG_G0_OUT_OF_RANGE_HI_LIMIT	10.0	// dBm, log if G0 is higher than this value.

//
// DEBUGGING
//-hiqutil
//#define PLOT_LOCK_OFF		// Disables a console graphic output of afc lock scan.  
//#define DEBUG_ANGLES		// Print Products.cpp angles rapidly in console and PpiScope Statusbar.
//#define DEBUG_REGISTERS	// Print out HiQ registers after each write, except PLL write.

//- abpWorker
//#define PRINT_IQDATA_FIFO_FULLNESS		// Print out how many records are in the incoming fifos.
//#define DEBUG_DP_ABP_PRINT_PULSE_NUM		// Print the pulse number from the abplib.cpp worker.
//#define DEBUG_PRINT_ABP_AND_DPPRODUCTS_FIFO_FULLNESS	// Print # of ABP records in the products.cpp incoming fifos.
//#define DEBUG_TEST_FIFO_RESTART			// Stop making data, so PRODUCTS fifo runs dry, then Restart.
//
//- dpWorker
//#define DEBUG_DP_CH0_BEAM_PULSE_TRACE		// Print the Channel 0 (H) beam & pulse # in the console.
//#define DEBUG_DP_CH1_BEAM_PULSE_TRACE		// Print the Channel 0 (H) beam & pulse # in the console.
//#define PRINT_DPWORKER_PULSE_AND_DIFF		// Print info in the DP products thread (pulse#, & pulse# diff).
//#define PRINT_DPWORKER_IQ_FIFO_FULLNESS		// Print info in the DP products thread (fifo fullness).
//#define PRINT_DPWORKER_2TITAN_PULSE_AND_DIFF	// Print pulse# after Pulse# offset correction to data.
//#define DEBUG_IQ_DATA_FROM_DP_PRODUCTS	// Send debug IQ data to TITAN.
//
//-productsWorker
//#define DEBUG_PRODUCTS_DP_PRINT_PULSE_NUM	// Print pulse # and diff (H-V) as packets are read from dpfifo 
//
//- IWRF packets
//#define DEBUG_SEND_ALL_IWRF_PKTS	// Cause each packet type to be sent with every pulse packet, in turn. 
//#define DEBUG_PRINT_EACH_IWRF_PACKET_TYPE_IN_CONSOLE
//#define DEBUG_PRINT_RCVRSERVER_SEQ_NUMBERS_TO_FILE	// Print IWRF packet seq #s to file jus prior to sending.
//#define DEBUG_PRINT_EACH_IWRF_PACKET_PULSE_AND_SEQ_TO_FILE	// Print generated seq#s from dpProdcuts to a file.
//#define DEBUG_PRINT_IWRF_PKT_HDRS_TO_FILE	// Write IWRF packet hdrs to file.  WARNING: File size is NOT limited!
//
//- PpiScope
#define DEBUG_SHOW_PPI_STATUS_INFO		// Display pulse number, and clicked XY coordinates on PPI Scope status bar.
//
//- RunHiq
