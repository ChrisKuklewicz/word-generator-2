// DDE.c module

// model of callback
//int (*ddeFuncPtr) (unsigned handle, char *topicName,
//                       char *itemName, int xType, int dataFmt,
//                       int dataSize, void *dataPtr,
//                       void *callbackData);
#include <stdio.h>
#include <ddesupp.h>
// need macros, typedef's
#include "dna.h"
#include "dde.h"
// need OpenIt()
#include "facetwo.h"
// need RunDone
#include "brain.h"  

char *fn;	// Needs global scope for message receipt time validity
// This is set to zero in Start_DDE 
// <DD_Serve called zero or more times....>
// freed, if neccessary at start of DDE_Serve
// allocated, if neccessary at end of in DDE_Serve
// <*fn used in MsgOpenIt->OpenIt->LoadIni-> Load ->Ini_ReadFromFile>
// DDE->(CVI Queue)->FACE-> FACE -> BRAIN ->MEMORY->Programmer's Toolbox
// freed, if neccesary in Stop_DDE

int CVICALLBACK DDE_Serve (unsigned handle, char *topicName, char *itemName, 
		int xType, int dataFmt, int dataSize, void *dataPtr, void *callbackData)
{
	int lex,len,err=0;
	char open[]="Open WGS, ";
	
//	printf("DDE_Serve\n");
	// ignore everything but commands
	if (xType!=DDE_EXECUTE)
		return dnaTrue;
	// protect against stopping a run
	if (0==RunDone)
		return dnaFalse;
	// Now item name is command string
	// format for command "Open WGS, %1"
	len = strlen(open);
	nullChk(itemName);
//	printf("ItemName (%s)\nopen (%s)\n",itemName,open);
	lex = strncmp(open, itemName, len);
//	printf("lex: %i\n",lex);
	if (0!=lex)
		return dnaFalse;
	if(fn) {
		free(fn);
		fn=0;
	};
	fn=StrDup(itemName+len);
	nullChk(fn);
//	printf("open(%s)\n",fn);
	errChk(PostDeferredCall (MsgOpenIt, fn));
// do not waste time in DDE callback to do opening
//	errChk(OpenIt(fn));
//	printf("exiting normally\n");
Error:	
//	printf("err=%i",err);
	return (err?dnaFalse:dnaTrue);
};

int Start_DDE(void)
{
	int err=0;
	fn=0;
//	printf("Start_DDE\n");
	errChk(RegisterDDEServer ("WG2", DDE_Serve, 0));
Error:
	return (err?-1:0);
};

int Stop_DDE(void)
{
	int err=0;
//	printf("Stop_DDE\n");
	if(fn) {
		free(fn);
		fn=0;
	};
	errChk(UnregisterDDEServer ("WG2"));
Error:
	return (err?-1:0);
};
