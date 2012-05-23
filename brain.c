#include <userint.h>
#include "dna.h"						// Types and defines and DNA prototypes
//#define BUGHUNT
// TESTING 
//double SECS=0.0;
// TESTING
//dnaTime TARGETTIME=0;					// in milliseconds
//double v7=0.0;
#if defined THREADS
HANDLE ThreadInfo = 0;
DWORD threadID;							// DWORD
#endif

short InBuffer[50000]={0};
const int InBufSize=4096*4;
//
//This is the BRAIN of Word Generator 2.
//
//This contains the implimentation of the core logic of the program.
//Internal global variable structures (the Memory) are declared here.
//Types for the stuctures are defines in the DNA code.
//
//The BRAIN uses the MOUTH to do GPIB communication, and the HAND to
//do the analog/digital input/output communication. FACE routines are
//called to configure/update the user interface.
//
//The FACE source code calls the instructions here to begin a run.
//
//Note: execution begins in the FACE main function


#include "brain.h"						// Exported BRAIN Function Prototypes
#include "face.h"
#include "facetwo.h"					// Exported FACE Function Prototypes
#include "hand.h"
#include "mouth.h"
#include "memory.h"
#include <analysis.h>					// needed for interpolation
#include "asynctmr.h"					// Should be used in this module
// These are legendary BRAIN.c globals 
// FACES.c references these as external global 

// These are to keep different callbacks/threads/etc. clued in : 
// TODO : Fix these darn magic numbers, use MACRO defines instead?
volatile int RunDone=-1;				// State of run
	// -1	means idle, before / between run, state
	// 0 	means run in progress
	// 1 	means run is over
volatile int RunMode=-1;
	// -1	means idle, before / between run, state
	// 0	means run has no (more) RF and is running in background
	// 1	means run is waiting for an RF sweep
	// 2	means run is performing an RF sweep
	// 3	means in transition, only ExecuteRun should do stuff
int RunStatusDialog=-1;  				// Handle for progress dialog
int ExperimentEnabled=1;				// Set to false if not running expt
dnaClusterArray Clusters;				// Main storage of digital data   
dnaARFGroupArray AGroups;				// Storage of analog data         
dnaARFGroupArray RFGroups;				// Storage of RF data
dnaInitIdle InitIdle;					// Storage of all idle values
//dnaByte IdleValues[dnaDigitalOutputs];  // Set by SetIdle function below
//double InitAnalog[dnaAnalogChannels];  	// Set by SetIdle function below

dnaAuxillaryInfo AuxillaryInfo;			// Used when Saving/Loading

// These are the buffers we output 
dnaBufferDigital dBuf1,dBuf2;			// Get two digital buffers
dnaBufferAnalog aBuf;					// AO buffer
dnaBufferRF rfBuf;						// RF Buffer
// These are used for config info for interpolation methods 
brainInterpData InterpData[dnaNumInterps]; 	// Info on the interp types;
static char * brainReservedLabels[dnaReservedParams] =
		{"Scale Factor","Scaled Offset"};	// Saves editing time

// Used for timeing purposes

dnaTime GLOBALTIME=0;					// in milliseconds
dnaTime CHKTIME=0;						// in milliseconds

// GOTCHA : Assign size of internal arrays by hand here 

#if WGFLAG == 2
#define brainOutLines 32				// Number of lines actually used
#define brainClusters 32				// Initial # of empty clusters
#define brainAGroups 4					// Initial # of empty analog groups
#define brainRFGroups 4					// Initial # of empty rf groups

#elif WGFLAG == 3
#define brainOutLines 64
#define brainClusters 32
#define brainAGroups 4
#define brainRFGroups 4

#endif

// This is for progress dialog boxes 
static int ProgressHandle=-1;			// for setup
static int PercentDone=0;				// for setup
int faceRunProgress=-1;					// for run


// These flag when a run will include analog or RF output 
static int AnalogQ=dnaFalse;
static int RFQ=dnaFalse;

//typedef enum {stateStartup,stateInit,state} brainState;

CRITICAL_SECTION critical= {0};
int criticalready=0;
	
//*********************************************************************
//																	   
//																	   
//   				 Interpolation Functions / Setup				   
//																	   
//																	   
//*********************************************************************

//typedef int (*brainISetup) (dnaARFGraphPointer graph);    
//typedef double (*brainInterp) (double t, dnaARFGraphPointer graph);

//**********************************************************************
//	Setup function must be ABSOLUTELY certain that if (graph) is passed
//	to the Interp function with any value for time t that the Interp
//	function will return a sensible result without encountering an error
//
// 	The Interp functions must be as fast as possible, and may assume
// 	that (graph) was not altered after the Setup function ran on it.
//
//  Steps to add new function
//		1) Add a new name for it in DNA.h dnaInterpTypes
//		2) Increment dnaNumInterps in DNA.h
//      3) Go to SetupInterps below and copy and paste one of the sections
//			and then edit it (see the dnaISpline section for more comments)
//		4) Copy and paste a setup function and an evaluator function below
//			and then edit them (see the Spline setup and Linear evaluator)
//		5) Test and debug your function
//
//	Guidelines:
//		1) The evaluator will be passed a t from 0 up to graph->Duration
//			in milliseconds (or fractions thereof) for most purposes
//		2) If t is outside this range you should probably return a constant
//			value (hold initial and final values, for instance)
//		3) If the user can enter inconsistant or invalid data then Setup must
//			detect this and return an error, possibly reporting the problem
//			to the user.
//		4) Your Setup function must never crash if passed a wierd graph
//			structure
//		5) If Setup sets dnaInterpReadyQ to dnaTrue then the evaluator
//			must never crashed if it is passed the resulting graph and and t.
//
//**********************************************************************

int GetIntFillNumber(int first, int last, int delta)
{	// returns the number of times that
	// double i; for(i=first; i<last; i+=delta) {command}
	// would execute the command
	double r;
	r=(last-first);
	r=TruncateRealNumber(r/delta);
	if (r*delta<last) 
		r+=1.0;
	return r;
}

int GetDoubleFillNumber(double first, double last, double delta)
{	// returns the number of times that
	// double i; for(i=first; i<last; i+=delta) {command}
	// would execute the command
	double r;
	r=(last-first);
	r=TruncateRealNumber(r/delta);
	if (r*delta<last) 
		r+=1.0;
	return r;
}

double funcNoInterp(double t, dnaARFGraphPointer graph)
{
	// Used as a do-nothing function 
	return 0.0;
}

int funcINoInterpFill (dnaARFGraphPointer graph, short *data,
	int delta, int last, int timebase, int board, int channel)
{
	return -1;
}

int funcNoInterpSetup(dnaARFGraphPointer graph)
{
	// This will never claim to be ready for output/plotting 
	// In fact it is an error to even try to set this up 

	graph->InterpReadyQ=dnaFalse;
	return 0;
}

double funcILinear(double t, dnaARFGraphPointer graph)
{
	int i;								// Need to loop
	double d;							// Intermediate slope calculation

	if (t<graph->XScaled[0]) {		// Should be t<0.0
		// left of all data 
		return graph->YScaled[0];
	}
	else {
		if (graph->XScaled[graph->NumberOfValues-1]<=t) {
			// Should be Duration <= t 
			// right of all values 
			return graph->YScaled[graph->NumberOfValues-1];
		}
	}
	// We should be in middle of data so we have to interpolate 
	for (i=1; i<graph->NumberOfValues; i++) {
		if (t<graph->XScaled[i]) {
			// calc it, return 
			d = (graph->YScaled[i] - graph->YScaled[i-1]) /
					(graph->XScaled[i] - graph->XScaled[i-1]);
			return graph->YScaled[i-1] + 
					d * (t - graph->XScaled[i-1]);
		}
	}
	//error state.... must return something....
	//Breakpoint();
	return graph->YScaled[graph->NumberOfValues-1];
}

int funcILinearFill (dnaARFGraphPointer graph, short *data,
	int delta, int last, double timebase, int board, int channel)
{
	double t,min,max;					// time, min & max voltage
	double d,slope;						// voltage, slope of line segment
	double val,valInc;					// next value, increment of value
	int err=0,i;							// position in data
	short s;							// digitized value
	int j,n;							// j poisition in XScales, j<n
// USERPIN need to use min and max
	errChk(GetYMin(graph,&min));	// look up min
	errChk(GetYMax(graph,&max));   // look up max
	t=0.0;
	i=0;
	n=graph->NumberOfValues;
	j=0;
	d=graph->YScaled[j];				// hold initial value at early times
	d=Pin(d,min,max);					// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));	// digitize
	// Next loop could be a tiny bit faster (calc the # of iterations)
	//  but will never be used in practice.
	while ((i<last) && (t<graph->XScaled[j])) {
		data[i]=s;
		i+=delta;
		t+=timebase;
	}
	++j;
	for (;j<n;j++) {
		slope = (graph->YScaled[j] - graph->YScaled[j-1]) /
				(graph->XScaled[j] - graph->XScaled[j-1]);
		valInc = slope*timebase;
		val = graph->YScaled[j-1] + slope * (t - graph->XScaled[j-1]);
		while ((i<last) && (t<graph->XScaled[j])) {
			d=Pin(val,min,max);			// coerce to prevent crashes
//			errChk(AO_VScale (board, channel, d, &s));
			// For speed purpose, do not error check the next line
			AO_VScale (board, channel, d, &s);
			data[i]=s;
			i+=delta;
			t+=timebase;
			val+=valInc;
		}
		if (last<=i) return 0;			// end of area to fill
		// else we go on to the next line segment
	}
	d=graph->YScaled[graph->NumberOfValues-1];	// hold final value
	d=Pin(d,min,max);					// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));
	while (i<last) {
		data[i]=s;
		i+=delta;
	}
	return 0;
Error:
	return -1;
}

/*
	Need: graph,end-start,delta,tb,igraph,&aBuf.Buffer[start],AOBoard
	for (i=0; i<aBuf.NumberOfOutputs; i++) {
		graph = graphs[i];			// load the graph
		igraph=aBuf.ChannelVector[i];	// setup igraph
		Interp = graph->Interp;   // look up function
		errChk(GetYMin(graph,&min));	// look up min
		errChk(GetYMax(graph,&max));   // look up max
		start = aBuf.Pos+i;			// interleaved start position
		for (j=start,t=0.0; j<end; j+=delta,t+=tb) {
			// not too swift 
			d = Interp(t,graph);	// get the voltage
			d = Pin (d, min, max);	// coerce to prevent crashes
			errChk(AO_VScale (AOBoard, igraph, d, &v.unipolar));
			aBuf.Buffer[j]=v;		// add to buffer
		}
		// FinalValues 
		t=TimeLimit/1000.0;			// make it exact
		d = Interp(t,graph);		// get the voltage
		d = Pin (d, min, max);	// coerce to prevent crashes
		// The next call is the slow step 
		errChk(AO_VScale (AOBoard, igraph, d, &v.unipolar));
		FinalValues[i]=v;
	}
*/


int funcILinearSetup(dnaARFGraphPointer graph)
{
	int err=0,i,dummy=-1;

	graph->InterpReadyQ=dnaFalse;	// Assume the worst.

	if (dnaILinear!=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	
// Since the affine scaling is already done we need to nothing here. 
	nullChk(graph);
	if (0<graph->NumberOfValues) {
		err=0;
	}
	else {
		graph->InterpReadyQ=dnaFalse;
		err=-1;
		goto Error;
	}

	//
	for (i=0; i<graph->NumberOfValues-1; i++) {
		// Check for possible divide by zeros ? 
		// Not necessary by form of algorithm used in funcILinear 
	}
	
	
	// Paranoia : 
	graph->Interp=InterpData[dnaILinear].Interp;
	graph->InterpReadyQ=dnaTrue;
Error:	
	return (err?-1:0);
}

double funcIStep(double t, dnaARFGraphPointer graph)
{
	int i;								// Need to loop
	double d;							// Intermediate slope calculation

	if (t<graph->XScaled[0]) {		// Should be t<0.0
		// left of all data 
		return graph->YScaled[0];
	}
	else {
		if (graph->XScaled[graph->NumberOfValues-1]<=t) {
			// Should be Duration <= t 
			// right of all values 
			return graph->YScaled[graph->NumberOfValues-1];
		}
	}
	// We should be in middle of data so we have to interpolate 
	for (i=1; i<graph->NumberOfValues; i++) {
		if (t<graph->XScaled[i]) {
			// calc it, return 
			return graph->YScaled[i-1];
		}
	}
	//error state.... must return something....
	//Breakpoint();
	return graph->YScaled[graph->NumberOfValues-1];
}

int funcIStepFill (dnaARFGraphPointer graph, short *data,
	int delta, int last, double timebase, int board, int channel)
{
	double t,min,max;					// time, min & max voltage
	double d;						// voltage, slope of line segment
	int err=0,i;								// position in data
	short s;							// digitized value
	int j,n;							// j poisition in XScales, j<n
// USERPIN 
	errChk(GetYMin(graph,&min));	// look up min
	errChk(GetYMax(graph,&max));   // look up max
	t=0.0;
	i=0;
	n=graph->NumberOfValues;
	// Up till XScaled[1] we use YScaled[0]
	for (j=1;j<n;j++) {
		d=graph->YScaled[j-1];
		d=Pin(d,min,max);		
		errChk(AO_VScale (board, channel, d, &s));	// digitize
		while ((i<last) && (t<graph->XScaled[j])) {
			data[i]=s;
			i+=delta;
			t+=timebase;
		}
		if (last<=i) return 0;			// end of area to fill
		// else we go on to the next line segment
	}
	// After (and including) XScaled[n-1] use YScaled[n-1]
	d=graph->YScaled[graph->NumberOfValues-1];		
	d=Pin(d,min,max);					// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));
	while (i<last) {
		data[i]=s;
		i+=delta;
	}
	return 0;
Error:
	return -1;
}

int funcIStepSetup(dnaARFGraphPointer graph)
{
	int err=0,i,dummy=-1;

	graph->InterpReadyQ=dnaFalse;	// Assume the worst.

	if (dnaIStep!=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	
// Since the affine scaling is already done we need to nothing here. 
	nullChk(graph);
	if (0<graph->NumberOfValues) {
		err=0;
	}
	else {
		graph->InterpReadyQ=dnaFalse;
		err=-1;
		goto Error;
	}

	//
	for (i=0; i<graph->NumberOfValues-1; i++) {
		// Check for possible divide by zeros ? 
		// Not necessary by form of algorithm used in funcILinear 
	}
	// Paranoia : 
	graph->Interp=InterpData[dnaIStep].Interp;
	graph->InterpReadyQ=dnaTrue;
Error:	
	return (err?-1:0);
}

double funcIPoly(double t, dnaARFGraphPointer graph)
{
	double d,e;
	PolyInterp (graph->XScaled, graph->YScaled,
			graph->NumberOfValues, t, &d, &e);
	return d;
}

// funcIPolyFill is not a real performance enhancement.
// It will not be used.  The code is left here as an example

int funcIPolyFill (dnaARFGraphPointer graph, short *data,
	int delta, int last, int timebase, int board, int channel)
{
	double t,min,max;					// time, min & max voltage
	double d,e;							// voltage, dummy error estimate
	int err=0,i;							// position in data
	short s;							// digitized value
	int j,n;							// j poisition in XScales, j<n
// USERPIN
	errChk(GetYMin(graph,&min));	// look up min
	errChk(GetYMax(graph,&max));   // look up max
	t=0.0;
	i=0;
	n=graph->NumberOfValues;
	j=0;
	d=graph->YScaled[j];				// hold initial value at early times
	d=Pin(d,min,max);					// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));	// digitize
	// Next loop could be a tiny bit faster (calc the # of iterations)
	//  but will never be used in practice.
	while ((i<last) && (t<graph->XScaled[j])) {
		data[i]=s;
		i+=delta;
		t+=timebase;
	}
	while ((i<last) && (t<graph->XScaled[n-1])) {
		errChk(PolyInterp (graph->XScaled, graph->YScaled,
				graph->NumberOfValues, t, &d, &e));
		d=Pin(d,min,max);					// coerce to prevent crashes
		errChk(AO_VScale (board, channel, d, &s));	// digitize
		data[i]=s;
		i+=delta;
		t+=timebase;
	}	
	d=graph->YScaled[graph->NumberOfValues-1];
	d=Pin(d,min,max);					// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));
	while (i<last) {
		data[i]=s;
		i+=delta;
	}
	return 0;
Error:
	return -1;
}

int funcIPolySetup(dnaARFGraphPointer graph)
{
	int err=0;
	int i,dummy;
	
	graph->InterpReadyQ=dnaFalse;
	
	if (dnaIPoly !=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	if (1>graph->NumberOfValues) {		// No Way !
		err=-1;
		goto Error;
	}

	// check for duplicate x values 
	for (i=0; i<graph->NumberOfValues-1; i++) {
		if (graph->XScaled[i]==graph->XScaled[i+1]) {
			MessagePopup ("Interpolation Failure",
						  "You cannot have duplicate X values");
			err=-1;
			goto Error;
		}			
	}
	
	graph->Interp=InterpData[dnaIPoly].Interp;
	graph->InterpReadyQ=dnaTrue;
Error:	
	return (err?-1:0);
}

double funcIRat(double t, dnaARFGraphPointer graph)
{
	double d,e;
	RatInterp (graph->XScaled, graph->YScaled,
					  graph->NumberOfValues, t, &d, &e);
	return d;
}

// funcIRatFill would not be a big performance enhancement
// Thus it is not included

int funcIRatSetup(dnaARFGraphPointer graph)
{
	int i,err=0;
	
	graph->InterpReadyQ=dnaFalse;
	
	if (dnaIRat !=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	
	if (1>graph->NumberOfValues) {		// No Way !
		err=-1;
		goto Error;
	}
	// check for duplicate x values 
	for (i=0; i<graph->NumberOfValues-1; i++) {
		if (graph->XScaled[i]==graph->XScaled[i+1]) {
			MessagePopup ("Interpolation Failure",
						  "You cannot have duplicate X values");
			err=-1;
			goto Error;
		}			
	}
	graph->Interp=InterpData[dnaIRat].Interp;
	graph->InterpReadyQ=dnaTrue;
Error:	
	return (err?-1:0);
}


double funcISpline(double t, dnaARFGraphPointer graph)
{
	double d;
	SpInterp (graph->XScaled, graph->YScaled, graph->SplineValues,
			  graph->NumberOfValues, t, &d);
	return d;
}

// funcISplineFill would not be a big performance enhancement
// Thus it is not included

int funcISplineSetup(dnaARFGraphPointer graph)
{
	int i,err=0;
	
	// Before this is called the X and Y value pairs, if present, have been
	// sorted in ascending X (time) order and the XValues were translated
	// into XScaled from 0 to Duration. Also YValues were multiplied by
	// P[0] and then had P[1] added to them and were placed in YScaled
	
	// The setup and evaluators currently look at these Scaled values, not
	// the original values the user entered.
	
	graph->InterpReadyQ=dnaFalse;		
	// Setting this value to true, if possible, is the job of a Setup function
	
	if (dnaISpline !=graph->InterpType) {	// Paranoia
		err=-1;
		goto Error;
	}
	
	// If there is a problem with what the user entered, return with
	// InterpReadyQ still set to false and with a negative return value.
	if (2>graph->NumberOfValues) {		// Check on nubmer of x,y values
		err=-1;
		goto Error;
	}
	// check for duplicate x values 
	for (i=0; i<graph->NumberOfValues-1; i++) {
		if (graph->XScaled[i]==graph->XScaled[i+1]) {
			MessagePopup ("Interpolation Failure",
						  "You cannot have duplicate X values");
			err=-1;
			goto Error;
		}			
	}
	
	// Now P[0] and P[1] are the scale factor and offset reserved parameters
	// P[0] and P[1] are always accessible on the panel
	// dnaReservedParams = 2 since this is a "magic number"
	// P[2],P[3],P[4],P[5] are the others the user can access on the panel
	// In spline we only need two parameters so only P[2] and P[3] are entered
	
	// To speed up processing there exist P[6],P[7],P[8], and P[9] so
	// intermediate values can be computed for use in the evaluation function.
	// This allows the scale factors to act on the paramters, e.g. dnaPExp
	// Here the user entered a slope of y values versus time in msec but the 
	// Y values are scaled by P[0] so we also need to scale the slope by the
	// same amount. We will store the scaled version of P[2] and P[3] in
	// P[6] and P[7].

	// We do not store them in P[4] and P[5] since if the user changed to
	// (for example) a sine function then the user will get access to the
	// P[4] and P[5] values, which does no harm but the user may be confused
	// where these numbers were coming from.
	
	// scale the slopes to be consistant 
	graph->P[0+dnaNumUIParams]=graph->P[0+dnaReservedParams]*graph->P[0];
	graph->P[1+dnaNumUIParams]=graph->P[1+dnaReservedParams]*graph->P[0];

// ALLOCATION (get memory for holding second derivatives to speed interpolation
	graph->SplineValues = calloc(graph->NumberOfValues,sizeof(double));
	nullChk(graph->SplineValues);
	// This is a library function that takes our XScaled and YScaled and
	// prepares the intermediate derivative to speed up interpolation
	errChk(Spline (graph->XScaled, graph->YScaled, graph->NumberOfValues,
			graph->P[0+dnaNumUIParams], graph->P[1+dnaNumUIParams], graph->SplineValues));
	// We now set the evaluator function. To allow for optimized evaluation,
	// the setup could choose different functions at this point, which allows
	// for alot of flexibility.  Currently they all use only one function
	// and this evaluator was stored in the InterpData as the program started
	graph->Interp=InterpData[dnaISpline].Interp;
	// Since all of the paramters were good and no error occured we can
	// tell the rest of the program that (graph) is ready to be passed
	// to the evaluator now.
	graph->InterpReadyQ=dnaTrue;
Error:	
	// If the err value is not zero then we found a problem and
	// return a negative value (-1) to indicate an error
	// If the err value is zero then return zero
	return (err?-1:0);
}


double funcPRamp(double t, dnaARFGraphPointer graph)
{
	double d;
	// GOTCHA : Hand code the offsets 
	if (t<graph->P[6])
		return graph->P[7];
	else
		if (graph->P[8]<t)
			return graph->P[9];
		else {
			d = (graph->P[9]-graph->P[7])/(graph->P[8]-graph->P[6]);
			return d*(t-graph->P[6])+graph->P[7];
		}
	// oooops....do something
	// return graph->P[9];
}

int funcPRampFill (dnaARFGraphPointer graph, short *data,
	int delta, int last, double timebase, int board, int channel)
{
	double t,min,max;					// time, min & max voltage
	double d,slope;						// voltage, slope of line segment
	double val,valInc;
	int err=0,i;						// position in data
	short s;							// digitized value
	int j,n;							// j is looping, j<n
	int N;
// USERPIN
	N=GetIntFillNumber(0,last,delta);
	errChk(GetYMin(graph,&min));	// look up min
	errChk(GetYMax(graph,&max));   // look up max
	t=0.0;
	i=0;
	// First flat region
	if (t<graph->P[6]) {
		n=GetDoubleFillNumber(t,graph->P[6],timebase);
		if (n>N) n=N;
		d=Pin(graph->P[7],min,max);		// coerce to prevent crashes
		errChk(AO_VScale (board, channel, d, &s));	// digitize
		for(j=0;j<n;j++) {
			// t=j*timebase in this loop
			data[i]=s;
			i+=delta;
		}
		if (i>=last) return 0;
		t+=n*timebase;
	}
	// Now t>graph->P[6]
	slope=(graph->P[9]-graph->P[7])/(graph->P[8]-graph->P[6]);
	valInc=slope*timebase;
	val=slope*(t-graph->P[6])+graph->P[7];
	while ((i<last) && (t<graph->P[8])) {
		d=Pin(val,min,max);					// coerce to prevent crashes
//		errChk(AO_VScale (board, channel, d, &s));	// digitize
		AO_VScale (board, channel, d, &s);	// digitize
		data[i]=s;
		i+=delta;
		t+=timebase;
		val+=valInc;
	}
	d=graph->P[9];						// hold final value at late times
	d=Pin(d,min,max);					// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));
	for (;i<last;i+=delta) {
		data[i]=s;
	}
	return 0;
Error:
	return -1;
}

int funcPRampSetup(dnaARFGraphPointer graph)
{
	int err=0;
	int n,m;

	graph->InterpReadyQ=dnaFalse;		// Assume the worst

	if (dnaPRamp !=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	
	n=dnaReservedParams;
	m=dnaReservedParams+InterpData[dnaPRamp].NumParams;
	// Arrange in Ascending order....
	if (graph->P[0+n]<=graph->P[2+n]) {
		graph->P[0+m]=graph->P[0+n];	// Time A
		graph->P[1+m]=graph->P[1+n]*graph->P[0]+graph->P[1]; // Volt A
		graph->P[2+m]=graph->P[2+n];	// Time B
		graph->P[3+m]=graph->P[3+n]*graph->P[0]+graph->P[1]; // Volt B
	}
	else {
		graph->P[2+m]=graph->P[0+n];	// Time A
		graph->P[3+m]=graph->P[1+n]*graph->P[0]+graph->P[1]; // Volt A
		graph->P[0+m]=graph->P[2+n];	// Time B
		graph->P[1+m]=graph->P[3+n]*graph->P[0]+graph->P[1]; // Volt B
	}
	graph->Interp=InterpData[dnaPRamp].Interp;
	graph->InterpReadyQ=dnaTrue;		// Whew

Error:
	return (err?-1:0);
}


double funcPPulse(double t, dnaARFGraphPointer graph)
{
	double d;
	// GOTCHA : Hand code the offsets 
	if (t<graph->P[6])
		return graph->P[9];
	else
		if (graph->P[8]<t)
			return graph->P[9];
		else {
			return graph->P[7];
		}
	// oooops....do something
	// return graph->P[9];
}

int funcPPulseFill (dnaARFGraphPointer graph, short *data,
	int delta, int last, double timebase, int board, int channel)
{
	double t,min,max;					// time, min & max voltage
	double d;							// voltage
	int err=0,i;						// position in data
	short s;							// digitized value
	int j,n;							// j is looping, j<n
	int N;
// USERPIN 
	N=GetIntFillNumber(0,last,delta);
	errChk(GetYMin(graph,&min));	// look up min
	errChk(GetYMax(graph,&max));   // look up max
	t=0.0;
	i=0;
	// First flat region
	if (i>=last) return 0;
	if (t<graph->P[6]) {
		n=GetDoubleFillNumber(t,graph->P[6],timebase);
		if (n>N) n=N;
		d=Pin(graph->P[9],min,max);		// coerce to prevent crashes
		errChk(AO_VScale (board, channel, d, &s));	// digitize
		for(j=0;j<n;j++) {
			data[i]=s;
			i+=delta;
		}
		if (i>=last) return 0;
		t+=n*timebase;
		N-=n;
	}
	// Now t>graph->P[6]
	if (t<graph->P[8]) {
		n=GetDoubleFillNumber(t,graph->P[8],timebase);
		if (n>N) n=N;
		d=Pin(graph->P[7],min,max);		// coerce to prevent crashes
		errChk(AO_VScale (board, channel, d, &s));	// digitize
		for(j=0;j<n;j++) {
			data[i]=s;
			i+=delta;
		}
		if (i>=last) return 0;
		t+=n*timebase;
		N-=n;
	}
	d=Pin(graph->P[9],min,max);			// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));
	for (;i<last;i+=delta) {
		data[i]=s;
	}
	return 0;
Error:
	return -1;
}

int funcPPulseSetup(dnaARFGraphPointer graph)
{
	int err=0;
	int n,m;

	graph->InterpReadyQ=dnaFalse;		// Assume the worst

	if (dnaPPulse !=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	
	n=dnaReservedParams;
	m=dnaReservedParams+InterpData[dnaPRamp].NumParams;

	// Arrange in Ascending order....
	if (graph->P[0+n]<=graph->P[2+n]) {
		graph->P[0+m]=graph->P[0+n];	// Time A
		graph->P[1+m]=graph->P[1+n]*graph->P[0]+graph->P[1]; // Volt A
		graph->P[2+m]=graph->P[2+n];	// Time B
		graph->P[3+m]=graph->P[3+n]*graph->P[0]+graph->P[1]; // Volt B
	}
	else {
		// DO NOT Reverse meaning of voltages, just times 
		graph->P[2+m]=graph->P[0+n];	// Time A
		graph->P[1+m]=graph->P[1+n]*graph->P[0]+graph->P[1]; // Volt A
		graph->P[0+m]=graph->P[2+n];	// Time B
		graph->P[3+m]=graph->P[3+n]*graph->P[0]+graph->P[1]; // Volt B
	}

	graph->Interp=InterpData[dnaPPulse].Interp;
	graph->InterpReadyQ=dnaTrue;		// Whew

Error:
	return (err?-1:0);
}


double funcPSquare(double t, dnaARFGraphPointer graph)
{
	// GOTCHA : Hand code the offsets
	// 6 : Duration A in msec
	// 7 : Voltage A
	// 8 : Duration A+B in msec
	// 9 : Voltage B
	// Behavior : 

	if (t<0)
		return graph->P[7];				// pre-hold at voltage A
	if (t>graph->Duration)
		t=(graph->Duration);			// post-hold at value at end time
	t=fmod(t,graph->P[8]);
	if (t<graph->P[6])
		return graph->P[7];
	else
		return graph->P[9];
	// oooops....do something
	// return graph->P[9];
}

int funcPSquareFill (dnaARFGraphPointer graph, short *data,
	int delta, int last, double timebase, int board, int channel)
{
	double t,min,max;					// time, min & max voltage
	double d;							// voltage
	int err=0,i;						// position in data
	short s;							// digitized value
	int j,n,m;							// j is looping.
	int N,M;
// USERPIN
	N=GetIntFillNumber(i,last,delta);
	M=GetDoubleFillNumber(0,graph->Duration,timebase);
	errChk(GetYMin(graph,&min));			// look up min
	errChk(GetYMax(graph,&max));   		// look up max
	
	t=0.0;
	i=0;
	// First flat region
	while ((i<last) && (0<M)) {
		if (t<graph->P[6]) {
			n=GetDoubleFillNumber(t,graph->P[6],timebase);
			if (n>N) n=N;
			if (n>M) n=M;
			d=Pin(graph->P[7],min,max);		// coerce to prevent crashes
			errChk(AO_VScale (board, channel, d, &s));	// digitize
			for(j=0;j<n;j++) {
				data[i]=s;
				i+=delta;
			}
			if (i>=last) return 0;
			t+=n*timebase;
			N-=n;
			M-=n;
		}
		if ((i<last) && (0<M) && (t<graph->P[8])) {
			n=GetDoubleFillNumber(t,graph->P[8],timebase);
			if (n>N) n=N;
			if (n>M) n=M;
			d=Pin(graph->P[9],min,max);		// coerce to prevent crashes
			errChk(AO_VScale (board, channel, d, &s));	// digitize
			for(j=0;j<n;j++) {
				data[i]=s;
				i+=delta;
			}
			if (i>=last) return 0;
			t+=n*timebase;
			N-=n;
			M-=n;
		}
		t=fmod(t,graph->P[8]);
	}
	d=Pin(graph->P[9],min,max);				// coerce to prevent crashes
	errChk(AO_VScale (board, channel, d, &s));
	for (;i<last;i+=delta) {
		data[i]=s;
	}
	return 0;
Error:
	return -1;
}

int funcPSquareSetup(dnaARFGraphPointer graph)
{
	int err=0;
	int n,m;
	double d;

	graph->InterpReadyQ=dnaFalse;		// Assume the worst

	if (dnaPSquare!=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	
	n=dnaReservedParams;				// 2
	m=dnaReservedParams+InterpData[dnaPRamp].NumParams; // 2+4=6

	d=(graph->Duration)/(graph->P[0+n]+graph->P[2+n]);

	graph->P[0+m]=graph->P[0+n];	// Duration A
	graph->P[1+m]=graph->P[1+n]*graph->P[0]+graph->P[1]; // Volt A, scaled
	graph->P[2+m]=graph->P[0+n]+graph->P[2+n];	// Duration A+B
	graph->P[3+m]=graph->P[3+n]*graph->P[0]+graph->P[1]; // Volt B, scaled

	graph->Interp=InterpData[dnaPSquare].Interp;
	graph->InterpReadyQ=dnaTrue;		// Whew

Error:
	return (err?-1:0);
}


double funcPSine(double t, dnaARFGraphPointer graph)
{
	double d;
	// GOTCHA : Hand code the offsets 

	return graph->P[0]*sin(t*graph->P[6]+graph->P[7])+graph->P[1];
}

// funcPSineFill would not be a big performance enhancement
// Thus it is not included

int funcPSineSetup(dnaARFGraphPointer graph)
{
	int err=0;
	int n,m;

	graph->InterpReadyQ=dnaFalse;		// Assume the worst

	if (dnaPSine !=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	
	graph->P[6]=graph->P[2]*TwoPi();
	graph->P[7]=graph->P[3]/180.0*Pi();

	graph->Interp=InterpData[dnaPSine].Interp;
	graph->InterpReadyQ=dnaTrue;		// Whew
Error:
	return (err?-1:0);
}

double funcPExp(double t, dnaARFGraphPointer graph)
{
	// GOTCHA : Hand code the offsets 
	// Prevent over and underflow (exp of 230 is about 10 to the 100 power)
	// brainMaxDouble is 1e100, so this is application consistant
	if (t*graph->P[6]>230.0) 
		return graph->P[7]*exp(230.0)+graph->P[1];
	else
		if (t*graph->P[6]<-230.0) 
		return graph->P[7]*exp(-230.0)+graph->P[1];
	
	return graph->P[7]*exp(t*graph->P[6])+graph->P[1];
}

// funcPExpFill would not be a big performance enhancement
// Thus it is not included

int funcPExpSetup(dnaARFGraphPointer graph)
{
	int err=0;
	int n,m;

	graph->InterpReadyQ=dnaFalse;		// Assume the worst

	if (dnaPExp !=graph->InterpType) {	// Houston, we have a problem
		err=-1;
		goto Error;
	}
	// P[0] is the coefficient in front of the exponential in, e.g. volts
	// P[1] is the offset added to function in, e.g. volts
	// P[2] is base of exponential, 0.0 selects base e
	// P[3] is the clock offset and is added to the time in milliseconds
	// P[4] is the time constant in 1/msec = kHz
	if (0.0==graph->P[2]) {			// Use Base e
		graph->P[6]=graph->P[4];	// Time factor (kHz)
	}
	else {	// absorb base of exponential into new time factor to use with e
		graph->P[6]=graph->P[4]*log(graph->P[2]);
	}
	// Set the Amplitude factor (V) 
	graph->P[7]=graph->P[0]*exp(graph->P[3]*graph->P[6]);
	
	graph->Interp=InterpData[dnaPExp].Interp;
	graph->InterpReadyQ=dnaTrue;		// Whew
Error:
	return (err?-1:0);
}

// 
//	parameters are selected for ease when calling from enableGraph.
// 	This builds up for an interpolation for use in a graph. 
//	old interpolation data is discarded, the Duration is taken
//	from group so group info is no longer needed. 
//
/* This functions return -1 on error, 0 if success and no sorting done, 
 * +1 if success and sorting was done 
 */
int MakeInterp(dnaARFGroup *group, dnaARFGraph *graph, int NumInterps)
{
	int err=0;
	int i,dummy,SortQ;
	double t,step;
	
// Very complicated function 

	// verify parameters 
	nullChk(graph);
	if(graph->InterpReadyQ) {
		// we need to blow away the old data 
		// this probably already should have been done ! 
		graph->InterpReadyQ = dnaFalse;	// Invalidate it ASAP.
		errChk(dna_TouchARFGraph (graph));	// free old memory blocks
	}
	nullChk(group);
	if (0>=NumInterps) {
		err=-1;
		goto Error;
	}
	if (1==NumInterps)					// we divide by NumInterps-1 later....
		NumInterps=2;


// Our tasks here:
//	1) get time duration info from group and store in graph
//	2) create and scale the X Y values since this is universal (if they exist)
//	3) allocate NumIterps space
//	4) perform interpolation method specific initialization
//	5) loop and call our function to get the job done


	graph->Duration = (group->TicksD * group->TimeUnitD)/1000.0;
	// Correct to Last Time for RF graphs. GOTCHA
	if (dnaAnalogChannels<=graph->RangeIndex)
		graph->Duration-=(group->Ticks * group->TimeUnit)/1000.0;
	
	// Need to call these in the right order 
	errChk(dna_CalcXYValuesMinMax (graph, 3));	// 2) "3"=both X and Y
	SortQ=SortValues(graph,&dummy);			// may return +1 if work done
	negChk(SortQ);
	errChk(dna_ScaleGraphValues (graph));		// 2)
	
	// 3) begins
	graph->NumberOfInterps = NumInterps;
// ALLOCATION free  // Should have been done by Touch before
	graph->InterpMemoryUsed=0;
	if (graph->InterpXValues)
		free(graph->InterpXValues);
	graph->InterpXValues = 0;
	if (graph->InterpYValues)
		free(graph->InterpYValues);
	graph->InterpYValues = 0;

	if (0==graph->NumberOfInterps) {
		// Nothing to do 
		err=0;
		goto Error;
	}

// ALLOCATION 
	graph->InterpMemoryUsed=NumInterps*sizeof(double);
	graph->InterpXValues = calloc(NumInterps,sizeof(double));
	nullChk(graph->InterpXValues);
	graph->InterpYValues = calloc(NumInterps,sizeof(double));
	nullChk(graph->InterpYValues);
	// 3) is done

	// 4) is a function pointer call 
	
	// The next line needs to go somewhere...should also be in ISetup call
	graph->Interp=InterpData[dnaNoInterp].Interp;

	// Now call specific hairy detail stuff 
	errChk(InterpData[graph->InterpType].ISetup(graph));	// easy
	
	// ISetup should  have set this true....double check 
	if (!graph->InterpReadyQ) {
		err=0;
		goto Error;
	}
	// Now there is no chance of error or failure 
	
	// 5) is real loop here 
	
	step = graph->Duration/(graph->NumberOfInterps-1);
	for (i=0, t=0.0; i<graph->NumberOfInterps; i++, t+=step) {
		graph->InterpXValues[i] = t;
		graph->InterpYValues[i] = graph->Interp(t,graph);
	}

Error:
	return (err?-1:SortQ);
}

		  
/* This function is run on startup to fill the InterpData array with info
on each type of interpolation method */		  
int SetupInterpData(void)
{	// The dnaISpline sectin below has comments to aid in adding new types
	int err=0;
	dnaInterpTypes i;
	int j;

	//InterData[i].NumParams should be<=(dnaNumUIParams-dnaReservedParams)=4

//ALLOCATION call for each label 	
	i=dnaNoInterp;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Unknown");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaFalse;
	InterpData[i].NumParams = 0;
	for (j=0; j<dnaReservedParams; j++) {
// ALLOCATION etc. 
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcNoInterpSetup;
	InterpData[i].IFillShort = 0;
	InterpData[i].Interp = funcNoInterp;
	
	i=dnaILinear;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Linear");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaTrue;
	InterpData[i].NumParams = 0;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcILinearSetup;
	InterpData[i].IFillShort = funcILinearFill;
	InterpData[i].Interp = funcILinear;

	i=dnaIStep;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Steps");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaTrue;
	InterpData[i].NumParams = 0;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcIStepSetup;
	InterpData[i].IFillShort = funcIStepFill;
	InterpData[i].Interp = funcIStep;
	
	i=dnaIPoly;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Polynomial");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaTrue;
	InterpData[i].NumParams = 0;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcIPolySetup;
	InterpData[i].IFillShort = 0;
	InterpData[i].Interp = funcIPoly;

	i=dnaIRat;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Rational Poly");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaTrue;
	InterpData[i].NumParams = 0;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcIRatSetup;
	InterpData[i].IFillShort = 0;
	InterpData[i].Interp = funcIRat;

	i=dnaISpline;						// Put enumerated constant name here
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Cubic Spline");	// Name on panel
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaTrue;	// whether to enable x,y pairs
	InterpData[i].NumParams = 2;		// 0,1,2,3, or 4 extra parameters
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	// Name of first parameter
	InterpData[i].ParamLabel  [0+dnaReservedParams]=StrDup("Initial Slope");
	// Minimum value of first parameter
	InterpData[i].MinimumValue[0+dnaReservedParams]=brainMinDouble;
	// Name of second parameter
	InterpData[i].ParamLabel  [1+dnaReservedParams]=StrDup("Final Slope");
	// Minumum value of second parameter
	InterpData[i].MinimumValue[1+dnaReservedParams]=brainMinDouble;
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcISplineSetup;	// Name of setup function
	InterpData[i].IFillShort = 0;
	InterpData[i].Interp = funcISpline;	// Name of evaluation function

	i=dnaPRamp;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Linear Ramp");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaFalse;
	InterpData[i].NumParams = 4;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ParamLabel  [0+dnaReservedParams]=StrDup("Time A (msec)");
	InterpData[i].MinimumValue[0+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel  [1+dnaReservedParams]=StrDup("Voltage A");
	InterpData[i].MinimumValue[1+dnaReservedParams]=brainMinDouble;
	InterpData[i].ParamLabel  [2+dnaReservedParams]=StrDup("Time B (msec)");
	InterpData[i].MinimumValue[2+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel  [3+dnaReservedParams]=StrDup("Voltage B");
	InterpData[i].MinimumValue[3+dnaReservedParams]=brainMinDouble;

	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcPRampSetup;
	InterpData[i].IFillShort = funcPRampFill;
	InterpData[i].Interp = funcPRamp;
	
	i=dnaPPulse;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Single Pulse");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaFalse;
	InterpData[i].NumParams = 4;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ParamLabel  [0+dnaReservedParams]=StrDup("Time A (msec)");
	InterpData[i].MinimumValue[0+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel  [1+dnaReservedParams]=StrDup("Middle Voltage");
	InterpData[i].MinimumValue[1+dnaReservedParams]=brainMinDouble;
	InterpData[i].ParamLabel  [2+dnaReservedParams]=StrDup("Time B (msec)");
	InterpData[i].MinimumValue[2+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel  [3+dnaReservedParams]=StrDup("Outside Voltage");
	InterpData[i].MinimumValue[3+dnaReservedParams]=brainMinDouble;
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcPPulseSetup;
	InterpData[i].IFillShort = funcPPulseFill;
	InterpData[i].Interp = funcPPulse;

	i=dnaPSquare;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Square Wave");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaFalse;
	InterpData[i].NumParams = 4;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ParamLabel  [0+dnaReservedParams]=StrDup("Duration A (msec)");
	InterpData[i].MinimumValue[0+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel  [1+dnaReservedParams]=StrDup("Voltage A");
	InterpData[i].MinimumValue[1+dnaReservedParams]=brainMinDouble;
	InterpData[i].ParamLabel  [2+dnaReservedParams]=StrDup("Duration B (msec)");
	InterpData[i].MinimumValue[2+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel  [3+dnaReservedParams]=StrDup("Voltage B");
	InterpData[i].MinimumValue[3+dnaReservedParams]=brainMinDouble;
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcPSquareSetup;
	InterpData[i].IFillShort = funcPSquareFill;
	InterpData[i].Interp = funcPSquare;

	i=dnaPSine;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Sinusoidal");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaFalse;
	InterpData[i].NumParams = 2;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ParamLabel[0+dnaReservedParams]=StrDup("Frequency (kHz)");
	InterpData[i].MinimumValue[0+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel[1+dnaReservedParams]=StrDup("Phase (degrees)");
	InterpData[i].MinimumValue[1+dnaReservedParams]=brainMinDouble;
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcPSineSetup;
	InterpData[i].IFillShort = 0;
	InterpData[i].Interp = funcPSine;

	i=dnaPExp;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Exponential");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaFalse;
	InterpData[i].NumParams = 3;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ParamLabel  [0+dnaReservedParams]=StrDup("Base (0.000 is e)");
	InterpData[i].MinimumValue[0+dnaReservedParams]=0.0;
	InterpData[i].ParamLabel  [1+dnaReservedParams]=StrDup("Time Offset (msec)");
	InterpData[i].MinimumValue[1+dnaReservedParams]=brainMinDouble;
 	InterpData[i].ParamLabel  [2+dnaReservedParams]=StrDup("Time Constant (kHz)");
	InterpData[i].MinimumValue[2+dnaReservedParams]=brainMinDouble;
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcPExpSetup;
	InterpData[i].IFillShort = 0;
	InterpData[i].Interp = funcPExp;

	i=dnaPSigmoid;
	InterpData[i].ID = i;
	InterpData[i].Label = StrDup("Sigmoidal");
	nullChk(InterpData[i].Label);
	InterpData[i].ValuesEnabledQ = dnaFalse;
	InterpData[i].NumParams = 0;
	for (j=0; j<dnaReservedParams; j++) {
		InterpData[i].ParamLabel[j]=StrDup(brainReservedLabels[j]);
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	for (j=InterpData[i].NumParams+dnaReservedParams; j<dnaNumParams; j++) {
		InterpData[i].ParamLabel[j]=0;
		InterpData[i].MinimumValue[j]=brainMinDouble;
	}
	InterpData[i].ISetup = funcNoInterpSetup;
	InterpData[i].IFillShort = 0;
	InterpData[i].Interp = funcNoInterp;
	
	return 0;
Error:
	return -1;
}

//*********************************************************************
//																	   
//																	   
//   						 Turn Me On								   
//																	   
//																	   
//*********************************************************************

// TODO : move into the DNA function tree
//int SetupAuxInfo(void)
//{
//	int i;
//
//	//HEADER
//	AuxillaryInfo.FileVersion=0;
//	AuxillaryInfo.FileS=0;
//	AuxillaryInfo.ClassS=0;
//	AuxillaryInfo.Index=-1;
//	AuxillaryInfo.DateS=0;
//	AuxillaryInfo.TimeS=0;
//	AuxillaryInfo.CommentS=StrDup("User Comment Area");
//	AuxillaryInfo.AutoCommentS=StrDup("Automated Comment");
//	
//	//CLUSTER
//	AuxillaryInfo.GlobalOffset=0;
//	AuxillaryInfo.OnlyEnabledQ=dnaFalse;
//	AuxillaryInfo.RepeatRunQ=dnaFalse;
//	AuxillaryInfo.CameraTriggerQ=dnaFalse;
//	AuxillaryInfo.UserTicks=100;
//	AuxillaryInfo.UserTimeUnit=1;
//	AuxillaryInfo.GTS=0;
//	
//	//ANALOG/RF
//	AuxillaryInfo.AnalogIndex=0;
//	AuxillaryInfo.RFIndex=0;
//	AuxillaryInfo.GraphSelected=-1;
//	AuxillaryInfo.PinRangeQ=dnaTrue;
//	// range
//	for (i=0; i<dnaAnalogUnipolar; i++)
//		AuxillaryInfo.Range[i]=dnaUnipolar;
//	for (;i<dnaAnalogChannels; i++)
//		AuxillaryInfo.Range[i]=dnaBipolar;
//	// GOTCHA, setup 10 and 11 as the RF controls
//	AuxillaryInfo.Range[i]=dnaAmplitude;
//	AuxillaryInfo.Range[i+1]=dnaFrequency;
//	// minimum and maximum
//	for (i=0; i<faceGRAPHS; i++) {
//		AuxillaryInfo.UserMin[i]=RangeMin[AuxillaryInfo.Range[i]];
//		AuxillaryInfo.UserMax[i]=RangeMax[AuxillaryInfo.Range[i]];
//	}
//	return 0;
//}
//
/* Old, before RangeIndex
int SetRanges(void)
// called from TurnMeOn only.
// adjusts the allowed output hardware range settings (card jumpers)
{
	int i,j;
// USERPIN
	// GOTCHA : very ugly code to set bipolar mode
	// see also HAND.c hand_ResetAll and hand_ResetAnalog
	// see also BRAIN.c SetAnalogIdle
	for (i=0; i<brainAGroups; i++) {
		for (j=0; j<dnaAnalogUnipolar; j++) {
			AGroups.ARFGroups[i].ARFGraphs[j].Range=dnaUnipolar;
		}
		for (; j<dnaAnalogChannels; j++) {
			AGroups.ARFGroups[i].ARFGraphs[j].Range=dnaBipolar;
		}
	}
	for (i=0; i<brainRFGroups; i++) {
		RFGroups.ARFGroups[i].ARFGraphs[0].Range=dnaAmplitude;
		RFGroups.ARFGroups[i].ARFGraphs[1].Range=dnaFrequency;
	}
	return 0;
}
*/

int SetRanges(void)
// called from TurnMeOn only.
// adjusts the allowed output hardware range settings (card jumpers)
{
	int i,j;
// USERPIN
	// GOTCHA : very ugly code to set bipolar mode
	// see also HAND.c hand_ResetAll and hand_ResetAnalog
	// see also BRAIN.c SetAnalogIdle
	for (i=0; i<brainAGroups; i++) {
		for (j=0; j<dnaAnalogChannels; j++) {
			AGroups.ARFGroups[i].ARFGraphs[j].RangeIndex=j;
		}
	}
	for (i=0; i<brainRFGroups; i++) {
		RFGroups.ARFGroups[i].ARFGraphs[0].RangeIndex=dnaAnalogChannels;
		RFGroups.ARFGroups[i].ARFGraphs[1].RangeIndex=dnaAnalogChannels+1;
	}
	return 0;
}

int SetDefaults(void)
{
	int i,j;
	
	for (i=0; i<brainRFGroups; i++) {
		RFGroups.ARFGroups[i].Ticks=60;
		RFGroups.ARFGroups[i].TimeUnit=dnaMSEC;
		RFGroups.ARFGroups[i].TicksD=1;
		RFGroups.ARFGroups[i].TimeUnitD=dnaSEC;
	}
	return 0;
}

int TurnMeOn(void)						// Called by FACE main after BuildControls
{
	// A lot of GOTCHA numbers are fed in here 
	int i,j;
	handErrRet err=0;					// short data type for daq use

	// AuxillaryInfo is not maintined by DNA.c, so we do it in BRAIN.c

	errChk(SetupInterpData());			// messy, put in sub-function
// DNA called to zero the data structures	
	errChk(dna_ResetInitIdle (&InitIdle));
// ALLOCATION call
	errChk(dna_InitAuxInfo (&AuxillaryInfo));	// erases AuxillaryInfo
// ALLOCATION call 
	errChk(dna_InitClusterArray (&Clusters, brainClusters, brainOutLines/8));	
// ALLOCATION call 
	errChk(dna_InitARFGroupArray (&AGroups, brainAGroups, dnaAnalogChannels));
// ALLOCATION call 
	errChk(dna_InitARFGroupArray (&RFGroups, brainRFGroups, 2));

	// This will set which range to apply to each Analog/RF output 
	errChk(SetRanges()); 	// loops through all graphs in system
	errChk(SetDefaults());  // Alter's defaluts for RF group timing

	errChk(faceSetNumberOfLines());		// Digital lines
	errChk(faceSetNumberOfClusters());	// Digital words
	errChk(faceSetOutputLabels());		// Digital line labels
	errChk(faceSetIdleValues());		// Digital Idle
	errChk(faceSetInterpTypes());		// ARF Interp combo box
	errChk(faceSetNumberOfAGroups ());	// ARF Analog groups	
	errChk(faceSetNumberOfRFGroups ());	// ARF RF groups
 	errChk(faceSetInitAnalog());		// ARF Analog idle
	errChk(faceSetARFRanges(-1));		// ARF User Min and Max
	
	if (ExperimentEnabled) {
 // Harware setup need to happen before idle output is setup 
		errChk(hand_ResetAll());		// This should put HAND in default state
		errChk(mouth_InitRF ());		// This will start access of DS345
		errChk(hand_ConfigAuxPorts ()); // On Analog Card
	};
	
	errChk(dna_InitBufferDigital (&dBuf1));	// Zero out the buffer
	errChk(dna_InitBufferDigital (&dBuf2));	// Zero out the buffer
	errChk(dna_InitBufferAnalog (&aBuf));
	
	return 0;
Error: 
	return -1;	
}

//*********************************************************************
//																	   
//																	   
//   						 Idle Control							   
//																	   
//																	   
//*********************************************************************

int OldSetIdleOutput(dnaByte setIdle)
{   
	int i;
	handErrRet err=0,max;
	// Note that this will eliminate any previous input port configuration
	require(0!=RunDone);
	if(setIdle){
		errChk(hand_ConfigDIOPorts (8*Clusters.NumberOfPorts, 0));
		max=8*Clusters.NumberOfPorts;
		if (max>dnaDigitalOutputs)
			max=dnaDigitalOutputs;		
		// sets the output to these values
		// Note: set serial, not in parallel
		// TODO : do in parallel !
		for (i=0; i<8*Clusters.NumberOfPorts;i++)
			errChk(hand_SetLine(i,InitIdle.IdleValues[i]));
	} 
	else {								// resets to zero on all ports
		errChk(hand_ConfigDIOPorts (8*Clusters.NumberOfPorts, 0));
		for (i=0; i<8*Clusters.NumberOfPorts;i++)
			errChk(hand_SetLine(i,0));
	}

Error:
	return (err?-1:0);
}

// DONE : This cannot handle > 4 ports!  (fixed 2000/03/28)
// APC / CEK modified to handle two cards  
int SetIdleOutput(dnaByte setIdle)
{   
	int i,card1ports;
	unsigned int A,B,C;
	handErrRet err=0,max;
	// Note that this will eliminate any previous input port configuration
	require(0!=RunDone);
	// Ensure everything is ready for output
	/* TODO FIXME : Why call hand_ConfigDIO here? hand_OutWord already handles this! */
	errChk(hand_ConfigDIO (Clusters.NumberOfPorts*8, 0));
	if (setIdle) {
		// Loop to build the output word for first card
		A=0;
		C=1;
		card1ports=Clusters.NumberOfPorts;
		if (4<card1ports)
			card1ports=4;
		for (i=0; i<8*card1ports;i++) {
			if (InitIdle.IdleValues[i])
				A=A+C;
			C*=2;
		}
		// Loop to build the output word for second card
		B=0;
		C=1;
		for (; i<8*Clusters.NumberOfPorts;i++) {
			if (InitIdle.IdleValues[i])
				B=B+C;
			C*=2;
		}
	}
	else {
		A=0;
		B=0;
	}
	errChk(hand_OutWord (A,B));
	
Error:
	return (err?-1:0);
}

int SetIdle(int line, dnaByte value)
{
	handErrRet err=0;
	if (0==RunDone) {
		errChk(MessagePopup ("Mode Error", 
				"You cannot use Idle while running"));
		goto Error;		
	}
	require(0<=line);
	require(line < dnaDigitalOutputs);
	InitIdle.IdleValues[line]=value;
Error:
	return (err?-1:0);
}

int SetIdleWord(int index)
{
	int i,err=0;
	if (0==RunDone) {
		errChk(MessagePopup ("Mode Error", 
				"You cannot use Idle while running"));
		goto Error;		
	}
	require(0<=index);
	require(index<Clusters.NumberOfClusters);
	
	errChk(hand_ConfigDIOPorts (8*Clusters.NumberOfPorts, 0));
	for (i=0; i<8*Clusters.NumberOfPorts;i++) {
		errChk(hand_SetLine(i,Clusters.Clusters[index].Digital[i]));
	}

Error:
	return (err?-1:0);
}

int SetAnalogIdle(void)
{
	int err=0;
	int i;
	double d;
	if (0==RunDone) {
		errChk(MessagePopup ("Mode Error", 
				"You cannot use Idle while running"));
		goto Error;		
	}
 	errChk(hand_ConfigAnalogIdle ());
// USERPIN
// GOTCHA : see also BRAIN.c SetRanges and HAND.c hand_ResetAll and
// hand_Reset Analog
	for (i=0; i<dnaAnalogChannels; i++) {	// GOTCHA
		if (i<dnaAnalogUnipolar)
			d=Pin(InitIdle.InitAnalog[i],RangeMin[dnaUnipolar],RangeMax[dnaUnipolar]);
		else
			d=Pin(InitIdle.InitAnalog[i],RangeMin[dnaBipolar],RangeMax[dnaBipolar]);
		errChk(hand_SetAnalogOutput (i, d));
	}
Error:
	return (err?-1:0);
}

int SetIdleRF(double Amp, double DC, double Freq)
// Added error recovery code, so user can turn SRS on 
// and acquire it by pressing "Idle RF" button
{
	int err=0;
	char * command=0;
	if (0==RunDone) {
		errChk(MessagePopup ("Mode Error", 
				"You cannot use Idle while running"));
		goto Error;		
	}
	errChk(mouth_MakeIdleCommand (&command, Amp, DC, Freq));
	if (command) {
//		errChk(mouth_InitRF ());
		err=(mouth_WriteDS345 (command, strlen(command)));
		if(err) {
			printf("Error (%i) in SetIdleRF from mout_WriteDS345, trying to mouth_InitRF\n",err);
			err=0;
			err=mouth_InitRF();
			if(err)
				printf("Attempt to mouth_InitRF failed with err=(%i)\n",err);
			else
				printf("Attempt to mouth_InitRF succeeded, try to idle again?\n",err);
			errChk(-1);	// Still fail
		}
	}
Error:
	return (err?-1:0);
}

//*********************************************************************
//																	   
//																	   
//   						 Sort Values							   
//																	   
//																	   
//*********************************************************************
// 1 if sorting happened , 0 if not, -1 if error 
int SortValues(dnaARFGraph *graph, int *index)
{
	int err=0;
	int swap;
	int swapflag;
	int i,j,max;
	double temp;
	
	nullChk(graph);
	max=graph->NumberOfValues;
	if(0>=max) { 						// some kind of error? nvermind
		return 0;
	}
	
//	Wow..32 values. Just use a bubble sort.  Nice and controlled
//	behavior for pre-sorted and identical values or for a single swap 

	swapflag=0;
	for (i=0,swap=1;swap && (i<max-1);i++) {
		swap=0;
		for(j=0; j<max-1-i; j++) {
			if (graph->XValues[j]>graph->XValues[j+1]) {
				swap=1;
				swapflag=1;
				if (j==(*index))
					(*index)=j+1;
				else
					if (j+1==(*index))
						(*index)=j;
				temp=graph->XValues[j];
				graph->XValues[j]=graph->XValues[j+1];
				graph->XValues[j+1]=temp;
				temp=graph->YValues[j];
				graph->YValues[j]=graph->YValues[j+1];
				graph->YValues[j+1]=temp;
			}
		}
	}
	return (swapflag?1:0);
Error:
	return -1;
}

//*********************************************************************
//   						 PrepInterp							   
//*********************************************************************
// graphOut is an output parameter 
// Afterwards, graphOut knows its own correct Interp function 
int PrepInterp(dnaARFGroupArray *groupArray, int IGroup, int IGraph,
		dnaARFGraph **graphOut)
{
	int err=0;
	int i,dummy=-1;							// Looping
	dnaARFGroup *group;
	dnaARFGraph *graph;
	
	(*graphOut)=0;						// Assume the worst
	nullChk(groupArray);
	if (groupArray->EnabledQ[IGraph]) {
		group=&groupArray->ARFGroups[IGroup];
		nullChk(group);
		graph=&group->ARFGraphs[IGraph];
		nullChk(graph);
		errChk(dna_TouchARFGraph (graph));
		graph->Duration=(group->TicksD * group->TimeUnitD)/1000.0;
		// Do this to adjust scaling of RF times. GOTCHA
		if (dnaAnalogChannels<=graph->RangeIndex)
			graph->Duration-=(group->Ticks * group->TimeUnit)/1000.0;
		errChk(dna_CalcXYValuesMinMax (graph, 3));	

		negChk(SortValues (graph,&dummy));

		errChk(dna_ScaleGraphValues (graph));		
		// Next should also be in ISetup call
		graph->Interp=InterpData[dnaNoInterp].Interp;
		// Now call specific hairy detail stuff 
		errChk(InterpData[graph->InterpType].ISetup(graph));
		if (!graph->InterpReadyQ) {
			err=-1;
			goto Error;
		}
		else {
			(*graphOut)=graph;			// Is ready to go !!!
		}		
	}
// TODO DEBUG : added else only as a debug aid, remove later
	else
		printf("Graph %i : %i Not Enabled Error in PrepInterp",IGroup,IGraph);
	nullChk(*graphOut);
Error:
	return (err?-1:0);
}

int GCF(int A, int B)  // Note: If A=0, R=0, Return B;
{
	int R;
	if (B) {
		while(R=A%B) {
			A=B;
			B=R;
		}
		return B;
	}
	else {
		return A;
	}
}

// returns the overall GCF and sets TD to the total duration. 
// NOTE: sets AnalogQ and RFQ =dnaTrue
int CalcResolution(int * TD)			// TD is total duration in microseconds
{
	int i;								// looping
	int G=0;							// running GCF, 0 is special start value
	int D;								// Ticks*TimeUnit duration
	dnaCluster *cluster;
	dnaARFGroup *group;	
	dnaByte AE,RFE;

	// if TD=0, just don't calculate it 
	if (TD) 
		(*TD) = 0;					// Initialize Total Duration (*TD)
	AnalogQ=dnaFalse;				// reset global flags
	RFQ=dnaFalse;
	AE=dnaFalse;					// reset local flags
	RFE=dnaFalse;
	for (i=0;(i<AGroups.NumberOfGraphs)&&(!AE);i++)
		if (AGroups.EnabledQ[i])
			AE=dnaTrue;
	for (i=0;(i<RFGroups.NumberOfGraphs)&&(!RFE);i++)
		if (RFGroups.EnabledQ[i])
			RFE=dnaTrue;
	for (i=0; i<Clusters.NumberOfClusters; i++) {
		cluster=&Clusters.Clusters[i];
		if (cluster->EnabledQ) {
			D=cluster->Ticks * cluster->TimeUnit;
			if (TD) 
				(*TD)+=D;			// increment (*TD), if exists
			G=GCF(G,D);					// G=0 initial implies G=D final
			if (AE&&(dnaARFStartGroup==cluster->AnalogSelector)) {
				AnalogQ=dnaTrue;
				group=&AGroups.ARFGroups[cluster->AnalogGroup];
				D=group->Ticks*group->TimeUnit;
				G=GCF(G,D);
				D=group->TicksD*group->TimeUnitD;
				G=GCF(G,D);
			}
			if (RFE&&(dnaARFStartGroup==cluster->RFSelector)) {
				RFQ=dnaTrue;
				group=&RFGroups.ARFGroups[cluster->RFGroup];
				D=group->Ticks*group->TimeUnit;
				G=GCF(G,D);
				D=group->TicksD*group->TimeUnitD;
				G=GCF(G,D);
			}
		}
	}
	// Now G is the greatest common value in microseconds for the timebase 
	return G;
}

//***********************************************************************
//                        ANALOG BUFFER
//***********************************************************************


// 
//	acts on aBuf, assuming Pos points to next word to be filled 
//	i will setup the graphs in parallel and fill the buffer one
//	graph at a time, taking care to interleave it.
//	Also put the final value in the FinalValueArray;
//	Then touch the graphs to unsetup them before returning
//	
//	TimeLimit is a realtive, not absolute, count in microseconds of how
//	long this group will be output.		 Valid ranges
//									9.99755859374 biplar,   -10.00244140624
//	                                9.998779296874 unipolar, -0.001220703124

/*
int AddAnalogGroupToBuffer(int IGroup, int TimeBase, int TimeLimit, 
		dnaAnalogValue *FinalValues)
{
	int err=0;
	int i,j,k;							// looping
	double t;							// time in milliseconds
	double tb;							// TimeBase in milliseconds
	double d;							// analog voltage
	double min,max;						// allowable range
	dnaAnalogValue v;					// digitized voltage
	int igraph;							// graph index;
	int start,delta,end;				// buffer indices
	dnaARFGraph *graph;					// a pointer
	dnaARFGraph *graphs[dnaAnalogChannels];	// several pointers
	brainInterp Interp;					// function pointer
	

	if (-1!=IGroup) {
		// prepare all the graphs for interpolation 	
		for (i=0;i<aBuf.NumberOfOutputs; i++) {
			igraph=aBuf.ChannelVector[i];				  
			// This will setup the graph to be ready for interpolation 
			errChk(PrepInterp(&AGroups,IGroup,igraph,&graph));
			nullChk(graph);
			graphs[i]=graph;			// save the graphs
		}
		// fill the buffer 
		delta = aBuf.NumberOfOutputs;	// delta is number of graphs
		end = aBuf.Pos+(TimeLimit/TimeBase)*delta;	// Position after end
		tb = TimeBase / 1000.0;			// convert to miliseconds
		for (i=0; i<aBuf.NumberOfOutputs; i++) {
			graph = graphs[i];			// load the graph
			igraph=aBuf.ChannelVector[i];	// setup igraph
			Interp = graph->Interp;   // look up function
			errChk(GetYMin(graph,&min));	// look up min
			errChk(GetYMax(graph,&max));   // look up max
			start = aBuf.Pos+i;			// interleaved start position
			for (j=start,t=0.0; j<end; j+=delta,t+=tb) {
				// not too swift 
				d = Interp(t,graph);	// get the voltage
				d = Pin (d, min, max);	// coerce to prevent crashes
				errChk(AO_VScale (AOBoard, igraph, d, &v.unipolar));
				aBuf.Buffer[j]=v;		// add to buffer
			}
			// FinalValues 
			t=TimeLimit/1000.0;			// make it exact
			d = Interp(t,graph);		// get the voltage
			d = Pin (d, min, max);	// coerce to prevent crashes
			// The next call is the slow step 
			errChk(AO_VScale (AOBoard, igraph, d, &v.unipolar));
			FinalValues[i]=v;
		}
		// clean up 
		for (i=0;i<aBuf.NumberOfOutputs; i++) {
			errChk(dna_TouchARFGraph(graph));
		}
		// advance fill pointer 
		aBuf.Pos=end;
	}	
	else {
		// just output final values 
		delta = aBuf.NumberOfOutputs;	// delta is number of graphs
		end = aBuf.Pos+(TimeLimit/TimeBase)*delta;	// Position after end
		for (i=0; i<aBuf.NumberOfOutputs; i++) {
			start = aBuf.Pos+i;			// interleaved start position
			v=FinalValues[i];			// look up FinalValue
			for (j=start; j<end; j+=delta) {
				aBuf.Buffer[j]=v;		// add to buffer
			}
		}
		// advance fill pointer 
		aBuf.Pos=end;
	}
Error:
	return (err?-1:0);
}
*/

int AddAnalogGroupToBuffer(int IGroup, int TimeBase, int TimeLimit, 
		dnaAnalogValue *FinalValues)
{
	int err=0;
	int i,j,k;							// looping
	double t;							// time in milliseconds
	double tb;							// TimeBase in milliseconds
	double d;							// analog voltage
	double min,max;						// allowable range
	dnaAnalogValue v;					// digitized voltage
	int igraph;							// graph index;
	int start,delta,end;				// buffer indices
	dnaARFGraph *graph;					// a pointer
	dnaARFGraph *graphs[dnaAnalogChannels];	// several pointers
	brainInterp Interp;					// function pointer
	

	if (-1!=IGroup) {
		// prepare all the graphs for interpolation 	
		for (i=0;i<aBuf.NumberOfOutputs; i++) {
			igraph=aBuf.ChannelVector[i];				  
			// This will setup the graph to be ready for interpolation 
			errChk(PrepInterp(&AGroups,IGroup,igraph,&graph));
			nullChk(graph);
			graphs[i]=graph;			// save the graphs
		}
		// fill the buffer 
		delta = aBuf.NumberOfOutputs;	// delta is number of graphs
		end = aBuf.Pos+(TimeLimit/TimeBase)*delta;	// Position after end
		tb = TimeBase / 1000.0;			// convert to miliseconds
		for (i=0; i<aBuf.NumberOfOutputs; i++) {
			graph = graphs[i];			// load the graph
			igraph=aBuf.ChannelVector[i];	// setup igraph
			start = aBuf.Pos+i;			// interleaved start position
// USERPIN
			errChk(GetYMin(graph,&min));	// look up min
			errChk(GetYMax(graph,&max));   // look up max
			Interp = graph->Interp;   // look up function
			if (0!=InterpData[graph->InterpType].IFillShort)
				InterpData[graph->InterpType].IFillShort(graph,
						&(aBuf.Buffer[start].bipolar),delta,end-start,
						tb,AOBoard,igraph);
			else {			
				for (j=start,t=0.0; j<end; j+=delta,t+=tb) {
					// not too swift, not errChk here for speed purposes
					// Polarity of output channel setup at startup
					// v.unipolar is just the memory address, NOT 
					// related to the polarity of the channel
					AO_VScale (AOBoard, igraph, 
							Pin( Interp(t,graph), min, max), &v.unipolar);
					aBuf.Buffer[j]=v;		// add to buffer
				}
			}
			// FinalValues 
			t=TimeLimit/1000.0;			// make it exact
			d = Interp(t,graph);		// get the voltage
			d = Pin (d, min, max);	// coerce to prevent crashes
			// The next call is the slow step 
			errChk(AO_VScale (AOBoard, igraph, d, &v.unipolar));
			FinalValues[i]=v;
		}
		// clean up 
		for (i=0;i<aBuf.NumberOfOutputs; i++) {
			errChk(dna_TouchARFGraph(graph));
		}
		// advance fill pointer 
		aBuf.Pos=end;
	}	
	else {
		// just output final values 
		delta = aBuf.NumberOfOutputs;	// delta is number of graphs
		end = aBuf.Pos+(TimeLimit/TimeBase)*delta;	// Position after end
		for (i=0; i<aBuf.NumberOfOutputs; i++) {
			start = aBuf.Pos+i;			// interleaved start position
			v=FinalValues[i];			// look up FinalValue
			for (j=start; j<end; j+=delta) {
				aBuf.Buffer[j]=v;		// add to buffer
			}
		}
		// advance fill pointer 
		aBuf.Pos=end;
	}
Error:
	return (err?-1:0);
}

int PrepFinalValues(dnaAnalogValue *  FinalValues)
{
	int err=0;
	int i,index;
	double d,min,max;
	dnaAnalogValue v;
// USERPIN	
	// GOTCHA : really ugly code to set ranges
	// See also BRAIN.c SetRanges
	// see also HAND.c hand_ResetAll and hand_ResetAnalog
	// see also BRAIN.c SetAnalogIdle
	for (i=0; i<aBuf.NumberOfOutputs; i++) {
		index=aBuf.ChannelVector[i];
		if (index<dnaAnalogUnipolar) {
			min=RangeMin[dnaUnipolar];	// look up min
			max=RangeMax[dnaUnipolar];   // look up max
		}
		else {
			min=RangeMin[dnaBipolar];	// look up min
			max=RangeMax[dnaBipolar];   // look up max
		}		
		d = Pin(InitIdle.InitAnalog[index],min,max);
		errChk(AO_VScale (AOBoard, index, d, &v.unipolar));
		FinalValues[i].unipolar=v.unipolar;
	}
Error:
	return (err?-1:0);
}

//*********************************************************************
//*********************************************************************


// This return 1 if invalid, 0 if valid, -1 if error
int ValidateChannels(void)
{   
	int i, err=0;
	
	require(aBuf.NumberOfOutputs>=1);
	require(aBuf.NumberOfOutputs<=10);
	if (0==aBuf.NumberOfOutputs) {
		errChk(MessagePopup ("FYI",
		"You have started analog output with zero channels activated"));
	}		
	if (1==aBuf.NumberOfOutputs) {
		if ((0<=aBuf.ChannelVector[0]) && (7>=aBuf.ChannelVector[0]))
			return 0;	// okay
		else
			return 1;	// Channels 8 and 9 forbidden
		}
	else
		// Must be in sequence, 0 1 2 3 etc...
		for(i=0; i<aBuf.NumberOfOutputs; i++)
			if (i!=aBuf.ChannelVector[i])
				return 1;
Error:
	return (err?-1:0);
}

// This is Built after the digital buffer(s) 
// This returns +1 if channel selection is invalid
int BuildAnalogBuffer(int Timebase,int TotalDuration)
{
	int err=0;
	int i,j,k;							// looping
	int t;								// time of start of next fill
	int st;								// commanded stop time for this group
	int cd;								// a duration of current cluster
	int d;								// duration accumulator
	int ad;								// duration for the group
	int igroup;							// current group to fill next
	int outputs,words;
	dnaAnalogValue FinalValues[dnaAnalogChannels];
	short Channels[dnaAnalogChannels];


	outputs=0;
	for (i=0; i<AGroups.NumberOfGraphs; i++)
		if (AGroups.EnabledQ[i]) {
			// acquire the channel vector info 
			Channels[outputs]=i;
			outputs++;
		}
	aBuf.NumberOfOutputs=outputs;
	words=TotalDuration/Timebase;
// ALLOCATION call 
	errChk(dna_SetupBufferAnalog (&aBuf, outputs, words));
	if (0==aBuf.NumberOfBytes) {		// We need do nothing
		err=0;
		goto Error;
	}
	// copy the channel vector info 
	for (i=0; i<outputs; i++) {
		aBuf.ChannelVector[i]=Channels[i];
// ADEBUG
//		printf("ChannelVector %i, Ch %i \n",i,aBuf.ChannelVector[i]);
	}
	err=ValidateChannels();
	if (1==err) {	// Give feedback about problem
		errChk(MessagePopup ("Analog Channel Error",
"You Cannot Select That Subset Of Analog Channels\n\
You may select zero through seven as a single channel,\n\
or several consecutive channels starting at zero."));
// DEBUG
		// now erase the memory allocated to the analog buffer
		// to try and stop a crash next time Execute is pressed
// ALLOCATION call
		errChk(dna_SetupBufferAnalog (&aBuf, 0, 0));
		return 1;							   
	};
	errChk(err);	// In case some error occured
	
	errChk(PrepFinalValues(FinalValues));
	
	// clear all the durations 
	t=0;
	st=0;
	cd=0;
	d=0;
	// initialize holding mode 
	igroup=-1;
	ad=0;
	// begin (i is cluster counter)
	for (i=0; i<Clusters.NumberOfClusters; i++) {
		if (Clusters.Clusters[i].EnabledQ) {
 			cd=Clusters.Clusters[i].Ticks*Clusters.Clusters[i].TimeUnit;
			switch (Clusters.Clusters[i].AnalogSelector) {
				case dnaARFContinue:
					// work with current group 
					d+=cd;				// Advance Accumulated allowed time
					if ((-1!=igroup) && (d>=ad)) {
						//finish old group
						st=t+ad;		// set the stop time
						errChk(AddAnalogGroupToBuffer(igroup,Timebase,
								ad,FinalValues));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStartGroup:
					// finish old group 
// ADEBUG
//					printf("dnaARFStartGroup i=%i igroup=%i",i,igroup);
					st=t+d;				// Set the stop time
					ad=d;				// truncate the requested time
										// if igroup==-1,ad==0, this sets ad
					errChk(AddAnalogGroupToBuffer(igroup,Timebase,
								ad,FinalValues));
					t=st;				// advance the time
					d=d-ad;				// Reset the accumulator
					// inilialize for new group 
					igroup=Clusters.Clusters[i].AnalogGroup;
							// get the new group index
					ad=AGroups.ARFGroups[igroup].TicksD*
							AGroups.ARFGroups[igroup].TimeUnitD;
							// get the requested time
					// work with new group 
					d+=cd;			 	// Advance Accumulated allowed time
					if (d>=ad) {
						st=t+ad;		// set the stop time
						errChk(AddAnalogGroupToBuffer(igroup,Timebase,
								ad,FinalValues));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStop:
					if (-1!=igroup) {
						//finish old group
						st=t+d;				// Set the stop time
						ad=d;				// truncate the requested time
						errChk(AddAnalogGroupToBuffer(igroup,Timebase,
								ad,FinalValues));
						t=st;				// Advance time
						d=d-ad;			 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;			// Go to holding mode
						ad=0;				// clear the requested time
						// work with new group 
						d+=cd;             	// Advance Accumulated allowed time
					}
					else { // Holding, so like continue would have been
						// work with current group 
						d+=cd;				// Advance Accumulated allowed time
					}					
					break;
			}
		}
	}
	// After all the clusters, stop and fill 	
	st=t+d;				// Set the stop time
	ad=d;				// truncate the requested time
						// if igroup==-1 then ad==0 and this sets ad
	errChk(AddAnalogGroupToBuffer(igroup,Timebase,
				ad,FinalValues));
	t=st;				// Advance the time
	d=d-ad;				// Reset the accumulator

Error:
	return (err?-1:0);
}

#if defined SYNCHGPIB
//***********************************************************************
//                        GPIB BUFFER
//***********************************************************************
//*
// OLDER SYSTEM 
// IGroup is RFGroup index to output 
// TimeLimit is duration that panel will run 
// Mask has bit to set 
// BeatHold is number of words to have high 
// ******************** SYNCH GPIB
int AddRFGroupToBuffer(int IGroup, int TimeLimit, int Mask, int BeatHold)
{
	int err=0;
	int i,j,k;							// looping
	double t;							// time in milliseconds
	double tb;							// TimeBase in milliseconds
	double da,df;							// analog voltage
	double amin,amax;					// Amplitude allowable range
	double fmin,fmax;					// Frequency allowable range
	int igraph;							// graph index;
	int start,delta,end;				// buffer indices
	int RFTime;							// update rate
	int commands;						// number of commands to add
	dnaARFGraph *agraph,*fgraph;		// pointers...
	dnaARFGraph *graphs[dnaAnalogChannels];	// several pointers
	brainInterp AInterp,FInterp;		// function pointer
	int TimeBase;						// Digital timebase
	unsigned int *DW;					// Digital word to play with
	
	TimeBase=dBuf1.TimeBase;
	if (-1!=IGroup) {
		RFTime=RFGroups.ARFGroups[IGroup].Ticks*
				RFGroups.ARFGroups[IGroup].TimeUnit;

//		rfBuf.CallTimebase=RFTime;
		errChk(PrepInterp(&RFGroups,IGroup,rfBuf.AmpIndex,&agraph));
		nullChk(agraph);
		errChk(PrepInterp(&RFGroups,IGroup,rfBuf.FreqIndex,&fgraph));
		nullChk(fgraph);

//		Note: TimeLimit % RFTime = 0
//		RFTime % TimeBase = 0  is assumed 
// USERPIN		
		amin=RangeMin[dnaAmplitude];	// look up min
		amax=RangeMax[dnaAmplitude];	// look up max
		fmin=RangeMin[dnaFrequency];	// look up min
		fmax=RangeMax[dnaFrequency];	// look up max
		AInterp = agraph->Interp;		// look up function
		FInterp = fgraph->Interp;		// look up function
		// fill the buffer 
		commands=TimeLimit/RFTime+1;		// Number of RF commands
		if (commands<2)	{				// Can't have this !
			err=-1;
			goto Error;
		}
		delta = RFTime/TimeBase;		// Updates per RF command
		if (delta<4*BeatHold) {		// Can't have this !
			err=-1;
			goto Error;
		}
		if (delta<3) {					// Can't have this !
			err=-1;
			goto Error;
		}
		start = dBuf1.Pos;				// starting index
		end = dBuf1.Pos+(TimeLimit/TimeBase);	// Position after end
		if (end>dBuf1.NumberOfWords)
			printf("dBuf1 size Error in AddRFGroupToBuffer\n");
		t=0.0;							// initial group time
		tb = RFTime / 1000.0;			// convert to miliseconds
		for (i=start,t=0.0; i<end; i+=delta,t+=tb) {
			da=AInterp(t,agraph);		// get amplitude in volts
			df=FInterp(t,fgraph);		// get frequency
			da = Pin (da, amin, amax);	// coerce to prevent crashes
			df = Pin (df, fmin, fmax);	// coerce to prevent crashes
			errChk(mouth_AddBufferCommand (&rfBuf, da, df));
			for (j=0,DW=&dBuf1.Buffer.asU32[i]; j<BeatHold; j++,DW++)
				(*DW) |=Mask;
		}
		da=AInterp(t,agraph);			// get amplitude in volts
		df=FInterp(t,fgraph);			// get frequency
		da = Pin (da, amin, amax);		// coerce to prevent crashes
		df = Pin (df, fmin, fmax);		// coerce to prevent crashes
		errChk(mouth_AddBufferCommand (&rfBuf, da, df));
		// Last update is done out of exact timing... 
		i-=end-2*BeatHold;					// back up;
		for (j=0,DW=&dBuf1.Buffer.asU32[i]; j<BeatHold; j++,DW++)
			(*DW) |=Mask;
		// clean up 
		errChk(dna_TouchARFGraph(agraph));
		errChk(dna_TouchARFGraph(fgraph));
		// advance fill pointer 
		dBuf1.Pos=end;
	}	
	else {
		// add a zero command to shut it off 
		if (dBuf1.Pos<dBuf1.MemoryUsed) {
			errChk(mouth_AddBufferCommand (&rfBuf, 0, 0));
			start = dBuf1.Pos;				// starting index
			i=start;		
			for (j=0,DW=&dBuf1.Buffer.asU32[i]; j<BeatHold; j++,DW++)
				(*DW) |=Mask;
		}
		// just update position in digital buffer 
		dBuf1.Pos+=TimeLimit/TimeBase;
	}
Error:
	return (err?-1:0);
}

// ******************** SYNCH GPIB
// OLDER SYSTEM 
// This is Built after the digital/analog buffer(s) 
int BuildRFBuffer(int line)				// line is digital output to tweak
{
	int err=0;
	int i,j,k;							// looping
	int t;								// time of start of next fill
	int st;								// commanded stop time for this group
	int cd;								// a duration of current cluster
	int d;								// duration accumulator
	int ad;								// duration for the group
	int igroup;							// current group to fill next
	int outputs,words;
	int size;							// memory allocated
	int BeatHold;						// # of digital words to hold high
	int Timebase,TotalDuration;			// duh
	unsigned int mask;					// or mask;
	
	dBuf1.Pos=0;
	Timebase=dBuf1.TimeBase;
	words=dBuf1.NumberOfWords;
	TotalDuration=words*Timebase;
	
	mask=(1<<line);
	BeatHold=10000/Timebase;
	outputs=2;
	// GOTCHA : The mother of all gotchas
	// TODO : make it self-growing
	size=(1000)*(30);					//	Guesstimate of size needed;
// ALLOCATION call 
	errChk(dna_SetupBufferRF (&rfBuf, size));
	mouth_ResetBuffer (&rfBuf);

	// clear all the durations 
	t=0;
	st=0;
	cd=0;
	d=0;
	// initialize holding mode 
	igroup=-1;
	ad=0;
	// begin 
	for (i=0; i<Clusters.NumberOfClusters; i++) {
		if (Clusters.Clusters[i].EnabledQ) {
 			cd=Clusters.Clusters[i].Ticks*Clusters.Clusters[i].TimeUnit;
			switch (Clusters.Clusters[i].RFSelector) {
				case dnaARFContinue:
					// work with current group 
					d+=cd;				// Advance Accumulated allowed time
					if ((-1!=igroup) && (d>=ad)) {
						//finish old group
						st=t+ad;		// set the stop time
						errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStartGroup:
					// finish old group 
					st=t+d;				// Set the stop time
					ad=d;				// truncate the requested time
										// if igroup==-1,ad==0, this sets ad
					errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
					t=st;				// advance the time
					d=d-ad;				// Reset the accumulator
					// initialize for new group 
					igroup=Clusters.Clusters[i].RFGroup;
							// get the new group index
					ad=RFGroups.ARFGroups[igroup].TicksD*
							RFGroups.ARFGroups[igroup].TimeUnitD;
							// get the requested time
					// work with new group 
					d+=cd;			 	// Advance Accumulated allowed time
					if (d>=ad) {
						st=t+ad;		// set the stop time
						errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStop:
					if (-1!=igroup) {
						//finish old group
						st=t+d;				// Set the stop time
						ad=d;				// truncate the requested time
						errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
						t=st;				// Advance time
						d=d-ad;			 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;			// Go to holding mode
						ad=0;				// clear the requested time
						// work with new group 
						d+=cd;             	// Advance Accumulated allowed time
					}
					else { // Holding, so like continue would have been
						// work with current group 
						d+=cd;				// Advance Accumulated allowed time
					}					
					break;
			}
		}
	}
	// After all the clusters, stop and fill 	
	st=t+d;				// Set the stop time
	ad=d;				// truncate the requested time
						// if igroup==-1 then ad==0 and this sets ad
	errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
	t=st;				// Advance the time
	d=d-ad;				// Reset the accumulator
	// clean up dBuf1 internal pointer 
	dBuf1.Pos=0;
	rfBuf.PosByte=0;
	rfBuf.PosCommand=0;
Error:
	return (err?-1:0);
}


//****************************************************************************
#if 0    // formerly excluded lines 

//*

// IGroup is RFGroup index to output 
// TimeLimit is duration that panel will run 
// Mask has bit to set 
// BeatHold is number of words to have high 
// ******************** DISABLED GPIB

int AddRFGroupToBuffer(int IGroup, int TimeLimit, int Mask, int BeatHold)
{
	int err=0;
	int i,j,k;							// looping
	double t;							// time in milliseconds
	double tb;							// TimeBase in milliseconds
	double da,df;							// analog voltage
	double amin,amax;					// Amplitude allowable range
	double fmin,fmax;					// Frequency allowable range
	int igraph;							// graph index;
	int start,delta,end;				// buffer indices
static int RFTime = 60000;				// update rate
	int commands;						// number of commands to add
	dnaARFGraph *agraph,*fgraph;		// pointers...
	dnaARFGraph *graphs[dnaAnalogChannels];	// several pointers
	brainInterp AInterp,FInterp;		// function pointer
	int TimeBase;						// Digital timebase
	unsigned int *DW;					// Digital word to play with
	
	TimeBase=dBuf1.TimeBase;
	if (-1!=IGroup) {
		RFTime=RFGroups.ARFGroups[IGroup].Ticks*
				RFGroups.ARFGroups[IGroup].TimeUnit;
		delta = RFTime/TimeBase;		// Updates per RF command

		errChk(PrepInterp(&RFGroups,IGroup,rfBuf.AmpIndex,&agraph));
		nullChk(agraph);
		errChk(PrepInterp(&RFGroups,IGroup,rfBuf.AmpIndex,&fgraph));
		nullChk(fgraph);

		// Note: TimeLimit % RFTime = 0
				RFTime % TimeBase = 0  is assumed 
// USERPIN		
		amin=RangeMin[dnaAmplitude];	// look up min
		amax=RangeMax[dnaAmplitude];	// look up max
		fmin=RangeMin[dnaFrequency];	// look up min
		fmax=RangeMax[dnaFrequency];	// look up max
		AInterp = agraph->Interp;		// look up function
		FInterp = fgraph->Interp;		// look up function
		// fill the buffer 
		commands=TimeLimit/RFTime;		// Number of RF commands
		if (commands<2)	{				// Can't have this !
			err=-1;
			goto Error;
		}
		if (delta<4*BeatHold) {		// Can't have this !
			err=-1;
			goto Error;
		}
		if (delta<3) {					// Can't have this !
			err=-1;
			goto Error;
		}
		start = dBuf1.Pos;				// starting index
		end = dBuf1.Pos+(TimeLimit/TimeBase);	// Position after end
		t=0.0;							// initial group time
		tb = RFTime / 1000.0;			// convert to miliseconds
		for (i=start,t=0.0; i<end; i+=delta,t+=tb) {
			da=AInterp(t,agraph);		// get amplitude in volts
			df=FInterp(t,fgraph);		// get frequency
			da = Pin (da, amin, amax);	// coerce to prevent crashes
			df = Pin (df, fmin, fmax);	// coerce to prevent crashes
			errChk(mouth_AddBufferCommand (&rfBuf, da, df));
			for (j=0,DW=&dBuf1.Buffer.asU32[i]; j<BeatHold; j++,DW++)
				(*DW) |=Mask;
		}
		da=AInterp(t,agraph);			// get amplitude in volts
		df=FInterp(t,fgraph);			// get frequency
		da = Pin (da, amin, amax);		// coerce to prevent crashes
		df = Pin (df, fmin, fmax);		// coerce to prevent crashes
		errChk(mouth_AddBufferCommand (&rfBuf, da, df));
		// Last update is done out of exact timing... 
		i-=2*BeatHold;					// back up;
		for (j=0,DW=&dBuf1.Buffer.asU32[i]; j<BeatHold; j++,DW++)
			(*DW) |=Mask;
		// clean up 
		errChk(dna_TouchARFGraph(agraph));
		errChk(dna_TouchARFGraph(fgraph));
		// advance fill pointer 
		dBuf1.Pos=end;
	}	
	else {
		// add a zero command to shut it off 
		
		if (0<TimeLimit) {
			errChk(mouth_AddBufferCommand (&rfBuf, 0, 0));
			start = dBuf1.Pos+RFTime/TimeBase;		// starting index
			i=start;		
			for (j=0,DW=&dBuf1.Buffer.asU32[i]; j<BeatHold; j++,DW++)
				(*DW) |=Mask;
		}
		
		// just update position in digital buffer 
		dBuf1.Pos+=TimeLimit/TimeBase;
	}
Error:
	return (err?-1:0);
}

// ******************** DISABLED GPIB

// This is Built after the digital/analog buffer(s) 
int BuildRFBuffer(int line)				// line is digital output to tweak
{
	int err=0;
	int i,j,k;							// looping
	int t;								// time of start of next fill
	int st;								// commanded stop time for this group
	int cd;								// a duration of current cluster
	int d;								// duration accumulator
	int ad;								// duration for the group
	int igroup;							// current group to fill next
	int outputs,words;
	int size;							// memory allocated
	int BeatHold;						// # of digital words to hold high
	int Timebase,TotalDuration;			// duh
	unsigned int mask;					// or mask;
	
	dBuf1.Pos=0;
	Timebase=dBuf1.TimeBase;
	words=dBuf1.NumberOfWords;
	TotalDuration=words*Timebase;
	
	mask=(1<<line);
	BeatHold=2000/Timebase;
	outputs=2;
	// GOTCHA : The mother of all gotchas
	// TODO : make it self-growing
	size=(1000)*(30);					//	Guesstimate of size needed;
// ALLOCATION call 
	errChk(dna_SetupBufferRF (&rfBuf, size));

	// clear all the durations 
	t=0;
	st=0;
	cd=0;
	d=0;
	// initialize holding mode 
	igroup=-1;
	ad=0;
	// begin 
	for (i=0; i<Clusters.NumberOfClusters; i++) {
		if (Clusters.Clusters[i].EnabledQ) {
 			cd=Clusters.Clusters[i].Ticks*Clusters.Clusters[i].TimeUnit;
			switch (Clusters.Clusters[i].RFSelector) {
				case dnaARFContinue:
					// work with current group 
					d+=cd;				// Advance Accumulated allowed time
					if ((-1!=igroup) && (d>=ad)) {
						//finish old group
						st=t+ad;		// set the stop time
						errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStartGroup:
					// finish old group 
					st=t+d;				// Set the stop time
					ad=d;				// truncate the requested time
										// if igroup==-1,ad==0, this sets ad
					errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
					t=st;				// advance the time
					d=d-ad;				// Reset the accumulator
					// initialize for new group 
					igroup=Clusters.Clusters[i].RFGroup;
							// get the new group index
					ad=RFGroups.ARFGroups[igroup].TicksD*
							RFGroups.ARFGroups[igroup].TimeUnitD;
							// get the requested time
					// work with new group 
					d+=cd;			 	// Advance Accumulated allowed time
					if (d>=ad) {
						st=t+ad;		// set the stop time
						errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStop:
					if (-1!=igroup) {
						//finish old group
						st=t+d;				// Set the stop time
						ad=d;				// truncate the requested time
						errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
						t=st;				// Advance time
						d=d-ad;			 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;			// Go to holding mode
						ad=0;				// clear the requested time
						// work with new group 
						d+=cd;             	// Advance Accumulated allowed time
					}
					else { // Holding, so like continue would have been
						// work with current group 
						d+=cd;				// Advance Accumulated allowed time
					}					
					break;
			}
		}
	}
	// After all the clusters, stop and fill 	
	st=t+d;				// Set the stop time
	ad=d;				// truncate the requested time
						// if igroup==-1 then ad==0 and this sets ad
	errChk(AddRFGroupToBuffer(igroup,ad,mask,BeatHold));
	t=st;				// Advance the time
	d=d-ad;				// Reset the accumulator
	// clean up dBuf1 internal pointer 
	dBuf1.Pos=0;
	rfBuf.PosByte=0;
	rfBuf.PosCommand=0;
Error:
	return (err?-1:0);
}
#endif

#endif
#if defined SOFTGPIB
// *************************** SOFTGPIB *************************************
int AddRFGroupToBuffer(int IGroup, int TimeLimit, int InitialTime)
// Initial time is in microseconds, we round down to milliseconds
{
	int err=0;
	int i,j,k;							// looping
	double t;							// time in milliseconds
	double tb;							// TimeBase in milliseconds
	double da,df;							// analog voltage
	double amin,amax;					// Amplitude allowable range
	double fmin,fmax;					// Frequency allowable range
	int igraph;							// graph index;
	int start,delta,end;				// buffer indices
	dnaTime RFTime=0;					// update rate
	// THE NEXT THREE VARIABLES ARE STATIC
	// they will only be zero in unison to indicate previous idle state
	static dnaTime StartTime=0;
	static dnaTime RFTimebase=0;
	static dnaTime LastTime=0;
	int commands;						// number of commands to add
	dnaARFGraph *agraph,*fgraph;		// pointers...
	dnaARFGraph *graphs[dnaAnalogChannels];	// several pointers
	brainInterp AInterp,FInterp;		// function pointer
	int TimeBase;						// Digital timebase
	unsigned int *DW;					// Digital word to play with
	
	TimeBase=dBuf1.TimeBase;
	if (0<=IGroup) {
		RFTime=RFGroups.ARFGroups[IGroup].Ticks*
				RFGroups.ARFGroups[IGroup].TimeUnit;
		rfBuf.CallTimebase=RFTime;		// current stage timebase

		// See if we are coming from idle state or another sweep
		if (0==RFTimebase) {
			// If from idle we can learn new timebase
			RFTimebase=RFTime/1000;		// SET THE STATIC RFTimebase
			// we can assume (0==StartTime)
			StartTime=InitialTime/1000;		// SET THE STATIC StartTime
		}
		else {
			// We need to see if we can run smoothly with same timebase
			if ((RFTimebase!=(RFTime/1000)) ||
				((InitialTime-LastTime) < RFTimebase) )
			{
				// Since not smooth then we must change stages
				// This will log out the last stage
				errChk(dna_AdvanceStage (&rfBuf, StartTime, RFTimebase));
				RFTimebase=RFTime/1000;
				StartTime=InitialTime;
			}
//			else 	// Smooth, we can combine stages
		}
		
		errChk(PrepInterp(&RFGroups,IGroup,rfBuf.AmpIndex,&agraph));
		nullChk(agraph);
		errChk(PrepInterp(&RFGroups,IGroup,rfBuf.FreqIndex,&fgraph));
		nullChk(fgraph);

//		Note: Assume TimeLimit % RFTime = 0 and
//		RFTime % TimeBase = 0
// USERPIN		
		amin=RangeMin[dnaAmplitude];	// look up min
		amax=RangeMax[dnaAmplitude];	// look up max
		fmin=RangeMin[dnaFrequency];	// look up min
		fmax=RangeMax[dnaFrequency];	// look up max
		AInterp = agraph->Interp;		// look up function
		FInterp = fgraph->Interp;		// look up function
		// no need to add one since we are using adjusted time scaling
		commands=TimeLimit/RFTime;		// Number of RF commands
		require(commands>=1);			// Can't have this !

		delta = RFTime/TimeBase;		// Updates per RF command

		start = dBuf1.Pos;				// starting index
		end = dBuf1.Pos+(TimeLimit/TimeBase);	// Position after end
		if (end>dBuf1.NumberOfWords) {
			MessagePopup ("Brain Error",
						  "dBuf1 size Error in AddRFGroupToBuffer\n");
			err=-1;
			goto Error;
		}
		t=0.0;							// initial group time
		tb = RFTime / 1000.0;			// convert to miliseconds
		for (i=start,t=0.0; i<end; i+=delta,t+=tb) {
			da=AInterp(t,agraph);		// get amplitude in volts
			df=FInterp(t,fgraph);		// get frequency
			da = Pin (da, amin, amax);	// coerce to prevent crashes
			df = Pin (df, fmin, fmax);	// coerce to prevent crashes
			errChk(mouth_AddBufferCommand (&rfBuf, da, df));
		}
		t=(t-tb)+InitialTime/1000.0;	// time of last command, msec
		LastTime=t;						// SET THE STATIC LastTime
		// clean up 
		errChk(dna_TouchARFGraph(agraph));
		errChk(dna_TouchARFGraph(fgraph));
		// advance fill pointer 
		dBuf1.Pos=end;
	}	
	else {
		// Advance to next stage if not at beginning of everything
		if (0!=rfBuf.NumberOfCommands) {
			errChk(dna_AdvanceStage (&rfBuf, StartTime, RFTimebase));
			StartTime=0;
			RFTimebase=0;
		}
		// add a zero command to shut it off 
//		if (dBuf1.Pos<dBuf1.MemoryUsed) {
//			errChk(mouth_AddBufferCommand (&rfBuf, 0, 0));
//			start = dBuf1.Pos;				// starting index
//			i=start;		
//			for (j=0,DW=&dBuf1.Buffer.asU32[i]; j<BeatHold; j++,DW++)
//				(*DW) |=Mask;
//		}
		// just update position in digital buffer 
		dBuf1.Pos+=TimeLimit/TimeBase;
	}
Error:
	return (err?-1:0);
}
 
// *** Above is SOFTGPIB  Below is SOFTGPIB
// OLDER SYSTEM 
// This is Built after the digital/analog buffer(s) 
int BuildRFBuffer(void)					// line is digital output to tweak
{
	int err=0;
	int i,j,k;							// looping
	int t;								// time of start of next fill
	int st;								// commanded stop time for this group
	int cd;								// a duration of current cluster
	int d;								// duration accumulator
	int ad;								// duration for the group
	int igroup;							// current group to fill next
	int outputs,words;
	int size;							// memory allocated
	int Timebase,TotalDuration;			// duh
	
	dBuf1.Pos=0;
	Timebase=dBuf1.TimeBase;
	words=dBuf1.NumberOfWords;
	TotalDuration=words*Timebase;
	outputs=2;
	// GOTCHA : The mother of all gotchas
	// TODO : make it self-growing
	size=(30000/60)*(30);				//	Guesstimate of size needed;
// ALLOCATION call 
	errChk(dna_SetupBufferRF (&rfBuf, size));
	mouth_ResetBuffer (&rfBuf);
	rfBuf.CriticalTime=500;				// one second = 1000 milliseconds

	// clear all the durations 
	t=0;
	st=0;
	cd=0;
	d=0;
	// initialize holding mode 
	igroup=-1;
	ad=0;
	// begin 
	for (i=0; i<Clusters.NumberOfClusters; i++) {
		if (Clusters.Clusters[i].EnabledQ) {
 			cd=Clusters.Clusters[i].Ticks*Clusters.Clusters[i].TimeUnit;
			switch (Clusters.Clusters[i].RFSelector) {
				case dnaARFContinue:
					// work with current group 
					d+=cd;				// Advance Accumulated allowed time
					if ((-1!=igroup) && (d>=ad)) {
						//finish old group
						st=t+ad;		// set the stop time
						errChk(AddRFGroupToBuffer(igroup,ad,t));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStartGroup:
					// finish old group 
					st=t+d;				// Set the stop time
					ad=d;				// truncate the requested time
										// if igroup==-1,ad==0, this sets ad
					errChk(AddRFGroupToBuffer(igroup,ad,t));
					t=st;				// advance the time
					d=d-ad;				// Reset the accumulator
					// initialize for new group 
					igroup=Clusters.Clusters[i].RFGroup;
							// get the new group index
					ad=RFGroups.ARFGroups[igroup].TicksD*
							RFGroups.ARFGroups[igroup].TimeUnitD;
							// get the requested time
					// work with new group 
					d+=cd;			 	// Advance Accumulated allowed time
					if (d>=ad) {
						st=t+ad;		// set the stop time
						errChk(AddRFGroupToBuffer(igroup,ad,t));
						t=st;			// Advance time
						d=d-ad;		 	// Reset the accumulator
						igroup=-1;		// Go to holding mode
						ad=0;			// clear the requested time
					}
					break;
				case dnaARFStop:
					if (-1!=igroup) {
						//finish old group
						st=t+d;				// Set the stop time
						ad=d;				// truncate the requested time
						errChk(AddRFGroupToBuffer(igroup,ad,t));
						t=st;				// Advance time
						d=d-ad;			 	// Reset the accumulator
						// initialize holding mode 
						igroup=-1;			// Go to holding mode
						ad=0;				// clear the requested time
						// work with new group 
						d+=cd;             	// Advance Accumulated allowed time
					}
					else { // Holding, so like continue would have been
						// work with current group 
						d+=cd;				// Advance Accumulated allowed time
					}					
					break;
			}
		}
	}
	// After all the clusters, stop and fill 	
	st=t+d;				// Set the stop time
	ad=d;				// truncate the requested time
						// if igroup==-1 then ad==0 and this sets ad
	errChk(AddRFGroupToBuffer(igroup,ad,t));
	t=st;				// Advance the time
	d=d-ad;				// Reset the accumulator
	if (-1!=igroup)
		errChk(AddRFGroupToBuffer(-1,0,0));	// tells it to close last stage
	// clean up dBuf1 internal pointer 
	dBuf1.Pos=0;
	// Reset rfBuf for output
	errChk(mouth_ResetBuffer (&rfBuf));

Error:
	return (err?-1:0);
}
// *** Above is SOFTGPIB


#endif

//****************************************************************************
//							BUILD BUFFER
//****************************************************************************
//
// This fills dBuf1 in preparation for a run.
//
// AuxD is an auxillary duration (such as for RF updates) that need to be 
//	taken into account in the GCF calculation.
// TimeBase is the small value that you want to use as timebase, if it is 
//	compatable with the final GCF then it will be used instead

// This can only handle Clusters.NumberOfPorts=1,2,4 right now 
// This will return +1 if cancel requested
int BuildBuffer(int AuxD,int TimeBase)		// AuxD, TimeBase in microseconds
{
	int err=0;
	int G,TD,W;							// Used to setup the buffer
	int i,j,k;								// looping
	int L;								// Number of output lines
	dnaCluster* cPtr;					// The cluster in the loop
	int D;								// Duration of the cluster
	int N;								// Number of words for the cluster
	int P;								// Target position for buffer loop;
	unsigned int Word;					// The all holy output word itself

	// Step 1 : Calculate Timebase and size of buffer and allocate it 
	// IMPORTANT TO KNOW: CalcResolution also sets AnalogQ and RFQ 	
	G=CalcResolution(&TD);				// Get GCF of clusters and TD
	if (0==TD) {						// Fail in a big way!
		err=-1;
		goto Error;
	}
	// If AuxD is zero this will not modify G 
	G=GCF(AuxD,G);						// Make compatable with AuxD
	if (G<=0) {							// check for validity
		err=-1;
		goto Error;
	}		
	
	// Timebase is the user suggestion on the Digital panel. It is not too useful
	// G is the timebase needed to do the all the digital/analog rf output
	// If the TimeBase requested is shorter than G and divides G then use it 
	// Otherwise ignore Timebase, or if Timebase is zero it is ignored 
	if ((0<TimeBase) && (TimeBase<G)) {
		if (G % TimeBase) {
			G=TimeBase;
		}
	}

	// G is now the final timebase that will be used to make the buffers
	W=TD / G;							// Number of words to output
	// W applies to both digital and analog output, but not RF
	if ((W<=0) || ((TD % G) != 0)) {	// Check that the math is right
		err=-1;
		goto Error;
	}

	// L will be the number of digital lines
	L=8*Clusters.NumberOfPorts;			// Number of lines ( bits ) for 1st card
	if (32<L) 
		L=32;

// ALLOCATION call 
	errChk(dna_SetupBufferDigital (&dBuf1, L, W));
	dBuf1.Pos = 0;						// Clear internal pointer
	dBuf1.TimeBase = G;					// Remember timebase

	// Now fill the buffer !!! 

	// There is a wierd order to how the bytes are read by		 
	// the harware.  This likely will need to be changed.....	 
	
	k=0;							// Index of first buffer entry to fill
	P=0;							// Index past end of filled buffer area
	cPtr=Clusters.Clusters;			// Point at first cluster
	for (i=0; i<Clusters.NumberOfClusters; i++,cPtr++) {
		if (cPtr->EnabledQ) {
			D = cPtr->Ticks * cPtr->TimeUnit;
			N = D / G;
			Word = 0;
			for (j=L-1; j>=0; j--) {// Start with data for MSB and work down
				Word=Word<<1;		// Make room, incease significance
				Word+=cPtr->Digital[j];	// Set the LSB
			}
			P+=N;					// Calculate end of this cluster in buffer
			for (;k<P;k++) {		// k is running count of buffer index
				/* Next: Put the owrd into the buffer at the right location */
				/* For clarity, we use a unoin of pointers and array referencing */
				/* This is NOT performance optimized ! */
				switch (L)	{		// Branch on width of variable type
					case 8:
						dBuf1.Buffer.asU8[k]=Word;
						break;
					case 16:
						dBuf1.Buffer.asU16[k]=Word;
						break;
					case 32:
						dBuf1.Buffer.asU32[k]=Word;
						break;
				}
			}
		}
	}

	// L will be the number of digital lines
	L=8*Clusters.NumberOfPorts;			// Number of lines ( bits )
	if (32<L) 
		L=L-32;						 	// Lines on 2nd card
	else
		L=0;							// Lines on 2nd card
	errChk(dna_SetupBufferDigital (&dBuf2, L, W));

#ifndef NODIOBOARD
	if (L>0) {
// ALLOCATION call 
		dBuf2.Pos = 0;						// Clear internal pointer
		dBuf2.TimeBase = G;					// Remember timebase

		// Now fill the buffer !!! 

		// There is a wierd order to how the bytes are read by		 
		// the harware.  This likely will need to be changed.....	 
	
		k=0;							// Index of first buffer entry to fill
		P=0;							// Index past end of filled buffer area
		cPtr=Clusters.Clusters;			// Point at first cluster
		for (i=0; i<Clusters.NumberOfClusters; i++,cPtr++) {
			if (cPtr->EnabledQ) {
				D = cPtr->Ticks * cPtr->TimeUnit;  // duration of word
				N = D / G;				// number of words
				Word = 0;				// reset word to output
				/* We have to add 32 to look at second half of output lines */
				for (j=32+L-1; j>=32; j--) {// Start with data for MSB and work down
					Word=Word<<1;		// Make room, incease significance
					Word+=cPtr->Digital[j];	// Set the LSB
				}
				P+=N;					// Calculate end of this cluster in buffer
				for (;k<P;k++) {		// k is running count of buffer index
					switch (L)	{		// Branch on width of variable type
						case 8:
							dBuf2.Buffer.asU8[k]=Word;
							break;
						case 16:
							dBuf2.Buffer.asU16[k]=Word;
							break;
						case 32:
							dBuf2.Buffer.asU32[k]=Word;
							break;
					}
				}
			}
		}
	}	
#endif	
	// Chain call the Analog buffer constructor 
	// It need the time base and duration info calculated in this function 	
	
	if (AnalogQ) {
// ADEBUG
//		printf("Calling BuildAnalogBuffer \n");
		err=UpdateProgressDialog (ProgressHandle, 25, 1);
		if (1==err)	// Cancel requested
			return 1;			
		errChk(err);
		err=BuildAnalogBuffer(G,TD);
		if (1==err) // Invalid channel selection
			return 1;
		errChk(err);
	}
	else {
		// Make sure we clear the analog buffer
		errChk(dna_SetupBufferAnalog (&aBuf, 0, 0));
	}

	if (RFQ) {
		err=UpdateProgressDialog (ProgressHandle, 60, 1);
		if (1==err)	// Cancel requested
			return 1;
		errChk(err);
#if defined SOFTGPIB
		errChk(BuildRFBuffer());	// used
#else							
		errChk(BuildRFBuffer(31));	// not used
#endif
		rfBuf.PosByte=0;
		rfBuf.PosCommand=0;
 	}
 	else {
 		// Make sure we clear the rf buffer
		errChk(dna_SetupBufferRF (&rfBuf, 0));
	}
	
	return 0;
Error:
	return -1;
}

//*********************************************************************

int RunAgain(void)
{
	int err=0;
	
//	errChk(hand_ConfigDigital (&dBuf1,AnalogQ));
	if (AnalogQ) {
		errChk(hand_ConfigAnalog(&aBuf));	// This will setup analog/RTSI
	
	}
	errChk(hand_ConfigureRTSI (AnalogQ));		  // This sets up all RTSI connections
	
	errChk(hand_OutBuffer (&dBuf1,&dBuf2)); 		// This starts everything running
//TODO : setup second digital buffer???
Error:
	return (err?-1:0);
}

//*********************************************************************
// GLOBAL GOTCHA : This can change the run status of the experiment

int CheckRunStatus(void)				
{
	unsigned long NumRemain=0;
	int err=0;
	int CancelQ=0;

	if ((0==RunDone) && (2!=RunMode)) { // do not interrupt GPIB
		errChk(hand_CheckProgress (&NumRemain));
#if VERBOSE > 3 // 1
		printf("CheckRunStatus{Remain %d, Done %d}\n",NumRemain,
				(dBuf1.NumberOfWords)-NumRemain);
#endif
		if (NumRemain)  {				// Update display
			PercentDone=100-(100*NumRemain)/(dBuf1.NumberOfWords);
			CancelQ=faceShowProgress(0, PercentDone);
			if (dnaTrue==CancelQ) {
				RunDone=1;
				errChk(hand_AbortRun ());
				errChk(faceEndRunningMode());
				RunMode=-1;
 				errChk(SetIdleOutput(dnaTrue));  // errChk added 2000/03/30 CEK
				errChk(SetAnalogIdle());  // added 2000/03/30 CEK
			}
		}
		else {
			if (RepeatRunQ()) {
				// Destroy and recreate dialog to work around user interface bug Johnny found
				// copy code from faceBeginRunningMode, need to encapsulate into a function
				DiscardProgressDialog (RunStatusDialog);
				RunStatusDialog = CreateProgressDialog ("Run in Progress","Percent Complete", 0,VAL_FULL_MARKERS, "__Cancel");
				negChk(RunStatusDialog);
				errChk(SetPanelAttribute (RunStatusDialog, ATTR_FLOATING,
							  VAL_FLOAT_APP_ACTIVE));	// Always on top
				errChk(SetPanelAttribute (RunStatusDialog, ATTR_HAS_TASKBAR_BUTTON, 1));
				errChk(SetPanelAttribute (RunStatusDialog, ATTR_CAN_MINIMIZE, 1));
 	
				errChk(ExecuteRun(dnaFalse));	// not first time
			}
			else {
				RunDone=1;				
				errChk(hand_RunOver ());
				errChk(faceEndRunningMode());
				RunMode=-1;
				SetIdleOutput(dnaTrue);
			}
		}
	}	
Error:
	return (err?-1:0);
}

// begin TESTING 

// item is CHKTIME+GTS
/*DWORD WINAPI MyThread(LPVOID item)
{
	double TA=-1.0,TB=-1.0,TC=-1.0 ;
	double Tv7;
	int i,flag=0;

//	SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS );
//	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);

	EnterCriticalSection(&critical);
	printf("Entering Loop Thread rfCommands %d Current %d DONE %d\n",
			rfBuf.NumberOfCommands,rfBuf.PosCommand,RunDone);
	TA=rfBuf.CallTimebase/1000000.0;	// in seconds, interval
	Tv7=0.0;							// initialize this voltage
	hand_SetAnalogOutput (7, Tv7);
	RunDone=0;							// set global to indicate readiness
	LeaveCriticalSection(&critical);
	while (-1.0==TC) {
		EnterCriticalSection(&critical);
		TC=CHKTIME;
		LeaveCriticalSection(&critical);
	}	
	TB=	TC+GTS+TA;						// 1st target time,initialize
	printf("First Target Time TB %f\n",TB+TA);
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
	while ((!flag) & (rfBuf.PosCommand<rfBuf.NumberOfCommands)) {
//		printf("Target Time TB %f\n",TB);
		TC=Timer();			// Current Time
		i=0;
		while (TC<TB) {
			i++;
			TC=Timer();		// Current Time
		}
		EnterCriticalSection(&critical);
		printf("%d, %f\n",i,TC-TB);
		Tv7=5.5-Tv7;					// switch voltage
		hand_SetAnalogOutput (7, Tv7);	// set new voltage
		mouth_OutBufferCommand (&rfBuf);	// GPIB
//		printf("7->%f , Current Time TC %f\n",v7,TC);
		flag=RunDone;
		LeaveCriticalSection(&critical);
		TB+=TA;								// Next target time set
		while (TC>TB) {
			TB+=TA;							// Target time must be future
//			printf("SKIPPING AHEAD\n");
		printf("%f\n",Timer()-TC);
		}
//		ProcessSystemEvents();				// Handle events
	}
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
//	SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS );
	printf("Waiting for end of run\n");
	while (!flag) {
		EnterCriticalSection(&critical);
		flag=RunDone;
		LeaveCriticalSection(&critical);
	}
	printf("Leaving Thread");
	return(0);
}
*/
//	Write a solid routine for the NOGPIB case, which I will then
//	leave alone (mostly) 


//*********************************************************************
//							Wait for Auxillary
//*********************************************************************

// return 0 for ok, 1 for cancel, -1 for error
// Puts A0 in on state while waiting
// Waits for line of aux port B to be value or cancel before returning
// Used to wait for signal from camera controller (Ready) before
// executing the run.
int	WaitForAux(int line, int value)
{
	int err=0;
	int inval,CancelQ;

	errChk(hand_ConfigAuxPorts ());

	errChk(hand_ReadAuxPort (line, &inval));  // auxwait or auxwaitQ
	if (inval==value)
		return 0;
	errChk(faceAuxWaitOn(line,value));
	errChk(hand_WriteAuxPort (0, 1));
	do{
		errChk(ProcessSystemEvents());
		negChk(CancelQ=faceAuxWaitStatus());
		errChk(hand_ReadAuxPort (line, &inval));
	} while ((!CancelQ) && (inval!=value));
	negChk(CancelQ=faceAuxWaitOff());
	errChk(hand_WriteAuxPort (0, 0));
	if (CancelQ)
		return 1;
	if (inval==value)
		return 0;
	err=-1;								// Bad state
Error:
	hand_WriteAuxPort (0, 0);		   // removed errChk from this line
	return (err?-1:0);
}

//*********************************************************************
//							FirstTime
//*********************************************************************

// returns +1 if cancel requested
int FirstTime(void)
{
	int err=0;

	// Only measures time to fill buffer, so only display first time
	ProgressHandle = CreateProgressDialog ("Creating Output Buffers",
		"Amount Complete (arb. units)", 1,VAL_NO_INNER_MARKERS, "__Cancel");
	negChk(ProgressHandle);
	errChk(UpdateProgressDialog (ProgressHandle, 0, 1));

	// clear global flags 
	AnalogQ=dnaFalse;
	RFQ=dnaFalse;

	// Above AnalogQ and RFQ flags are set in the next function 
	err=(BuildBuffer(AuxillaryInfo.UserTicks*
			AuxillaryInfo.UserTimeUnit,0)); //does digital,analog,rf
	if (1==err)
		return 1;
	errChk(err);
	errChk(UpdateProgressDialog (ProgressHandle, 66, 1));   // TODO FIXME Check for +1 return value
Error:
	return err;
}

#if defined SOFTGPIB

//*********************************************************************
//	This is the routine I am using and is the only maintained one															   
//																	   
//   							ExecuteRun								   
//								( SOFTGPIB )
//																	   
//*********************************************************************

int ExecuteRun(dnaByte FirstTimeQ)
{
	int err=0;
	int flag=1,CancelQ;
	dnaTime Time;						// make use SDK later
	dnaByte CE;	// ClusterEnabled;
	int i;
	// This if() Must be the first thing checked.
	if (!ExperimentEnabled)				// Lockout as last moment!
		return 0;
	if(0<VERBOSE) printf("ExecuteRun{SOFTGPIB}\n");
	// If first time, sanity check that something is enabled
	if (FirstTimeQ) {					
		// See if any clusters are enabled
		CE=dnaFalse;
		for (i=0; (i<Clusters.NumberOfClusters)&&(!CE); i++)
			if (Clusters.Clusters[i].EnabledQ)
				CE=dnaTrue;
		if (!CE) {
			errChk(MessagePopup ("Cluster Error", "You must enabled at least 1 cluster!"));
			err=0;
			goto Error;
		}
	}
	else {
		ProgressHandle=-1;	// Done as insurance
	}
	// Reset our crappy global flags
	RunDone=-1;
	RunMode=-1;
	CHKTIME=-1.0;
	GLOBALTIME=-1.0;
	// If first time, then build the buffers
	if (FirstTimeQ) {
		err=FirstTime();
		if (1==err) {	// Cancel requested
			err=0;
			goto Error;
		}
		errChk(err);
	}
	if (RFQ) {
		if(0<VERBOSE) printf("ExecuteRun{RF IS ON, #Commands=%d}\n",rfBuf.NumberOfCommands);
		if(0<VERBOSE) printf("IdleCommand being issued\n");
// Next line removed because it runs out of gpib device handles
//		errChk(mouth_InitRF ()); 
		// Reset rfBuf for output
		errChk(mouth_ResetBuffer (&rfBuf));
		require(0!=rfBuf.Buffer);	// ensure we have buffer allocated
		errChk(mouth_ZeroDS345 ()); // start with NO rf output
	}
	// Check to see if we should "Wait for it"
	// Had to move this before Digital Config calls since it calls
	// the SetIdleOutput function  which uses the manual output mode
	// TODO : decide if we also need to do something to analog idle here
	// The rf (if used) is set to zero above. (mouth_ZeroDS345())
	if (CameraTriggerQ()) {
		SetIdleOutput(dnaTrue);
		CancelQ=WaitForAux(0,1);
		if (0!=CancelQ)
			goto Error;					// Exit if user escapes
	}
	// This sets up the digital output
	errChk(hand_ConfigDigital (&dBuf1,&dBuf2,AnalogQ,0));	// zero input ports
	if (-1!=ProgressHandle) {
		err=UpdateProgressDialog (ProgressHandle, 82, 1);
		if (1==err) {	// Cancel requested
			err=0;
			goto Error;
		}
		errChk(err);
	}	
	if (AnalogQ) {
		if(0<VERBOSE) printf ("ExecuteRun{ANALOG IS ON}\n");
		// This sets up the analog board mode and RTSI bus
		errChk(hand_ConfigAnalog(&aBuf));	// This will setup analog
	}
	//This sets up all the RTSI connections
	errChk(hand_ConfigureRTSI (AnalogQ));
	
	if (-1!=ProgressHandle) {
		err=UpdateProgressDialog (ProgressHandle, 100, 1);
		if (1==err) {	// Cancel requested
			err=0;
			goto Error;
		}
		errChk(err);
		// We are now done with the setup, destroy the progress meter
		DiscardProgressDialog (ProgressHandle);
		ProgressHandle=-1;
	}
	RunDone=0;
	RunMode=3;							// Maximum lock mode paranoia
	if (FirstTimeQ) errChk(faceBeginRunningMode(dBuf1.TimeBase));
// *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
//  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
 	err=hand_OutBuffer (&dBuf1,&dBuf2); 		// This starts everything running
//  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
// *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
	if (0!=err) {
		RunDone=-1;
		RunMode=-1;
		if(0<VERBOSE) printf("ExecuteRun{hand_OutBuffer call failed}\n");
		goto Error;
	}
	// adjust global time by user defined offset
	GLOBALTIME=CHKTIME+AuxillaryInfo.GTS;		
//  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
// 	NOW HANDLE GPIB OUTPUT IN SOFTWARE
//  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
//  Possibly useful commands
//	SetSleepPolicy (VAL_SLEEP_MORE);			// CVI function
//	SetThreadPriority(GetCurrentThread(),__);	// Windows SDK function
//	SetPriorityClass(GetCurrentProcess(),__);   // Windows SDK function
	// loop through each stage	
	if (!RFQ)
		goto SkipRF;
	while (rfBuf.CurrentStage<rfBuf.NumberOfStages) {
		// Get elapsed time in milliseconds
		Time=GetTickCount()-GLOBALTIME;
		if (rfBuf.TargetTime > Time + rfBuf.CriticalTime) {  
			// we have time to kill, so to speak
			RunMode=1;					// run is waiting for an RF sweep    
			SetSleepPolicy (VAL_SLEEP_SOME);
#if defined PRIORITY
			SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
//			SetThreadPriority(GetCurrentThread(),__);
#endif
#if defined LOCKOUT
			SetPanelAttribute (RunStatusDialog, ATTR_WINDOW_ZOOM, VAL_NO_ZOOM);
			SetPanelAttribute (RunStatusDialog, ATTR_CAN_MINIMIZE, TRUE);
			SetPanelAttribute (RunStatusDialog, ATTR_CLOSE_ITEM_VISIBLE, TRUE);
			SetPanelAttribute (RunStatusDialog, ATTR_MOVABLE, TRUE);
			SetPanelAttribute (RunStatusDialog, ATTR_SYSTEM_MENU_VISIBLE, TRUE);
			EnableTaskSwitching();
#endif 
			if(0<VERBOSE) printf("ExecuteRun{RunMode=1}\n");
			do {
				if(1<VERBOSE) printf("ExecuteRun{RunMode %d,TickTime %d,Target %d}\n", RunMode,Time,rfBuf.TargetTime);
				// Next line listens for cancel button, for instance
				errChk(ProcessSystemEvents ());
				err=faceShowProgress(0,PercentDone);
				if (1==err) {	// Cancel Requested
	 				RunDone=1;
					errChk(hand_AbortRun ());
					SetIdleOutput(dnaTrue);			// go to a safe output
					err=0;
					goto Error;
				}
				errChk(err);
				Time=GetTickCount()-GLOBALTIME;
			} while (rfBuf.TargetTime > Time + rfBuf.CriticalTime);
		}
		// We need to get cranky and stop processing events
		RunMode=2;						// run is performing an RF sweep
		SetSleepPolicy (VAL_SLEEP_NONE);
#if defined PRIORITY
		SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);
//		SetThreadPriority(GetCurrentThread(),__);
#endif
#if defined LOCKOUT
		// Makes the progress dialog cover screen 
		DisableTaskSwitching();
		SetActivePanel(RunStatusDialog);
		SetPanelAttribute (RunStatusDialog, ATTR_CAN_MINIMIZE, FALSE);
		SetPanelAttribute (RunStatusDialog, ATTR_CAN_MAXIMIZE, FALSE);
		SetPanelAttribute (RunStatusDialog, ATTR_CLOSE_ITEM_VISIBLE, FALSE);
		SetPanelAttribute (RunStatusDialog, ATTR_MOVABLE, FALSE);
		SetPanelAttribute (RunStatusDialog, ATTR_SIZABLE, FALSE);
		SetPanelAttribute (RunStatusDialog, ATTR_SYSTEM_MENU_VISIBLE, FALSE);
		SetPanelAttribute (RunStatusDialog, ATTR_WINDOW_ZOOM, VAL_MAXIMIZE);
#endif 
		if(0<VERBOSE) printf("ExecuteRun{RunMode=2}\n");
		// Wait for first command in stage
		while (rfBuf.TargetTime > Time ) {
			Time=GetTickCount()-GLOBALTIME;
		}
		if(1<VERBOSE) printf("ExecuteRun{RunMode 2,TickTime %d,TargetTime %d}\n",Time,rfBuf.TargetTime);
		// Now to begin processing the commands in this stage
		// Output a command and update counters in rfBuf
		// first command in stage
		if (0<rfBuf.CommandCounter)
			errChk(mouth_OutBufferCommand (&rfBuf));
		while(rfBuf.CommandCounter) {
			do {						// wait for time for next command
				Time=GetTickCount()-GLOBALTIME;
			} while (rfBuf.TargetTime > Time );
			if(1<VERBOSE) printf("ExecuteRun{RunMode 2,TickTime %d,TargetTime %d}\n",Time,rfBuf.TargetTime);
			// Output a command and update counters in rfBuf
			err=(mouth_OutBufferCommand (&rfBuf));
			if(err) {
				printf("\n\n***mouth_OutBufferCommand error during RF sweep***\n\n",err);
				errChk(-666);
			}
		}			
		if(0<VERBOSE) printf("ExecuteRun{End of rfBuf.CurrentStage=%d}\n",rfBuf.CurrentStage);
		errChk(dna_AdvanceRunStage (&rfBuf));
	} // end while loop for RF
SkipRF:
	if (2==RunMode) {
		SetSleepPolicy (VAL_SLEEP_SOME);
#if defined PRIORITY
		SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
//		SetThreadPriority(GetCurrentThread(),__);
#endif
#if defined LOCKOUT
		SetPanelAttribute (RunStatusDialog, ATTR_WINDOW_ZOOM, VAL_NO_ZOOM);
		SetPanelAttribute (RunStatusDialog, ATTR_CAN_MINIMIZE, TRUE);
		SetPanelAttribute (RunStatusDialog, ATTR_CLOSE_ITEM_VISIBLE, TRUE);
		SetPanelAttribute (RunStatusDialog, ATTR_MOVABLE, TRUE);
		SetPanelAttribute (RunStatusDialog, ATTR_SYSTEM_MENU_VISIBLE, TRUE);
		EnableTaskSwitching();
#endif 
	}
	RunMode=0;			// run has no (more) RF and is running in background   

	if(0<VERBOSE) printf("ExecuteRun{Exiting Normally}\n");
	return (err?-1:0);					// normal return

//  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *  *
Error:
	// -666==err for RF error, which we respond differently to
	if (RunMode==2) {
			SetSleepPolicy (VAL_SLEEP_SOME);
#if defined PRIORITY
			SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
//			SetThreadPriority(GetCurrentThread(),__);
#endif
#if defined LOCKOUT
			if (-1!=RunStatusDialog) {
				SetPanelAttribute (RunStatusDialog, ATTR_WINDOW_ZOOM, VAL_NO_ZOOM);
				SetPanelAttribute (RunStatusDialog, ATTR_CAN_MINIMIZE, TRUE);
				SetPanelAttribute (RunStatusDialog, ATTR_CLOSE_ITEM_VISIBLE, TRUE);
				SetPanelAttribute (RunStatusDialog, ATTR_MOVABLE, TRUE);
				SetPanelAttribute (RunStatusDialog, ATTR_SYSTEM_MENU_VISIBLE, TRUE);
			}
			EnableTaskSwitching();
#endif 
	}
	RunMode=0;

	// need to be careful about cleaning up the progress window
	if ((-666!=err)&&(0!=ProgressHandle)) {
		// keep if err==-666
		DiscardProgressDialog (ProgressHandle);
		ProgressHandle=-1;
	}
	if(-666!=err)
		faceEndRunningMode();			// Must be a harmless call
	if(0<VERBOSE) printf("ExecuteRun{Exiting with err=%d}\n",err);
	if(-666==err) {
		Beep();
		mouth_InitRF ();
		mouth_ZeroDS345();
		return 0;	// clear error here
	}
	return (err?-1:0);
}

#endif	// SOFTGPIB

//
int TickRFTimer(void)
{
	int err=0;
Error:
	return (err?-1:0);
}

// TESTING 

int CVICALLBACK RFTimer (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_TIMER_TICK:
			if (rfBuf.PosCommand<rfBuf.NumberOfCommands)
				errChk(mouth_OutBufferCommand (&rfBuf));
			else  
				SetCtrlAttribute (panel, control, ATTR_ENABLED,
					  dnaFalse);
			break;
	}
Error:
	if (err) {
		Beep();
		SetCtrlAttribute (panel, control, ATTR_ENABLED,dnaFalse);
	}
	return 0;
}


// TESTING 


void myInCallback1(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam)
{
	GLOBALTIME = Timer();
	printf("\n*myInCallback1*GT %f   GT-CHKTIME  %f\n",
			GLOBALTIME,GLOBALTIME-CHKTIME);
}

/*
void myCallback(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam)
{
	float d,f;
//	long N=0;	
	double S;
	S=Timer();
//	if (!(wparam& 0xFF00)) {
//		hand_CheckProgress (&N);
//		N=(dBuf1.NumberOfWords)-N;
//	}
	
//	if (((dBuf1.TimeBase*lparam)/message) % 5==1) {
		d=dBuf1.TimeBase*lparam;
		d/=1000.0;
		if (0.0==SECS)
			SECS=S-d/1000.0;
	//	e=dBuf1.TimeBase*N/1000.0;
		f=(S-SECS)*1000.0;
	//		printf("myCallback Thread ID = %d\n",CurrThreadId ());
		printf("%d %d %d ( %f - %f = %f )\n",message,wparam,lparam,f,d,f-d);
//	}
}
*/
/*
void myCallback2(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam)
{
	GLOBALTIME = Timer();
	printf("\n********GT %f   GT-CHKTIME  %f\n",GLOBALTIME,GLOBALTIME-CHKTIME);
	Config_DAQ_Event_Message (wparam, 0, "DO0", 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

// CHANGES GLOBAL TIME, CHKTIME 
void myCallback3(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam)
{
	EnterCriticalSection(&critical);
	if (-2.0!=CHKTIME) {
		// Try to see when this happens on scope
		v7=5.5-v7;
		hand_SetAnalogOutput (7, v7);
		GLOBALTIME = Timer();
		printf("\n7-> %f , ********GT %f   GT-CHKTIME  %f\n",
				v7,GLOBALTIME,GLOBALTIME-CHKTIME);
		Config_DAQ_Event_Message (DOBoard, 2, "DO0", 0, 1, 0, 0, 0, 0,
										0, 0, myCallback3);
		GLOBALTIME=CHKTIME+GTS;
		CHKTIME=-2.0;							// -2 = discarded
	}
	else
		printf("Called myCallback3 too many times\n");
	LeaveCriticalSection(&critical);
}

*/
/*
// changes GLOBALTIME 
void myEndCallback1(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam)
{
	EnterCriticalSection(&critical);
	if (-2.0==GLOBALTIME)
		printf("Called myEndCallback1 too many times\n");
	else 
		if (-1.0!=GLOBALTIME) {
			hand_RunOver ();
			
			v7=5.5-v7;
			hand_SetAnalogOutput (7, v7);
			printf("****7-> %f , End of Run Time %f",v7,Timer()-GLOBALTIME);
//			Config_DAQ_Event_Message (DOBoard, 2, "DO0", 2, 1, 0, 0, 0, 0,
//											0, 0, myEndCallback1);
			GLOBALTIME=-2.0;					// -2 = discarded;
		}
		else
			printf("myEndCallback1 called while GLOBALTIME==-1.0 ?\n"); 
	LeaveCriticalSection(&critical);
}
*/


/*
void myEndCallback2(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam)
{
	EnterCriticalSection(&critical);
	RunDone=1;
	printf("DONE=1");
	if (-2.0==GLOBALTIME)
		printf("Called myEndCallback1 too many times\n");
	else 
		if (-1.0!=GLOBALTIME) {
			hand_RunOver ();
			
//			v7=5.5-v7;
//			hand_SetAnalogOutput (7, v7);
			printf("\n\n End of Run Time %f\n\n",Timer()-GLOBALTIME);
//			Config_DAQ_Event_Message (DOBoard, 2, "DO0", 2, 1, 0, 0, 0, 0,
//											0, 0, myEndCallback1);
			GLOBALTIME=-2.0;					// -2 = discarded;
		}
		else
			printf("myEndCallback1 called while GLOBALTIME==-1.0 ?\n"); 
	LeaveCriticalSection(&critical);
}
*/

void NOGPIBEndCallback(DAQEventHandle hwnd, DAQEventMsg message, 
		DAQEventWParam wparam, DAQEventLParam lparam)

{
	int err=0;
#if VERBOSE > 0
	printf("NOGPIBEndCallback{Entering function}\n");
#endif
#ifdef THREADS
	EnterCriticalSection(&critical);
#endif
	RunDone=1;
	errChk(hand_RunOver ());
	RunDone=1;				
	errChk(hand_RunOver ());
	errChk(faceEndRunningMode());
Error:
	if (err) {Beep();}					// Universal behavior on error
#if VERBOSE > 0
	if (err) printf("NOGPIBEndCallback{encountered an error}\n");
#endif
#ifdef THREADS
	LeaveCriticalSection(&critical);
#endif
}
/*
// changes only target time, rfBuf
int CVICALLBACK myAsynch2 (int reserved,int timerId,int event,
		void *callbackData,int eventData1,int eventData2)
{
	double TIME;
static int spam=0;
	EnterCriticalSection(&critical);
	spam++;
	printf("spam %d ::::::\n",spam);
	if (1==spam) {
		if (-2.0!=TARGETTIME) {
			if (-1.0!=GLOBALTIME) {
				if (-2.0==GLOBALTIME) {
					printf("*TIME %f* GT=-2 DEAD\n",TIME,TARGETTIME);
					DiscardAsyncTimer (timerId);
					TARGETTIME=-2.0;			// -2 = discarded
					spam--;
					LeaveCriticalSection(&critical);
					return 0;
				}
				if (0.0==TARGETTIME) {
					SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_TIME_CRITICAL);
				}
				TIME=Timer()-GLOBALTIME;
				if ((TIME<0.0)||(TIME>10.0))	{
					printf("*TIME %f* DEAD\n",TIME,TARGETTIME);
					DiscardAsyncTimer (timerId);
					TARGETTIME=-2.0;			// -2 = discarded
					spam--;
					LeaveCriticalSection(&critical);
					return 0;
				}
				printf("DELAY SINCE GT %f\n",TIME*1000.0);
				if ((TIME > TARGETTIME ) && (TIME<10.0)) {
					printf("DELAY SINCE GT %f\n",TIME*1000.0);
					if (rfBuf.PosCommand<rfBuf.NumberOfCommands) {
///////////////////////////////////////////////////////////////////////////
						v7=5.5-v7;
						hand_SetAnalogOutput (7, v7);
						mouth_OutBufferCommand (&rfBuf);
						TARGETTIME+=rfBuf.CallTimebase/1000000.0;
						while (TIME > TARGETTIME) {
							TARGETTIME+=rfBuf.CallTimebase/1000000.0;
							dna_AdvanceBufferRF (&rfBuf);
							printf("**\t\t\t**SKIP**\n");
						}
						printf("7-> %f , NEXT OUTPUT  @ %f\n",
								v7,TARGETTIME*1000.0);
					printf("END DELAY SINCE GT %f\n",
							(Timer()-GLOBALTIME)*1000.0);
					}
					else {
						printf("********** DEAD\n",TARGETTIME);
						DiscardAsyncTimer (timerId);
						SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
	//					GLOBALTIME=0.0;
						TARGETTIME=-2.0;			// -2 = discarded
					}
				}
	//			else
	//				printf("Waiting for next tick\n");
			}
	//		else
	//			printf("waiting for run to start\n");
		}
		else
			printf(" myAsynch2 WAS DISCARDED ! \n");
	}
	spam--;
	LeaveCriticalSection(&critical);
	return 0;
}
*/


int SetRangeMin(int index, double value, double *setvalue)
{
	int err=0;
	require(0<=index);
	require(index<faceGRAPHS);
	nullChk(setvalue);
	(*setvalue) = dna_SetRangeMin (&AuxillaryInfo, index, value);
	errChk(faceSetARFRanges(index));
Error:
	return (err?-1:0);
}

int SetRangeMax(int index, double value, double *setvalue)
{
	int err=0;
	require(0<=index);
	require(index<faceGRAPHS);
	nullChk(setvalue);
	(*setvalue) = dna_SetRangeMax (&AuxillaryInfo, index, value);
	errChk(faceSetARFRanges(index));
Error:
	return (err?-1:0);
}

double GetRangeMin(int index, double value)
{
	int err=0;
	require(0<=index);
	require(index<faceGRAPHS);
	return(AuxillaryInfo.UserMin[index]);
Error:
	printf("INDEX %i OUT OF RANGE",index);
	return 0.0;
}

double GetRangeMax(int index, double value)
{
	int err=0;
	require(0<=index);
	require(index<faceGRAPHS);
	return(AuxillaryInfo.UserMax[index]);
Error:
	printf("INDEX %i OUT OF RANGE",index);
	return 0.0;
}

int GetYMin(dnaARFGraph *graph, double *value)
{
	int err=0,index;
	
	nullChk(graph);
	nullChk(value);

	index=graph->RangeIndex;
	require(0<=index);
	require(index<faceGRAPHS);
	
	(*value)=AuxillaryInfo.UserMin[index];	
Error:
	return (err?-1:0);
}

int GetYMax(dnaARFGraph *graph, double *value)
{
	int err=0,index;
	
	nullChk(graph);
	nullChk(value);

	index=graph->RangeIndex;
	require(0<=index);
	require(index<faceGRAPHS);
	
	(*value)=AuxillaryInfo.UserMax[index];	
Error:
	return (err?-1:0);
}


// ACCESS TO MEMORY.c FUNCTIONS :

int SaveIni(void)
{
	int err=0;
// Get time and date this file was created 
	errChk(faceSaveAuxillaryInfo());	// Get all the extra data
	AuxillaryInfo.Index=0;				// no internal index
	AuxillaryInfo.DateS=DateStr();
	AuxillaryInfo.TimeS=TimeStr();
	errChk(Save(0));
Error:
	return (err?-1:0);
}

int LoadIni(const char filename[])
{
	int err=0;
	errChk(Load(0,filename));
Error:
	return (err?-1:0);
}
