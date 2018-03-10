//#include	<Afx.h>			// force link order http://support.microsoft.com/kb/q148652/
#pragma once

#define CONFIG_FILE_DIR		"C:\\"
#define CONFIG_FILE_NAME	"hiqrcvr.cfg"
#define CONFIG_FILE_NAME_EXTENSION	"cfg"

#include <tchar.h>
#include	"piraqx.h"
PIRAQX   *initpiraqx(char *filename);
void readpiraqx(char *fname, PIRAQX *px);
void readCfg(char *fname, PIRAQX *px, CFG *cfg);
void getDefaultCfgFilename( TCHAR buf[], int bufsize );
bool isCfgVersionValid(int aCfgVersion);

