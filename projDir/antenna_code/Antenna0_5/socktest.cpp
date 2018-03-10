// socktest.cpp : Defines the entry point for the console application.
//
//#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include "../udpsock.h"
//#include "winsock2.h"

void main(void)
   {
   int	outsock,insock,len,maxlen;
   bool b;

   outsock = open_udp_out(FALSE);
   insock = open_udp_in(21010);

   len = sizeof(int);
// TODO: Needs Winsock2 to compile.  Fix later.  20080617 WPQ
//   getsockopt(outsock,SOL_SOCKET,SO_MAX_MSG_SIZE,(char *)&maxlen,&len);
//   printf("SO MAX MSG SIZE = %ld\n",maxlen);
   
   len = sizeof(int);
   getsockopt(outsock,SOL_SOCKET,SO_SNDBUF,(char *)&maxlen,&len);
   printf("SO SNDBUF = %ld\n",maxlen);

   len = sizeof(bool);
   getsockopt(outsock,SOL_SOCKET,SO_BROADCAST,(char *)&b,&len);
   printf("SO BROADCAST = %s\n",b?"on":"off");

   len = sizeof(int);
   getsockopt(insock,SOL_SOCKET,SO_RCVBUF,(char *)&maxlen,&len);
   printf("SO RCVBUF = %ld\n",maxlen);

   len = sizeof(int);
   getsockopt(insock,SOL_SOCKET,SO_TYPE,(char *)&maxlen,&len);
   printf("SO TYPE = %ld\n",maxlen);
   if(maxlen == SOCK_DGRAM) printf("datagram!\n");

   close_udp(insock);
   close_udp(outsock);
   }

