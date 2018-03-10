#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#pragma once

#include <stdio.h>
FILE *openoutputfile(char *filename, char *path);
void getkeyval(char *line,char *keyword,char *value);
int arcSet(char *str, char *fmt, void *parm, char *keyword, int line, char *fname);
int arcParse(char *keyword,char *parms[]);
void makefilename(char *pfilename, char *filename, char *path);
char *getArcTimeStamp(void);

//PIRAQX   *initpiraqx(char *filename);
//void readpiraqx(char *fname, PIRAQX *px);
