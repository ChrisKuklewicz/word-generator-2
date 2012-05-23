#include <cvidef.h>

/*
HAND
*/
/* From DNA.h:
typedef short handErrRet;				// Error code return type
typedef unsigned char dnaByte;			// Useful small storage unit 
#define dnaTrue 1
#define dnaFalse 0
#define AnalogOutputChannels 10			// Number of channels in the AO board
*/

/*
Current jobs for analog outputs:

Trapping frequency vco AOM
Trapping intensity mixer AOM
Sideband intensity variation mixer EOM
F=1 Probe frequency vco
F=2 Probe frequency vco
Curvature power supply
Graidant power supply
Bias power supply

*/

/*
 A number of static global constants to hold harware configurations.
 In the future these may be detected automatically or access from an ini file.
*/

/* The AOCode and HSCode must match what is in the computer */
/* If not, an error is reported on start up */
/* See hand_ResetAll */

static const short DIO32HS_Code=211;
static const short PCI6713_Code=263;
static const short ATAO10_Code=26;

static const HSCode = 211;			// Board cose id for DIO-32HS

#if WGFLAG == 2
/* This is the AT-AO-10 output board */
static const short AOBoard = 1;			// This is the AO board number
/* This is the primary Digital board */
static const short DOBoard = 2;			// The number for the output only DIO
/* This is the secondary Digital board */
static const short DIOBoard = 3;			// The number for the input/output DIO
static const AOCode = 26;			// Board code id for AT-AO-10

#elif WGFLAG == 3 
static const short AOBoard = 3;			// This is the AO board number
/* This is the primary Digital board */
static const short DOBoard = 2;			// The number for the output only DIO
/* This is the secondary Digital board */
static const short DIOBoard = 1;			// The number for the input/output DIO

static const AOCode = 263;			// Board code id for our 6713
#endif 



// Included here as a reminder
//typedef void (CVICALLBACK *DAQEventCallbackPtr) (DAQEventHandle handle, 
//												 DAQEventMsg    msg,
//												 DAQEventWParam wParam,
//												 DAQEventLParam lParam);

#include <Dataacq.h> 

handErrRet CVIFUNC hand_ResetAll (void);

handErrRet CVIFUNC hand_Error (handErrRet err, const char *message,
							   dnaByte userQ, dnaByte warnQ, dnaByte abortQ);

handErrRet CVIFUNC hand_ResetAnalog (void);

handErrRet CVIFUNC hand_ConfigDIOPorts (int numberofOutputLines, int inputLines);

handErrRet CVIFUNC hand_SetLine (int line, int value);

handErrRet CVIFUNC hand_ConfigAnalogIdle (void);

handErrRet CVIFUNC hand_SetAnalogOutput (int index, double voltage);

handErrRet CVIFUNC hand_ConfigAuxPorts (void);

handErrRet CVIFUNC hand_WriteAuxPort (int outputLine, int value);

handErrRet CVIFUNC hand_ReadAuxPort (int inputLine, int *value);

handErrRet CVIFUNC hand_ConfigDIO (int numberofOutputLines, int inputLines);

handErrRet CVIFUNC hand_ConfigPattern (dnaBufferDigital *dBuf, int ports,
                                       int analogQ);
handErrRet CVIFUNC hand_ClearRTSI (void);

handErrRet CVIFUNC hand_ConfigureRTSI (int analoginuse);

handErrRet CVIFUNC hand_OutBuffer (dnaBufferDigital *dBuf1,
                                   dnaBufferDigital *dBuf2);

handErrRet CVIFUNC hand_ConfigDigital (dnaBufferDigital *digitalBuffer1,
                                       dnaBufferDigital *digitalBuffer2,
                                       int analogQ, int inputPorts);

handErrRet CVIFUNC hand_ConfigAnalog (dnaBufferAnalog *analogBuffer);

handErrRet CVIFUNC hand_OutWord (unsigned int AWord, unsigned int BWord);

handErrRet CVIFUNC hand_ConfigRFCallback (void);

handErrRet CVIFUNC hand_InstallEndCallback (DAQEventCallbackPtr functionName);

handErrRet CVIFUNC hand_CheckProgress (unsigned long *numberCompleted);

handErrRet CVIFUNC hand_AbortRun (void);

handErrRet CVIFUNC hand_RunOver (void);
