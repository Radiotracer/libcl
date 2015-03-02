/*
	cmdline.c

	$Id: cmdline.c 56 2009-04-07 17:34:07Z binhe $

 	Copyright 2002-2005 The Johns Hopkins University. ALL RIGHTS RESERVED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mip/printmsg.h>
#include <mip/errdefs.h>
#include <mip/miputil.h>
#include <mip/getline.h>

#include "cmdline.h"
#include "getparms.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static char *(*pfGlobalUsageFunc)(void);
static int bCmdLineInited=FALSE;
static int iGlobalArgc;
static char **ppchGlobalArgv=NULL;

#define PARM_TYPE_VAL 0
#define PARM_TYPE_TRUE 1
#define PARM_TYPE_FALSE 2

typedef struct {
	char *pchDescString;
	int iType; /* containes one of the parm_types defined above */
} PARM_ALIAS;

#define MAXPARMALIAS 255

PARM_ALIAS *psGlobalParmAliasTbl[MAXPARMALIAS+1];

void vInitParseCmdLine(char *pchParmAliasesFname, int iArgc, char **ppchArgv, char *(*pchUsageFunc)(void))
     /* initializes the command line parsing package  by:
		-copies the number and location of the argument array
		to static global storage.
		-reads and parses the file of PARMeter aliases from the specified file
		(unless the ptr to the filename is null). */
{
	int		i, chParmChar, iLineNum=0, iLinesRead;
	char	*pchTmp, *pchDescString,*pchLine;
	FILE	*fpIn;
	
	pfGlobalUsageFunc = pchUsageFunc;
	iGlobalArgc = iArgc - 1;
	ppchGlobalArgv =  ppchArgv+1;
	bCmdLineInited=TRUE;
	
	for(i=0; i <= MAXPARMALIAS; ++i)
		psGlobalParmAliasTbl[i]=NULL;

	if (pchParmAliasesFname != NULL){
		fpIn = fopen(pchParmAliasesFname, "r");
		if (fpIn == NULL){
			vPrintMsg(6,"vInitParseCmdLine: Could not open parameter alias file %s. No parameter aliases will be recognized.\n", pchParmAliasesFname);
			//vErrorHandler(ECLASS_WARN, ETYPE_FOPEN, "vInitParseCmdLine", "Could not open parameter alias file %s\n%s", pchParmAliasesFname, "No parameter aliases will be recognized.");
		}else{
			while((pchLine = pchGetLine(fpIn, FALSE, &iLinesRead)) != NULL){
				iLineNum += iLinesRead;
				if (pchLine == NULL){
					vErrorHandler(ECLASS_WARN, ETYPE_INFO, "vInitParseCmdLine", "Error readling line %d in %s, continuing", iLineNum, pchParmAliasesFname);
					continue;
				}
				pchTmp = strtok(pchLine, " \t");
				if (pchTmp == NULL){
					vErrorHandler(ECLASS_WARN, ETYPE_INFO, "vInitParseCmdLine", "Couldn't parse parameter alias on line %d in %s", iLineNum, pchParmAliasesFname);
					continue;
				}
				if (strlen(pchTmp) != 1){
					vErrorHandler(ECLASS_WARN, ETYPE_INFO, "vInitParseCmdLine", "parameter alias on line %d is not a single character:%s ignoring", iLineNum, pchTmp);
					continue;
				}
				chParmChar = *pchLine;
				if (((int)chParmChar) < 0 || ((int)chParmChar) > MAXPARMALIAS){
					vErrorHandler(ECLASS_WARN, ETYPE_INFO, "vInitParseCmdLine", "parameter alias character %c on line %d in %s is out of range", chParmChar, iLineNum, pchParmAliasesFname);
					continue;
				}
				pchDescString = strtok(NULL, " \t");
				if (pchDescString == NULL){
					vErrorHandler(ECLASS_WARN, ETYPE_INFO, "vInitParseCmdLine", "Couldn't parse description string on line %d in %s ignoring", iLineNum, pchTmp);
					continue;
				}
				pchTmp = strtok(NULL, " \t");
				if (pchTmp == NULL){
					vErrorHandler(ECLASS_WARN, ETYPE_INFO, "vInitParseCmdLine", "Couldn't parse parameter type on line %d in %s", iLineNum, pchParmAliasesFname);
					continue;
				}
				psGlobalParmAliasTbl[chParmChar] = (PARM_ALIAS *)pvIrlMalloc(sizeof(PARM_ALIAS), "InitParseCmdLine:GlobalParmAliasTbl");
	
				psGlobalParmAliasTbl[chParmChar]->pchDescString =  pchIrlStrdup(pchDescString);
				switch(toupper(*pchTmp)){
					case 'T': 
						psGlobalParmAliasTbl[chParmChar]->iType = PARM_TYPE_TRUE;
						break;
					case 'F': 
						psGlobalParmAliasTbl[chParmChar]->iType = PARM_TYPE_FALSE;
						break;
					default:
						psGlobalParmAliasTbl[chParmChar]->iType = PARM_TYPE_VAL;
						break;
				}
			}
			for(i=0; i<MAXPARMALIAS; ++i){
				if (psGlobalParmAliasTbl[i] != NULL){
					switch (psGlobalParmAliasTbl[i]->iType){
						case PARM_TYPE_VAL:
							pchTmp="val";
							break;
						case PARM_TYPE_TRUE:
							pchTmp="true";
							break;
						case PARM_TYPE_FALSE:
							pchTmp="false";
							break;
						default:
							pchTmp="???";
							break;
					}
					vPrintMsg(9,"-%c %s => --%s=%s\n",(char)i, psGlobalParmAliasTbl[i]->iType==PARM_TYPE_VAL ? "val" : "", psGlobalParmAliasTbl[i]->pchDescString, pchTmp);
				}
			}
			fclose(fpIn);
			vFreeLineBuf();
		}
	}
}

int iDoneCmdLine()
     /*cleans up after command line parsing is done:
       -frees memory used to store paramter aliases */
{
	int i;
	
	if (!bCmdLineInited)
		vErrorHandler(ECLASS_FATAL, ETYPE_INFO, "iDoneCmdLine", "Trying to uninitialize before initializing");
	
	for(i=0; i<MAXPARMALIAS; ++i){
		if (psGlobalParmAliasTbl[i] != NULL){
			IrlFree(psGlobalParmAliasTbl[i]->pchDescString);
			IrlFree(psGlobalParmAliasTbl[i]);
			psGlobalParmAliasTbl[i]=NULL;
		}
	}
	bCmdLineInited=FALSE;
	return iGlobalArgc;
}

char *pchParseCmdLine(int *inumargc)
     /* Looks at command line arguments and interprets any command line switches 
				found as PARMeters. These are put into the PARMeters table (using the 
				vAddParm function). A pointer is returned to the first non-switch
				argument. Subsequent calls will repeat this process, returning the
				next non-switch argument. If there are no remaining non-switch arguments,
				the a NULL is returned. The switch to PARMeter mapping is done as follows
				switches that start with -- are interpreted as parameter names
				followd by a value. The value may be either the next argument or
				follow the name by an equal sign. For example,
				--parm=value or --parm value
				Note that the vAddParms file treats parameter names as being 
				case-insensitive, so the paramter name added above would be PARM.
				For arguments beginning with a single switch, each character in the switch
				is treated as a parameter alias. The translation from aliases to
				parameter names is controlled by the aliases file which is parsed
				by bInitParseCmdLine. This file contains 3 columns. The first,
				a single character, is the parameter alias. The second, is the
				parameter name, and the third is a string. If the first letter of
				this string is t or T, then this is a parameter alias that doesn't
				take a value from the CL, but instead the corresponding parameter is set 
				to true.   Similarly, if the first or letter of this string is f or F, then,
				again, the parameter doesn't take a value from CL, but the corresponding
				parameter is assigned a value "false". If the first character is not
				one of these, then the option takes a value from the next
				command line argument. For example, assume that the alias file contains
				d debug true
				t test false
				a nangles val
				k kdim val
				Then the command line
				-tdak 128 64 
				is equivalent to 
				--debug=true --test=false --nangles=128 -kdim=64
				Note that the syntax -a128 is not allowed and would look for
				paramter aliases for a, 1, 2, and 8.
		 */
{
	int  chTmp;
	char *pchTmp, *pchVal, *pchDesc;
	
	if (!bCmdLineInited)
		vErrorHandler(ECLASS_FATAL, ETYPE_INFO, "iParseCmdLine", "Trying to parse cmd line before initializing");
	
	while (iGlobalArgc > 0) {
		iGlobalArgc--;
		pchTmp = *ppchGlobalArgv++;
		if(*pchTmp != '-' || isdigit(*(pchTmp +1)) || *(pchTmp+1) == '.'){
			*inumargc = (int)iGlobalArgc;
			return(pchTmp);
		}
		if (*++pchTmp == '-'){
			++pchTmp; /* skip the second - */
			
			/* long option. Check if the value is in this argument by looking for the equal sign */
			pchVal = strchr(pchTmp, '=');
			if (pchVal == NULL){
				/* no equals sign in argument, it must be the next argument */
				if (iGlobalArgc-- <= 0)
					vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "ParseCmdLine", "ran out of arguments for long option %s\n%s", pchTmp, (*pfGlobalUsageFunc)());
				pchVal = *ppchGlobalArgv++;
			}else{
				if (pchVal == pchTmp)
					vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "ParseCmdLine", "Empty parameter name: %s\n%s", pchTmp, (*pfGlobalUsageFunc)());
				*pchVal++ = '\0';
			}
			vAddUserParm(pchTmp, pchVal, "Command Line");
		}else{
			/* short option, go through option string looking for aliases*/
			while((chTmp=*pchTmp++) != '\0'){
				if (psGlobalParmAliasTbl[chTmp] == NULL)
					vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "ParseCmdLine", "unknown parameter alias in command line switch: %c\n%s", chTmp, (*pfGlobalUsageFunc)());
				
				pchDesc = psGlobalParmAliasTbl[chTmp]->pchDescString;
				switch (psGlobalParmAliasTbl[chTmp]->iType){
					case PARM_TYPE_VAL:
						if (iGlobalArgc <= 0)
							vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE,  "ParseCmdLine", "ran out of arguments for short option %c\n%s", chTmp, (*pfGlobalUsageFunc)());
						iGlobalArgc--;
						pchVal = *ppchGlobalArgv++;
						break;
					case PARM_TYPE_TRUE:
						pchVal="true";
						break;
					case PARM_TYPE_FALSE:
						pchVal="false";
						break;
					default:
						vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "ParseCmdLine", "Unknown parameter type: %d\n", psGlobalParmAliasTbl[chTmp]->iType);
						break;
				}
				vAddUserParm(pchDesc, pchVal, "Command Line Alias");
			}
		}
	}
	return (NULL);
}
