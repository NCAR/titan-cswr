//#include "Winsock2.h"
#include <windows.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "smfifo.h"
#include "sockout.h"
#include "udpsock.h"

// CONSOLE display
#define CONSOLE true

#define	MAXNAME	80

static int Sock,Inetaddr,Port;
static char	SrcFifoName[MAXNAME];
static HANDLE Thread; 
static int Abort = 0;

DWORD WINAPI SockoutThreadProc( LPVOID lpParam ) 
{ 
    HANDLE hStdout;
//    PMYDATA pData;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if( hStdout == INVALID_HANDLE_VALUE )
        return 1;

    sockoutworker();
	return(0);
}


// This function reads integer ABP's from a fifo and writes them to a socket.
void sockoutworker(void)
   {
   FIFO		*srcfifo = NULL;
   int		tail = 0;
   int		size,test; 
   void		*vptr;

printf("This is the program that reads %s FIFO\n",SrcFifoName);
printf("And writes to a broadcast socket on port %d\n",Port);

top:
   printf("\nOpening fifo '%s'.\n",SrcFifoName);

   /* open up a FIFO buffer */
   while(!(srcfifo = fifo_check_usable(SrcFifoName, &tail)) && !Abort)	Sleep(300);   // wait forever but with a way out!
   if(Abort)	goto bottom;

   while(1)
      {
      // wait for a new beam of data in the src fifo
      test = fifo_safe_wait(srcfifo, SrcFifoName, &tail, 1, 10, &Abort);
      if(test == FIFO_WAITABORT)	goto bottom;
      if(test == FIFO_GONE)			goto top;

  	// Visually indicate reception of a new record (beam) of APB data.
	  if(CONSOLE) { printf("+"); fflush(stdout); }

      // Get pointer to the newly available record from the source FIFO
      vptr = (void *)fifo_get_address(srcfifo,tail,0);	// get the address of the first valid record

      // Get the size of the data at this record
      size = fifo_get_rec_size(srcfifo,tail);
      
      // Send the data out on an ethernet datagram UDP packet
      send_udp_packet(Sock, Inetaddr, Port, vptr, size);
      }
      
bottom:

   if(srcfifo)	fifo_disconnect(srcfifo);		// disconnect from the source fifo if you were using it.
   }


void sockout_open(char *src_fifo, int inetaddr, int port)
   {
   DWORD	dwThreadId;

   strncpy(SrcFifoName,src_fifo,MAXNAME);		// save copy of source fifo name used in worker thread
   Inetaddr = inetaddr;
   Port = port;

   // open the output socket
   Sock = open_udp_out((inetaddr == 0xFFFFFFFF) ? 1 : 0);


        Thread = CreateThread( 
            NULL,              // default security attributes
            0,                 // use default stack size  
            SockoutThreadProc,     // thread function 
            NULL,             // argument to thread function		send it nothing!
//            pData,             // argument to thread function 
            0,                 // use default creation flags 
            &dwThreadId);   // returns the thread identifier 
 
        // Check the return value for success. 
 
        if (Thread == NULL) 
        {
            ExitProcess(0);
        }
   }


void sockout_close(void)
   {
   Abort = 1;
   WaitForSingleObject(Thread,10000);	// wait 10 seconds for thread to stop
   CloseHandle(Thread);
   close_udp(Sock);
   }




