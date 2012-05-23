#include <userint.h>
#include "face.h"


//This is the HAND of Word Generator 2

//Here is where analog/digital input/output routines are.
//A function panel may be built to ease interface with this library.

//These are used by the BRAIN.

//Errors are checked for and handled here by hand_Error which writes status
//information to stdout and waits for Enter before possibly aborting.

//If function does not force abort, error codes are returned to the caller.


#include "dna.h"
#include "hand.h"
#include "mouth.h"
#include "asynctmr.h"					// TESTING ?
//TESTING

extern dnaTime CHKTIME;					// from BRAIN.c

static dnaAnalogChannelConfig AOConfig[dnaAnalogChannels];

// Module variables 
static dnaByte AOInit = dnaFalse;		// True after 1st reset
static dnaByte DOInit = dnaFalse;		// True after 1st reset
static dnaByte DIOInit = dnaFalse;		// True after reset
static dnaByte AuxInit = dnaFalse;

/* These are the possible values of dioMode global variable below */
static const int dioModeUnknown		= 0;
static const int dioModeReset 		= 1;  // Boards were just reset
static const int dioModePort 		= 2;  // Boards are in idle mode (port output)
static const int dioModeGroup 		= 3;  // Boards are in run mode (buffer output)

/* These are the possible values of analogMode global variable below */
static const int analogModeUnkown	= 0;
static const int analogModeReset 	= 1;  // Board was just reset
static const int analogModeManual	= 2;  // Board is in idle mode
static const int analogModeGroup 	= 3;  // Board is in buffered output mode

/* These keep track of what the baords are doing */
static int dioMode = 0; //dioModeUnknown		// last set mode of digital boards
static int analogMode = 0;  //analogModeUnkown	// last set mode of analog baord

static int LinesOut = 0;
static int LinesIn = 0;

/* This is currently unused */
static dnaByte EndCallbackQ = dnaFalse;

handErrRet CVIFUNC hand_Error (handErrRet err, const char *message,
							   dnaByte userQ, dnaByte warnQ, dnaByte abortQ)
{
 
	if ((err<0) || (warnQ && (err>0))) {
		char OutMessage[1024]={0};
		char *daqMessage=0;
		int button;
		if (!userQ) {
			daqMessage = GetNIDAQErrorString (err);
		} 
		else {
			daqMessage = "User Defined Error";
		}
		Beep();							// Universal Error behavior
#if VERBOSE >0
		// Use standard I/O window
		FmtOut("An error or warning in the HAND module has occured\n");
		FmtOut("Error #%d (%s)\n",err,daqMessage);
		FmtOut("Message: %s\n",message);
		if(abortQ)
			FmtOut("Abort has been requested\n");
		else
			FmtOut("Abort has not been requested\n");		
		FmtOut("Press enter to continue (g will override abort "\
		        ", a will force abort, c will clear error)\n");
		ScanIn ("%s", daqMessage);		// Wait for enter to be pressed
		if (daqMessage) {
			if (daqMessage[0]=='a')
				abortQ = dnaTrue;
			if (daqMessage[0]=='g')
				abortQ = dnaFalse;
			if (daqMessage[0]=='c') {
				abortQ = dnaFalse;
				err=0;
			}
		}
		if (abortQ) { 
			QuitUserInterface (-1);
			DeleteCriticalSection(&critical);
			criticalready=0;
			abort ();
		}
#else
		// Use pop up to explain
		Fmt(OutMessage,"An error or warning in the HAND module has occured\n");
		Fmt(OutMessage,"%s[a]<Error #%d (%s)\n",err,daqMessage);
		Fmt(OutMessage,"%s[a]<Message: %s\n",message);
		if(abortQ) {
			Fmt(OutMessage,"%s[a]<Abort has been requested\n");
			button=VAL_GENERIC_POPUP_BTN1;
		}
		else {
			Fmt(OutMessage,"%s[a]<Abort has not been requested\n");		
			button=VAL_GENERIC_POPUP_BTN2;
		}
		button = GenericMessagePopup ("Error in HAND.c module", OutMessage,
				"Abort", "No Abort", "Clear Error", 0, 0, 0, button,
				VAL_GENERIC_POPUP_NO_CTRL, VAL_GENERIC_POPUP_NO_CTRL);
		switch(button) {
			case VAL_GENERIC_POPUP_BTN1:
				QuitUserInterface(-1);  // try to shut it down
#ifdef THREADS
				DeleteCriticalSection(&critical);
				criticalready=0;
#endif // THREADS
				abort();				// halt execution
				break;
			case VAL_GENERIC_POPUP_BTN2:
				break;					// let someone else handle error
			case VAL_GENERIC_POPUP_BTN3:
				err=0;					// Clear error
				break;
		}
#endif  // VERBOSE > 0
	}
	return err;
}


//
//Initialized the three known boards and checks their id codes.
//Return 0 if sucessful, -1 if unhandled error occurs.

//Setup to abort right now.



handErrRet CVIFUNC hand_ResetAll (void)
{
	short bc;
	short i;
	handErrRet err;
	
#ifndef NOAOBOARD
	err = Init_DA_Brds (AOBoard, &bc);
	err = hand_Error(err, "hand_ResetAll{AO Board failed to initialize}",dnaFalse,dnaTrue,dnaTrue);
	if (err) goto Error;
	err = (bc!=AOCode)?bc:0;
	err = hand_Error(err,"hand_ResetAll{AO Board code wrong}",dnaTrue,dnaTrue,dnaTrue);    
	if (err) goto Error;
	AOInit = dnaTrue;
	// Maintain global variables at all convienent points in the code 
	analogMode = analogModeReset;
	// Now set jumper configuration of Analog Output Board

	// GOTCHA : a lot of hand coded parameters 
	// to switch channels to bipolar mode in driver.....
	// see also BRAIN.c SetRanges and hand_ResetAnalog below
	for (i=dnaAnalogUnipolar; i<dnaAnalogChannels; i++) {
		err = AO_Configure (AOBoard, i, 0, 0, 10.0, 0);
		if (err) printf("%d\n",i);
		err = hand_Error(err, "hand_ResetAll{AO Board Config failed}",dnaFalse,dnaTrue,dnaFalse);
		if (err) goto Error;
	}
#endif
#ifndef NODOBOARD
	err = Init_DA_Brds (DOBoard, &bc);
	err = hand_Error(err, "hand_ResetAll{DO Board failed to initialize}",dnaFalse,dnaTrue,dnaFalse);
	if (err) goto Error;
	err = (bc!=HSCode)?bc:0;
	err = hand_Error(err,"hand_ResetAll{DO Board code wrong}",dnaTrue,dnaTrue,dnaTrue);    
	if (err) goto Error;
	DOInit = dnaTrue;
#endif	
#ifndef NODIOBOARD
	err = Init_DA_Brds (DIOBoard, &bc);
	err = hand_Error(err, "hand_ResetAll{DIO Board failed to initialize}",dnaFalse,dnaTrue,dnaTrue);
	if (err) goto Error;
	err = (bc!=HSCode)?bc:0;
	err = hand_Error(err,"hand_ResetAll{DIO Board code wrong}",dnaTrue,dnaTrue,dnaTrue);    
	if (err) goto Error;
	DIOInit = dnaTrue;
#endif	
	// Maintain global variables at all convienent points in the code 
	dioMode = dioModeReset;
	LinesOut=0;
	LinesIn=0;
	
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_ResetAnalog (void)
{
	handErrRet err=0;
	short bc;
	int i;
#ifndef NOAOBOARD	
	err=Init_DA_Brds (AOBoard, &bc);
	err=hand_Error (err, "hand_ResetAnalog{Init_DA_Brds failed}", 0, 1, 1);
	if(err) goto Error;
	if (bc!=AOCode) {
		err = hand_Error (-1,"hand_ResetAnalog{Init_DA_Brds board code wrong}",
						  dnaTrue, dnaTrue, dnaTrue);
	}
	// keep the modes set correctly 
	analogMode=analogModeReset;

	// GOTCHA : really ugly code to set bipolar mode in driver
	// see also BRAIN.c SetRanges and hand_ResetAll above
	for (i=dnaAnalogUnipolar; i<dnaAnalogChannels; i++) {
		err = AO_Configure (AOBoard, i, 0, 0, 10.0, 0);
		if (err) printf("%d\n",i);
		err = hand_Error(err, "hand_ResetAnalog{AO Board Config failed}",dnaFalse,dnaTrue,dnaFalse);
		if (err) goto Error;
	}
#endif
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_ConfigDIOPorts (int numberofOutputLines, int inputLines)
{
	handErrRet err;
	int n;
	short bc;
	n=numberofOutputLines;

// I have decided that state checking here is not worth the effort 

#ifdef NODOBOARD
	return 0;
#endif

#ifdef NODIOBOARD
	if (DOInit)
#else
	if (DOInit && DIOInit) 
#endif
		{
	
		// First reset the boards to known state 
//		err = Init_DA_Brds (DOBoard, &bc);
//		err = hand_Error(err, "hand_ConfigDIOPorts{DO Board failed to initialize}",dnaFalse,dnaTrue,dnaTrue);
//		if(err) goto Error;
//#ifndef NODIOBOARD
//		err = Init_DA_Brds (DIOBoard, &bc);
//		err = hand_Error(err, "hand_ConfigDIOPorts{DIO Board failed to initialize}",dnaFalse,dnaTrue,dnaTrue);
//		if(err) goto Error;
//#endif
//		// Maintain global variables at all convienent points in the code 
//		dioMode = dioModeReset;
//		LinesOut = 0;
//		LinesIn = 0;
		
		// Now do not go to group mode, instead setup each port 
		if (dioModeGroup==dioMode)
			err = DIG_Grp_Config (DOBoard, 1, 0, 0, 1);
		if (n>=8) {
			err = DIG_Prt_Config (DOBoard, 0, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
		if (n>=16) {
			err = DIG_Prt_Config (DOBoard, 1, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
		if (n>=32) {
			err = DIG_Prt_Config (DOBoard, 2, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
			err = DIG_Prt_Config (DOBoard, 3, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
#ifndef NODIOBOARD
		if (n>=40) {
			err = DIG_Prt_Config (DIOBoard, 0, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
		if (n>=48) {
			err = DIG_Prt_Config (DIOBoard, 1, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
		if (n>=64) {
			err = DIG_Prt_Config (DIOBoard, 2, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
			err = DIG_Prt_Config (DIOBoard, 3, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
		if ((n<=48) && (inputLines>=8)) {
			err = DIG_Prt_Config(DIOBoard, 2, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
		if ((n<=48) && (inputLines>=16)) {
			err = DIG_Prt_Config(DIOBoard, 3, 0, 1);
			err = hand_Error (err, "hand_ConfigDIOPorts{DIG_Prt_Config failed}", 0, 1, 1);
			if(err) goto Error;
		}
#endif
		// Maintain global variables at all convienent points in the code 
		dioMode = dioModePort;
		LinesOut = n;
		LinesIn = inputLines;
		err=0;
	}
	else {
		err=1;
	}
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_SetLine (int line, int value)
{
	handErrRet err;
  
#ifdef NODOBOARD
  	return 0;
#endif
	if ((dioMode==dioModePort)&&(line<LinesOut))  {
		value = (value?1:0);
	  	if (line < 32) {
			err = DIG_Out_Line (DOBoard, line/8, line%8, value);
			err = hand_Error(err, "hand_SetLine{DIG_Out_Line failed}",dnaFalse,dnaTrue,dnaTrue);
			if (err) goto Error;
		}
#ifndef NODIOBOARD
		else if (line >= 32) {
			err = DIG_Out_Line (DIOBoard, line/8-4, line%8, value);
			err = hand_Error(err, "hand_SetLine{DIG_Out_Line failed}",dnaFalse,dnaTrue,dnaTrue);
			if (err) goto Error;
		}
#endif
	} 
	else {
		err=0;
	}
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_ConfigAnalogIdle (void)
{
	handErrRet err=0;
#ifdef NOAOBOARD
	return 0;
#endif
	if (analogModeManual!=analogMode) {
		err = hand_ResetAnalog ();
		err = hand_Error (err, "hand_ConfigAnalogIdle{hand_ResetAnalog failed}",
				0, 1, 0);
		if(err) goto Error;
	}	
	analogMode=analogModeManual;
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_SetAnalogOutput (int index, double voltage)
{
	handErrRet err=0;

	//TESTING OVERRIDE 
//	if (analogModeManual==analogMode) {
		AO_VWrite (AOBoard, index, voltage);
//	}
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_ConfigAuxPorts (void)
{
	handErrRet err=0;
#ifdef NOAOBOARD
	return 0;
#endif
	err=DIG_Prt_Config (AOBoard, 0, 0, 0);
	err=hand_Error(err,"hand_ConfigAuxPorts{DIG_Prt_Config (AOBoard,0,0,0) failed}",0,1,0);
	if(err) goto Error;
	/* TODO : Use DIG_Line_Config to make some lines output */
	/* The 6713 only hast 1 logical port (port zero) so comment out other call */
//	err = DIG_Prt_Config (AOBoard, 1, 0, 0);
//	err=hand_Error(err,"hand_ConfigAuxPorts{DIG_Prt_Config (100) failed}",0,1,0);
//	if(err) goto Error;
	AuxInit=dnaTrue;
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_WriteAuxPort (int outputLine, int value)
{
	handErrRet err=0;
	require(AuxInit);
	err = DIG_Out_Line (AOBoard, 0, outputLine, value);
	err=hand_Error(err,"hand_WriteAuxPort{DIG_Out_Line failed}",0,1,0);
	if(err) goto Error;
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_ReadAuxPort (int inputLine, int *value)
{
	// Hide the use of the short data type
	handErrRet err=0;
//#ifndef NOAOBOARD
	short val;
	require(AuxInit);
	val=(*value);
	err = DIG_In_Line (AOBoard, 0, inputLine, &val);
	err=hand_Error(err,"hand_ReadAuxPort{DIG_In_Line failed}",0,1,0);
	if(err) goto Error;
	(*value)=val;
//#endif
Error:
	return (err?-1:0);
}


// Assumption is made that Parameters are valid

handErrRet CVIFUNC hand_ConfigDIO (int numberofOutputLines, int inputLines)
{
	handErrRet err=0;
	short bc;
	// I have decided that state checking here is not worth the effort 
	
#ifdef NODOBOARD   
	return 0;	
#endif

#ifdef NODIOMODE	
	if (DOInit)
#else
	if (DOInit && DIOInit) 
#endif
		{
		
		// GLITCHFIX
		// Added intelligent code to reduce use of reset commands
		// BUG just fixed: was	       (numberofOutputLines=LinesOut)  2000/03/24
		if ((dioModeGroup==dioMode) && (numberofOutputLines==LinesOut) &&
				(inputLines==LinesIn))
			return 0;
		
		// First reset the boards to known state 
//		err = Init_DA_Brds (DOBoard, &bc);
//		err = hand_Error(err, "hand_ConfigDIO{DO Board failed to initialize}",dnaFalse,dnaTrue,dnaTrue);
//		if(err) goto Error;
//#ifndef NODIOBOARD   
//		err = Init_DA_Brds (DIOBoard, &bc);
//		err = hand_Error(err, "hand_ConfigDIO{DIO Board failed to initialize}",dnaFalse,dnaTrue,dnaTrue);
//		if(err) goto Error;
//#endif
//		// Maintain global variables at all convienent points in the code 
//		dioMode = dioModeReset;
//		LinesOut = 0;
//		LinesIn = 0;
	
		// Now go into a group mode that happens to also lock out port commands 
		// Group #1, Start at Port 0, Latched Output is mode 3 
		if (numberofOutputLines<=32)  {
			/* Group 1 */
			err = DIG_Grp_Config (DOBoard, 1, numberofOutputLines/8, 0, 1);
			err = hand_Error(err, "hand_ConfigDIO{Config DOBoard failed}",dnaFalse,dnaTrue,dnaTrue);
			if (err) goto Error;
		}
		else {
			/* Group 1 */
			err = DIG_Grp_Config (DOBoard, 1, 4, 0, 1);
			err = hand_Error(err, "hand_ConfigDIO{Config DOBoard failed}",dnaFalse,dnaTrue,dnaTrue);
			if (err) goto Error;
#ifndef NODIOBOARD
			/* Group 1 */
			err = DIG_Grp_Config (DIOBoard, 1, numberofOutputLines/8-4, 0, 1);
			err = hand_Error(err, "hand_ConfigDIO{Config DIOBoard output failed}",dnaFalse,dnaTrue,dnaTrue);
			if (err) goto Error;
#endif
		}
#ifndef NODIOBOARD
		/* Typically, inputLines will be zero */
		if (inputLines>0) {
			/* Group 2 */
			err = DIG_Grp_Config (DIOBoard, 2, inputLines/8, 2, 0);
			err = hand_Error(err, "hand_ConfigDIO{Config DIOBoard input failed}",dnaFalse,dnaTrue,dnaTrue);
			if (err) goto Error;
		}
#endif
		// Maintain global variables at all convienent points in the code 
		dioMode = dioModeGroup;
		LinesOut = numberofOutputLines;
		LinesIn = inputLines;
		err=0;
	}
	else {  // if not initialized !
		err=-1;
	}
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_ConfigPattern (dnaBufferDigital *dBuf, int ports,
                                       int analogQ)
{
	handErrRet err=0;
	int units,ticks;
	
#ifdef NODOBOARD
	return 0;
#endif

	units = 1;				// represents time units of 1 microsecond
	ticks = (*dBuf).TimeBase;	// duration in microseconds

  	if (2>ticks) {
		err=-1;
		err = hand_Error (err,
				"With Digital you cannot go faster than 2 usec, go slower.",
				dnaTrue, dnaTrue, dnaFalse);
		if (err) goto Error;
	}

#ifndef NOAOBOARD
	/* See is we have an AT-AO-10 or a PCI-6713 */
	if ((ATAO10_Code==AOCode) && (analogQ) && (10>ticks)) {
		err=-1;
		err = hand_Error (err,
				"With AT-AO-10 Analog you cannot go faster than 10 usec, go slower.",
				dnaTrue, dnaTrue, dnaFalse);
		if (err) goto Error;
	}
#endif

	/* ticks has to be an unsigned short (2^16-1=65535) */
	/* Get that be changing the scale of units */
	/* Initially ticks in in microseconds (units==1) */
	while ((ticks>65535) && (ticks%10==0)) {
		units++;
		ticks/=10;
	}
	
	/* Check for failure */
	if (ticks>65535) {
		err=-1;
		err = hand_Error (err,
				"Ticks is > 65535..Error in hand_ConfigPattern.",
				dnaTrue, dnaTrue, dnaFalse);
		if (err) goto Error;
	}
		
	// Group 1, Single Buffer, Internal Clock, No External gating 
	// Seems to always be a needed command
	err = DIG_Block_PG_Config (DOBoard, 1, 2, 0, units, ticks, 0);
	err = hand_Error(err,"hand_ConfigPattern{DIG_Block_PG_Config call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
#ifndef NODIOBOARD					  
	if (4<ports) {
		/* Need to do DIOBoard as well */
		/* Group 1 */
		/* TODO FIXME : Check the clock signal setting on this function */
		err = DIG_Block_PG_Config (DIOBoard, 1, 2, 1, units, ticks, 0);
		err = hand_Error(err,"hand_ConfigPattern{DIG_Block_PG_Config call failed}",
					  dnaFalse, dnaTrue, dnaTrue);

		
	}
#endif
	if (err) goto Error;
Error:
	return (err?-1:0);
}

// Called from Brain.c Execute Run
// TODO FIXME : Make this work with DIOBoard
/* Typically the InputPorts will be zero */
/* AnalogQ is used on WG2 AT-AO-10 to slow down ticks to 10 usec from 2 usec limit */
handErrRet CVIFUNC hand_ConfigDigital (dnaBufferDigital *digitalBuffer1,
                                       dnaBufferDigital *digitalBuffer2,
                                       int analogQ, int inputPorts)
{
	handErrRet err=0;
	int ports;
	// need to configure group 
#ifdef NODOBOARD   
	return 0;
#endif
#ifdef NODIOBOARD
	ports=(*digitalBuffer1).NumberOfPorts;
#else
	ports=(*digitalBuffer1).NumberOfPorts+(*digitalBuffer2).NumberOfPorts;
#endif
	// pass off to a wrapped function
	// This will handle both boards
	err = hand_ConfigDIO (8*ports, 8*inputPorts);
	err = hand_Error (err, "hand_ConfigDigital{ConfigDIO call failed}",
					  dnaTrue, dnaTrue, dnaTrue);
	if (err) goto Error;

	// need to setup pattern generation in a wrapped function
	// This will handle both boards
	err = hand_ConfigPattern (digitalBuffer1,ports,analogQ);
	err = hand_Error (err, "hand_ConfigDigital{ConfigPattern call failed}",
					  dnaTrue, dnaTrue, dnaTrue);
	if (err) goto Error;
	
	/* Trigger are not used, latency is upredictable */
	// need to configure triggers, if any
//	err = DIG_Trigger_Config (DOBoard, 1, 0, 1, 0, 2, 2, 0, 0);
//	err = hand_Error(err,"hand_ConfigDigital{DIG_Block_Out call failed}",
//					  dnaFalse, dnaTrue, dnaTrue);
//	if (err) goto Error;
	
Error:
	return (err?-1:0);
} 

handErrRet CVIFUNC hand_ClearRTSI (void)
{
	handErrRet err=0;
#ifdef NODOBOARD
	return 0;
#endif
	err = RTSI_Clear (DOBoard);
	err = hand_Error(err,"hand_ClearRTSI{RTSI_Clear (DO) call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
#ifndef NODIOBOARD
	err = RTSI_Clear (DIOBoard);
	err = hand_Error(err,"hand_ClearRTSI{RTSI_Clear (DIO) call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
#endif
#ifndef NOAOBOARD
	if (ATAO10_Code==AOCode) {
		err = RTSI_Clear (AOBoard);
		err = hand_Error(err,"hand_ClearRTSI{RTSI_Clear (AO) call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
	}
#endif    
Error:
	return (err?-1:0);
}


handErrRet CVIFUNC hand_ConfigureRTSI (int analoginuse)
{
	handErrRet err=0;
#ifdef NODOBOARD
	return 0;
#endif
	require(dioModeGroup==dioMode);

	err = RTSI_Clear (DOBoard);
	err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Clear (DO) call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;

	/* obtain RTSI_Clock from DOBoard*/
	err = RTSI_Clock (DOBoard, 1, 1);
	err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Clock from DOBoard failed}",
				  	dnaFalse, dnaTrue, dnaTrue);

	if (err) goto Error;
	
	// 0 = REQ1 signal on board, 0 = RTSI line 0, 1 = transmit 
	err = RTSI_Conn (DOBoard, 0, 0, 1);
	err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Conn (DO) call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
	
	

#ifndef NODIOBOARD
	if (32<LinesOut) {

		err = RTSI_Clear (DIOBoard);
		err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Clear (DIO) call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;


		/* send RTSI_Clock to DIOBoard*/
		err = RTSI_Clock (DIOBoard, 1, 0);
		err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Clock to DIOBoard failed}",
					  	dnaFalse, dnaTrue, dnaTrue);
		
		if (err) goto Error;
		
		// 0 = REQ1, 0 = RTSI line 0, 0 = receive 
		err = RTSI_Conn (DIOBoard, 0, 0, 0);
		err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Conn (DIO) REQ1 call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;

	}	
#endif
	
#ifndef AOBOARD
	
	if ((dnaTrue==analoginuse) &&(ATAO10_Code==AOBoard)) {  
		// Stick a set of RTSI commands here: 
		err = RTSI_Clear (AOBoard);
		err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Clear (AO) call failed}",
					  	dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;

		// 5 = EXTUPDATE, 0 = RTSI line 0, 0 = receive 
		err = RTSI_Conn (AOBoard, 5, 0, 0);
		err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Conn (AO) call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
		// WFM_Rate (1000.0, 0, , );	// Externally driven do not need
		// Now tell it to use external clock triggering 	
		// GOTCHA 1 is group number 
		// 1st GOTCHA 0 is external, rest are irrelevant 
		err = WFM_ClockRate (AOBoard, 1, 0, 0, 0, 0);
	//	err = WFM_ClockRate (AOBoard, 1, 0, 0, 0, 0);
		err = hand_Error(err,"hand_ConfigureRTSI{WFM_ClockRate call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
	}
	
	
	if ((dnaTrue==analoginuse) && (PCI6713_Code==AOBoard)) {  
		// Stick a set of RTSI commands here: 
		err = RTSI_Clear (AOBoard);
		err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Clear (AO) call failed}",
					  	dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
		
		
		/* send RTSI_Clock to PCI6713Board*/
		err = RTSI_Clock (AOBoard, 1, 0);
		err = hand_Error(err,"hand_ConfigureRTSI{RTSI_Clock to AOBoard failed}",
					  	dnaFalse, dnaTrue, dnaTrue);
		
		if (err) goto Error;
		
		// FIXME : Check signal high->low polarity with oscilloscope !!!!!
		/* Connects the UPDATE* signal to listen to RTSI0 from DOBoard */
		err = Select_Signal (AOBoard, ND_OUT_UPDATE, ND_RTSI_0,
							 ND_HIGH_TO_LOW);
		err = hand_Error(err,"hand_ConfigureRTSI{Select_Signal to AOBoard failed}",
					  	dnaFalse, dnaTrue, dnaTrue);
		
		if (err) goto Error;
		
	}
#endif
Error:
	return (err?-1:0);
}


handErrRet CVIFUNC hand_ConfigAnalog (dnaBufferAnalog *analogBuffer)
{
	handErrRet err=0;
	int i;
	short bc;
#ifdef NOAOBOARD
	return 0;
#endif
	err = hand_ResetAnalog ();
	err = hand_Error (err,
					  "hand_ConfigAnalogIdle{hand_ResetAnalog failed}", 1,
					  1, 0);
	if(err) goto Error;
	// Need to setup group, GOTCHA 1 is for group number 
	err = WFM_Group_Setup (AOBoard, 0, 0, 1);
	err = hand_Error(err,"hand_ConfigAnalog{WFM_Group_Setup call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
	err = WFM_Group_Setup (AOBoard, (*analogBuffer).NumberOfOutputs,
			(*analogBuffer).ChannelVector, 1);
	err = hand_Error(err,"hand_ConfigAnalog{WFM_Group_Setup call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
	// Keep mode on the ball 	
	analogMode=analogModeGroup;
#ifdef NODOBOARD
	// Without digital board we need analog to clock its own output internally
	// 25 usec => timebase 1 update interval 25
	err = WFM_ClockRate (AOBoard, 1, 0, 1, 25, 0);
	err = hand_Error(err,"hand_ConfigAnalog{WFM_ClockRate call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
#endif

// Need to attach buffer to group 
	err = WFM_Load (AOBoard, (*analogBuffer).NumberOfOutputs,
			  (*analogBuffer).ChannelVector, &(*(*analogBuffer).Buffer).unipolar,
			  (*analogBuffer).NumberOfOutputs*(*analogBuffer).NumberOfWords,
			  1, 0);
	err = hand_Error(err,"hand_ConfigAnalog{WFM_Load call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
	
	// 1st GOTCHA 1 is group number 
	// 2nd GOTCHA 1 is start 
	err = WFM_Group_Control (AOBoard, 1, 1);
	err = hand_Error(err,"hand_ConfigAnalog{WFM_Group_Control call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
Error:
	return (err?-1:0);
}

/* begin TESTING  */
/* This section was used while testing alternate methods of doing things,
 * often this involved callback routines / threads / message passing

void CVICALLBACK myDCB(void *callbackData)
{
	int err=0;
//	err = Config_DAQ_Event_Message (DOBoard, 1, "DO0", 1, (int)callbackData,
//			0, 0, 0, 0,0, 1000, myCallback);
	printf("my DCB Thread ID %d and err=%d\n",CurrThreadId (),err);
}
static double TT=0.0;
static int spams=0;

int CVICALLBACK myAsynch (int reserved,int timerId,int event,void *callbackData,
	int eventData1,int eventData2)
{
	BOOL SS;
	if (0.0!=TT) {
		printf("My Thread %d ",CurrThreadId());
		printf("myAsynch Time = %f speed=%d  Spams= %d\n",
				Timer()-TT,GetThreadPriority(GetCurrentThread()),spams++);
	}
	else {
		TT=Timer();
		spams=0;
//		SS=SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
//		SS=SetPriorityClass(GetCurrentThread(),HIGH_PRIORITY_CLASS);
		if (FALSE==SS)
			printf("ooops\n");
	}
	return 0;
}


void myEndCallback2(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam) ;
void myCallback3(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam) ;
int CVICALLBACK myAsynch2 (int reserved,int timerId,int event,void *callbackData,
	int eventData1,int eventData2);
extern double SECS;

//extern double GLOBALTIME;
//extern double TARGETTIME;
//extern double v7;
//extern dnaBufferRF rfBuf;

// TESTING
//extern short InBuffer[50000];

/* end TESTING */

/*
handErrRet CVIFUNC hand_OutBuffer (dnaBufferDigital *dBuf)
{
	handErrRet err=0;
//	int N;
	
//	int t;
//	int time=60000;
//	int R;					  	

	// begin TESTING 
//
//	time=rfBuf.CallTimebase;
//	printf ("hi mom\n\n");

//	v7=0.0;
//	hand_SetAnalogOutput (7, v7);

//	t=time/(*dBuf).TimeBase;
//	printf("t=%d  Timebase=%d\n",t,(*dBuf).TimeBase);
						  
//	printf("hand_OutBuffer Thread ID %d\n",CurrThreadId ());
	
//	err = Config_DAQ_Event_Message (DOBoard, 1, "DO0", 1, t, 0, 0, 0, 0,
//									0, time, myCallback);
//	err = Config_DAQ_Event_Message (DOBoard, 1, "DO0", 0, 1, 0, 0, 0, 0,
//									0, 0, myCallback3);
//	err = Config_DAQ_Event_Message (DOBoard, 1, "DO0", 2, 1, 0, 0, 0, 0,
//									0, 0, myEndCallback2);
//	err = hand_Error(err,"hand_OutBuffer{Config_DAQ_Event_Message call failed}",
//					  dnaFalse, dnaTrue, dnaTrue);
//	if (err) goto Error;
//	PostDelayedCall (myDCB, (void *)t, 0.0);
//	TT=0.0;
//	SECS=0.0;
//	spams=0;
//	if (rfBuf.Buffer)  {
//		mouth_OutBufferCommand (&rfBuf);
//		TARGETTIME=0.0;
//		printf("myAsynch installed");
//		NewAsyncTimer (0.006, 500, 1, myAsynch2, (void *)t);
//	}
	
//	printf("DELAY 1.0\n");
//	Delay(1.0);
	
//	SetPriorityClass(GetCurrentThread(),HIGH_PRIORITY_CLASS);
//	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
	// need to output the data ! Send to group 1 
//	mouth_OutBufferCommand (&rfBuf);
	// This lets the thread know to start going 
	
//	err = hand_ConfigRFCallback();
//	err = hand_Error(err,"hand_OutBuffer{hand_ConfigRFCallback call failed}",
//					  dnaTrue, dnaTrue, dnaTrue);
//	if (err) goto Error;
	// end TESTING
*/


/***************************************************************/
/* This start the output for the run */
handErrRet CVIFUNC hand_OutBuffer (dnaBufferDigital *dBuf1,
                                   dnaBufferDigital *dBuf2)
{
	handErrRet err=0;
#ifdef THREADS
	EnterCriticalSection(&critical);
	CHKTIME=GetTickCount();   
	LeaveCriticalSection(&critical);
#else
	CHKTIME=GetTickCount();   
#endif

#ifndef NODIOBOARD	
	if (0<dBuf2->NumberOfPorts) {
		err = DIG_Block_Out (DIOBoard, 1, (*dBuf2).Buffer.asU16,
							 (*dBuf2).NumberOfWords);
	}
#endif
	err = DIG_Block_Out (DOBoard, 1, (*dBuf1).Buffer.asU16,
						 (*dBuf1).NumberOfWords);
	err = hand_Error(err,"hand_OutBuffer{DIG_Block_Out call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
Error:
	return (err?-1:0);
}

// TODO FIXME : Make this work with second board
// TODO : This seems to have now been made to work with second board
//        still need to test output of second board.
/* This routine impelemts the idle output */
/* It does this in pattern output mode to update all lines
 * at the same instant.  For some reason, the buffer short word[12] 
 * can hold 6 32-bit words, the buffer is filled with 3 32-bit words
 * and 2 32-bit words are output.  Why did I do that?
 */
 /* Answer: DIG_Block_Out requires at least two points. */
// APC / CEK modified to handle two cards
handErrRet CVIFUNC hand_OutWord (unsigned int AWord, unsigned int BWord)
{
	handErrRet err=0;
	static short wordsA[12],wordsB[12];  //buffers for the 2 digital boards
	unsigned int *p; // itertor over wordsA and wordsB
	unsigned int numberRemain; 

#ifdef NODOBOARD
	return 0;
#endif
	err=hand_ClearRTSI();
	err = hand_Error(err,"hand_OutWord{hand_ClearRTSI call failed}",
					  dnaTrue, dnaTrue, dnaTrue);
	if (err) goto Error;
	
	if (dioModeGroup==dioMode) {
		p = (unsigned int *)&wordsA;
		*p=AWord;
		p++;
		*p=AWord;
		p++;
		*p=AWord;
		p = (unsigned int *)&wordsB;
		*p=BWord;
		p++;
		*p=BWord;
		p++;
		*p=BWord;
#ifdef NODIOBOARD
		err = hand_ConfigDIO(32,0);
#else
		err = hand_ConfigDIO(64,0);
#endif
		err = hand_Error(err,"hand_OutWord{hand_ConfigDIO call failed}",
						  dnaTrue, dnaTrue, dnaTrue);
		if (err) goto Error;
		
		err = DIG_Block_PG_Config (DOBoard, 1, 2, 0, 1, 10, 0);
		err = hand_Error(err,"hand_OutWord{DIG_Block_PG_Config DO call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
		err = DIG_Block_Out (DOBoard, 1, wordsA, 2);
		err = hand_Error(err,"hand_OutWord{DIG_Block_Out DO call failed}",
				dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error; 

#ifndef NODIOBOARD
		err = DIG_Block_PG_Config (DIOBoard, 1, 2, 0, 1, 10, 0);
		err = hand_Error(err,"hand_OutWord{DIG_Block_PG_Config DIO call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
		err = DIG_Block_Out (DIOBoard, 1, wordsB, 2);
		err = hand_Error(err,"hand_OutWord{DIG_Block_Out DIO call failed}",
				dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error; 
#endif		
		
		numberRemain=3;
		do {
			/* We must call DIG_Block_Check to satisfy the Pattern generation output */
			err = DIG_Block_Check (DOBoard, 1, &numberRemain);
			err = hand_Error(err,"hand_CheckProgress{DIG_Block_Check call failed}",
							  dnaFalse, dnaTrue, dnaTrue);
			if (err) goto Error;

		} while (numberRemain!=0);
#ifndef NODIOBOARD
		do {
			/* We must call DIG_Block_Check to satisfy the Pattern generation output */
			err = DIG_Block_Check (DIOBoard, 1, &numberRemain);
			err = hand_Error(err,"hand_CheckProgress{DIG_Block_Check call failed}",
							  dnaFalse, dnaTrue, dnaTrue);
			if (err) goto Error;

		} while (numberRemain!=0);
#endif
	}
	else
		err=-1;
		err = hand_Error(err,"hand_OutWord{dioMode is not dioModeGroup}",
				dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error; 
Error:
	return (err?-1:0);
}



// TESTING
//void myInCallback1(DAQEventHandle hwnd, DAQEventMsg message, 
//		DAQEventWParam wparam, DAQEventLParam lparam);
/* 2000-03-24 Make this do nothing we do not use callbacks. This function is broken */
handErrRet CVIFUNC hand_ConfigRFCallback (void)
{
	handErrRet err=0;
	int R;
	
	// hand_ConfigDigial called hand_ConfigDIO to set input group already 
	// we need to setup pattern generation, triggers, and then begin the
	// transfer 
//	SetPriorityClass(GetCurrentThread(),NORMAL_PRIORITY_CLASS);
//	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
/*
#ifndef NODIOBOARD
	err = DIG_Block_PG_Config (DIOBoard, 2, 2, 0, 4, 10, 0);
	err = hand_Error(err,"hand_ConfigRFCallback{hand_ConfigRFCallback call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
//	err = DIG_Trigger_Config (DIOBoard, 2, 0, 1, 0, 1, 2, 255*256+255, 255*256+255);
//	err = hand_Error(err,"hand_ConfigRFCallback{hand_ConfigRFCallback call failed}",
//					  dnaFalse, dnaTrue, dnaTrue);
//	if (err) goto Error;

//	err = Config_DAQ_Event_Message (DIOBoard, 1, "DI16", 1, 1000, 0, 0, 0,
//									0, 0, 0, myInCallback1);
//	err = hand_Error(err,"hand_ConfigRFCallback{hand_ConfigRFCallback call failed}",
//					  dnaFalse, dnaTrue, dnaTrue);
//	if (err) goto Error;


	err = DIG_Block_In (DIOBoard, 2, InBuffer,100);
	err = hand_Error(err,"hand_ConfigRFCallback{hand_ConfigRFCallback call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
	
	err = DIG_Block_Check (DIOBoard, 2, &R);
	printf("R= %d\n",R);
	err = hand_Error(err,"hand_CheckProgress{DIG_Block_Check call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;

#endif
*/
Error:
	return (err?-1:0);
}

/* 2000-03-24 Make this do nothing we do not use callbacks. This function is broken */
handErrRet CVIFUNC hand_InstallEndCallback (DAQEventCallbackPtr functionName)
{
	handErrRet err=0;
/*
#ifndef NODIOBOARD

	err = Config_DAQ_Event_Message (DIOBoard, 1, "DI16", 1, 1000, 0, 0, 0,
									0, 0, 0, myInCallback1);
	err = hand_Error(err,"hand_ConfigRFCallback{hand_ConfigRFCallback call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
	EndCallbackQ=dnaTrue;
#endif
*/
Error:
	return (err?-1:0);
}


/* This is called during a run to see if the run has been completed */
handErrRet CVIFUNC hand_CheckProgress (unsigned long *numberCompleted)
{
	handErrRet err=0;

	err = DIG_Block_Check (DOBoard, 1, numberCompleted);
	err = hand_Error(err,"hand_CheckProgress{DIG_Block_Check  (DO) call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;

	if (32 < LinesOut) {
		err = DIG_Block_Check (DIOBoard, 1, numberCompleted);
		err = hand_Error(err,"hand_CheckProgress{DIG_Block_Check (DIO) call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
	}
Error:
	return (err?-1:0);
}

/* At one point AbortRun and RunOver did the same thing */

handErrRet CVIFUNC hand_AbortRun (void)
{
	handErrRet err=0;
// DO NOT DO THIS IN HAND
#ifdef PRIORITY
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL );
//	SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS );
#endif
	if (EndCallbackQ) {					// Ensure this is uninstalled
		err = Config_DAQ_Event_Message (DIOBoard, 0, "DI16", 1, 1000, 0, 0, 0,
										0, 0, 0, 0);
		err = hand_Error(err,"hand_AbortRun{Config_DAQ_Event_Message call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
		EndCallbackQ=dnaFalse;
	}
	
	if (dioModeGroup==dioMode) {
		// This will interrupt the transfer
		err = DIG_Block_Clear (DOBoard, 1);
		if (32<LinesOut) {
			err = DIG_Block_Clear (DIOBoard, 1);
		}
		if (-10608==err) err=0;			// ignore this error
		err = hand_Error(err,"hand_AbortRun{DIG_Block_Clear call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
		err = hand_ResetAll();
		err = hand_Error (err, "hand_AbortRun{hand_ResetAll call failed}",
						  dnaTrue, dnaTrue, dnaFalse);
		if (err) goto Error;
	}
//	err = DIG_Block_PG_Config (DOBoard, 1, 0, 0, 0, 0, 0);
//	err = hand_Error(err,"hand_AbortRun{DIG_Block_PG_Config call failed}",
//					  dnaFalse, dnaTrue, dnaTrue);
//	if (err) goto Error;
	
Error:
	return (err?-1:0);
}

handErrRet CVIFUNC hand_RunOver (void)
{
  	int err=0;
int R;					  	
// TESTING	
//	err = DIG_Block_Check (DIOBoard, 2, &R);
//	printf("R3= %d\n",R);
	if (EndCallbackQ) {					// Ensure this is uninstalled
		err = Config_DAQ_Event_Message (DIOBoard, 0, "DI16", 1, 1000, 0, 0, 0,
										0, 0, 0, 0);
		err = hand_Error(err,"hand_RunOver{Config_DAQ_Event_Message call failed}",
						  dnaFalse, dnaTrue, dnaTrue);
		if (err) goto Error;
		EndCallbackQ=dnaFalse;
	}
	
	
//	if (dioModeGroup==dioMode) {
//		err = DIG_Block_Clear (DOBoard, 1);
//		err = hand_Error (err, "hand_RunOver{DIG_Block_Clear call failed}",
//						  dnaFalse, dnaTrue, dnaFalse);
//		if (err) goto Error;
//		err = hand_ResetAll ();
//		err = hand_Error (err, "hand_RunOver{hand_ResetAll call failed}",
//						  dnaTrue, dnaTrue, dnaFalse);
//		if (err) goto Error;
//	}	  
	err = DIG_Block_PG_Config (DOBoard, 1, 0, 0, 0, 0, 0);
	err = hand_Error(err,"hand_RunOver{DIG_Block_PG_Config call failed}",
					  dnaFalse, dnaTrue, dnaTrue);
	if (err) goto Error;
Error:
	return (err?-1:0);
}
