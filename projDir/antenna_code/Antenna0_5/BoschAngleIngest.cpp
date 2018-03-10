// BoschAngleIngest.cpp A simple application to read the Bosch ACU output
// strings, decode it and stuff shared memory with the binary data.
// Usage: BoschAngleIngest Hostname port
// Copyright 2010 Advanced Radar Corporation.
// Author: F. Hage July 2010

#include "stdafx.h"
#include <sys/timeb.h>
#include <time.h>

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Winternl.h>

#include "BoschAngleData.h"

#define TCPIP_BUFLEN 128
//#define DEBUG 1   // Print out diagnostic info
//#define ADD_DELAY 1 // Add a small delay between stuffing lines into memory
int  Tcp_client(int argc, char **argv);
int  Fake_client(int argc, char **argv);

int wait(int returnCode)
{
	printf("Press any key to exit ...");
	while(!_kbhit()){;}	// Pause until keystroke.
    return returnCode;
}

/**************************************************
*	Name: BoschAngleIngest
*	Purpose: Connect and accept TCP data from Bosch ACU.
*	Usage:	BoschAngleIngest. - Starts from RunHiq
**************************************************/
int __cdecl main(int argc, char **argv) 
{
    int iResult;

	// Validate runtime parameters
    if (argc != 3) {
        printf("usage: %s server-name port\n", argv[0]);
		printf("Use: '%s fake 0' for Simulated angles.\n", argv[0]);
		wait(1);
		exit(-1);
    }
	if(strncmp(argv[1],"fake",4) != 0) {
		iResult = Tcp_client(argc, argv);
	} else {
		iResult = Fake_client(argc, argv);
	}
	return iResult;
}
///////////////////////////////////////////////////////
// TCP_CLIENT: Conect to the Bosch Angle Serive and Stuff into Shared memory
int  Tcp_client(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo     hints;
	struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    char recvbuf[TCPIP_BUFLEN];
    Bosch_ACU_data_t Acu;
	Bosch_ACU_data_t *acup;
    int iResult;

	printf("----------Bosch ACU Angle Ingest Client Starts------------\n");

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("BoschAngleIngest:getaddrinfo failed: %d\n", iResult);
		wait(1);
 		exit(-1);
   }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], argv[2], &hints, &result);
	//iResult = getaddrinfo(ACU_HOST_NAME, ACU_PORT, &hints, &result);

    if ( iResult != 0 ) {
        printf("BoschAngleIngest: getaddrinfo failed. Error: %d\n", iResult);
        WSACleanup();
		wait(1);
		exit(-1);
    }
    ptr=result;
	// Create a SOCKET for connecting to server
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
        printf("BoschAngleIngest:Socket Error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
		wait(1);
		exit(-1);
     }

    // Connect to server.
    iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        ConnectSocket = INVALID_SOCKET;
		printf("BoschAngleIngest:Problem connecting to %s port %s\n",argv[1],argv[2]);
	} else {
		printf("BoschAngleIngest: Connected to Angle Service host; %s, port %s.\n",argv[1],argv[2]); 
	}
    freeaddrinfo(result);
    
	// From Example at:  http://msdn.microsoft.com/en-us/library/aa366551(VS.85).aspx
    HANDLE hMapFile;
    PFLOAT pBuf;

    hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // Mem Paging file
                 NULL,                    // default security 
                 PAGE_READWRITE,          // read/write access
                 0,                       //  Max Size (high-order DWORD) 
                 MBUF_SIZE,                // Max Size (low-order DWORD)  
                 SHM_Name);                 // File Name
 
   if (hMapFile == NULL) { 
	printf("BoschAngleIngest: Could not create memory mapped file. Error: (%d)\n",GetLastError());
	wait(1);
	exit(-1);
   }

   pBuf = (PFLOAT) MapViewOfFile(hMapFile,   // handle to map object
						FILE_MAP_ALL_ACCESS, // read/write permission
						0,                   
						0,                   
						MBUF_SIZE);           
 
   if (pBuf == NULL) { 
	printf("ACU Client: Could not map view of file. Error: (%d).\n",GetLastError()); 
	CloseHandle(hMapFile);
	wait(1);
	exit(-1); 
   }

   acup = (Bosch_ACU_data_t *) pBuf; 
   int buf_position = 0; // Buffer position.
   int line_count = 0;
   char line[MBUF_SIZE];   // space for one line of data;
   char *line_ptr = line;
   char *buf_ptr = recvbuf;
   ZeroMemory(&Acu, sizeof(Acu)); // Clear the structure
   unsigned long t_hibits,t_lobits,nfields;
   int	i1,i2;
  
	// Receive until the peer closes the connection
	do 
	{
		ZeroMemory(recvbuf, TCPIP_BUFLEN);
		char *rbuf_ptr = recvbuf;
		int	 req_size = TCPIP_BUFLEN;
		do { // Read a Buffer of bytes 
			iResult = recv(ConnectSocket, rbuf_ptr, req_size, 0);
			rbuf_ptr += iResult;
			req_size -= iResult;
			if (iResult < 0) {
				printf("BoschAngleIngest: Recv failed. WSA Error:%d\n", WSAGetLastError());
			}
		} while( iResult >= 0 && req_size > 0) ;

		buf_ptr = recvbuf;
		buf_position = 0;

		// Transfer a whole line or until end of the buffer
		while (*buf_ptr != '\n' && buf_position < TCPIP_BUFLEN ) {
			*line_ptr++ = *buf_ptr++;
			buf_position++;
		}
		if(*buf_ptr == '\n') { // reached the end of the data line
			*line_ptr++ = *buf_ptr++;   // Copy the newline
			buf_position++;
			*line_ptr++ = '\0';
			// Process the data line & stuff shared memory
			//nfields = sscanf(line,"%f,%f,%d,%f,%f,%d,%d,0x%x,0x%x,%d,%d,%d,%u,%u,%f,%f,%f,%f",
			  nfields = sscanf(line,"%f,%f,%d,%f,%f,%d,%d,%x,%x,%d,%d,%d,%u,%u,%f,%f,%f,%f",
				&Acu.az_target,&Acu.el_target,
				&Acu.transition_flag,
				&Acu.az_velocity,&Acu.el_velocity,
				//&Acu.az_temper,&Acu.el_temper,
				&i1,&i2, 
				&Acu.az_status,&Acu.el_status,
			
				&Acu.scan_type,&Acu.tilt_number, &Acu.volume_number,
				&t_hibits,&t_lobits,          // Time as high and low 32bit words - Microseconds
				&Acu.az_position,&Acu.el_position,
				&Acu.az_error,&Acu.el_error);

				Acu.az_temper = (float) i1;
				Acu.el_temper  = (float) i2;
				
				Acu.d_time = (4294.967296 * (double)t_hibits) + ((double)t_lobits / 1000.0);
				Acu.not_clear_to_read = 0; 


#ifdef DEBUG
				if(line_count < 20 || nfields != 18)  {
					printf("SRC: %s\n",line);
					printf("Line:%d NF:%d T%.6f Tgts:%05.1f,%05.1f, Tflg:%1d, Status:%x,%x\n",
						line_count,nfields,Acu.d_time,
						Acu.az_target,Acu.el_target,
						Acu.transition_flag,
						Acu.az_status,Acu.el_status
						);
					printf("ScanT:%1d, Tilt:%02d, Vol:%02d, Az:%07.3f, EL: %07.3f, Errs:%.3f,%.3f\n",
						Acu.scan_type,Acu.tilt_number,Acu.volume_number,
						Acu.az_position,Acu.el_position,
						Acu.az_error,Acu.el_error);
				}
#endif
				

				if(nfields > 17) {
					line_count++;
					Acu.line_number = line_count;
					acup->not_clear_to_read = 1;
					CopyMemory((PVOID)pBuf,(PVOID) &Acu, sizeof(Acu)); // Copy the struct to SHMEM
				}
#ifdef ADD_DELAY
				Sleep(4); // DEBUG - ADD DELAY to simulate output of Bosch
#endif			
				line_ptr = line; // set back to the beginning of the line.
				ZeroMemory(line, MBUF_SIZE); // Clear the line for the next string

				while (buf_position < TCPIP_BUFLEN ) { // copy the remaining chars in the recv_buffer to the parsing buffer.
					*line_ptr++ = *buf_ptr++;
					buf_position++;
				};
			} // if at end of line.

    } while( iResult > 0 );

    // cleanup
	printf("BoschAngleIngest: Closing socket.\n");
    closesocket(ConnectSocket);
    WSACleanup();
	//wait(0);
	
   UnmapViewOfFile(pBuf);
   CloseHandle(hMapFile);

   return 0;
}


///////////////////////////////////////////////////////////////////////////
// Simulate Volume scans coming from the Bosch
//
#define ANTENNA_RATE 10.0 // degrees/sec
#define ANGLE_STEP (ANTENNA_RATE/500)  // 500 steps/sec. (2 msec)
#define CIRCLE_STEPS (360.0/ANGLE_STEP)

int Fake_client(int argc, char **argv) 
//int _tmain(int argc, _TCHAR* argv[])
{
    Bosch_ACU_data_t Acu;
	Bosch_ACU_data_t *acup;
    int iResult;

	printf("----------Fake Angle Ingest Client Starts------------\n");

	// From Example at:  http://msdn.microsoft.com/en-us/library/aa366551(VS.85).aspx
    HANDLE hMapFile;
    PFLOAT pBuf;

    hMapFile = CreateFileMapping(
                 INVALID_HANDLE_VALUE,    // Mem Paging file
                 NULL,                    // default security 
                 PAGE_READWRITE,          // read/write access
                 0,                       //  Max Size (high-order DWORD) 
                 MBUF_SIZE,                // Max Size (low-order DWORD)  
                 SHM_Name);                 // File Name
 
   if (hMapFile == NULL) { 
	printf("BoschAngleIngest: Could not create memory mapped file. Error: (%d)\n",GetLastError());
	wait(1);
	exit(-1);
   }

   pBuf = (PFLOAT) MapViewOfFile(hMapFile,   // handle to map object
						FILE_MAP_ALL_ACCESS, // read/write permission
						0,                   
						0,                   
						MBUF_SIZE);           
 
   if (pBuf == NULL) { 
	printf("ACU Client: Could not map view of file. Error: (%d).\n",GetLastError()); 
	CloseHandle(hMapFile);
	wait(1);
	exit(-1); 
   }
   struct _timeb timebuffer;

   acup = (Bosch_ACU_data_t *) pBuf;
   ZeroMemory(&Acu, sizeof(Acu)); // Clear the structure
   
   Acu.transition_flag = 1;
   Acu.el_target = 0.5;
   Acu.az_position = 0.0;
   Acu.el_position = 0.0;
   Acu.az_temper = 31;
   Acu.el_temper  = 32;
   Acu.az_velocity = 40.0;
   Acu.el_velocity = 0.0;
   Acu.az_status = 0x9D;
   Acu.el_status = 0x9D;
   Acu.scan_type = PPI_VOLUME_SCAN;
   Acu.tilt_number = 0;
   Acu.az_error = 0.0;
   Acu.el_error = 0.0;

   // Get the system time 
   _ftime64_s( &timebuffer );
   Acu.d_time = (double) timebuffer.time + (timebuffer.millitm * 0.001); 
   Acu.line_number = 0;

	// Fake angles at 10 deg/sec movement
   while(1) {  // DO Volumes forever.
	  Acu.transition_flag = 0;
	  Acu.tilt_number = 0;
	  Acu.el_target = 0.0;
	  Acu.el_velocity = 0.0;
	  Acu.volume_number++;
	  for(int i = 0; i < 5; i++){ // Five tilts.
		  Acu.el_velocity = 0.0;
		  // Rotate 360 degrees  in the current tilt.
		  for(int j = 0; j < CIRCLE_STEPS; j++) { // 4500*0.8 = 360 degrees per tilt.
			  Acu.az_position += (double)ANGLE_STEP;
			  if(Acu.az_position > 360.0) Acu.az_position -= 360.0;
			  Acu.line_number++;
			   _ftime64_s( &timebuffer );
			   Acu.d_time = (double) timebuffer.time + (timebuffer.millitm * 0.001); 
 
			  acup->not_clear_to_read = 1;
			  CopyMemory((PVOID)pBuf,(PVOID) &Acu, sizeof(Acu)); // Copy the struct to SHMEM
			  Sleep(1); //  DELAY to simulate output of Bosch
		  }
		  // Move up to next tilt - 1 degree higher.
		  Acu.transition_flag = 1;
		  Acu.el_velocity = 1.0;
		  Acu.el_target += 1.0;
		  for(int j = 0; j < 500; j++) { // 500 steps of transition  to next tilt.
			  Acu.az_position += (double)ANGLE_STEP;
			  if(Acu.az_position > 360.0) Acu.az_position -= (double)360.0;
			  Acu.el_position += (double) 0.002; 

			   _ftime64_s( &timebuffer );
			   Acu.d_time = (double) timebuffer.time + (timebuffer.millitm * 0.001); 
 			  Acu.line_number++;
			  acup->not_clear_to_read = 1;
			  CopyMemory((PVOID)pBuf,(PVOID) &Acu, sizeof(Acu)); // Copy the struct to SHMEM
			  Sleep(1); //  DELAY to simulate output of Bosch
		  }
		  Acu.transition_flag = 0;
		  Acu.tilt_number++;
	  }
	  Acu.el_velocity = 0.0;
	  // Rotate 360 degrees  in the current tilt.
	  for(int j = 0; j < CIRCLE_STEPS; j++) { // 4500*0.8 = 360 degrees per tilt.
		  Acu.az_position += (double)ANGLE_STEP;
		  if(Acu.az_position > 360.0) Acu.az_position -= 360.0;
		  Acu.line_number++;
		   _ftime64_s( &timebuffer );
		   Acu.d_time = (double) timebuffer.time + (timebuffer.millitm * 0.001); 
 		  acup->not_clear_to_read = 1;
		  CopyMemory((PVOID)pBuf,(PVOID) &Acu, sizeof(Acu)); // Copy the struct to SHMEM
		  Sleep(1); //  DELAY to simulate output of Bosch
	  }
	 // After the 6th tilt - Move the Radar back down to starting elevation.
	  Acu.transition_flag = 1;
	  Acu.el_target = 0.0;
	  for(int j = 0; j < 2500; j++) { // 2000 steps to transition down to next volume.
		  Acu.az_position += (double)ANGLE_STEP;
		  if(Acu.az_position > 360.0) Acu.az_position -= (double)360.0;
		  Acu.el_position -= (double) 0.002;  
		   _ftime64_s( &timebuffer );
		   Acu.d_time = (double) timebuffer.time + (timebuffer.millitm * 0.001); 
 		  Acu.line_number++;

		  acup->not_clear_to_read = 1;
		  CopyMemory((PVOID)pBuf,(PVOID) &Acu, sizeof(Acu)); // Copy the struct to SHMEM
		  Sleep(1); // DELAY to simulate output of Bosch
	  }
    };
	
   UnmapViewOfFile(pBuf);
   CloseHandle(hMapFile);

   return 0;
}