#pragma once

DWORD WINAPI SockoutThreadProc( LPVOID lpParam );
void sockoutworker(void);
void sockout_open(char *src_fifo, int inetaddr, int port);
void sockout_close(void);

