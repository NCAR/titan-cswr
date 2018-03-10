/********************************************************************/
/* set up the additional internal infrastructure for software FIFOs */
/********************************************************************/
#pragma once
/* MOVED TO smfifo.h
#define	FIFONAMESIZE	80

typedef	struct
{
	char	name[FIFONAMESIZE];		// name that identifies the type of FIFO buffer
	int		magic;			// magic number indicating the FIFO has been initialized
	int		size;			// total size of allocated memory for this FIFO
	int		header_off;		// offset to the user header (can't use absolute address here)
	int		fifobuf_off;	// offset to fifo base address
	int		record_size;	// size in bytes of each FIFO record
	int		record_num;		// number of records in FIFO buffer
	int		head;			// index to the head of the fifo 
	HANDLE	hmap;			// handle to the shared mem CreateFileMapping in windows 
} FIFO;


#define	FIFO_GONE		1
//#define	FIFO_WAITABORT	2

// fifo.cpp 
// note: use fifo_close only for fifo's created with fifo_alloc (created)
//       otherwise, use fifo_disconnect for fifo's created with fifo_open (already created, just opened)

// EXPOSED DLL METHODS (see smfifo.def)
FIFO *fifo_alloc(char *name, int headersize, int recordsize, int recordnum);
FIFO *fifo_check_usable(char *fifoname, int *tail);
void  fifo_clear(FIFO *fifo, int *tail);
int	  fifo_close(FIFO *fifo);
int   fifo_disconnect(FIFO *fifo);
void *fifo_get_address(FIFO *fifo, int ptr, int offset);
void *fifo_get_header_address(FIFO *fifo);
int   fifo_get_rec_size(FIFO *fifo, int tail);
void *fifo_get_write_address(FIFO *fifo);
int	  fifo_inc_and_wait(FIFO *fifo, int *tail, int advance);
void  fifo_increment_head(FIFO *fifo);
FIFO *fifo_open(char *name);
int   fifo_safe_wait(FIFO *fifo, char *name, int *tail, int advance, int waitime, int *semiphore);
void  fifo_set_rec_size(FIFO *fifo, int size);
FIFO *fifo_usable(char *fifoname, int *tail);
*/

#define	FIFO_MAGIC		0x12345678

// INTERNAL METHODS (NOT EXPOSED)
int   fifo_advance_tail(FIFO *fifo, int tail, int advance);
int   fifo_exist(char *name);
int   fifo_hit(FIFO *fifo, int tail);
int   fifo_increment_tail(FIFO *fifo, int *tail);
int	  fifo_test_advance(FIFO *fifo, int tail, int advance);
int	  fifo_timeout(FIFO *fifo, int tail);
DWORD WINAPI arcWarnMsgBox( LPVOID lpParam );
