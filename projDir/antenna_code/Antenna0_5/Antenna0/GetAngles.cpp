#undef UNICODE
#define WIN32_LEAN_AND_MEAN
//#define MONITOR_IN
//#define MONITOR_OUT
#define WAITms		1		// Loop time in milliseconds
#define SYNC_FREQ	100		// Send info packets every SYNC_FREQ loops
#define PRINT_MOD	10		// Print time/antenna data every PRINT_MOD loops
#define NORMAL_PRINT		// Normal printout enabled
//#define DEBUGGING_01		// For debugging
//#define DEBUGGING_03		// For debugging
#define NO_ENTRY_DEFAULT  9999	// Default value for 'no entry' condition

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include "../pmac.h"
#include "../iqdata.h"
#include "../angles.h"
#include "../hiqutil.h"
#include "../iwrf_data.h"
#include "../mydata.h"

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 8192
#define DEFAULT_PORT 27015			// Data port

static float az = 0; 
static float el = 0;
static float fang = 0;
static int transition = 0;			// PMAC scan transition flag
static int s_num = 0;				// PMAC scan number
static int v_num = 0;				// PMAC volume number
static int s_mode = 0;				// PMAC scan mode identifier
static int counter = 0;				// General purpose counter
static si64 pulsectr = 0, packetctr = 0, printctr = 0;  // Self explanatory counters

void init_ts_processing(iwrf_ts_processing_t *aa);
void init_radar_info(iwrf_radar_info_t *bb);
void init_packet_info(iwrf_packet_info_t *a);
void init_scan_segment(iwrf_scan_segment_t *b);
void init_sync(iwrf_sync_t *c);
void init_pulse_header(iwrf_pulse_header_t *d);

int main(int argc, char *argv[])
{
	struct	addrinfo *result = NULL, hints;
	int		iResult, iSendResult = 1 , sendstat = 0;
	unsigned short *pPMdpr, *pChw;
	IQRECORD *pArec;
	int Asrc = ANGLE_SRC_PMAC_PCI;
	float Ascl = .0054932;
	uint8 psec, pnano;

	iwrf_ts_processing_t *TSPinfo;
	iwrf_radar_info_t *Rinfo;
	iwrf_packet_info_t *PinfoH;
	iwrf_scan_segment_t *Sseg;
	iwrf_sync_t *Sync;
	iwrf_pulse_header_t *PulseH;

	int cntr = 0, cmodulus = 10;
	si64 pulsectr = 0, packetctr = 0;


	const char *sccp;
	char buffer[1000];

	WSADATA WsaDat;

	TSPinfo = (iwrf_ts_processing_t *)malloc(sizeof(iwrf_ts_processing_t));
	Rinfo = (iwrf_radar_info_t *)malloc(sizeof(iwrf_radar_info_t));
	PinfoH = (iwrf_packet_info_t *)malloc(sizeof(iwrf_packet_info_t)); //Allocate packet info hdr
	Sseg = (iwrf_scan_segment_t *)malloc(sizeof(iwrf_scan_segment_t));	// Allocate Sseg memory	
	Sync = (iwrf_sync_t *)malloc(sizeof(iwrf_sync_t)); // Allocate sync packet memory
	PulseH = (iwrf_pulse_header_t *)malloc(sizeof(iwrf_pulse_header_t)); // Allocate pulse header memory

// Do forever

	do{	
		init_ts_processing(TSPinfo);	// Initialize various packets
		init_radar_info(Rinfo);
		init_packet_info(PinfoH);
		init_scan_segment(Sseg);
		init_sync(Sync);
		init_pulse_header(PulseH);

	
		printf("Hello from Delta Tau.\n");
		pPMdpr = (unsigned short *)get_pmac_dpram();	// Get phys address of PMAC DPRAM
		printf("PMAC addr = 0x%x\n", pPMdpr);				// Print for debugging purposes

// Initialize communication software
	
		if(WSAStartup(MAKEWORD(2,2),&WsaDat)!=0)
		{
			printf("WSA Initialization failed!\r\n");
			WSACleanup();
			system("PAUSE");
			return 0;
		}
		printf("WSA initialized\n");

	// Try to create a socket

	//	SOCKET Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		SOCKET Socket=socket(AF_INET,SOCK_STREAM,0);
		if(Socket==INVALID_SOCKET)
		{
			printf("Socket creation failed.\r\n");
			WSACleanup();
			system("PAUSE");
			return 0;
		}
		printf("Socket created. Port %d\n", DEFAULT_PORT);

		SOCKADDR_IN serverInf;
		serverInf.sin_family=AF_INET;
		serverInf.sin_addr.s_addr=INADDR_ANY;
		serverInf.sin_port=htons(DEFAULT_PORT);	

	// Bind the socket

		if(bind(Socket,(SOCKADDR*)(&serverInf),sizeof(serverInf))==SOCKET_ERROR)
		{
			printf("Unable to bind socket!\r\n");
			WSACleanup();
			system("PAUSE");
			return 0;
		}
	// Listen for a connection
		listen(Socket,1);

		SOCKET TempSock=SOCKET_ERROR;
		while(TempSock==SOCKET_ERROR)
		{
			printf("Waiting for incoming connections...\n");
			TempSock=accept(Socket,NULL,NULL);
		}
		Socket=TempSock;

		printf("Client connected!\r\n\r\n");

		pulsectr = 0;		// Zero the pulse sequence counter
		packetctr = 0;		// Zero the packet sequence counter

	// Do until the socket is disconnected or another comm error occurs
		do
		{
		// Pick up PMAC DPRAM data
			az = (float)pPMdpr[0]*Ascl;		// Get az binary and convert to FP
			el = (float)pPMdpr[1]*Ascl;		// Get el binary and convert to FP
			s_mode = pPMdpr[2];				// Get mode type flag
			s_num = pPMdpr[3];				// Get scan number
			v_num = pPMdpr[4];				// Get volume number
			fang = pPMdpr[5]*Ascl;			// Get fixed angle
			transition = pPMdpr[6];			// Get transition flag

		// Get current UTC seconds and nanoseconds
			getUtcTime(&psec, &pnano);		// Get accurate time 

		// Update the dynamic part of the iwrf packet structures 
			Rinfo->packet.radar_id = 1;				// Phoney radar ID
			Rinfo->packet.seq_num = pulsectr;		// Pulse (loop) counter
			Rinfo->packet.version_num = 5;			// Version of this software
			Rinfo->packet.time_secs_utc = psec;		//Insert UTC epoch seconds	in scan segment header
			Rinfo->packet.time_nano_secs = pnano;	// and nanoseconds		
			TSPinfo->packet.radar_id = 1;			// The following are the same
			TSPinfo->packet.seq_num = pulsectr;			// as above
			TSPinfo->packet.version_num = 5;
			TSPinfo->packet.time_secs_utc = psec;	// Time Series Processing
			TSPinfo->packet.time_nano_secs = pnano;			
			Sseg->packet.radar_id = 1;				// Scan Segment
			Sseg->packet.seq_num = pulsectr;
			Sseg->packet.version_num = 5;
			Sseg->packet.time_secs_utc = psec;		
			Sseg->packet.time_nano_secs = pnano;	
			Sync->packet.radar_id = 1;
			Sync->packet.seq_num = pulsectr;		// Sync packet
			Sync->packet.version_num = 5;
			Sync->packet.time_secs_utc = psec;		
			Sync->packet.time_nano_secs = pnano;	
			PulseH->packet.radar_id = 1;			// Pulse header
			PulseH->packet.seq_num = pulsectr;
			PulseH->packet.version_num = 5;
			PulseH->packet.time_secs_utc = psec;	
			PulseH->packet.time_nano_secs = pnano;	
			
			Sseg->scan_mode = s_mode;	//Scan mode
			Sseg->volume_num = v_num;	//Volume number
			Sseg->sweep_num = s_num;	//Sweep number
			Sseg->current_fixed_angle = fang;	//Fixed angle
			PulseH->azimuth = az;	// Proper place for az
			PulseH->elevation = el;	// Proper place for el
			PulseH->antenna_transition = transition;	//Proper place for transition flag
			PulseH->fixed_az = NO_ENTRY_DEFAULT; // General fixed angle--depends on scan type
			PulseH->fixed_el = NO_ENTRY_DEFAULT; // Same
			if(s_mode == IWRF_SCAN_MODE_SECTOR)	 // Put fixed angle in right slot
			{									 //  and put NO_ENTRY_DEFAULT in other
				PulseH->fixed_el = fang;
			}	
			if(s_mode == IWRF_SCAN_MODE_AZ_SUR_360)
			{
				PulseH->fixed_el = fang;
			}
			if(s_mode == IWRF_SCAN_MODE_RHI)
			{
				PulseH->fixed_az = fang;
			}
			
			PulseH->scan_mode = s_mode;			// Update Pulse Header values
			PulseH->sweep_num = s_num;
			PulseH->volume_num = v_num;
			PulseH->pulse_seq_num = pulsectr;
#ifdef NORMAL_PRINT
			if(printctr++ % PRINT_MOD == 0)
			{
				printf("T% 011ld ", psec);
				printf("az %5.1f el %5.1f ", az, el);
				printf("type %02d ", s_mode);
				printf("vol%5d ", v_num);
				printf("num%6d ", s_num);
				printf("fixed%5.1f ", fang);
				printf("tr %1d ", transition);
				printf("\n");
			}
#endif

#ifdef DEBUGGING_01
			Sseg->az_manual = az;	// Not as Dixon et al intended--just for debugging
			Sseg->el_manual = el;	// Same here
			Sseg->time_limit = transition; // Not as Dixon et al intended--just for debugging
#endif
			cntr++;					// Increment the utility counter

			if(pulsectr % SYNC_FREQ == 0) // Send sync packet every SYNC_FREQ pulses
			{		
				send(Socket,(const char *)Sync,sizeof(iwrf_sync_t),0);
				packetctr++;	// Increment the packet counter
			}

			if(pulsectr % SYNC_FREQ == 5) // Send sync packet every SYNC_FREQ pulses
			{		
				send(Socket,(const char *)Rinfo, sizeof(iwrf_radar_info_t),0);		
				packetctr++;	// Increment the packet counter
			}

			if(pulsectr % SYNC_FREQ == 10) // Send sync packet every SYNC_FREQ pulses
			{		
				send(Socket,(const char *)TSPinfo, sizeof(iwrf_ts_processing_t), 0);		
				packetctr++;	// Increment the packet counter
			}


			send(Socket,(const char *)Sseg,sizeof(iwrf_scan_segment_t),0);
#ifdef MONITOR_OUT
			printf("ID %02x ", Sseg->packet.id & 0xFF);
#endif
			packetctr++;	// Increment the packet counter
			
			sendstat = (int)send(Socket,(const char *)PulseH,sizeof(iwrf_pulse_header_t),0);
#ifdef MONITOR_OUT
			printf("ID %02x ", PulseH->packet.id & 0xFF);		
#endif
#ifdef DEBUGGING_03
			printf("packet %011d ", packetctr);
			printf("pulse %011d  ", pulsectr);
			printf("stat %d ", sendstat);
			printf("\n");
#endif
			packetctr++;	// Increment the packet counter

			pulsectr++;		// Increment the pulse sequence counter

			Sleep(WAITms);
		}while(sendstat > 0);

		// Socket error has terminated inner loop

		printf("Socket disconnected. Sendstat = %d. Reinitialize\n", sendstat);
		closesocket(Socket);	// Shut down socket
		WSACleanup();			// Clean up communications			
	}while(1);					// Go back and restart
	return(0);					
}

void init_ts_processing(iwrf_ts_processing_t *ts)
{
	ts->packet.id = IWRF_TS_PROCESSING_ID;
	ts->packet.len_bytes = sizeof(iwrf_ts_processing_t);
	ts->max_gate = 0;
	ts->prt_usec = 1000;
	ts->prt2_usec = 1000;
	ts->prf_mode = 0;
	ts->pulse_type = 0;
	ts->gate_spacing_m = 150;
	ts->clutter_filter_number = 0;
	ts->integration_cycle_pulses = 1;
	ts->cal_type = 0;
	ts->range_gate_averaging = 1;
	ts->range_start_km = 0;
	ts->range_stop_km = 0;
	return;
}

void init_radar_info(iwrf_radar_info_t *ri)
{
	ri->packet.id = IWRF_RADAR_INFO_ID;
	ri->packet.len_bytes = sizeof(iwrf_radar_info_t);
	ri->beamwidth_deg_v = 1;
	ri->beamwidth_deg_h = 1;
	ri->latitude_deg = 0;
	ri->longitude_deg = 0;
	ri->platform_type = 0;
	return;
}

void init_packet_info(iwrf_packet_info *pinfo)
{
	pinfo->radar_id = 0;
	pinfo->seq_num = 0;
	pinfo->version_num = 5;
	return;
}
void init_scan_segment(iwrf_scan_segment_t *s)
{
	s->packet.id = IWRF_SCAN_SEGMENT_ID;
	s->packet.len_bytes = sizeof(iwrf_scan_segment_t);
	return;
}

void init_sync(iwrf_sync_t *sy)
{
	sy->packet.id = IWRF_SYNC_ID;
	sy->packet.len_bytes = sizeof(iwrf_sync_t);
	sy->magik[0] = IWRF_SYNC_VAL_00;
	sy->magik[1] = IWRF_SYNC_VAL_01;
	return;
}

void init_pulse_header(iwrf_pulse_header_t *ph)
{
	ph->packet.id = IWRF_PULSE_HEADER_ID;
	ph->packet.len_bytes = sizeof(iwrf_pulse_header_t);
	ph->gate_spacing_m = 150;
	ph->hv_flag = 0;
	ph->n_channels = 0;
	ph->n_gates = 0;
	ph->n_data = 0;
	ph->hv_flag = 0;
	ph->prt = 0.1;
	ph->iq_offset[0] = 0;
	ph->iq_offset[1] = 0;
	ph->iq_offset[2] = 0;
	ph->iq_offset[3] = 0;
	ph->burst_mag[0] = 0;
	ph->burst_mag[1] = 0;
	ph->burst_mag[2] = 0;
	ph->burst_mag[3] = 0;
	ph->burst_arg[0] = 0;
	ph->burst_arg[1] = 0;
	ph->burst_arg[2] = 0;
	ph->burst_arg[3] = 0;
	ph->burst_arg_diff[0] = 0;
	ph->burst_arg_diff[1] = 0;
	ph->burst_arg_diff[2] = 0;
	ph->burst_arg_diff[3] = 0;
	ph->follow_mode = 0;
	ph->phase_cohered = 0;
	ph->status = 0;
	ph->n_gates_burst = 0;
	ph->scale = 0;
	ph->offset = 0;
	ph->prt_next = 0.1;
	ph->pulse_width = 150;
	ph->range_offset_m = 0;
	ph->follow_mode = IWRF_FOLLOW_MODE_NONE;

	return;
}