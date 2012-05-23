//	FACETWO header file
//	Function prototypes for BRAIN to call.

// Setup functions 
int faceSetInterpTypes(void);			// Ring controls, mainly
int faceSetNumberOfLines(void);			// Digital output lines
int faceSetNumberOfClusters(void);		// Number of defined words
int faceSetOutputLabels(void);			// Names of the digital lines
int faceSetNumberOfAGroups(void);		// Number of analog groups
int faceSetNumberOfRFGroups(void);		// Number of RF groups
int faceSetIdleValues(void);			// The idle output settings
int faceSetInitAnalog(void);			// The initial analog voltages
int	faceSaveAuxillaryInfo(void);		// Display specific info
int faceLoadAuxillaryInfo(void);		// Display specific info
// Range info for index has changed, index=-1 to change all of them
int faceSetARFRanges(int index);		

int faceInvalidatePointers(void);		// Notification from BRAIN
int faceSendIdleRF(void);				// called before running

// Camera trigger waiting display
int faceAuxWaitOn(int line, int value);	// 0 ok, -1 error
int faceAuxWaitStatus(void);			// 0 normal, 1 cancel, -1 error
int faceAuxWaitOff(void);				// 0 ok, 1 cancel, -1 error

// Running mode message functions
int faceBeginRunningMode(int Timebase);	// Called by ExecuteRun
		// Will disable Idle ouput and enable timing contol and abort control
int faceShowProgress(int clusterIndex, int percentdone);	
		// Called by CheckRunStatus
		// Highlights the currently output index
int faceEndRunningMode(void);			// Called by CheckRunStatus
		// undoes the BeginRunningMode changes

// Mode control functions 
dnaByte	RepeatRunQ(void);				// Loop again and again...?
dnaByte	CameraTriggerQ(void);			// Wait for camera...?

// Used by DDE.c
// void CVICALLBACK MsgSaveIt(void * filename);
void CVICALLBACK MsgOpenIt(void * filename);

/* Used by recent.c */
int CVICALLBACK OpenIt(char * filename);
