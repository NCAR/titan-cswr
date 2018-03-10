#pragma once

typedef enum {	STOPPED_SUCESSFULLY = 0,
				NONE_RUNNING,
				NOT_FOUND
} STOP_PROCESS_STATE;

int process_start(char *name);
int process_stop(char *name);
void process_stopall(void);

