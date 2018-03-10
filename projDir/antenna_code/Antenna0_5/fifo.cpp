/********************************************************/
// Generic FIFO routines using shared memory		 
//                                                   
// Each FIFO buffer consists of a generic header for    
// handling the shared memory and basic FIFO functions  
// (i.e. head pointer, base addresses, etc.), 
// a user header which describes something common about 
// the FIFO records (i.e. gate spacing, scaling factor, 
// etc.), and an array of FIFO records.			
//                                                      
// If the FIFO is full, the data just overwrites the    
// last FIFO record.					
//                    
// The FIFO writer pays no attention to the FIFO readers
// Each FIFO reader acts completely independently.
//
// Read at the tail, write at the head.
//
// It seems that if a B is attached to a fifo that A created,
// and A is terminated, no matter how badly, then the fifo
// persists until B is terminated. This is very nice!!!!!
// Otherwise before each access to fifo memory, a test would
// have to be made - but even then, the fifo could disappear
// at just that instant and give a memory exception.
//
// Using this as a working assumption would change the way
// some of these routines were written. This wasn't known
// when the routines were originally written.
/********************************************************/
#include "stdafx.h"

/* for shmem calls */
#include	<string.h>
#include	<stdio.h>
//#include	<windows.h>		// for Sleep() function

#include	"smfifo.h"		// Public
#include	"fifo.h"		// Private
#include	"shared_mem.h"

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
#include "fixed_files.h"

using namespace log4cxx;
using namespace log4cxx::helpers;
static LoggerPtr logger;

/* Allocate memory for a new fifo buffer. */
/* If the FIFO is already open, then just use it */
/* This assumes that a FIFO with a specific name is always */
/* used the same way by any task (i.e. size, structure type, etc.) */
FIFO *fifo_alloc(char *name, int headersize, int recordsize, int recordnum)
   {
   int 		size;
   HANDLE	hmap;
   FIFO		*fifo;

   // Fifo's total memory requirements are:
   // The FIFO header struct +
   // The User Header. +
   // recordnum  * (Record Size + 1 Int for Message Size)
   size = sizeof(FIFO) + headersize + (recordnum * (recordsize + sizeof(int)));
    
   fifo = (FIFO *)shared_mem_create(name, size, &hmap);

   if(fifo)
      {
	  /* initialize the fifo structure */
	  if(strncpy_s(fifo->name,sizeof(fifo->name),name,FIFONAMESIZE)){printf("FATAL");exit(99);}
	  fifo->header_off = sizeof(FIFO);		/* pointer to the user header */
	  fifo->fifobuf_off = fifo->header_off + headersize + sizeof(int);	/* pointer to fifo base address */ // step over the fifo record size
	  fifo->record_size = recordsize;				/* size in bytes of each FIFO record */
	  fifo->record_num = recordnum;					/* number of records in FIFO buffer */
	  fifo->head = 0;						/* indexes to the head and tail records */
      fifo->size = size;
	  fifo->hmap = hmap;
	  fifo->magic = FIFO_MAGIC;  /* do this last so clients can use it as done flag */
      }

   return(fifo);
   }

/* Get a pointer to the next available FIFO. */
/* Returns NULL if none available (already allocated */
/* by fifo_create). */
FIFO *fifo_open(char *name)
   {
   FIFO	*fifo;
   
   fifo = (FIFO *)shared_mem_open(name);
   return(fifo);
   }

// this is what you do to close a fifo that you created
int fifo_close(FIFO *fifo)
   {
   fifo->magic = 0;
   
   shared_mem_close(fifo,fifo->hmap);
   return(0);
   }

// this is what you do when you are done reading a fifo you didn't create
int fifo_disconnect(FIFO *fifo)
   {
   shared_mem_unmap(fifo);
   return(0);
   }

/* Return a pointer to the next working write record. */
void *fifo_get_write_address(FIFO *fifo)
   {
   return((char *)fifo + fifo->fifobuf_off + fifo->record_size * fifo->head);
   }

/* reset the FIFO pointers so that the FIFO appears empty */
void fifo_clear(FIFO *fifo, int *tail)
   {
   *tail = fifo->head;
   }

/* Return a pointer to the next working read record. */
/* Return a NULL if the FIFO is empty */
/* Return -1 if error */
void *fifo_get_address(FIFO *fifo, int ptr, int offset)
   {
   int	offptr;
   
   if(!fifo)	return((void *)0);

   offptr = ptr + offset;
   if(offptr >= fifo->record_num)	offptr %= fifo->record_num;
   while(offptr < 0) offptr += fifo->record_num;
   return((char *)fifo + fifo->fifobuf_off + fifo->record_size * offptr); 
   }

void fifo_set_rec_size(FIFO *fifo, int size)
   {
   *(int *)((char *)fifo + fifo->fifobuf_off - sizeof(int) + fifo->record_size * fifo->head) = size;
   }

int fifo_get_rec_size(FIFO *fifo, int tail)
   {
   return(*(int *)((char *)fifo + fifo->fifobuf_off - sizeof(int) + fifo->record_size * tail));
   }


/* Increment the FIFO head pointer */
/* pay no attention to the tail because there could be multiple readers */
/* the best you can do is just march along and hope everyone can keep up */
void fifo_increment_head(FIFO *fifo)
   {
   fifo->head = (fifo->head + 1) % fifo->record_num;		   /* increment head */
   }


/* Increment the FIFO tail pointer by advance. Return the number of used */
/* records or 0 on error */
int fifo_increment_tail(FIFO *fifo, int *tail)
   {
   if(!fifo)	return(-1);

   if(fifo_hit(fifo,*tail))
      *tail = (*tail + 1) % fifo->record_num;	/* increment tail */
   
   return(fifo_hit(fifo,*tail));
   }


// advance the tail by an arbitrary amount
int fifo_advance_tail(FIFO *fifo, int tail, int advance)
   {
   if(!fifo)	return(-1);

   while(advance < 0)	advance += fifo->record_num;  // handle negative advances (because mod doesn't work right for negative numbers)
   return((tail + advance) % fifo->record_num);
   }


/* Return number of records waiting to be read */
/* records or -1 on error */
int fifo_hit(FIFO *fifo, int tail)
   {
   int	used;

   if(!fifo)	return(-1);

   used = fifo->head - tail;
   if(used < 0)	
   {
	   used += fifo->record_num;
   }
  
   return(used);
   }

// This goes out to the system to see if the name is valid
int fifo_exist(char *name)
   {
   return(shared_mem_exist(name));
   }

void *fifo_get_header_address(FIFO *fifo)
   {
   return((char *)fifo + fifo->header_off);
   }
   
//*******************************************************************************
// Higher level functions
//*******************************************************************************

// return records ready to be read.
// Place fifo name and #of recs to be read in a buffer as another way to use.
int prtFifoFullness(char *aBuf, FIFO *aFifo, int aHead, int aTail)
{
	int fullness = (aHead<aTail) ? (aHead+(aFifo->record_num))-aTail : aHead-aTail;	// # of records in fifo.
	sprintf(aBuf, "%s %d\n", aFifo->name, fullness);
	return fullness;	
}

// wait forever until a fifo is open and ready to use
FIFO *fifo_usable(char *fifoname, int *tail)
{
	FIFO	*fifo=NULL;
	int		timeCount = 0;	// timeout and display error dialog

	do
	{
		while(!(fifo = fifo_open(fifoname)))
		{
			Sleep(300);	// wait forever until a fifo is opened
			timeCount++;
		}
		while(fifo->magic != FIFO_MAGIC && fifo_exist(fifoname))
		{
			Sleep(300);	// wait forever until 
			timeCount++;
		}
		if(	timeCount == 100) // 30 sec
		{
			// Start new process with message box.
			HANDLE	thread;
			LPDWORD dwThreadId = NULL;
			thread = CreateThread( 
								NULL,			// default security attributes
								0,				// use default stack size  
								arcWarnMsgBox,	// thread function 
								fifoname,		// argument to thread function, or NULL 
								0,				// use default creation flags 
								dwThreadId		// returns the thread identifier
								);		 
		}
	}   
	while(!fifo_exist(fifoname));

	fifo_clear(fifo,tail);

	return(fifo);
}

// Check once to see if a fifo is open and ready to use
FIFO *fifo_check_usable(char *fifoname, int *tail)
   {
   FIFO	*fifo;
   
   *tail = 0;

   if(fifo = fifo_open(fifoname))
      if(fifo->magic == FIFO_MAGIC)
         {
         fifo_clear(fifo,tail);
         return(fifo);
         }
         
   return(NULL);
   }

/*
Wait forever for 'advance' records or for the fifo to become invalid
Return a 1 if the fifo goes away (failure)
This absolutely requires that the fifo is longer than 10ms!!!!!!
Or that this routine is not suspended for longer than a fifo round.
This is valid, since if that was the case data will be lost no matter what.
Note: this routine advances the tail!!!! Do not also fifo_increment_tail()!
This can lock if the fifo is destroyed and a new one (with same name) is created before the tick timeout.
It is thought that this only happens if the user is clicking to close and open programs fast.
1/2 second timeout should make this possible only for the fastest clickers.
Leaves tail pointing to a valid record!
Arguments:
fifo - the fifo to operate on.
tail - the end of fifo marker.
advance - the number of records to wait for.
Returns:
0 on Success.
non zero on Failure.
*/
int	fifo_inc_and_wait(FIFO *fifo, int *tail, int advance)
{
	int tick;
	char	name[FIFONAMESIZE];
	int		timeCount = 0;

	// save the name so it can be checked later
	if(strncpy_s(name,sizeof(name),fifo->name,FIFONAMESIZE))
	{
		printf("FATAL");
		exit(99);
	}

	*tail = (*tail + advance) % fifo->record_num;	// Increment tail.

	// fifo_hit() returns the number of records waiting to be read.
	for( tick=0; 
		 (fifo->magic == FIFO_MAGIC) && (fifo_hit(fifo,*tail) >= fifo->record_num - advance || !fifo_hit(fifo,*tail)); 
		 tick++ )
	{
		Sleep(10);		// milliseconds/tick
		if(tick > 50)	// test after fraction of a second
		{
			tick = 0;
			if(!fifo_exist(name))
				return(1);				// if so, get out
		}

		// Warn user of hang.
		if(	timeCount == 1000) // ticks after entry to a Warning MessageBox.
		{
			// Start a new process with message box.
			HANDLE	thread;
			LPDWORD dwThreadId = NULL;
			thread = CreateThread( 
								NULL,			// default security attributes
								0,				// use default stack size  
								arcWarnMsgBox,	// thread function 
								fifo->name,		// argument to thread function, or NULL 
								0,				// use default creation flags 
								dwThreadId		// returns the thread identifier
								);		 
		}
		timeCount++;
	}

	return(!(fifo->magic == FIFO_MAGIC));
}

// Returns a zero until the fifo has enough data past tail
// On success, the tail can be advanced, and data at tail will be valid
int	fifo_test_advance(FIFO *fifo, int tail, int advance)
{
   int test;
   
   test = fifo_advance_tail(fifo,tail,advance);
   return((fifo_hit(fifo,test) < fifo->record_num - advance && fifo_hit(fifo,test)));
}

   
// This method of waiting for the next record (unlike fifo_wait() does not increment the tail
// You must explicitely call fifo_increment_tail()
#define	TIMEOUT	100
int	fifo_timeout(FIFO *fifo, int tail)
   {
   for(int tick=0; tick<TIMEOUT && !fifo_hit(fifo,tail); tick++)	Sleep(10);
   return(!fifo_hit(fifo,tail));
   }
   
// New way to wait!  This way gives a mechanism to get out
// Tests for a closed fifo no sooner than every 1/2 second
// 'waittime' 'time between FIFO exists checks'	'frequency of advance tests'	
//	1				500								1
//	10				500								10
int fifo_safe_wait(FIFO *fifo, char *name, int *tail, int advance, int waitime, int *semiphore)
   {
   int	tick,tickout;
   
   if(!waitime)	waitime = 1;
   tickout = 500 / waitime;	if(!tickout)	tickout = 1;
   
   for(tick=0;  fifo->magic == FIFO_MAGIC && !fifo_test_advance(fifo, *tail, advance) && !*semiphore; tick++)
   {
	   Sleep(waitime);
	   if(tick > tickout)
	   {
		   tick = 0;	// Check forever, unil an Abort (semaphore value).
		   if(!fifo_exist(name))
		   {
			   fifo_disconnect(fifo);
			   return(FIFO_GONE);
		   }
	   }
   }

   if(*semiphore)
   {
	   return(FIFO_WAITABORT);			// did someone want to abort?
   }
   
   if(fifo->magic != FIFO_MAGIC)	
   {
	   fifo_disconnect(fifo); 
	   return(FIFO_GONE);
   }

   *tail = fifo_advance_tail(fifo,*tail,advance);
   return(0);
}

DWORD WINAPI arcWarnMsgBox( LPVOID lpParam ) 
{ 
    HANDLE hStdout;
	const size_t BUFSIZE = 256; 
	TCHAR msgbuf[BUFSIZE];
	TCHAR *p_msg;
	TCHAR *aCaption= "Receiver Input Problem";

	// Delete watchdog file as a signal for a restart.
	sprintf_s(msgbuf, BUFSIZE, "del %s", RESTART_WATCHDOG_FIFO_FILE);
	system(msgbuf);

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if( hStdout == INVALID_HANDLE_VALUE )
	{
        return(1);
	}

    // Cast the parameter to the correct data type.
	sprintf_s(msgbuf,BUFSIZE,"Data is not being received at fifo:'%s'.  \n\nThere could be \n\n",(char*)lpParam);
	p_msg = strcat(msgbuf,"a) no trigger if externally triggered, or\n");
	p_msg = strcat(msgbuf,"b) an improper configuration file, or\n");
	p_msg = strcat(msgbuf,"c) a system software failure.\n\n");
	p_msg = strcat(msgbuf,"The Receiver application will automatically restart soon.");
	LOG4CXX_WARN(logger, msgbuf);
	MessageBox(NULL, TEXT(p_msg), TEXT(aCaption), MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
	return(0);
}


/* DEBUG functions */
/*
FIFO *dump_record_data(FIFO aFifo, char *targetName)
{
	// Dump record hdr part.
	if(strcmp(aFifo->name,targetName)==0)
	{
		printf("----------------- %s :HEADER BELOW--------------------",pData->dstfifoIq->name);
		for(char *j=writeIqRecord; j<(char *)(writeIqRecord+sizeof(PIRAQX));j++)
		{
		printf("adx='%8X',hex='%2X',char='%c'.\n",j,*j,*j);	// BAD PRINTOUT!  WHY?
		}
		printf("----------------- %s :HEADER ABOVE--------------------",pData->dstfifoIq->name);
	}
}
	

FIFO *dump_record_data(FIFO aFifo, char *targetName)
{
	// Dump record hdr part.
	if(strcmp(aFifo->name,targetName)==0)
	{
		printf("----------------- %s :DATA BELOW--------------------",targetName);
		// Dump record data part.
		i=0;
		for(float *j=(float *)(writeIqRecord+sizeof(PIRAQX)); i<6*fixedpx->gates;j++)
		{
			printf("adx='%8X',hex='%8X',float='%f'.\n",j,*j,*j);	// bad printout!  why?
			i++;
		}
		printf("----------------- %s :DATA ABOVE--------------------",targetName);
	}
}
*/
