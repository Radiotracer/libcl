/*
	getparms.c
	
	$Id: getparms.c 67 2011-09-14 16:27:32Z frey $

 	Copyright 2002-2005 The Johns Hopkins University. ALL RIGHTS RESERVED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mip/errdefs.h>
#include <mip/getline.h>
#include <mip/printmsg.h>
#include <mip/linklist.h>
#include <mip/miputil.h>

#include "getparms.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef struct PARM_S {
	char *pchName;
	char *pchData;
	char *pchSource;
	int iTimesUsed;
} PARM;

/* Used for calls to LinkList functions TraverseList() and ReleaseList() */

typedef void (*TraverseFcn)(void*);
typedef void (*ReleaseFcn)(void*);

#define DESC_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_."

#define SKIP_WS(pc) pc += strspn(pc, " \t\n")

static char *strnup(char *pc, int n)
		 /* translates up to n characters of the string pointed to by pc to upper case*/
{
	char *pchSave = pc;
	
	while (n-- > 0 && *pc != '\0'){
		*pc=toupper(*pc);
		++pc;
	}
	return(pchSave);
}

static char *pchStrToUpper(char *pc)
		 /* translates characters of the string pointed to by pc to upper case*/
{
	char *pchSave = pc;
	
	while ( *pc != '\0'){
		*pc=toupper(*pc);
		++pc;
	}
	return(pchSave);
}

static void ReleaseNodeData(PARM *psData)
{
	IrlFree(psData->pchName);
	IrlFree(psData->pchData);
	IrlFree(psData->pchSource);
	IrlFree(psData);
}

static int iCompareNames(PARM *psParm, char *pchName)
{
	return(strcmp(pchName, psParm->pchName));
}

static LINK_LIST *psNewNode(void)
{
	LINK_LIST *psNode;
	PARM *psParm;
	
	psNode=(LINK_LIST *)pvIrlMalloc(sizeof(LINK_LIST),"NewNode:Node");
	psParm=(PARM *)pvIrlMalloc(sizeof(PARM),"NewNode:Parm");
	psNode->pvData = (void *)psParm;
	psNode->psNext = NULL;
	return(psNode);
}

static void vAddParm(void **ppvParmsList, char *pchName, char *pchData, char *pchSource)
{
	LINK_LIST *psList, **ppsLast, *psNode;
	PARM *psParm;
	int iCmpr;
	
	psList=(LINK_LIST *)*ppvParmsList;
	ppsLast = NULL;
	while(psList != NULL && (iCmpr=iCompareNames((PARM *)psList->pvData, pchName)) > 0){
		ppsLast = (LINK_LIST **)ppvParmsList;
		ppvParmsList = (void **)&(psList->psNext);
		psList = psList->psNext;
	}
	if (psList == NULL){
		*ppvParmsList = (void *)psNewNode();
		psList = (LINK_LIST *)*ppvParmsList;
	}
	else{
		if (iCmpr == 0){
			/* names equal */
			psParm = (PARM *) psList->pvData;
			vPrintMsg(3, "  Warning: replacing parm %s from %s by %s (old=%s, new=%s)\n", pchName, psParm->pchSource, pchSource, psParm->pchData, pchData);
			/*make sure and free current data for this node*/
			psParm = (PARM *) psList->pvData;
			IrlFree(psParm->pchName);
			IrlFree(psParm->pchData);
			IrlFree(psParm->pchSource);
		}else{
			/* insert above */
			psNode = psNewNode();
			psNode->psNext = psList;
			psList = psNode;
			*ppvParmsList = (void *)psNode;
		}
	}
	psParm = (PARM *)psList->pvData;
	psParm->pchName = pchName;
	psParm->pchData = pchData;
	psParm->pchSource = pchSource;
	psParm->iTimesUsed = 0;
}

static void vReadParms(char *pchParFileName, void **ppvParmsList)
{
	FILE *fpIn;
	int iLineNum=0;
	int iLinesRead;
	int n,m, iLen;
	int iEqualSignPos;
	char *pchLine;
	char *pchName;
	char *pchData;
	char *pchSource;
	char pchBuf[30];
	
	fpIn = fopen(pchParFileName, "r");
	if (fpIn == NULL){
		vErrorHandler(ECLASS_FATAL, ETYPE_FOPEN, "vReadParms", "file=%s", pchParFileName);
		exit(1);
	}
	
	while ((pchLine=pchGetLine(fpIn, FALSE, &iLinesRead)) != NULL){
		iLineNum += iLinesRead;
		iLen = strlen(pchLine);	
		n = strcspn(pchLine, "=");
		if (n==iLen){
			vPrintMsg(3, "  vReadParms: missing '=' at line %d in %s, ignoring", iLineNum, pchParFileName);
			continue;
		}
		iEqualSignPos=n;
		/* strip trailing whitespace */
		while(n-1 >= 0 && (pchLine[n-1] == ' ' || pchLine[n-1] == '\t'))
			--n;
		pchLine[n] = '\0';
		/* parameter names are case-insensitive */
		strnup(pchLine,n);
		m=strspn(pchLine,DESC_CHARS);
		if (m != n){
			vPrintMsg(3, "  vReadParms: illegal characters in parameter name at line %d in %s, ignoring", iLineNum, pchParFileName);
			continue;
		}
		pchName = pchIrlStrdup(pchLine);
		pchLine += iEqualSignPos+1;
		SKIP_WS(pchLine);
		pchData=pchIrlStrdup(pchLine);
		sprintf(pchBuf,"%d",iLineNum);
		
		/* now make the source strings which tells which line in
		   which file the parameter came from
		   */
		
		iLen = strlen(pchBuf) + strlen(pchParFileName) + 10;
		pchSource=(char *)pvIrlMalloc(iLen,"vReadParms:Source");
		sprintf(pchSource,"line %s in %s",pchBuf,pchParFileName);
		
		vAddParm(ppvParmsList, pchName, pchData, pchSource);
	}
	fclose(fpIn);
	vFreeLineBuf();
}

static void *pvGlobalParmsList=NULL;
void vReadParmsFile(char *pchFname)
{
	vReadParms(pchFname, &pvGlobalParmsList);
}

void vAddUserParm(char *pchParmName, char *pchParmVal, char *pchParmSource)
{
	pchParmName = pchIrlStrdup(pchParmName);
	pchStrToUpper(pchParmName);
	pchParmVal = pchIrlStrdup(pchParmVal);
	pchParmSource = pchIrlStrdup(pchParmSource);
	vAddParm(&pvGlobalParmsList, pchParmName, pchParmVal, pchParmSource);
}

static void vPrintParm(PARM *psParm)
{
	vPrintMsg(5,"  %-20s\t= %-20s\t (from %s, used %d times)\n", psParm->pchName, psParm->pchData, psParm->pchSource, psParm->iTimesUsed);
}

void vPrintAllParms(void)
{
	TraverseList((LINK_LIST *)pvGlobalParmsList, (TraverseFcn)vPrintParm);
}


static PARM *psFindParm(char *pchParmName)
{
	LINK_LIST *psList;
	PARM *psData;
	int iCmpr;
	
	pchParmName=pchIrlStrdup(pchParmName);
	pchStrToUpper(pchParmName);
	psList = (LINK_LIST *)pvGlobalParmsList;
	while(psList != NULL && (iCmpr=iCompareNames((PARM *) psList->pvData, pchParmName)) > 0)
		psList = psList->psNext;
	
	IrlFree(pchParmName);
	if (psList == NULL || iCmpr != 0)
		return(NULL);
	else{
		psData = (PARM *)psList->pvData;
		psData->iTimesUsed++;
		return (psData);
	}
}

char *pchGetStrParm(char *pchParmName, int *pbFound, char *pchDefault)
{
	PARM *psParm;
	char *pchVal;
	
	if ((psParm=psFindParm(pchParmName)) == NULL){
		if (pbFound == NULL)
			vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "GetStrParm", "Required string parameter %s was not found", pchParmName);
		*pbFound = FALSE;
		pchVal = pchDefault;
		vPrintMsg(3, "  GetStrParm: parameter %s not found, returning default: %s\n", pchParmName, pchDefault);
	}
	else{
		if (pbFound != NULL)
		  *pbFound = TRUE;
		pchVal = psParm->pchData;
	}
	return(pchVal);
}

int iGetIntParm(char *pchParmName, int *pbFound, int iDefault)
{
	int iVal;
	char *pchVal, *pchEndPtr;
	PARM *psParm;
	
	if ((psParm=psFindParm(pchParmName)) == NULL) {
		if (pbFound == NULL)
			vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "GetIntParm", "Required int parameter %s was not found", pchParmName);
		*pbFound = FALSE;
		vPrintMsg(3, "  GetIntParm: parameter %s not found, returning default: %d\n", pchParmName, iDefault);
		return(iDefault);
	}
	pchVal = psParm->pchData;
	iVal = strtol(pchVal, &pchEndPtr, 10);
	if (*pchEndPtr == '\0'){
		if (pbFound != NULL)
			*pbFound = TRUE;
		return iVal;
	}
	vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "GetIntParm", "Error getting integer parameter %s=%s", pchParmName, pchVal);
}


double dGetDblParm(char *pchParmName, int *pbFound, double dDefault)
{
	double dVal;
	char *pchVal, *pchEndPtr;
	PARM *psParm;
	
	if ((psParm=psFindParm(pchParmName)) == NULL) {
		if (pbFound == NULL)
			vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "GetDblParm", "Required double parameter %s was not found", pchParmName);
		*pbFound = FALSE;
		vPrintMsg(3, "  GetDblParm: parameter %s not found, returning default: %.4e\n", pchParmName, dDefault);
		return(dDefault);
	}
	pchVal = psParm->pchData;
	dVal = strtod(pchVal, &pchEndPtr);
	if (*pchEndPtr == '\0'){
		if (pbFound != NULL)
			*pbFound = TRUE;
		return dVal;
	}
	vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "GetDblParm", "Error getting double parameter %s=%s", pchParmName, pchVal);
}

int bGetBoolParm(char *pchParmName, int *pbFound, int bDefault)
{
	int bVal;
	char *pchVal;
	PARM *psParm;
	
	if ((psParm=psFindParm(pchParmName)) == NULL) {
		if (pbFound == NULL)
		  vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "GetIntParm", "Required int parameter %s was not found", pchParmName);
		*pbFound = FALSE;
		vPrintMsg(3, "  GetBoolParm: parameter %s not found, returning default: %s\n", pchParmName, bDefault ? "true" : "false");
		return(bDefault);
	}
	pchVal = psParm->pchData;
	switch(toupper(*pchVal)){
	/* added 0 to false, 1 to true, ecf, 5/1/98*/
	case 'T':
	case 'Y':
	case '1':
		bVal = TRUE;
		break;
	case 'F':
	case 'N':
	case '0':
		bVal = FALSE;
		break;
	default:
		vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, "GetIntParm", 
				"Error getting boolean parameter %s=%s", pchParmName, pchVal);
	}

	if (pbFound != NULL)
		*pbFound = TRUE;
	return(bVal);
}

#define DELIMS " \t," /* delimterss for strings in list*/
#define LIST_CHUNKSIZE 10 /* allocate memory for the list this much at a time*/

char **ppchGetStrListParm(char *pchParmName, int *pbFound, int *piNumFound, char *pchDefault)
/* this rerturns an array of pointers to strings from the parameter with
	name pchParmName. The strings are delimited by space, tab or comma.
	The number of Strings found is returned in piNumFound.
	Also, the returned array of pointers will have iNumFound+1 items
	and the Last one will be set to Null. This means that the
	end of the array of found pointers can be found by searching for a
	NULL pointer. The memory can be freed by calling vFreeStrList. 
*/
{
	int iListSize=0;
	int iNumFound=0;
	char **ppchList=NULL;
	char *pch, *pchTok;
	char *pchData;

	/* allocate  initial space for list*/
	ppchList = (char **)pvIrlMalloc( LIST_CHUNKSIZE*sizeof(char *), "pchGetStrList:List");
	iListSize = LIST_CHUNKSIZE;
	/* strdup the parameter because strtok will mess it up*/
	pchData = pchIrlStrdup(pchGetStrParm(pchParmName,pbFound,pchDefault));
	pch=pchData;
	while((pchTok=strtok(pch,DELIMS)) != NULL){
		pch=NULL; /* change to null so subsequent calls to strtok will start
						 where we left off */
		iNumFound++;
		/* make sure the list is long enough for both the current item and the
		 	terminating NULL pointer */
		if (iListSize < iNumFound+1){
			/* use realloc to allocate the memor. This takes advantage of the
				fact that IrlRealloc does a malloc if the input pointer is NULL*/
			ppchList = (char **)pvIrlRealloc((void *)ppchList, (iListSize+LIST_CHUNKSIZE)*sizeof(char *), "pchGetStrList:List");
			iListSize += LIST_CHUNKSIZE;
		}
		ppchList[iNumFound-1] = pchIrlStrdup(pchTok);
	}
	/* last list item is NULL pointer */
	ppchList[iNumFound] = NULL;
	IrlFree(pchData);/* need to free this since we strdup-ed it*/
	/* shrink the list down to the size needed*/
	ppchList=(char **)pvIrlRealloc((void *)ppchList, (iNumFound+1)*sizeof(char *), "pchGetStrList:List");
	*piNumFound = iNumFound;
	return(ppchList);
}


void vFreeStrList(char **ppchList)
{
	char **ppch;

	if (ppchList == NULL)return;/*list is already freed*/
	/* free the strings in the list */
	for(ppch=ppchList; *ppch != NULL; ++ppch)
		IrlFree(*ppch);
	
 	/* now free the list itself */
	IrlFree(ppchList);
}

int *piGetIntArrayParm(char *pchParmName, int *pbFound, int *piNumFound,
	char *pchDefault)
/* returns an array of integers with the number of integers listed in
	piNumFound. If no parameter sting with name *pchParmName is found 
	and pbFound is NULL, then the program will abort with an illegal
	value error. If pbFound is non-null, then pchDefault will be assumed
	to be the default string and parsed into an array of integers,
	*piNumFound is set accordingly, and *pbFound is set to false. If
	a string is associated with pchParmName, then it is parsed into
	an array of integers, *pbFound set to true, and *piNumFound set
	accordingly.*/
{
	char **ppchStrList = NULL;
	int i;
	int *piIntArray;

	ppchStrList = ppchGetStrListParm(pchParmName, pbFound, piNumFound, 
		pchDefault);
	if (*piNumFound <= 0){
		*piNumFound = 0;
		return NULL;
	}

	piIntArray = ivector(*piNumFound,"GetIntParm:IntArray");
	for(i=0; i < *piNumFound; ++i)
		piIntArray[i] = atoi(ppchStrList[i]);
	vFreeStrList(ppchStrList);
	return(piIntArray);
}

int *pbGetBoolArrayParm(char *pchParmName, int *pbFound, int *piNumFound,
	char *pchDefault)
/* returns an array of integers that are either 0 or 1 (false or true). 
	The number of iterms is returned in the pointer indexed by piNumFound. 
	If no parameter sting with name *pchParmName is found 
	and pbFound is NULL, then the program will abort with an illegal
	value error. If pbFound is non-null, then pchDefault will be assumed
	to be the default string and parsed into an array of Boolean values.
	*piNumFound is set accordingly, and *pbFound is set to false. If
	a string is associated with pchParmName, then it is parsed into
	an array of Booleans, *pbFound set to true, and *piNumFound set
	accordingly. The items can be delimited by space, tab or comma. 
	As for a single Boolean strings begining with Y, T, or 1 are true 
	and N, F or 0 are considered false. Case is ignored. */
{
	char **ppchStrList = NULL;
	int i;
	int *piBoolArray;

	ppchStrList = ppchGetStrListParm(pchParmName, pbFound, piNumFound, 
		pchDefault);
	if (*piNumFound <= 0){
		*piNumFound = 0;
		return NULL;
	}

	piBoolArray = ivector(*piNumFound,"GetIntParm:IntArray");
	for(i=0; i < *piNumFound; ++i){
		switch(toupper(*ppchStrList[i])){
			case 'T':
			case 'Y':
			case '1':
				piBoolArray[i] = TRUE;
				break;
			case 'F':
			case 'N':
			case '0':
				piBoolArray[i] = FALSE;
				break;
			default:
				vErrorHandler(ECLASS_FATAL, ETYPE_ILLEGAL_VALUE, 
				"GetBoolArrayParm", "Error getting boolean parameter %d %s=%s", 
				i,pchParmName, ppchStrList[i]);
		}
	}
	vFreeStrList(ppchStrList);
	return(piBoolArray);
}

double *pdGetDblArrayParm(char *pchParmName, int *pbFound, int *piNumFound,
	char *pchDefault)
/* returns an array of integers with the number of doubles listed in
	piNumFound. If no parameter sting with name *pchParmName is found 
	and pbFound is NULL, then the program will abort with an illegal
	value error. If pbFound is non-null, then pchDefault will be assumed
	to be the default string and parsed into an array of doubles,
	*piNumFound is set accordingly, and *pbFound is set to false. If
	a string is associated with pchParmName, then it is parsed into
	an array of doubles, *pbFound set to true, and *piNumFound set
	accordingly.*/
{
	char **ppchStrList = NULL;
	int i;
	double *pdDblArray;

	ppchStrList = ppchGetStrListParm(pchParmName, pbFound, piNumFound, 
		pchDefault);
	if (*piNumFound <= 0){
		*piNumFound = 0;
		return NULL;
	}

	pdDblArray = dvector(*piNumFound,"GetDblArrayParm:DblArray");
	for(i=0; i < *piNumFound; ++i)
		pdDblArray[i] = atof(ppchStrList[i]);
	vFreeStrList(ppchStrList);
	return(pdDblArray);
}

static void vCheckUsedParm(PARM *psParm)
{
	if (psParm->iTimesUsed > 0)
		vPrintMsg(4,"  %-20s\t= %-20s\t (from %s)\n", psParm->pchName, psParm->pchData, psParm->pchSource);
}

int iGlobalNumUnusedParms;
static void vCheckUnusedParm(PARM *psParm)
{
	if (psParm->iTimesUsed < 1){
		iGlobalNumUnusedParms++;
		vPrintMsg(4,"  %-20s\t= %-20s\t (from %s)\n", psParm->pchName, psParm->pchData, psParm->pchSource);
	}
}

int iDoneWithParms()
{
	vPrintMsg(4,"The following parameters were used:\n");
	TraverseList((LINK_LIST *)pvGlobalParmsList, (TraverseFcn)vCheckUsedParm);
	vPrintMsg(4,"\n");
	
	/*use TraverseList to go through list and print and tally
		number of unused parameters. The tally is kept in
		the global variable iGlobalNumUnusedParms*/
	iGlobalNumUnusedParms=0;
	vPrintMsg(4,"The following parameters were input but not used:\n");
	TraverseList((LINK_LIST *)pvGlobalParmsList, (TraverseFcn)vCheckUnusedParm);
	if (iGlobalNumUnusedParms == 0) vPrintMsg(4,"  N/A\n");
	vPrintMsg(4,"\n");
	
	/*Now free the parameters list*/
	ReleaseList((LINK_LIST **)&pvGlobalParmsList, (ReleaseFcn) ReleaseNodeData);
	return(iGlobalNumUnusedParms);
}
