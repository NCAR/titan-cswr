#include "stdafx.h"
#include        <stdio.h>
#include        <time.h>
#include        <string.h>
#include        <ctype.h>

#include	"filesub.h"

char	timestamp[17];			// timestamp
char	*myTS = &timestamp[0];

void getkeyval(char *line,char *keyword,char *value)
{
	int  i,j;

	for(i=0; line[i] && isspace(line[i]); i++);  /* move to non-whitespace */

	/* transfer nonwhitespaces from line to keyword */
	for(j=0; line[i] && !isspace(line[i]); i++,j++)  keyword[j] = line[i];
	keyword[j] = 0;   /* null terminate */

	for(; line[i] && isspace(line[i]); i++);  /* move to non-whitespace */

	/* transfer all remaining characters to value */
	/* ignoring comments delineated by ; */
	for(j=0; line[i] && line[i] != ';'; i++,j++)  value[j] = line[i];
	value[j] = 0;

	/* remove all whitespaces at the end of value[] */
	for(; j && (isspace(value[j]) || !value[j]); j--)   value[j] = 0;
}


/* read in logic parameter with error catching */
/* fmt string is exactly like that for scanf except for:  */
/*   q   expects to scan for on or off or 1 or 0 */
/* returns 1 if success, 0 if failure */
int arcSet(char *str, char *fmt, void *parm, char *keyword, int line, char *fname)
{
	if(fmt[1] == 'q')   /* scan for on/off command */
	{
		if(!_stricmp(str,"on"))         *(int *)parm = 1;
		else if(!_stricmp(str,"1"))     *(int *)parm = 1;
		else if(!_stricmp(str,"off"))   *(int *)parm = 0;
		else if(!_stricmp(str,"0"))     *(int *)parm = 0;
		else if(!_stricmp(str,"odd"))   *(int *)parm = 2;
		else if(!_stricmp(str,"even"))  *(int *)parm = 4;
		else if(!_stricmp(str,"both"))  *(int *)parm = 6;
		else 
		{
			printf("unrecognized q value \"%s\" for parameter \"%s\" in line %d of %s\n",str,keyword,line,fname);
			*(int *)parm = 0;
			return(0);
		}
	}
	else /* then let sscanf do the work */
	{
		if(sscanf_s(str,fmt,parm,sizeof(str)) != 1)
		{
			printf("unrecognized %c value \"%s\" for parameter \"%s\" in line %d of %s\n",fmt[1],str,keyword,line,fname);
			((char *)parm)[0] = 0;
			return(0);
		}
	}
	return(1);
}

int arcParse(char *keyword,char *parms[])
{
	int  i;

	for(i=0; *parms[i]; i++)
		if(!_stricmp(keyword,parms[i]))
			break;

	return(i);
}

void makefilename(char *pfilename, char *filename, char *path)
{
	time_t      time_of_day;
	struct      tm      *fff;
	int			numChars = -1;
	char		*self = "makefilename";

	if(strlen(filename) == 0)    /* if filename not defined */
	{
		time_of_day = time(NULL);
		fff = gmtime(&time_of_day);

		numChars = sprintf_s(pfilename,sizeof(pfilename),"%s%s%02d%02d%02d%02d.%02dZ",path
			,(strlen(path)>0 && path[strlen(path)-1] != '\\')?"\\":"",fff->tm_year%100,fff->tm_mon+1,
			fff->tm_mday,fff->tm_hour,fff->tm_min);
		if(numChars == -1)
		{
			printf("Failure writing filename to buffer!"); 
			//exit(99);
		}
	}
	else
	{
		/* insert the path in front of the filename */
		if(0>sprintf_s(pfilename,sizeof(pfilename),"%s%s%s",path,
			(strlen(path)>0 && path[strlen(path)-1] != '\\')?"\\":"",filename))
		{
			printf("FATAL: %s: sprintf_s()", self);
			//exit(99);
		}
	}
}

FILE    *openoutputfile(char *filename, char *path)
{
	//time_t       time_of_day;
	//struct       tm      *fff;
	char         fname[80];
	FILE         *fp;
	errno_t		err;

	makefilename(fname,filename,path);
	if((err = fopen_s(&fp,fname,"wb")) !=0){printf("cannot open data output file %s\n",fname); /* exit(-1); */}
	return(fp);
}

// return a timestamp of Now in the pointed to string.
char *getArcTimeStamp(void)
{
	time_t      time_of_day;
	struct      tm  *now;
	int			numChars = -1;

		time_of_day = time(NULL);
		now = localtime(&time_of_day);

		numChars = sprintf_s(timestamp,sizeof(timestamp),
			"%02d%02d%02d_%02d:%02d:%02d",
			now->tm_year%100,
			now->tm_mon+1,
			now->tm_mday,
			now->tm_hour,
			now->tm_min,
			now->tm_sec);
		if(numChars == -1)
		{
			// handle error.
			printf("Failure writing timestamp to buffer!"); 
			//exit(99);
		}
		return(myTS);
}


