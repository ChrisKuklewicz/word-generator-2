#ifndef __BRAINh__
#define __BRAINh__
// BRAIN.c

// Here are prototype functions for FACE to call to let BRAIN know
// about what the user has changed.

//	External references to BRAIN global variables.
//	Easier than a complicated passing scheme or making it one file.

// Set to false if there is another copy running
extern int ExperimentEnabled;			// see Brain.c for details
// These indicate run status
extern volatile int RunDone;			// see BRAIN.c for details
extern volatile int RunMode;			// see BRAIN.c for details
// These are the famous BRAIN globals
extern dnaClusterArray Clusters;		// BRAIN.c Digital cluster data
extern dnaARFGroupArray AGroups;		// Storage of analog data         
extern dnaARFGroupArray RFGroups;		// Storage of RF data
extern dnaInitIdle InitIdle;			// Storage of all idle values
extern brainInterpData InterpData[dnaNumInterps];	// configuration data
extern dnaAuxillaryInfo AuxillaryInfo;	// Used when Saving/Loading
extern int RunStatusDialog; 			// Used to hold window handle

#if defined THREADS
extern HANDLE ThreadInfo;
#endif

// Called by main in FACE.c after it creates all of the
// panels and controls, so there is something to turn on. This
// creates all BRAIN.c global variables and puts them in a valid,
// even if trivial, state.  FACETWO.h functions are called to
// connect these morsels to the interface 
int TurnMeOn(void);						// return err -1 or 0

int SetIdleOutput(dnaByte setIdle);		// return err -1 or 0

int SetIdle(int line, dnaByte value);	// return err -1 or 0

int SetIdleWord(int index);				// return err -1 or 0

int SetAnalogIdle(void);

int SetIdleRF(double Amp, double DC, double Freq);

// parameters will be added. User selection of timebase,etc... 
int ExecuteRun(dnaByte FirstTimeQ);		// return err -1 or 0

// This gets a graph ready to be interpolated for the buffer 
// old interpolation data is discarded, the Duration is taken
// from group so group info is no longer needed 
// graph is the output parameter
int PrepInterp(dnaARFGroupArray *groupArray, int IGroup, int IGraph,
		dnaARFGraph **graphOut);


// parameters are selected for ease when calling from enableGraph 
// This builds up for an interpolation for use in a graph 
// old interpolation data is discarded, the Duration is taken
// from group so group info is no longer needed 

int MakeInterp(dnaARFGroup *group, dnaARFGraph *graph, int NumInterps);


int CheckRunStatus(void);				
	// Called by a timer control callback during RunningMode
	// Decides how much of buffer is done and finds current cluster
	// and calls faceShowProgress or faceEndRunningMode


// This sorts the XY value pairs for a graph by increasing x value 
// Returns -1 on error, 0 on nothing changed, 1 on changed made 
// index is passed by reference to keep track of swaps 
int SortValues(dnaARFGraph *graph, int *index);


// Now for user control of analog / RF ranges

int SetRangeMin(int index, double value, double *setvalue);

int SetRangeMax(int index, double value, double *setvalue);

double GetRangeMin(int index, double value);

double GetRangeMax(int index, double value);

int GetYMin(dnaARFGraph *graph, double *value);

int GetYMax(dnaARFGraph *graph, double *value);

// Used to save and load settings 
int SaveIni(void);
int LoadIni(const char filename[]);    

// NOTE: popupCLUSTER menu functions are implimented in BRAIN 

#endif

