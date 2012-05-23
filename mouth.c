#include "dna.h" 
#include "mouth.h"
#include <gpib.h>
//

//This is the MOUTH of Word Generator 2.
//The purpose of this source code is to wrap the GPIB interface functions
//into a seperate file (and eventually function panel).

//The BRAIN uses these commands to communicate with the GPIB
//instruments: SRS DS345.


#include "toolbox.h"

static const char gpibDS345Name[] = "DEV2";			// setup in driver panel
static const int gpibDS345Addr = 19;				// set on front panel of SRS
static int gpibDS345=-1;							// Handle (-1 is error value)

// Sort of a magic number
#define NoValue 577.5698e-4;						
// SMART CACHE While adding buffer commands: 
double LastAmp=NoValue;
double LastFreq=NoValue;

int CVIFUNC mouth_InitRF (void)
// Same for all the GPIB modes
{
	int err=0;
	// We only need to do this once
	if (-1==gpibDS345)
		gpibDS345 = ibdev (0, gpibDS345Addr, NO_SAD, T3s, 1, 0);
	errChk(mouth_Error("mouth_InitRF{ibdev call failed}",0,0));
	LastAmp=NoValue;
	LastFreq=NoValue;
Error:
	return (err?-1:0);
}

int CVIFUNC mouth_Error (const char message[], dnaByte abortQ,
						 dnaByte forceErrorQ)
{
	int status,err;
	long count;
	char mouthMessage[80]={0};
#if VERBOSE > 0
	if (forceErrorQ) {
		FmtOut ("%s\n", message);
		Beep();
		Breakpoint();
		ScanIn ("%s", mouthMessage);		// Wait for enter to be pressed
		if (abortQ) abort();
		return -1;
	}		
	status = ThreadIbsta ();
	if (status & ERR) {						// ERR is defined in gpib.h
		err=ThreadIberr();
		count = ThreadIbcntl();
		FmtOut ("%s\n", message);
		FmtOut ("status %d  error %d count %d\n", status,err,count);
		FmtOut ("Library error so Abort always requested.\n");
		FmtOut ("Reminder: Did you turn on the SRS? to GPIB ID #19?\n");
		Beep();
		Breakpoint();
		ScanIn ("%s", mouthMessage);		// Wait for enter to be pressed
		if (abortQ) abort();
		return -1;
	}		
#else
	if (forceErrorQ) {
		Beep();
		MessagePopup ("Mouth Error", message);
		Breakpoint();
		if (abortQ) abort();
		return -1;
	}
	status = ThreadIbsta ();
	if (status & ERR) {
		err=ThreadIberr();
		count = ThreadIbcntl();
		Fmt (mouthMessage, "%s<%s\n", message);
		Fmt (mouthMessage, "%s[a]<status %d  error %d count %d\n", status,err,count);
		if (abortQ)
			Fmt (mouthMessage, "%s[a]<Software Error and Abort has been requested.\n");
		Fmt (mouthMessage, "%s[a]<Reminder: Did you turn on the SRS? to GPIB ID #19?\n");
		Beep();
		MessagePopup ("Mouth Error", mouthMessage);
		Breakpoint();
//		if (abortQ) abort();
		return -1;
	}		

#endif

	return 0;    
}


int CVIFUNC mouth_WriteDS345 (const char command[], int numberofBytes)
{
 	int err=0;

//	ibwrt (gpibDS345, command, numberofBytes);
	ibwait (gpibDS345, 0);					// re-synch
	errChk(mouth_Error("mouth_WriteDS345{ibwait call failed}",0,0));
	ibwrta (gpibDS345, command, numberofBytes);
	errChk(mouth_Error("mouth_WriteDS345{ibwrta call failed}",0,0));
Error:
	return (err?-1:0);
}

int CVIFUNC mouth_ZeroDS345 (void)
{
 	int err=0,l;
	char *command;

	mouth_MakeIdleCommand(&command,0.0,0.0,0.0);
	l=strlen(command);
//	ibwrt (gpibDS345, "OFFS, numberofBytes);
	ibwait (gpibDS345, 0);					// re-synch
	errChk(mouth_Error("mouth_ZeroDS345{ibwait call failed}",0,0));
	ibwrta (gpibDS345, command, l);
	errChk(mouth_Error("mouth_ZeroDS345{ibwrta call failed}",0,0));
Error:
	return (err?-1:0);
}

int CVIFUNC mouth_MakeIdleCommand (char ** command, double amp, double DCOffset,
								   double freq)
// Takes frequency in MHz, Offset V, Ampl Vpp
{
	int err=0;
	char buf[255];
	int pos=0;
	
	freq*=1000000.0;					// Convert to Hertz
	FillBytes (buf, 0, 255, 0);
	pos=Fmt(buf,"%s<OFFS %f;AMPL %fVP;FREQ %f",DCOffset,amp,freq);
	if (pos<0) {
		err=-1;
		command=0;
		goto Error;
	}
	(*command)=StrDup((char *)&buf);
	
Error:
	return (err?-1:0);
}

int CVIFUNC mouth_AddBufferCommand (dnaBufferRF*buffer, double amp, double freq)
// Takes frequency in MHz, Ampl Vpp
{
	int err=0;
	if (freq!=LastFreq)
		if (amp!=LastAmp) {
			LastAmp=amp;
			LastFreq=freq;
			freq*=1000000.0;			// Convert to Hz
			Fmt(&buffer->Buffer[buffer->PosByte],"%s<AMPL %fVP;FREQ %f",amp,freq);
#if VERBOSE > 1
			printf("%s\n",&buffer->Buffer[buffer->PosByte]);
#endif
			// Order of the next two commands is important 
			buffer->NumberOfCommands++;
			errChk(dna_AdvanceBufferRF (buffer));
		}
		else
			errChk(mouth_AddBufferFreq(buffer,freq));
	else
		if (amp!=LastAmp)
			errChk(mouth_AddBufferAmp(buffer,amp));
		else {
			buffer->Buffer[buffer->PosByte]=0;
			// Order of the next two commands is important 
			buffer->NumberOfCommands++;
			errChk(dna_AdvanceBufferRF (buffer));
		}			
Error:
	return (err?-1:0);
}

int CVIFUNC mouth_AddBufferZero (dnaBufferRF*buffer)
{
	int err=0;
	LastAmp=0.0;
	LastFreq=0.0;
	Fmt(&buffer->Buffer[buffer->PosByte],"%s<AMPL %fVP;FREQ %f",0.0,0.0);
#if VERBOSE > 1
	printf("%s\n",&buffer->Buffer[buffer->PosByte]);
#endif
	// Order of the next two commands is important 
	buffer->NumberOfCommands++;
	errChk(dna_AdvanceBufferRF (buffer));
Error:
	return (err?-1:0);
}


int CVIFUNC mouth_AddBufferFreq (dnaBufferRF*buffer, double freq)
{
	int err=0;
	LastFreq=freq;
	freq*=1000000.0;					// Convert to Hz
	Fmt(&buffer->Buffer[buffer->PosByte],"%s<FREQ %f",freq);
#if VERBOSE > 1
	printf("%s\n",&buffer->Buffer[buffer->PosByte]);
#endif
	// Order of the next two commands is important 
	buffer->NumberOfCommands++;
	errChk(dna_AdvanceBufferRF (buffer));
Error:
	return (err?-1:0);
}

int CVIFUNC mouth_AddBufferAmp (dnaBufferRF*buffer, double amp)
{
	int err=0;
	Fmt(&buffer->Buffer[buffer->PosByte],"%s<AMPL %fVP",amp);
#if VERBOSE > 1
	printf("%s\n",&buffer->Buffer[buffer->PosByte]);
#endif
	// Order of the next two commands is important 
	buffer->NumberOfCommands++;
	errChk(dna_AdvanceBufferRF (buffer));
	LastAmp=amp;
Error:
	return (err?-1:0);
}
															
int CVIFUNC mouth_ResetBuffer (dnaBufferRF*buffer)
// Gets buffer ready for building or output
{
	LastAmp=NoValue;
	LastFreq=NoValue;

	buffer->CurrentStage=0;
	buffer->TargetTime=buffer->StartTimes[0];
	buffer->CommandCounter=buffer->Commands[0];
	buffer->CallTimebase=buffer->RFTimebase[0];
	buffer->PosByte=0;
	buffer->PosCommand=0;
	return 0;    
}


int CVIFUNC mouth_OutBufferCommand (dnaBufferRF*buffer)
{
	int l;
	int err=0;
	require(0<buffer->CommandCounter);
	l = dna_LengthBufferRF (buffer);
	if (l) {
//		ibwrt (gpibDS345, buffer->Buffer+buffer->PosByte, l);
		ibwait (gpibDS345, 0);					// re-synch
		if (ThreadIbsta() & ERR)
			return -2;							// TODO BUG GOTCHA : make more verbose
//		errChk(mouth_Error("mouth_WriteDS345{ibwait call failed}",0,0));
		ibwrta (gpibDS345, buffer->Buffer+buffer->PosByte, l);
		if (ThreadIbsta() & ERR)
			return -3;
//		err = mouth_Error("mouth_WriteDS345{ibwrta call failed}",0,0);
#if VERBOSE > 1
		printf("%s\n",&buffer->Buffer[buffer->PosByte]);
		if (err)
			printf("mouth_OutBufferCommand{ibwrt call failed} %d %d %d %d %d\n",
				buffer->PosByte,buffer->PosCommand,ERR,ThreadIbsta(),ThreadIberr());
#endif
		if (err) goto Error;
		errChk(dna_AdvanceRunBufferRF (buffer));
	}
	else {
		errChk(dna_AdvanceRunBufferRF (buffer));
#if VERBOSE > 0
		printf("mouth_OutBufferCommand{Length was zero} %d %d\n",
				buffer->PosByte,buffer->PosCommand);
#endif
	}
Error:
	return (err?-1:0);
}
