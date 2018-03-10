#include <windows.h>  // for Sleep call
#include <iostream>
#include <tchar.h>
#include <conio.h>

#include "sockout.h"
//#include "winsock2.h"

// This function reads integer ABP's from a fifo and writes them to a socket.
void main(void)
{
	//   sockout_open("/PRODUCTSH",0xFFFFFFFF,21010);
	sockout_open("/PRODUCTSH",inet_addr("169.254.122.144"),21010);

	printf("press any key to quit\n");
	while(!_kbhit())  Sleep(100);	_getch();

	sockout_close();
}

