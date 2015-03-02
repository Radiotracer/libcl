/*
	getparms.h

	$Id: getparms.h 67 2011-09-14 16:27:32Z frey $
*/

#ifndef GETPARMS_H
#define GETPARMS_H

/*prototypes*/

#ifdef __cplusplus
extern "C" {
#endif

	/*reads parms from file*/
	void vReadParmsFile(char *pchFname);

	/*adds a single parm*/
	void vAddUserParm(char *pchParmName, char *pchParmVal, char *pchParmSource);

	/* prints out all parms to the specified stream*/
	void vPrintAllParms(void);

	/*routines for getting parm values*/
	char *pchGetStrParm(char *pchParmName, int *pbFound, char *pchDefault);
	int iGetIntParm(char *pchParmName, int *pbFound, int iDefault);
	double dGetDblParm(char *pchParmName, int *pbFound, double dDefault);
	int bGetBoolParm(char *pchParmName, int *pbFound, int bDefault);
	char **ppchGetStrListParm(char *pchParmName, int *pbFound, int *piNumFound,
		char *pchDefault);
	void vFreeStrList(char **ppchStrList);
	int *piGetIntArrayParm(char *pchParmName, int *pbFound, int *piNumFound,
		char *pchDefault);
	double *pdGetDblArrayParm(char *pchParmName, int *pbFound, int *piNumFound,
		char *pchDefault);
	int *pbGetBoolArrayParm(char *pchParmName, int *pbFound, int *piNumFound,
		char *pchDefault);

	/* checks for and returns number unused parameters, frees parm structures */
	int iDoneWithParms(void); 

#ifdef __cplusplus
}
#endif

#endif /* GETPARMS_H */
