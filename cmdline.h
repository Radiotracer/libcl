/*
	cmdline.h

	$Id: cmdline.h 51 2009-03-05 17:40:54Z mjs $
*/

#ifndef CMDLINE_H
#define CMDLINE_H

#ifdef __cplusplus
extern "C" {
#endif

	/* must be called before calls to pchParseCmdLine. 
	Any parameter aliases (that allow use of short options)
	are read in from the specified file. The number of command line
	arguments and a pointer to the argument string table are stored
	for internal use.
	*/
	void vInitParseCmdLine(char *pchParmAliasesFname, 
		int iArgc, 
		char **ppchArgv,
		char *(*pchUsageFunc)(void));


	/*This is the call used to parse command line switches  (which are used
	to set paramter values and return the next non-switch option. If the
	returned pointer is NULL, then there are no more options. Note that
	only switches _before_ this argument will be included in the
	list of parameter values
	*/
	char *pchParseCmdLine(int *inumargs);

	/* called after all command line arguments are parsed to free up memory used
	*/
	int iDoneCmdLine(void);

#ifdef __cplusplus
}
#endif

#endif /* CMDLINE_H */
