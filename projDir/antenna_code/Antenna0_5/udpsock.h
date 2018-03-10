#pragma once

/****************************************************/
/* UDP defines                                      */
/****************************************************/

typedef	float	float4;
typedef	unsigned __int32	uint4;
typedef	unsigned __int64	uint8;

#define	UDP_MAGIC		0x87654321
#define	MAXPACKET	1200

/* define UDP packet types */
#define	UDPTYPE_CMD		1
#define	UDPTYPE_IQDATA	2
#define	UDPTYPE_ABPDATA	3
#define	UDPTYPE_PRQDATA	4
// for consistency make these equal to config dataformat values: 
#define	UDPTYPE_PIRAQ_ABPDATA				16
#define	UDPTYPE_PIRAQ_ABPDATA_STAGGER_PRT	17

/* global define of number of records in the various FIFO's */
#define	PIRAQ_FIFO_NUM		58		// 136KB packets in 8MB shared memory
#define	ABPDATA_FIFO_NUM	20

/* define the data communications port numbers */
#define	IQDATA_DATA_PORT		21060
#define	ABPDATA_DATA_PORT		21065

/************************************************************/
/* set up the infrastructure for intraprocess communication */
/************************************************************/
typedef struct {
    uint4 magic;		/* must be 'UDP_MAGIC' value above */
    uint4 type;			/* e.g. DATA_SIMPLEPP, defined in piraq.h */
    uint4 sequence_num;	/* increments every beam */
    uint4 totalsize;	/* total amount of data only (don't count the size of this header) */
    uint4 pagesize;		/* amount of data in each page INCLUDING the size of the header */
    uint4 pagenum;		/* packet number : 0 - pages-1 */
    uint4 pages;		/* how many 'pages' (packets) are in this group */
} UDPHEADER;

typedef struct {
	UDPHEADER		udphdr;
	char				buf[MAXPACKET];
	} UDPPACKET;

typedef struct {
	int		type;
	int		count;
	int		flag;		/* done, new, old, handshake, whatever, ..... */
	int		arg[5];
	}	COMMAND;
	
//typedef struct {			/* this structure must match the non-data portion of the PACKET structure */
//	UDPHEADER	udp;
//	COMMAND		cmd;
//	INFOHEADER	info;
//	} PACKETHEADER;

#define	HEADERSIZE		sizeof(PACKETHEADER)
#define	IQSIZE			(sizeof(short) * 4 * MAXGATES) 
#define	ABPSIZE			(sizeof(float) * 12 * MAXGATES)
#define DIAGNOSTICSIZE	2*(sizeof(float))*2*10*256	// 2*(sizeof(float))*2channels*10gates*256hits = 160*256 = 40960 

int open_udp_out(int broadcast);
int open_udp_in(int port);
void close_udp(int sock);
unsigned int send_udp_packet(int sock, int inetaddr, int port, void *ptr, int size);
int receive_udp_packet(int sock, void *ptr, int maxbufsize);
