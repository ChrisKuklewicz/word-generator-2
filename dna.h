#include <cvidef.h>

// DNA.h header file
#ifndef __DNAh__
#define __DNAh__
//Note : enum definitions cannot be repeated, therefore we use the
//__DNAh__ compiler define to avoid doing so


/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
 * This controls whether to compile for WG2 or WG3
 * Then use #if WGFLAG == 2 or if (WGFLAG==2) as needed
 */
//#define WGFLAG 2
#define WGFLAG 3

/*$$$$$$$$$ Check for valid definition above $$$$$$$$*/
#if WGFLAG == 2
#elif WGFLAG == 3
#else
#error "Invalid value of WGFLAG ! Must be 2 or 3"
#endif

// Put project specific compiler defines here
#define VERBOSE 0						// control printf compilation
// 0 for no messages, 1 for simple messages, 2 for looping messages


/* These constants should only be used in the hand.c file ! */
/* These should all work */

#if WGFLAG == 2
//#define NODOBOARD     					// prevents cluster output
#define NODIOBOARD						// prevents input, use of second board
//#define NOAOBOARD						// prevents use of Analog board

#elif WGFLAG == 3
//#define NODOBOARD     					// prevents cluster output
//#define NODIOBOARD						// prevents input, use of second board
#define NOAOBOARD						// prevents use of Analog board
#endif

/* This controls compiling of support for the Windows API SDK headers */
#define SDK	1							// requests windows.h of some level
// 0 for no SDK, 1 for extra lean, 2 for lean and mean, 3 for full

/* Control GPIB behavioe (not all defines work: use SOFTGPIB ) */
//#define NOGPIB							// prevent RF buffers and output
#define SOFTGPIB						// requests software only GPIB control
//#define SYNCHGPIB						// requests output->input->message GPIB
//#define ENDCALLBACK					// rquests use of post run message

/* These modify program behavior duing GPIB handling */
/* Threading is NOT implemented ! */
//#define THREADS						// indicates use of extra threads
/* Priority ought to work */
#define PRIORITY						// indicates use of priorities
/* Lockout is imperfect, but ought to work */
//#define LOCKOUT						// indicates use of lockout calls

/*  Stolen from toolbox.h
	Thus, you must include this before toolbox.h

	The errChk and nullChk macros are useful for implementing a consistent error
	handling system.  These can macros can be place around function calls to
	force an  automatic jump to a function's error handling code when an error 
	occurs.  This is analogous to exception handling, and is even easier to use.

	Notes:
		1)  The following local declaration of an error code variable is
			made in every function in which they are used:

			int err = 0; (Any integer type will do)

		2)  Every function in which they are used contains a goto label
			named Error which precedes the error handling code for the function.

		3)  errChk and negChk are used only with integer compatable parameters,
				and capture the error code in the err variable.
		
		4)	nullChk is useful for calloc,malloc,realloc, etc. nullchk and require
				use a -1 value for err to indicate the error state.
		
		5)  require is an Assert implimentation that uses the goto Error scheme

*/							

#ifndef errChk
#define errChk(fCall) if (err = (fCall), err != 0) \
{printf("errchk File: %s  Line: %i  Err: %i\n",__FILE__,__LINE__,err); goto Error;} else 
#endif

// negChk is designed to ignore fCall>0 values by setting err=0 in this case
#ifndef negChk
#define negChk(fCall) {if (err = (fCall), err < 0) \
{printf("negchk File: %s  Line: %i  Err: %i\n",__FILE__,__LINE__,err); goto Error;} else {err=0;}}
#endif

// null check is used on pointer so we have to cast to integer
#ifndef nullChk
#define nullChk(fCall) {if (err = (int)(fCall),err == 0) \
{printf("nullchk File: %s  Line: %i  Err: %i\n",__FILE__,__LINE__,err); err = -1; goto Error;} else {err=0;}}
#endif

#ifndef require
#define require(fCall) if (!(fCall)) \
{printf("require File: %s  Line: %i \n",__FILE__,__LINE__); err=-1; goto Error;} else 
#endif

//****************************** INCLUDE WINDOWS.H ***********************
// Must be done before CVI includes
#ifdef SDK
#if SDK>0

#if SDK<2
#define   WIN32_EXTRA_LEAN     
#endif

#if SDK<3
#define   WIN32_LEAN_AND_MEAN
#endif

#include "windows.h"

#endif
#endif

//****************************** CVI Includes ***********************
#include "CVIDEF.H"			// Needed to be a function panel code source
#include <formatio.h>
#include <utility.h>
#include <ansi_c.h> // This needs to be done after the above macros
#include "toolbox.h" 

//*******************

// MAGIC TYPES

// HAND function return type
typedef short handErrRet ;
// A useful, usually boolean hosting, type
typedef unsigned char dnaByte;			// Useful small storage unit
typedef unsigned int dnaTime;			// DWORD

//*******************

// MAGIC NUMBERS

// My Boolean defines					
#define dnaTrue 1			// Non-zero, meaning true
#define dnaFalse 0			// must be zero, means false


#if WGFLAG == 2
// Parameters of physical system
#define dnaAnalogChannels 10			// Hardware Parameter
// Next parameter was originally 6, the last 4 being bipolar.
// Now all ten are unipolar
#define dnaAnalogUnipolar 10			// Jumper setup on AT-AO-10 is unipolar
#define dnaDigitalOutputs 48			// Hardware Parameter
// These control data allocation for UI information
// 2 for RF plus dnaAnalogChannels
#define faceGRAPHS 12					// Number of Graphs in the UI

#elif WGFLAG == 3
// Parameters of physical system
#define dnaAnalogChannels 8				// Hardware Parameter
#define dnaAnalogUnipolar 0				// All PCI-6713 outputs are bipolar
// WG2->3
// APC was 48 (why?) now 64 !
#define dnaDigitalOutputs 64			// Hardware Parameter
// These control data allocation for UI information
// 2 for RF plus dnaAnalogChannels
#define faceGRAPHS 10					// Number of Graphs in the UI
#endif

// defines that affect face and brain
#define dnaNumParams 10					// # of arf graph function parameters
#define dnaNumUIParams 6				// # of parameters on UI for user
#define dnaReservedParams 2				// Number of universal parameters
// Thus scaling and offset are 1st "2" parameters
// The first 6 parameters are on the UI, (sometimes disabled)
// The last 4 ( = 10 - 6 ) paramters are used internally to speed calculation


// Hard code a limit here to reduce use of dynamic memory (GOTCHA, TODO)
#define dnaRFStages	16					// Max # of stages in rf buffer

// Largest Positive and Smallest Negative useful values
#define brainMinDouble -1e100
#define brainMaxDouble 1e100

// These control data allocation for UI information
#if WGFLAG == 2
#define faceCLUSTERS 16                 // Number of Clusters on UI
#elif WGFLAG == 3
#define faceCLUSTERS 12                 // Number of Clusters on UI
#endif
#define faceXYValues 32					// Number of XY Values in the UI

//*******************

// ENUMERATED TYPES
 
// These are used in clusters to specify the duration.
// Note that the enumeration constants give the unit duration in microseconds.
// This counting system allows up to 2147 seconds (35 minutes).
// (2 to the 31st power microseconds)
typedef enum {dnaUSEC=1,dnaMSEC=1000,dnaSEC=1000000} dnaTimeUnits;

// These are special values for analog panel selection in dnaCluster
// They are negative for no particualr reason
typedef enum {dnaARFContinue=-1,dnaARFStartGroup=-2,dnaARFStop=-3} 
		dnaARFSelectors;

//*******************

// These count # enum constants
/* Compare this with what is in BRAIN ? */
#define dnaNumInterps 12				// Number of dnaInterTypes values
#define dnaNumRanges 5					// # of emunerated constants, dnaRanges

// These are used in dnaARFGraph
// dnaNumInterps defined above needs to know how many values are listed here!
// Naming convention is "I" for Interpolation if x,y values are entered
// and "P" if no x,y values are entered so only Parameters are used
// All "I" come befure all "P" at the moment in this list. The listed
// order is the order they will be in the user interface control
typedef enum {dnaNoInterp,dnaILinear,dnaIStep,dnaIPoly,dnaIRat,dnaISpline,
	dnaPRamp,dnaPPulse,dnaPSquare,dnaPSine,dnaPExp,dnaPSigmoid} dnaInterpTypes;
// Adding types in the middle will caused saved files to load with wrong value
// The user will need to notice this and set it back by hand/
// So from now on, new types must only be APPENDED to the above list

// These constants are for pinning the analog values to valid ranges
typedef enum {dnaNoRange,dnaUnipolar,dnaBipolar,dnaAmplitude,
		dnaFrequency} dnaRanges;
// The valid ranges for each constant are stored in this BRIAN global

//************************

// Physical parameters (actually defined in DNA.c)

/* These probably are the same for both AT-AO-10 and PCI-6713 */
extern const double RangeMax[dnaNumRanges];
extern const double RangeMin[dnaNumRanges];
// Actually, the 1.0 is the amplifier limit, SRS has a 5.0 volt limit.
//={ 1e100, 9.998779296874,  9.99755859374, 1.0, 30.0};// max
//={-1e100,-0.001220703124,-10.00244140624, 0.0,  0.0};// min
// If you take the different of the max and min you get:
//={ 2e200, 9.999999999998, 19.99999999998, 1.0, 30.0};// Width of range

//************************

// 2a) digital UI info
//
// This is organized as an arracy of clusters.
//
// dnaClusterArray	: Properties of the array as a whole and array of clusters 
// dnaCluster 		: Properties of a cluster and an array of digital data
//	more flag information will be added to these structures as needed

// dnaCluster usually only exist as part of an array
// (or in the Clipboard structure)

// NOTE: MEMORY USED is always in BYTES of memory and refers to the amount
// of memory allocated by the corresponding pointer.  Thus it tells how
// much memory is available before realloc or calloc needs to be called again

// if (0<MEMORYUSED), do not assume the pointer is !=NULL. Always test an
// unknown pointer before de-referencing it or freeing it.
// Similarly, if (0==MEMORYUSED), do not assume that the pointer==NULL.
// If no memory is owned by the pointer, it will be set to NULL
// If an unkown pointer is not NULL, assume it points to memory it owns.

typedef struct {
// This holds the data for one "Word", the vertical cluster of data.
	dnaByte			LabelSize;			// Length of label w/o NULL
	char*			Label;				// Label
		// Some use specified label for this cluster
	int				Ticks;				// Duration divided by time units
	dnaTimeUnits	TimeUnit;			// See above for enum type
	dnaByte			EnabledQ;			// 1 if enables, 0 if disabled
	dnaARFSelectors AnalogSelector;		// see constants above
	int				AnalogGroup;		// Index of group to start
	dnaARFSelectors RFSelector;			// see constant above
	int				RFGroup;			// Index of group to start 
	int				NumberOfValues;		// 8, 16, 32, etc...
	int				MemoryUsed;			// Size of Digital allocated
	dnaByte			*Digital;			// array of digital 1 or 0 values
} dnaCluster;

// TODO : make this allocation more flexible?
// To reduce dependence on dynamic memory, I have hard coded an array here.
typedef struct {
// This is the array structure to hold all the clusters (aka "Words")
	int				NumberOfClusters;	// Length of the array of clusters
	int				NumberOfPorts;		// 1,2,4,5,6, or 8 ports may be used  
	dnaByte			OutputLabelSize[dnaDigitalOutputs];	// GOTCHA : dna.h
	char*			OutputLabels[dnaDigitalOutputs];	// GOTCHA : dna.h
//	int				GlobalOffset;		
		// Translates the UI offset to a cluster array index
	int				MemoryUsed;			// Size allocated to UIClusters
	dnaCluster		*Clusters;			// The array of BRAIN clusters
} dnaClusterArray;


// 2b) and 2c)

typedef struct {
// Channel jumper config info is stored in dnaAnalogChannelConfig struct in
// array type dnaAnalogConfigArray, implimented in BRAIN.
	short			UnipolarQ;			// 0 for Bipolar, 1 for Unipolar
	short			ExternalQ;			// 0 for Internal, 1 for External
	double 			RefVoltage;			// 10.0 for internal.
	/* UpdateMode is unused, might need to put a magic number in here */
	short			UpdateMode;			// 0,1 or 2.  We want 2 // UNUSED
} dnaAnalogChannelConfig;															


//********************
//  Refer to better documentation at top of BRAIN.c
//********************
// Forward declare a pointer dnaARFGraphPointer to a structure that is missing. 
// The compiler can then keep track of type checking and so it is happy
typedef struct dnaARFGraphS * dnaARFGraphPointer;

// The setup function has the job of certifying that a graph is ready to
// be evaulated (and for preparing extra parameters, and setting graph.Interp)
// We use the dnaARFGraphPointer to refer to dnaARFGraph
typedef int (*brainISetup) (dnaARFGraphPointer graph);    

// This is the brainInterp function that calculates an interpolated value.
// The paramerter t is the time in milliseconds from start of group output.
// The return double value is in Volts
// The parameter graph is passed to it by reference.
// We use the dnaARFGraphPointer to refer to dnaARFGraph
typedef double (*brainInterp) (double t, dnaARFGraphPointer graph);

// This is used to sequentially fill a short array with digitized value.
// Time starts at zero, goes up by timebase. The index of data is
// incremented by delta each time, and is kept less than last.
// We use the dnaARFGraphPointer to refer to dnaARFGraph
typedef int (*brainIFillShort) (dnaARFGraphPointer graph, short *data,
	int delta, int last, double timebase, int board, int channel);    
//*********************

typedef struct {
// Each interp type has a record of this structure in an array InterpData
// in BRAIN (one for each of linear, polynomial, spline, etc....)
	dnaInterpTypes	ID;					// dnaInterpType being defined
	char *			Label;				// Label to use in controls
	dnaByte			ValuesEnabledQ;		// Whether to accept XY Values
	int				NumParams;			// How many extra parameters to accept
	char *			ParamLabel[dnaNumParams];	// Name of each parameter
	double			MinimumValue[dnaNumParams];	// Minimum values for each
	brainISetup 	ISetup;				// Function to use for setup
	brainInterp		Interp;				// Default evaluation function
	brainIFillShort	IFillShort;			// Used to fill buffer quickly
} brainInterpData;


//******************** 

// Short data type used to be compatable with analog functions for DAQ
// I use double to be compatable with analysis functions.

// dnaARFGraphS is a structure type, see typedef below for explanation of why
struct dnaARFGraphS {
// This is what holds the data for a graph in a group, see typedef below
// This first bit is setup by the user with FACE.c
// Some per group/per channel header information is duplicated in each
// graph in each cluster, instead of being listed as an array type
// within the dnaARFGroup or dnaARFGroupArray structure below.
// The "dnaRanges Range" enumerated type is an example of this per channel.
// The "double Duration" type is an example of this per group
// This is done for effient encapsulation so that many functions need'
// only be passed (a pointer to) the dnaARFGraph structure to have all
// the needed information.
	int				NumberOfValues;		// Number of X,Y value pairs used
//	dnaByte			ClearAfterQ;		// Whether to follow with zero Volts
	double			P[dnaNumParams];	// Used as interpolation parameters
	double			XMin,XMax;			// Used to validate entries
	double			YMin,YMax;			// Used to validate entries  
	int				RangeIndex;				// Selects allow range for scaled Y
	dnaInterpTypes	InterpType;			// Type of Interpretation done
	int				ValueMemoryUsed;	// Size of XValues or YValues
	double			*XValues;			// Temporal X values
	double			*YValues;			// Voltage/Frequency Y values

// This last bit is setup by BRAIN.c
	int				NumberOfInterps;	// Number of values to interpret
	dnaByte			InterpReadyQ;		// Ready to interpolate or not
	brainInterp		Interp;				// pointer to function to use
	double			Duration;			// In milliseconds
	double			*XScaled;			// Scaled to match duration
	double			*YScaled;			// Scale by affine transform
	double			*SplineValues;		// Need for Spline interpolations
	int				InterpMemoryUsed;	// Size of InterpXValues or Y
	double			*InterpXValues;		// Large array for display/generation
	double			*InterpYValues;		// Large array for display/generation
} ;

typedef struct dnaARFGraphS  dnaARFGraph;	
	// defined as a struct and then a typedef as a way to allow
	// fields in structure to be of a type that is a
	// pointer to a function Interp that takes a pointer to this structure as
	// a parameter. Or something hairy like that.

typedef struct {
// This is an analog or rf group of graphs.
	dnaByte			LabelSize;			// Length of the name w/o NULL
	char *			Label;				// Name for the Group
	int				Ticks;				// Reolution divided by TimeUnit
	dnaTimeUnits	TimeUnit;			// See above for enum type
	int				TicksD;				// Duration divided by TimeUnitD
	dnaTimeUnits	TimeUnitD;			// See above for enum type
	dnaByte			NumberOfGraphs;		// Number of Graphs used
	int				MemoryUsed;			// Size of memory allocated
	dnaARFGraph		*ARFGraphs;			// Array of Graphs
} dnaARFGroup;

typedef struct {
// This is all the analog groups or all the rf groups
	int				NumberOfGroups;		// Number of Graph Groups
	dnaByte			NumberOfGraphs;		// 2 for RF, up to 10 for Analog
	dnaByte			*EnabledQ;			// Whether a given graph index is active
	dnaByte			LabelSize[dnaAnalogChannels];	
		// Length of labels w/o NULL
	char *			Labels[dnaAnalogChannels];	// Labels of the graphs
	int				MemoryUsed;			// Memory size of ARFGroups
	dnaARFGroup		*ARFGroups;			// Array of Groups
} dnaARFGroupArray;


//********************

typedef union {					
// Used in dnaBufferDigital
// This is used to make more compatable
	void			*asVoid;
	dnaByte			*asU8;
	unsigned short	*asU16;
	unsigned long 	*asU32;	    // This is what i typically use
} dnaIntPointers;

typedef struct {
// This holds the data to be sent to a digital upon execution
	int				Pos;				// Position within buffer
	int				TimeBase;			// in microseconds
	int				NumberOfLines;		// Number of bits in a word
	int				NumberOfPorts;		// Number of 8 bit ports
	int				NumberOfWords;		// Number of words in used buffer
	int				MemoryUsed;			// Memory allocated to buffer
	int				NumberOfBytes;		// Amount of buffer memory used
	dnaIntPointers	Buffer;				// Pointer to buffer
} dnaBufferDigital;

//********************

// Short data type to be compatable with analog output board functions
typedef union {					
// Used in dnaBufferAnalog
// This is used to make life more difficult. TODO : Remove from program ?
	short			bipolar;			// -2408 to 2047
	unsigned short	unipolar;			// 0 to 4095
} dnaAnalogValue;

typedef struct {
// This holds the data for the analog output card execution
	int				Pos;				// Position within buffer
	int				NumberOfOutputs;	// Number of outputs enabled
	short			ChannelVector[dnaAnalogChannels];	// List of outputs
	int				NumberOfWords;		// Number of updates in buffer
	int				NumberOfBytes;		// Amount of buffer memory used
	int				MemoryUsed;			// Memory allocated to buffer
	dnaAnalogValue	*Buffer;
} dnaBufferAnalog;

//********************

// For RF we need an array of amplitude and frequency values.
// We could use double values (8 bytes) or preconvert to string
// values (also about 8 to 10 bytes).
// A 60ms timebase for 180 seconds with 9 character for ampliture + NULL
// and the same for frequency is 60,000 bytes.  Peanuts.
// 
// 
// Thus the plan is to preconvert to the string format (speeds update processing)
//
// The format string and conversion will be handled by BRAIN or MOUTH
//

//#if defined SOFTGPIB					// Used for tightloop control

typedef struct {
// If we have this much time, we can relax a bit
	dnaTime			CriticalTime;		// Used to control UI during run
// current values
	int				CurrentStage;		// Index of current stage
	dnaTime			TargetTime;			// time for next action
	dnaTime			CallTimebase;		// current command timing
	int				CommandCounter;		// Number of Commands left in stage
// array of all the values
	int				NumberOfStages;		// # of stages in use
	dnaTime			StartTimes[dnaRFStages]; // Time to start each stage
	dnaTime			RFTimebase[dnaRFStages]; // Update interval of each stage
	int				Commands[dnaRFStages];	// # of commands in each stage
// constants that are useful
	int				AmpIndex;			// 0, Index in RFGroups of AmpGraph
	int				FreqIndex;			// 1, Index in RFGroups of FreqGraph
// Where we are in the big flat character buffer 
	int				PosByte;			// Position within buffer in Bytes
	int				PosCommand;			// Position within buffer in Commands
	int				NumberOfCommands;	// Total # of commands in buffer
	int				MemoryUsed;			// Size of memory allocated
	char			*Buffer;			// The buffer itself
} dnaBufferRF;

// dnaAuxillaryInfo is used to gather all the session information
// which needs to be saved to the *.wg files. This interface
// structure helps keep things organized.
typedef struct {
	// File header information
	int				FileVersion;		// Describes file structure
	char *			FileS;				// Filename
	char *			ClassS;				// A class name (a set of runs)
	int				Index;				// Index within this class
	char * 			DateS;				// date written to file
	char * 			TimeS;				// time written to file
	char * 			CommentS;		   	// User comment
	char *			AutoCommentS;		// Automated comment
	// Cluster window settings
	int				GlobalOffset;
	int				OnlyEnabledQ;
	int				RepeatRunQ;
	int				CameraTriggerQ;
	int				UserTicks;
	int				UserTimeUnit;
	int				GTS;				// RF Start time offset
	// ARF window settings
	int				AnalogIndex;
	int				RFIndex;
	int				GraphSelected;
	int				PinRangeQ;
	dnaRanges		Range[faceGRAPHS];		// NOT SAVED TO FILE
	double			UserMin[faceGRAPHS];	// GOTCHA
	double			UserMax[faceGRAPHS];
} dnaAuxillaryInfo;
 
typedef struct {
	dnaByte			IdleValues[dnaDigitalOutputs];	// Digital idle word
	double			InitAnalog[dnaAnalogChannels];	// Idle/Initial analog (V)
	double			RFAmplitude;					// (Vpp)
	double			RFOffset;						// (V)
	double			RFFrequency;					// (MHz)
} dnaInitIdle;
 
//********************

//	These are used in the FACE module to hold handles of new controls
//	added programmatically. Procedure to initialize these are handled
//	in the face module.

typedef struct {						// Handles, Handles, Handles, pointer
// This holds the UI information from when the interface was built up
	int				INDEX;				// Index in main array displayed
	int 			LABEL;
	int				ENABLEDQ;
	int				TICKS;
	int				TIMEUNIT;
	int				ANALOGRING;
	int				ANALOGGROUP;
	int				RFRING;
	int				RFGROUP;
	dnaCluster		*ClusterData;		// Pointer to BRAIN.c cluster entry 
} faceClusterHandles;

typedef struct {
// This holds the UI information from when the interface was built up
/* CEK 2000/04/07
 * Add handle "vframe" to child window for vertically scrolling canvas
 * Also need handle "CANVAS" to refer to new copy of canvas
 */
	int				panel;				// the panel the contols are on.
	int				vframe;				// handle to child window
	int				GlobalOffset;		// Value of GLOBALOFFSET control
	int				OnlyEnabledQ;		// Value of ONLYENABLEDQ control
	int				NumberOfOutputs;	// 8,16,32,40,48
	int				OUTPUTLABEL[dnaDigitalOutputs];	// GOTCHA dnaDigitalOutputs
	int				IDLE[dnaDigitalOutputs];
	int				CANVAS;				// handle to canvas grid
	int				Top,Left;			// Corner of canvas
	int				DigitalWidth;		// Typically 70
	int				DigitalHeight;		// Typically 19
	int				NumClusters;		// Number of Clusters in panel
	int				MemoryUsed;			// Memory allocated to hold 
 	faceClusterHandles	*Cluster;
} faceClusterArray;


typedef struct {
// This holds the UI information from when the interface was built up
// GOTCHA : All of the array lengths can be considered gotcha values
	int				panel;				// the panel the controls are on.
	int				NumValues;			// Number of UI XY Values
	int				XVALUE[faceXYValues];	// Handles of XValues
	int				YVALUE[faceXYValues];	// Handles of YValues
	int				NumParams;			// Number of Parameters in the UI
	int				PARAM[dnaNumUIParams];	// GOTCHA from top of DNA.h
	int				NumGraphs;			// Number of Graphs in the UI
	int				GraphSelected;		// Which graph is selected, -1 for none
	int				GRAPH[faceGRAPHS];	// Handles of the UI graphs, GOTCHA
	int				ENABLEDQ[faceGRAPHS];	// Handles of the UI boxes, GOTCHA
	int				PLOT[faceGRAPHS];	// Handles for values plots
	int				IPLOT[faceGRAPHS];	// Handles for interps plots
	int				INITIALANALOG[faceGRAPHS];	// handles
	int				MINIMUMANALOG[faceGRAPHS];	// handles
	int				MAXIMUMANALOG[faceGRAPHS];	// handles
	int				Top,Left,LTop;		// corner of first graph
	int				GraphWidth;
	int				GraphHeight;
	int				AnalogIndex;
	int				RFIndex;   
	dnaARFGroup		*AnalogGroup;		// Pointer to analog data set
	dnaARFGroup		*RFGroup;			// Pointer to RF data set
} faceARFGraphs;

typedef struct {
// A right click handler will fill this with relevant local
// details as to which item was clicked on before calling up the
// pop-up context menu
	int				MENU;				// Menu bar handle;
	int				MenuID;				// Which menu was called last;
	int				panel;				// Which panel it was called from
	int				control;			// Which control it was called from
	void *			callback;			// index (callback data) of control
	int				eventData1;			// from control callback
	int				eventData2;			// from control callback
} facePOPUPData;

typedef struct {
// This is used to hold pointers to temporary copies used during
// copy/cut/paste operations.
	dnaCluster		*Cluster; // Entire "Word"
	dnaARFGraph		*Graph;   // Analog or RF Graph
	double			x,y;	  // XY point
} faceClipboard;

#ifdef THREADS
extern CRITICAL_SECTION critical;
extern int criticalready;
#endif // THREADS

//*** End of my entries in DNA.h ***
#endif
//*** Beginning of Function Panel auto-definitions ***

int CVIFUNC dna_InitClusterArray (dnaClusterArray *clusterArray,
								  int numberOfClusters, int ports);

int CVIFUNC dna_ReInitClusterArray (dnaClusterArray *clusterArray,
									int numberOfClusters, int ports, int option);

dnaCluster * CVIFUNC dna_NewCluster (char *clusterName,
									 int numberofDigitalValues);

int CVIFUNC dna_SetOutputLabel (dnaClusterArray *clusterArray, int index,
								char *outputName);

int CVIFUNC dna_InsertCluster (dnaClusterArray *clusterArray, int index);

int CVIFUNC dna_DeleteCluster (dnaClusterArray *clusterArray, int index);

int CVIFUNC dna_InitCluster (dnaCluster *clusterPointer,
							 int numberofDigitalValues);

int CVIFUNC dna_ReInitCluster (dnaCluster *clusterPointer,
							   int numberofDigitalValues, int option);

int CVIFUNC dna_SetClusterLabel (dnaCluster *clusterPointer, char *clusterName);

int CVIFUNC dna_SetClusterLength (dnaCluster *clusterPointer,
								  int numberofDigitalValues, int option);

int CVIFUNC dna_CopyClusterData (dnaCluster *target, dnaCluster *source);

int CVIFUNC dna_InitARFGroupArray (dnaARFGroupArray *groupArray, int groups,
								   dnaByte graphsinaGroup);

int CVIFUNC dna_DeleteGroup (dnaARFGroupArray *groupArray, int index);

int CVIFUNC dna_SetGraphLabel (dnaARFGroupArray *groupArray, int graphIndex,
							   char *graphName);

int CVIFUNC dna_ReInitARFGroupArray (dnaARFGroupArray *groupArray, int groups,
									 dnaByte graphsinaGroup, int values,
									 int option);
									 
int CVIFUNC dna_InitGroup (dnaARFGroup *groupPointer, char *groupName,
						   dnaByte graphs);

int CVIFUNC dna_ReInitGroup (dnaARFGroup *groupPointer, char *groupName,
							 dnaByte graphs, int values, int option);

int CVIFUNC dna_CopyARFGroup (dnaARFGroup *destination, dnaARFGroup *source);

int CVIFUNC dna_SetGroupLabel (dnaARFGroup *groupPointer, char *groupName);

int CVIFUNC dna_SetGroupGraphs (dnaARFGroup *groupPointer, dnaByte graphs);

int CVIFUNC dna_DuplicateGraphValue (dnaARFGraph *graphPointer, int index);

int CVIFUNC dna_DeleteGraphValue (dnaARFGraph *graphPointer, int index);

int CVIFUNC dna_CalcXYValuesMinMax (dnaARFGraph *graphPointer, int which);

int CVIFUNC dna_ScaleGraphValues (dnaARFGraph *graphPointer);

int CVIFUNC dna_PinGraphValues (dnaARFGraph *graphPointer);

int CVIFUNC dna_InitARFGraph (dnaARFGraph *graphPointer);

int CVIFUNC dna_ReInitARFGraph (dnaARFGraph *graphPointer, int values,
								int option);

int CVIFUNC dna_CopyARFGraph (dnaARFGraph *destination, dnaARFGraph *source);

int CVIFUNC dna_TouchARFGraph (dnaARFGraph *graphPointer);

int CVIFUNC dna_SetGraphValues (dnaARFGraph *graphPointer, int values);

int CVIFUNC dna_SetupBufferRF (dnaBufferRF *buffer, int bytes);

int CVIFUNC dna_LengthBufferRF (dnaBufferRF *buffer);

int CVIFUNC dna_AdvanceBufferRF (dnaBufferRF *buffer);

int CVIFUNC dna_AdvanceRunBufferRF (dnaBufferRF *buffer);

int CVIFUNC dna_AdvanceStage (dnaBufferRF *buffer, unsigned int startTime,
							  unsigned int timeBase);	  
							  
int CVIFUNC dna_AdvanceRunStage (dnaBufferRF *buffer);

int CVIFUNC dna_InitBufferDigital (dnaBufferDigital *buffer);

int CVIFUNC dna_SetupBufferDigital (dnaBufferDigital *buffer, int lines,
									int words);

int CVIFUNC dna_InitBufferAnalog (dnaBufferAnalog *buffer);

int CVIFUNC dna_SetupBufferAnalog (dnaBufferAnalog *buffer, int outputs,
								   int words);

int CVIFUNC dna_ResetInitIdle (dnaInitIdle *idle);

int CVIFUNC dna_InitAuxInfo (dnaAuxillaryInfo *auxInfo);

int CVIFUNC dna_ResetAuxInfo (dnaAuxillaryInfo *auxInfo);

double CVIFUNC dna_SetRangeMin (dnaAuxillaryInfo *auxInfo, int index,
								double value);

double CVIFUNC dna_SetRangeMax (dnaAuxillaryInfo *auxInfo, int index,
								double value);



int CVIFUNC dna_InitBufferRF (dnaBufferRF *buffer);

