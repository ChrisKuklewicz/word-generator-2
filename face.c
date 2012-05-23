#include "dna.h"						// Basic data types come first
#include "scroll.h"
#include "hand.h"
#include "face.h"						// UIR constant file
#include "facetwo.h"					// Hold constants and (extern) globals
#include "brain.h"						// Function prototypes, BRAIN globals
#include "dde.h"
#include "recent.h"
#include <userint.h>

//TESTING
//#include "memory.h"
//#define BUGHUNT2
// UIR panel hanndle variables 
//
//This is the FACE of Word Generator 2.
//
//Here is the user interface, much of which is created programatically.
//
//Note: execution begins in this file
//
//As this is an event driven interface, all execution starts here (almost)
//and so all the error returning that is going on ends up here as well.
//
//Error handling will likely be done in the future. for now it is
//largely ignored.
//

					   
#include <cvirte.h>
	// Needed if linking in external compiler; harmless otherwise 
	// Redundant since also included in FACE.h automatically

static int arf=-1;
static int wg=-1;
static int auxwait=-1;					// Handle to window for J5
static int auxwaitQ=dnaFalse;			// True when displayed

// UI constants that are only needed in FACE.c 

static const int NumColors = 4;

/* TODO FIXME : The orange or green is too bright, it rounds off to white in Black
 * and white printing.  Make it darker. */
								//    RRGGBB  RR=Red,GG=Green,BB=Blue component
static const int Red 			= 0x00FF0000;
static const int Blue 			= 0x000000FF;
//static const int Orange 		= 0x00FF8200;  // This was too bright
static const int Orange 		= 0x00FF7000;

static const int Green 			= 0x00009a00;
static const int ColorBorder 	= 0x00000000;
static const int ColorGraphOn   = 0x00009a00;
static int ColorCanvas=0;     			// loaded from default
static int ColorGraphOff=0;				// loaded from default

//static const int Colors[4] = {0x00FF0000,0x000000FF,0x00FF8000,0x00009900};
// 65536 color mode needs:
static const int Colors[/*NumColors*/4] = {/*Red*/0x00FF0000,/*Blue*/0x000000FF,
									/*Orange*/0x00FF7000,/*Green*/0x00009a00};  

// Hand Coded parameters for making the GUI 
// May be from ini file in the future 

/* UIClusters is the number of words on the panel at a time */
static const int UIClusters = faceCLUSTERS;

#if WGFLAG == 2
/* Why is this 48 and not 32 ? */
/* UIDigitalOutputs is the number of outputs displayed  */
static const int UIDigitalOutputs = 48;
/* UIUsedOutputs is the number of outputs enabled */
static const int UIUsedOutputs = 48;

#elif WGFLAG == 3
/* UIDigitalOutputs is the number of outputs displayed  */
// WG2->3 APC, was 48 (why?) now 64
static const int UIDigitalOutputs = 64;	
/* UIUsedOutputs is the number of outputs enabled */
// WG2->3 APC, was 48 (why?) now 64
static const int UIUsedOutputs = 64;
#endif

/* UIXYValues is the number of x,y points displayed for editing graphs */
static const int UIXYValues = faceXYValues;

// These labels are the defaults 

// GOTCHA : 13 is minimum needed length of string bufferd to hold values.
#if WGFLAG == 2
static char GraphLabelPrefix[faceGRAPHS][13] = {"AO 0","AO 1","AO 2",
		"AO 3","AO 4","AO 5","AO 6","AO 7","AO 8","AO 9",
		"RF Amplitude","RF Frequency"};
#elif WGFLAG == 3
static char GraphLabelPrefix[faceGRAPHS][13] = {"AO 0","AO 1","AO 2",
		"AO 3","AO 4","AO 5","AO 6","AO 7",
		"RF Amplitude","RF Frequency"};
#endif

/* This allows for easier customiztion of terms */

static const char Unknown[] = "Unknown";
static const char Unused[] = "Unused";
static const char XValueStr[] = "X Values";
static const char YValueStr[] = "Y Values";

// These are the major FACES.c global structures 

static faceARFGraphs ARFGraphs;
static faceClusterArray ClusterArray;
static facePOPUPData POPData;
static faceClipboard Clipboard;


//************ Forward declarations ****************
int changedGraphSelection(int index);
int enableXYValues(void);
int AddRecentFilename(char * filename);
//**************************************************


//*********************************************************************
//																	   
//																	   
//					 BRAIN Global and other Accessors						   
//																	   
//																	   
//*********************************************************************
// These hide the dichotomy of storage from other functions 

// index is a cluster index 
dnaCluster * GetCluster(int index)
{
	if (Clusters.Clusters)
		if (index < Clusters.NumberOfClusters)
			return &(Clusters.Clusters[index]);
		else
			return 0;
	else
		return 0;
}

// This detects whether "invalidate pointers" was called 
// index is a graph index 
dnaByte ValidGroup(int index)
{
	if (-1==index) {
		return dnaFalse;					// Paranoia
	}
	if (index < dnaAnalogChannels) {
		if (ARFGraphs.AnalogGroup)
			return dnaTrue;
		else
			return dnaFalse;
	}
	else {
		index -= dnaAnalogChannels;
		if (ARFGraphs.RFGroup)
			return dnaTrue;
		else
			return dnaFalse;
	}								
}

// index is a graph index 
// index is set to the corect value for the BRAIN group int the array 
dnaARFGroupArray *GetGroupArray(int *index)	//note index is modified here
{				   
	if (!index)								// Paranoia
		return 0;
	if ((*index) < dnaAnalogChannels)
		return (&AGroups);
	else {
		(*index) -= dnaAnalogChannels;
		if ((*index)<RFGroups.NumberOfGraphs)
			return (&RFGroups);
		else
			return 0;						// Paranoia
	}
}

// index is a graph index 
// index is set to the corect value for the BRAIN group 
dnaARFGroup *GetGroup(int *index)
{
	if (ValidGroup(*index)) {
		if ((*index) < dnaAnalogChannels)
			return (ARFGraphs.AnalogGroup);
		else {
			(*index) -= dnaAnalogChannels;
			if ((*index)<RFGroups.NumberOfGraphs)
				return (ARFGraphs.RFGroup);
			else
				return 0;						// Paranoia
		}
	}
	else 
		return 0;
}

// index is a graph index 
// index is NOT chaged 
dnaARFGraph *GetGraph(int index)
{
	dnaARFGroup * group;
	group=GetGroup(&index);
	if (!group)
		return 0;
	if ((group->ARFGraphs) && (index<group->NumberOfGraphs))
		return &(group->ARFGraphs[index]);
	else
		return 0;
}


// index is a graph index 
dnaByte GetEnabledQ(int index)
{
	dnaARFGroupArray *groupArray;
	if (index<0)
		return dnaFalse;
	groupArray=GetGroupArray(&index);
	if (index>groupArray->NumberOfGraphs)
		return dnaFalse;
	if (!groupArray)
		return dnaFalse;						// Paranoia
	if (groupArray->EnabledQ)
		return (groupArray->EnabledQ[index]);
	else
		return dnaFalse;						// Paranoia
}

int GetClusterIndex(int index)
{
	int err=0,BrainIndex=0;
	nullChk(ClusterArray.Cluster);
	errChk(GetCtrlVal(ClusterArray.panel,ClusterArray.Cluster[index].INDEX,
			&BrainIndex));
Error:
	return (err?-1:BrainIndex);
}

//*********************************************************************
//																	   
//																	   
//   			 Functions to access control states					   
//																	   
//																	   
//*********************************************************************

dnaByte	PinValuesQ(void)
{
	int MyValue=dnaFalse;
	GetCtrlVal (ARFGraphs.panel, ARF_PINRANGEQ, &MyValue);
	return MyValue;
}

dnaByte	RepeatRunQ(void)				// FACETWO.h
{
	int MyValue=dnaFalse;
	GetCtrlVal (ClusterArray.panel, WG_REPEATRUN, &MyValue);
	return MyValue;
}

dnaByte	CameraTriggerQ(void)			// FACETWO.h
{
	int MyValue=dnaFalse;
	GetCtrlVal (ClusterArray.panel, WG_TRIGGERRUN, &MyValue);
	return MyValue;
}

//*********************************************************************
//																	   
//																	   
//   				 Functions to redisplay data					   
//																	   
//																	   
//*********************************************************************

//
//	Parameter index is the FACE UI Graph index, assumed to be valid

//ShutoffQ false declares that the checkbox should be 
//	enabled in an unchecked state.


//ShufoffQ declares the graph at index is not a possible functioning unit
//of the interface. It need never even exist as a control.

int disableGraph(int index,int ShutoffQ)
{
	int err=0;
	int panel,GRAPH;					// short handles....
	
	// dim the graph and NOT set its label to the default value 
	errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.GRAPH[index], 
			ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.GRAPH[index],
			ATTR_LABEL_TEXT,GraphLabelPrefix[index]));
	errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.GRAPH[index],
			ATTR_LABEL_LEFT,VAL_AUTO_CENTER));
//	errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.ENABLEDQ[index],dnaFalse));

	// Do not leave erroneous plots on the screen....
	panel=ARFGraphs.panel;				// FACES.h constant handle
	GRAPH=ARFGraphs.GRAPH[index];		// FACES.c constant handle
	if (ARFGraphs.PLOT[index]>=0) {		// there is so delete it...
		DeleteGraphPlot (panel, GRAPH, ARFGraphs.PLOT[index], VAL_DELAYED_DRAW);
		ARFGraphs.PLOT[index]=-1;		// Invalidate handle
	}
	if (ARFGraphs.IPLOT[index]>=0) {	// there is so delete it...
		DeleteGraphPlot (panel, GRAPH, ARFGraphs.PLOT[index], VAL_DELAYED_DRAW);
		ARFGraphs.IPLOT[index]=-1;		// Invalidate handle
	}

	// ensure the checkbox has the right value 
	errChk(SetCtrlVal(ARFGraphs.panel, ARFGraphs.ENABLEDQ[index],dnaFalse));
	if (ShutoffQ)						// If terminating with prejudice 
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.ENABLEDQ[index], 
				ATTR_DIMMED, dnaTrue));
	else								// We can let them click on it		
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.ENABLEDQ[index], 
			ATTR_DIMMED, dnaFalse));
Error:
	return err;
}


//
//	Parameter i is the FACE UI cluster offset
//	Parameter j is the digital line
//	Parameter Value is obvious (i hope)
//
//	This assumes i and j are valid !!!!!!
//
//	It always plots onto the canvas.

int setDigitalValue(int i, int j, dnaByte Value)
{
	int err=0;
	int color;
	Rect R;

	R.height=ClusterArray.DigitalHeight;
	R.width=ClusterArray.DigitalWidth;
	R.left=R.width*i;
	R.top=R.height*j;
	R.top++;
	R.left++;
	R.width-=2;
	R.height-=2;

	if(Value) 
		color=Colors[j%NumColors];
	else 
		color=ColorCanvas;

	errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.CANVAS,
			ATTR_PEN_FILL_COLOR, color));
	
	errChk(CanvasDrawRect(ClusterArray.vframe, ClusterArray.CANVAS, R, VAL_DRAW_INTERIOR));

Error:
	return (err?-1:0);
}


//	Parameter i is the UI cluster index, assumed to be valid.
int disableCluster(int i, dnaByte ClearDigitalQ)
{
	int err=0;
	int j;								// looping variable
	faceClusterHandles *fPtr;
	int panel;
	
	nullChk(ClusterArray.Cluster);
	fPtr=&ClusterArray.Cluster[i];		// desired structure with handles

// See if Already disabled 

	errChk(GetCtrlVal (ClusterArray.panel, fPtr->INDEX, &j));
	if(-1==j) {
		err=0;
		goto Error;
	}
	
	panel = ClusterArray.panel;
//	errChk(SetCtrlAttribute(panel,fPtr->INDEX, 		ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->LABEL, 		ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->ENABLEDQ,		ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->TICKS, 		ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->TIMEUNIT,		ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->ANALOGRING, 	ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->ANALOGGROUP,	ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->RFRING, 		ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel,fPtr->RFGROUP,		ATTR_DIMMED, dnaTrue));
	
	errChk(SetCtrlVal(panel,fPtr->INDEX, 			-1));
	errChk(SetCtrlVal(panel,fPtr->LABEL,	 		Unknown));
	errChk(SetCtrlVal(panel,fPtr->ENABLEDQ, 		dnaFalse));
	errChk(SetCtrlVal(panel,fPtr->TICKS, 			1));
	errChk(SetCtrlVal(panel,fPtr->TIMEUNIT,		dnaSEC));
	errChk(SetCtrlVal(panel,fPtr->ANALOGRING,		dnaARFContinue));
	errChk(SetCtrlVal(panel,fPtr->ANALOGGROUP,	-1));
	errChk(SetCtrlVal(panel,fPtr->RFRING,			dnaARFContinue));
	errChk(SetCtrlVal(panel,fPtr->RFGROUP,		-1));
	
	fPtr->ClusterData=0;
	
	if (ClearDigitalQ) {
		for (j=0; j<ClusterArray.NumberOfOutputs; j++) {
			errChk(setDigitalValue(i,j,dnaFalse));
		}
	}
	
	err=0;
Error:
	return (err?-1:0);
}

//
//	Parameter i is the FACE UI cluster offset
//	Parameter j is the BRAIN cluster data offset
	
//	This assumes i and j are valid !!!!!!
	
//	It always writes the digital values to the canvas!

int loadCluster(int i,int j)
{
	int k,err=0;
	int panel,max;
	faceClusterHandles *fPtr;
	dnaCluster *dPtr;
	char name[32];  // GOTCHA FIXME : make this less hard coded
	
#if defined BUGHUNT
	printf("i=%d, (i<NumClusters)=%d\n",i,(i<ClusterArray.NumClusters));
	if (i>=15) printf("loadCluster{ClusterArray.NumClusters=%d}\n",ClusterArray.NumClusters);
#endif
	require(i<ClusterArray.NumClusters);
	nullChk(ClusterArray.Cluster);
	fPtr=&ClusterArray.Cluster[i];		// FACE.c structure
	nullChk(fPtr);
	dPtr=GetCluster(j);					// BRAIN.c structure
	nullChk(dPtr);
	
	fPtr->ClusterData=dPtr;				// Remember where data is taken from
	panel = ClusterArray.panel;

#if defined BUGHUNT
	if (i>=15) printf("loadCluster{Set dim state}\n");
#endif
	// set dim state 
	errChk(SetCtrlAttribute (panel,fPtr->INDEX, 		ATTR_DIMMED, dnaFalse));
	errChk(SetCtrlAttribute (panel,fPtr->LABEL, 		ATTR_DIMMED, dnaFalse));
	errChk(SetCtrlAttribute (panel,fPtr->ENABLEDQ, 	ATTR_DIMMED, dnaFalse));
	errChk(SetCtrlAttribute (panel,fPtr->TICKS, 		ATTR_DIMMED, dnaFalse));
	errChk(SetCtrlAttribute (panel,fPtr->TIMEUNIT, 	ATTR_DIMMED, dnaFalse));
	errChk(SetCtrlAttribute (panel,fPtr->ANALOGRING,	ATTR_DIMMED, dnaFalse));
	errChk(SetCtrlAttribute (panel,fPtr->RFRING,		ATTR_DIMMED, dnaFalse));
#if defined BUGHUNT
	if (i>=15) printf("loadCluster{Load values}\n");
#endif
	
	// set values 
	errChk(SetCtrlVal (panel, fPtr->INDEX, j));
	if (!dPtr->Label) {
		negChk(Fmt(name,"%s<#%d",j));
		errChk(dna_SetClusterLabel (dPtr, name));
	}
	errChk(SetCtrlVal (panel, fPtr->LABEL, dPtr->Label));

	errChk(SetCtrlVal (panel, fPtr->ENABLEDQ,		dPtr->EnabledQ));
	errChk(SetCtrlVal (panel, fPtr->TICKS, 		dPtr->Ticks));
	errChk(SetCtrlVal (panel, fPtr->TIMEUNIT,		dPtr->TimeUnit));
	errChk(SetCtrlVal (panel, fPtr->ANALOGRING,	dPtr->AnalogSelector));
	errChk(SetCtrlVal (panel, fPtr->ANALOGGROUP,	dPtr->AnalogGroup));
	errChk(SetCtrlVal (panel, fPtr->RFRING,		dPtr->RFSelector));
	errChk(SetCtrlVal (panel, fPtr->RFGROUP,		dPtr->RFGroup));

	// Conditional dim state of Analog and RF Group indices 
	switch (dPtr->AnalogSelector) {
		case dnaARFContinue :
		case dnaARFStop :
			errChk(SetCtrlAttribute(panel,fPtr->ANALOGGROUP,
					ATTR_DIMMED,dnaTrue));
			break;
		case dnaARFStartGroup:
			errChk(SetCtrlAttribute(panel,fPtr->ANALOGGROUP,
					ATTR_DIMMED,dnaFalse));
			break;
	}
	switch (dPtr->RFSelector) {
		case dnaARFContinue :
		case dnaARFStop :
			errChk(SetCtrlAttribute (panel, fPtr->RFGROUP, 
					ATTR_DIMMED, dnaTrue));
			break;
		case dnaARFStartGroup :
			errChk(SetCtrlAttribute (panel, fPtr->RFGROUP, 
					ATTR_DIMMED, dnaFalse));
			break;
	}
	
	// Check to make sure we assign the right amount of data to canvas 
#if defined BUGHUNT
	if (i>=15) printf("loadCluster{SetDigitalValue loop}\n");
#endif
	max = dPtr->NumberOfValues;
	if (max>ClusterArray.NumberOfOutputs)
		max=ClusterArray.NumberOfOutputs;
	for (k=0;k<max;k++) {
#if defined BUGHUNT
		if (i>=15) printf("loadCluster{SetDigitalValue(%d,%d,%d)}\n",i,k,dPtr->Digital[k]);
#endif
		errChk(setDigitalValue(i,k,dPtr->Digital[k]));
	}
Error:
	return (err?-1:0);
}

// 
//	Clusters.NumberOfClusters should be correct (vis a vis offset).
/* This sets both the numerical control and the horiz. scroll bar */
int changedGlobalOffset(int offset)
{
	int err=0;
	int i,j;								// looping
	int n,max;							// Short names for easy of use;
	dnaCluster *cPtr;
	
#if defined BUGHUNT
	printf("changedGlobalOffset{}\n");
#endif
	n=Clusters.NumberOfClusters;		// Number that exist in memory
	if (n<=offset) {					// Paranoia
		err=-1;
		goto Error;
	}
	if (offset<0) {						// No Clusters, cannot handle
		err=-1;
		goto Error;
	}
	ClusterArray.GlobalOffset=offset;   // First to display
	errChk(SetCtrlVal (ClusterArray.panel, WG_GLOBALOFFSET, offset));
	errChk(ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL, ATTR_SB_VALUE,offset));
	if (ClusterArray.OnlyEnabledQ) {
#if defined BUGHUNT
		printf("changedGlobalOffset{OnlyEnabledQ True}\n");
#endif
		i=0;							// FACE UI index, ClusterArray
		j=offset;						// BRAIN index, Clusters
		cPtr=GetCluster(j);
		while ((i<ClusterArray.NumClusters) && (j<n)) {
			if (cPtr->EnabledQ) {
				errChk(loadCluster(i,j));
				i++;
			}
			j++;
			cPtr=GetCluster(j);
		}
		while (i<ClusterArray.NumClusters) {
			errChk(disableCluster(i,dnaTrue));
			i++;
		}
	}
	else {
#if defined BUGHUNT
		printf("changedGlobalOffset{OnlyEnabledQ False}\n");
#endif
		max = offset+ClusterArray.NumClusters-1; // Last index display could fit
		if (max>n-1)							 // Reduce if memory smaller
			max=n-1;
		// Now max is the last index to be displayed 
		for(i=0; i<=max-offset; i++) {
#if defined BUGHUNT
			printf("changedGlobalOffset{loadCluster(%d,%d)}\n",i,i+offset);
#endif
			errChk(loadCluster(i,i+offset));
		}
		// continue and shut off extra displayed columns 
		for(; i<ClusterArray.NumClusters; i++) {
#if defined BUGHUNT
			printf("changedGlobalOffset{disableCluster(%d,1)}\n",i);
#endif
			errChk(disableCluster(i,dnaTrue));	// Also will clear canvas
		}
	}
#if defined BUGHUNT
	printf("changedGlobalOffset{Exiting Normally}\n");
#endif
	
Error:
	return (err?-1:0);
}

//
//	enableGraph does the ACTUAL PLOTTING.

//	This assumes that the ENABLEDQ for this graph should be made checked

//	The graph enabling function.  Interpolated values need to be constructed
//	for all the enabled graphs of the groups being displayed.
	
//	Disabled graphs are only first drawn when enabled.
	
//	Assumes that the checkbox and the memory byte are set correctly
int enableGraph(int index)		// index is 0..11 of the UI index!
{
	int err=0;
	int panel,ENABLEDQ,GRAPH;
	dnaARFGroupArray *groupArray;
	dnaARFGroup *group;
	dnaARFGraph *graph;
	char * Label;
	int n,dummy;							// Number of points to interpolate
	int max,SortQ;							// Number of pixels wide of plot area
	int BrainIndex,dummyindex;
	int TicksLT, TimeUnitLT;
	// Grab some short names for what we are up to 
	panel=ARFGraphs.panel;				// FACES.h constant handle
	GRAPH=ARFGraphs.GRAPH[index];		// FACES.c constant handle
	ENABLEDQ=ARFGraphs.ENABLEDQ[index]; // FACES.c constant handle
	// get the group array
	BrainIndex=index;
	groupArray=GetGroupArray(&BrainIndex);	//  changes Brain Index
	nullChk(groupArray);
	// get the label 
	if (groupArray->Labels)
		Label=groupArray->Labels[BrainIndex];
	else {
		// If no label, employ the default
		Label=GraphLabelPrefix[index];
	}
	if (!Label) {
		errChk(dna_SetGraphLabel(groupArray,BrainIndex,
				GraphLabelPrefix[index]));
		Label=groupArray->Labels[BrainIndex];
	}		
	// get the group (for duration) 
	BrainIndex=index;				   	//  changes Brain Index
	group=GetGroup(&BrainIndex);
	nullChk(group);						// Cannot enable if invalidated
	// get the graph (of course) 
	graph=GetGraph(index);
	nullChk(graph);	
	// we are enabled 	
	errChk(SetCtrlVal (panel,ENABLEDQ,dnaTrue));
	// we are not dimmed 
	errChk(SetCtrlAttribute(panel,GRAPH,ATTR_DIMMED, dnaFalse));
	errChk(SetCtrlAttribute(panel,ENABLEDQ,ATTR_DIMMED,dnaFalse));

	// we have a name 
			
	
	if (Label)
		errChk(SetCtrlAttribute(panel,GRAPH,ATTR_LABEL_TEXT,Label));
	else // we should have a name.....
		errChk(SetCtrlAttribute(panel,GRAPH,ATTR_LABEL_TEXT,Unknown));
	errChk(SetCtrlAttribute (panel, GRAPH,
			ATTR_LABEL_LEFT,VAL_AUTO_CENTER));

	// See if there are existing plots 
	if (ARFGraphs.PLOT[index]>=0) {		// there is so delete it...
		DeleteGraphPlot (panel, GRAPH, ARFGraphs.PLOT[index], VAL_DELAYED_DRAW);
		ARFGraphs.PLOT[index]=-1;		// Invalidate handle
	}
	if (ARFGraphs.IPLOT[index]>=0) {	// there is so delete it...
		DeleteGraphPlot (panel, GRAPH, ARFGraphs.PLOT[index], VAL_DELAYED_DRAW);
		ARFGraphs.IPLOT[index]=-1;		// Invalidate handle
	}

	// Now we are ready to look at the actual plot data 
	
	/* I think I remember why we do this */
	/* After saving and loading an experiment which does not use XY values, it
	is possible that there is no memory allocated.  This will catch that case
	the first time the group is used / displayed
	*/
	if(0==graph->NumberOfValues) {		// we need to allocate memory
		// guarantee that we have allocated enough memory to hold faceXYValues number of XY pairs
		dna_SetGraphValues (graph, faceXYValues);	// GOTCHA from DNA.h
	}

	/* This does ILinear if IUnknown was selected */
	if(dnaNoInterp==graph->InterpType) {
		graph->InterpType=dnaILinear;
	}
	
	// Do we have values to plot...
	if (!graph->InterpReadyQ) {
		// get the number of pixels as the maximum number of points to plot 
		errChk(GetCtrlAttribute (panel, GRAPH, ATTR_PLOT_AREA_WIDTH, &max));
		// calculate the minimum number of request points to plot 
		n=group->TicksD * group->TimeUnitD;
		n=(n/(group->Ticks * group->TimeUnit))+1;
		if (n>max)						// reduce if too many to display
			n=max;
		SortQ=MakeInterp(group,graph,n);	// Pass off work to Brain
		negChk(SortQ);
		if (SortQ)
			errChk(enableXYValues());
	}

	// Want to update TicksLT and TimeUnitLT ?
	if (index>=dnaAnalogChannels) {
		TicksLT  = (group->TicksD * group->TimeUnitD);
		TicksLT -= (group->Ticks * group->TimeUnit);
		TimeUnitLT=1;					// usec
		if (0==TicksLT%1000L) {
			TicksLT /= 1000L;
			TimeUnitLT*=1000L;			// msec
		}
		if (0==TicksLT%1000L) {
			TicksLT /= 1000L;
			TimeUnitLT*=1000L;			// sec
		}
		SetCtrlVal (panel, ARF_RFTICKSLT, TicksLT);
		SetCtrlVal (panel, ARF_RFTIMEUNITLT, TimeUnitLT);
	}	
	// Now actualy update the plots
	if (graph->InterpReadyQ) {
// USERPIN : Always run through the PinGraphValues to act user min/max?
		if (PinValuesQ())
			errChk(dna_PinGraphValues (graph));
		if (InterpData[graph->InterpType].ValuesEnabledQ)
			ARFGraphs.PLOT[index] = PlotXY (panel, GRAPH, graph->XScaled,
										graph->YScaled,
										graph->NumberOfValues, VAL_DOUBLE,
										VAL_DOUBLE, VAL_SCATTER,
										VAL_SOLID_SQUARE, VAL_SOLID, 1,
										VAL_BLACK);
		ARFGraphs.IPLOT[index]= PlotXY (panel, GRAPH, graph->InterpXValues,
										graph->InterpYValues,
										graph->NumberOfInterps, VAL_DOUBLE,
										VAL_DOUBLE, VAL_THIN_LINE,
										VAL_EMPTY_SQUARE, VAL_SOLID, 1,
										VAL_RED);
	}
	else {
		RefreshGraph (panel, GRAPH);
	}
		
Error:
	return (err?-1:0);
}


// 
//	This assumes any pointers are to old stuff.
//	The ARFGraphs and control are both changed.
//	Old graphing interpolation data is touched (deleted).

int changedAnalogIndex(int index)
{
	int err=0;
	int i;								// looping
	int old;
	int panel;
	char name[32];   // GOTCHA FIXME  : Make this less hard coded
	int dummy,min,max;
	dnaARFGroupArray * groupArray;
	dnaARFGraph * graph;
	
	if (index>=AGroups.NumberOfGroups)	// Paranoia
		index=-1;						// Handle by shutting off
	// make certain no graph data is still being edited 
	if (-1!=ARFGraphs.GraphSelected)
		changedGraphSelection(-1);

	if (ValidGroup(0)) {				// Checks for valid old Analog Group
		old = ARFGraphs.AnalogIndex;
		if (0<=old)
			// We ought to free memory by touching all graph data 
			for (i=0; i<dnaAnalogChannels; i++) {
				graph = GetGraph(i);
				if (graph)
					errChk(dna_TouchARFGraph (graph));
			}
	}
	panel = ARFGraphs.panel;
	// Next statement may be redundant
	errChk(SetCtrlVal(panel,ARF_AINDEX,index));
	ARFGraphs.AnalogIndex = index;

	if (index<0) {						// Need to disable stuff
		// Dim everything in header 
		errChk(SetCtrlAttribute(panel,ARF_ALABEL,		ATTR_DIMMED,dnaTrue));
		errChk(SetCtrlAttribute(panel,ARF_ATICKS,		ATTR_DIMMED,dnaTrue));
		errChk(SetCtrlAttribute(panel,ARF_ATIMEUNIT,	ATTR_DIMMED,dnaTrue));
		errChk(SetCtrlAttribute(panel,ARF_ATICKSD,		ATTR_DIMMED,dnaTrue));
		errChk(SetCtrlAttribute(panel,ARF_ATIMEUNITD,	ATTR_DIMMED,dnaTrue));

		ARFGraphs.AnalogGroup = 0;		// NULL
		// Label of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ALABEL,Unused));
		// Timebase Ticks of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATICKS,-1));
		// Timebase Unit of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATIMEUNIT,dnaMSEC));
		// Duration Ticks of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATICKSD,-1));
		// Duration Unit of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATIMEUNITD,dnaMSEC));
		// GOTCHA : lots of values here ... 
		min=0;
		max=dnaAnalogChannels;
		if (max>ARFGraphs.NumGraphs)
			(max=ARFGraphs.NumGraphs);
		for (i=min; i<max; i++) {			
			errChk(disableGraph(i,dnaTrue));	// check box also disabled
		}
	}
	else {
		// un-dim everything in header 
		errChk(SetCtrlAttribute(panel,ARF_ALABEL,		ATTR_DIMMED,dnaFalse));
		errChk(SetCtrlAttribute(panel,ARF_ATICKS,		ATTR_DIMMED,dnaFalse));
		errChk(SetCtrlAttribute(panel,ARF_ATIMEUNIT,	ATTR_DIMMED,dnaFalse));
		errChk(SetCtrlAttribute(panel,ARF_ATICKSD,		ATTR_DIMMED,dnaFalse));
		errChk(SetCtrlAttribute(panel,ARF_ATIMEUNITD,	ATTR_DIMMED,dnaFalse));

		// Get a valid group pointer 
		dummy=0;						// GOTCHA
		groupArray=GetGroupArray(&dummy);
		nullChk(groupArray);
		nullChk(groupArray->ARFGroups);
		ARFGraphs.AnalogGroup=&(groupArray->ARFGroups[index]); 

		// Label of analog group being displayed 
		if (!ARFGraphs.AnalogGroup->Label) {
			negChk(Fmt(name,"%s<Analog Group #%d",index));
			dna_SetGroupLabel (ARFGraphs.AnalogGroup, name);
		}
		if (ARFGraphs.AnalogGroup->Label)
			errChk(SetCtrlVal(panel,ARF_ALABEL,ARFGraphs.AnalogGroup->Label));
		else
			errChk(SetCtrlVal(panel,ARF_ALABEL,Unknown));
		// Timebase Ticks of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATICKS,ARFGraphs.AnalogGroup->Ticks));
		// Timebase Unit of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATIMEUNIT,
				ARFGraphs.AnalogGroup->TimeUnit));
		// Duration Ticks of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATICKSD,ARFGraphs.AnalogGroup->TicksD));
		// Duration Unit of analog group being displayed 
		errChk(SetCtrlVal(panel,ARF_ATIMEUNITD,
				ARFGraphs.AnalogGroup->TimeUnitD));
		// GOTCHA : lots of values here ... 
		min=0;
		max=dnaAnalogChannels;			// Number in memory
		if (max>ARFGraphs.NumGraphs)	// Number on panel
			(max=ARFGraphs.NumGraphs);
		if (!(groupArray->EnabledQ))	// if there is no array to check
			max=min;					// then disabled completely
		for (i=min; i<max; i++) {
			if (groupArray->EnabledQ[i])
				errChk(enableGraph(i));	// plot the data
			else
				errChk(disableGraph(i,dnaFalse));	// dim/plot nothing
		}
		for (;i<dnaAnalogChannels;i++)	// disable the rest
			errChk(disableGraph(i,dnaTrue));	// dim/plot nothing
	}
	
Error:
	return (err?-1:0);
}

// 						  
//	This assumes that the RF Index control is already setup.
//	(specifically its DIM status and MAX attribute) and the value
//	in ARFGraphs matches.

int changedRFIndex(int index)
{
	int err=0;
	int i;								// looping
	int old;
	int panel;
	int dummy,min,max;
	int TicksLT;
	char name[32];  // GOTCHA FIXME  : Make this less ahrd coded
	dnaTimeUnits UnitLT;
	dnaARFGroupArray * groupArray;
	dnaARFGraph * graph;

  	if (index>=RFGroups.NumberOfGroups)	// Paranoia
		index=-1;
	
	old = ARFGraphs.RFIndex;
		// make certain no graph data is still being edited 
	if (-1!=ARFGraphs.GraphSelected)
		changedGraphSelection(-1);

	if (ValidGroup(dnaAnalogChannels)) {	// GOTCHA
		old = ARFGraphs.RFIndex;
		if (0<=old)
			// We ought to free memory by touching all graph data 
			for (i=dnaAnalogChannels; i<ARFGraphs.NumGraphs; i++) {
				graph = GetGraph(i);
				if (graph)
					errChk(dna_TouchARFGraph (graph));
			}
	}

	panel = ARFGraphs.panel;
	// Next statement may be redundant
	errChk(SetCtrlVal(panel,ARF_RFINDEX,index));
	ARFGraphs.RFIndex = index;

	if (index<0) {							// Need to disable stuff
		errChk(SetCtrlAttribute(panel, ARF_RFLABEL,		ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlAttribute(panel, ARF_RFTICKS,		ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNIT,	ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlAttribute(panel, ARF_RFTICKSD,	ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNITD,	ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlAttribute(panel, ARF_RFTICKSLT,	ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNITLT,ATTR_DIMMED, dnaTrue));

		ARFGraphs.RFGroup=0;			// NULL
		// Label of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFLABEL,Unused));
		// Timebase Ticks of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTICKS,-1));
		// Timebase Unit of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTIMEUNIT,dnaMSEC));
		// Duration Ticks of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTICKSD,-1));
		// Duration Unit of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTIMEUNITD,dnaMSEC));
		// Ticks for time of last command of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTICKSLT,-1));
		// Time Unit for itme of last command of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTIMEUNITLT,dnaMSEC));
		
		min=dnaAnalogChannels;			// GOTCHA
		max=ARFGraphs.NumGraphs;		// GOTCHA
		for (i=min; i<max; i++) {		
			errChk(disableGraph(i,dnaTrue));	// check box also disabled
		}
	}
	else {
		errChk(SetCtrlAttribute(panel, ARF_RFLABEL,		ATTR_DIMMED, dnaFalse));
		errChk(SetCtrlAttribute(panel, ARF_RFTICKS,		ATTR_DIMMED, dnaFalse));
		errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNIT,	ATTR_DIMMED, dnaFalse));
		errChk(SetCtrlAttribute(panel, ARF_RFTICKSD,	ATTR_DIMMED, dnaFalse));
		errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNITD,	ATTR_DIMMED, dnaFalse));
		errChk(SetCtrlAttribute(panel, ARF_RFTICKSLT,	ATTR_DIMMED, dnaFalse));
		errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNITLT,ATTR_DIMMED, dnaFalse));

		// Get a valid group pointer 
		dummy=dnaAnalogChannels;				// GOTCHA
		groupArray=GetGroupArray(&dummy);
		nullChk(groupArray);
		nullChk(groupArray->ARFGroups);
		ARFGraphs.RFGroup=&(groupArray->ARFGroups[index]); 
		
		// Label of RF group being displayed 
		if (!ARFGraphs.RFGroup->Label) {
			negChk(Fmt(name,"%s<RF Group #%d",index));
			dna_SetGroupLabel (ARFGraphs.RFGroup, name);
		}
		if (ARFGraphs.RFGroup->Label)
			errChk(SetCtrlVal(panel,ARF_RFLABEL,ARFGraphs.RFGroup->Label));
		else
			errChk(SetCtrlVal(panel,ARF_RFLABEL,Unknown));
		// Timebase Ticks of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTICKS,ARFGraphs.RFGroup->Ticks));
		// Timebase Unit of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTIMEUNIT,ARFGraphs.RFGroup->TimeUnit));
		// Duration Ticks of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTICKSD,ARFGraphs.RFGroup->TicksD));
		// Duration Unit of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTIMEUNITD,
				ARFGraphs.RFGroup->TimeUnitD));
		TicksLT= ARFGraphs.RFGroup->TicksD*ARFGraphs.RFGroup->TimeUnitD-
				ARFGraphs.RFGroup->Ticks*ARFGraphs.RFGroup->TimeUnit;
		UnitLT=dnaUSEC;
		TicksLT=TicksLT/1000;			// always can convert to msec
		UnitLT=dnaMSEC;
		if (0==(TicksLT % 1000)) {
			TicksLT/=1000;				// sometimes convert to sec
			UnitLT=dnaSEC;
		}
		// Ticks for time of last command of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTICKSLT,TicksLT));
		// Time Unit for itme of last command of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTIMEUNITLT,UnitLT));

		// Duration Unit of RF group being displayed 
		errChk(SetCtrlVal(panel,ARF_RFTIMEUNITD,
				ARFGraphs.RFGroup->TimeUnitD));
		min=dnaAnalogChannels;			// GOTCHA
		max=ARFGraphs.NumGraphs;		// GOTCHA
		if (!(groupArray->EnabledQ))	// if there is no array to check
			max=min;					// then disabled completely
		for (i=min; i<max; i++) {
			if (RFGroups.EnabledQ[i-min])
				errChk(enableGraph(i));	// So much work for one line of code....
				else
				errChk(disableGraph(i,dnaFalse));	// off at the moment
		}
		for (;i<ARFGraphs.NumGraphs;i++)	// disable the rest
			errChk(disableGraph(i,dnaTrue));	// dim/plot nothing
	}
	
Error:
	return (err?-1:0);
}


//
//	If GraphSelected != -1 then it will set the graph Interp type.
//	If InterpReadyQ, it will "Touch" it.
//	need to be "invalidated" aware

int setArfInterp(int IType, dnaByte DimQ)
{
	int err=0;
	int i,index,max;
	dnaARFGraph *graph;

	// Find and try to get the graph data 
	index=ARFGraphs.GraphSelected;
	graph = GetGraph(index);
	if (!graph) {						// We need to shut off
		IType=dnaNoInterp;
		DimQ=dnaTrue;
	}
	// Set the panel controls with data from BRAIN 
	errChk(SetCtrlAttribute(ARFGraphs.panel,ARF_INTERP,
			ATTR_DIMMED, DimQ));
	errChk(SetCtrlVal (ARFGraphs.panel, ARF_INTERP,
			IType));
	if (graph) {
		graph->InterpType=IType;
		if (graph->InterpReadyQ)
			errChk(dna_TouchARFGraph (graph));
		for (i=0; i<dnaNumUIParams; i++)
			errChk(SetCtrlVal(ARFGraphs.panel,ARFGraphs.PARAM[i],graph->P[i]));
	}
	else
		for (i=0; i<dnaNumUIParams; i++)
			errChk(SetCtrlVal(ARFGraphs.panel,ARFGraphs.PARAM[i],0.0));
	
	// Case dependent behavior 
	// check for the null type first  to get out of way 
	if (dnaNoInterp==IType) {
		// Shut off all, even reserved, parameter controls 
		for (i=0; i<dnaNumUIParams; i++) {
			errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.PARAM[i],
					ATTR_DIMMED, dnaTrue));
			errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i],
					ATTR_LABEL_TEXT, Unused));
		}
		return 0;
	}
	// With that out of way we can do reserved fields 
	// These hold the scale factor and the scaled offset, respectively
	for (i=0; i<dnaReservedParams; i++) {
		errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.PARAM[i],
				ATTR_DIMMED, dnaFalse));	// Turn it on
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i],
				ATTR_LABEL_TEXT,  InterpData[IType].ParamLabel[i])); // Name it
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i], 
				ATTR_MIN_VALUE, InterpData[IType].MinimumValue[i])); // Minimum
	}
	// We know from Type info structure about the # of params to enable 
	max = dnaReservedParams + InterpData[IType].NumParams;
	if (max > dnaNumUIParams)
		max = dnaNumUIParams;
	for (i=dnaReservedParams; i<max; i++) {
		errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.PARAM[i],
				ATTR_DIMMED, dnaFalse));	// Turn it on
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i],
				ATTR_LABEL_TEXT, InterpData[IType].ParamLabel[i])); // Name it
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i], 
				ATTR_MIN_VALUE, InterpData[IType].MinimumValue[i])); // Minimum
	}
	// Turn off the unsused parameters... 
	for (i=max; i<dnaNumUIParams; i++) {
		errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.PARAM[i],
				ATTR_DIMMED, dnaTrue));	// Turn it on
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i],
				ATTR_LABEL_TEXT,  Unused)); // Name it
	}
Error:
	return (err?-1:0);
}

// this looks are ARFGraphs and sets up the Y Value controls 
// needs to be "invalidated" aware
int setYValueRange(void)
{
	int err=0;
	int i;								// looping
	int index;
	double min,max;
	dnaARFGraph *graph;					

	index = ARFGraphs.GraphSelected;
	// Alot of GOTCHA values feed in here from DNA.h 
	if (!PinValuesQ() || (index < 0) || !ValidGroup(index)) {
// USERPIN : alter min / max
		min=RangeMin[dnaNoRange];		// look up min
		max=RangeMax[dnaNoRange];   	// look up max
	}
	else {
		graph=GetGraph(index);
		nullChk(graph);
		errChk(GetYMin(graph,&min));
		errChk(GetYMax(graph,&max));
		// rescale to user's system using offset(1) and scale(0)
		min = (min-graph->P[1])/graph->P[0];
		max = (max-graph->P[1])/graph->P[0];
	}
	// do not round variables that cannot fit into integers 
	if (fabs(min)>1e7)
		min = brainMinDouble;
	else
		min=RoundRealToNearestInteger (min*100.0)/100.0;
	if (fabs(max)>1e7)
		max = brainMaxDouble;
	else
		max=RoundRealToNearestInteger (max*100.0)/100.0;
	// Now set them all 
	for (i=0; i<faceXYValues; i++) {
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.YVALUE[i],
								 ATTR_MIN_VALUE, min));
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.YVALUE[i],
								 ATTR_MAX_VALUE, max));
	}
Error:
	return (err?-1:0);
}

// does not need to be "invalidated" aware 
int disableXYValues(void)
{
	int err=0;
	int i;								// looping
	
	// Set labels at top of columns to default values 
	// We may still have a graph selected so that control is not
	//	refrenced in this function 
	errChk(setYValueRange());			// Make sure we do this
	errChk(SetCtrlVal(ARFGraphs.panel,ARF_NUMVALUES,0));
	errChk(SetCtrlAttribute(ARFGraphs.panel,ARF_NUMVALUES,
				ATTR_DIMMED, dnaTrue));

	errChk(SetCtrlVal (ARFGraphs.panel, ARF_XVALUELABEL, XValueStr));
	errChk(SetCtrlVal (ARFGraphs.panel, ARF_YVALUELABEL, YValueStr));

	for (i=0; i<ARFGraphs.NumValues; i++) {
		errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.XVALUE[i],
				0.00));
		errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.XVALUE[i],
				ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.YVALUE[i],
				0.00));
		errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.YVALUE[i],
				ATTR_DIMMED, dnaTrue));
	}
	
Error:
	return (err?-1:0);
}

// This looks are ARFGraphs.GraphSelected and sets up the XY values 
// This also calls setYValueRange to setup min and max control values 
// SetArfinterp is called first to setup the parameters 
int enableXYValues(void)
{
	int err=0;
	int index;							// index of graph in UI
	int i;								// looping
	int n;								// Number of Values;
	int max;							// Number in GUI
	dnaARFGraph *graph;					// data to load, possibly
	
	index = ARFGraphs.GraphSelected;
	graph = GetGraph(index);
	if (0==graph) {						// Nothing to load (detect validity)
		errChk(disableXYValues());		// shut it off
		return 0;
	}
	
	n=graph->NumberOfValues;		// How many points

	if ((n) && ((!graph->XValues) || (!graph->YValues))) {
		// The graph structure is invalid 
		err=-1;
		goto Error;
	}
	
	if (InterpData[graph->InterpType].ValuesEnabledQ) {
		errChk(setYValueRange());			// Make sure we do this
		max=ARFGraphs.NumValues;		// Slots to put them in
		if (n>max)						// do not over run UI
			n=max;
		errChk(SetCtrlVal (ARFGraphs.panel, ARF_NUMVALUES,n));
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_NUMVALUES,
				ATTR_DIMMED,dnaFalse));
		for (i=0; i<n; i++) {			// what is enabled
			errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.XVALUE[i],
					ATTR_DIMMED, dnaFalse));
			errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.YVALUE[i],
					ATTR_DIMMED, dnaFalse));
			errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.XVALUE[i],
					graph->XValues[i]));
			errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.YVALUE[i],
					graph->YValues[i]));
		}
		for (; i<max; i++) {			// disable the rest
			errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.XVALUE[i],
					ATTR_DIMMED, dnaTrue));
			errChk(SetCtrlAttribute(ARFGraphs.panel,ARFGraphs.YVALUE[i],
					ATTR_DIMMED, dnaTrue));
			if (i*sizeof(double)<graph->ValueMemoryUsed) {
				errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.XVALUE[i],
						graph->XValues[i]));
				errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.YVALUE[i],
						graph->YValues[i]));
			}
			else {
				errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.XVALUE[i],
						0.00));
				errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.YVALUE[i],
						0.00));
			}
		}
	}
	else {
		errChk(disableXYValues());
		max=ARFGraphs.NumValues;		// Slots to put them in
		if (n>max)						// do not over run UI
			n=max;
		for (i=0; i<n; i++) {
			errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.XVALUE[i],
					graph->XValues[i]));
			errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.YVALUE[i],
					graph->YValues[i]));
		}
		for (; i<max; i++) {			// zero-out the rest
			errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.XVALUE[i],
					0.00));
			errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.YVALUE[i],
					0.00));
		}
	}
Error:
	return (err?-1:0);
	
}

// 
//	TOGGLE behaviour if old==index

//	This makes one assumtion that ARFGraphs.GraphSelected reflects
//	the current displayed data.

//	The correct graph is highlighted/unhighlighted.
//	The index control is updated to show this value, perhaps redundantly
//	The interp type is changed by setarfinterp call, which in turn sets
//		the parameters
//	The XY values are enabled/disabled and loaded with correct values

//	need to be "invalidated" aware


int changedGraphSelection(int index)
{
	int err=0;
	int i,old,panel,GRAPH;
	dnaARFGraph *graph;
	
	if (!GetEnabledQ(index))			// Paranoia
		index=-1;
	if (!ValidGroup(index))				// Paranoia
		index=-1;
	
	panel=ARFGraphs.panel;
	// Need to shut off any previous graph (selecting -1) 
	old = ARFGraphs.GraphSelected;
	if (-1!=old) {
		// We need to shut off the face controls, unhighlight graph 
		GRAPH=ARFGraphs.GRAPH[old];
		if (ValidGroup(old)) {
			// We need to update the plots 
			graph = GetGraph(old);
			errChk(dna_TouchARFGraph (graph));
			errChk(enableGraph(old));
		}
		else {
			// We need to ensure nothing is plotted 
			if (ARFGraphs.PLOT[old]>=0) {		// there is so delete it...
				DeleteGraphPlot(panel,GRAPH,ARFGraphs.PLOT[old],
						VAL_DELAYED_DRAW);
				ARFGraphs.PLOT[old]=-1;		// Invalidate handle
			}
			if (ARFGraphs.IPLOT[old]>=0) {	// there is so delete it...
				DeleteGraphPlot(panel,GRAPH,ARFGraphs.PLOT[old],
						VAL_DELAYED_DRAW);
				ARFGraphs.IPLOT[old]=-1;		// Invalidate handle
			}
		}
		errChk(SetCtrlVal (ARFGraphs.panel, ARF_GRAPHINDEX, -1));
		SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.GRAPH[old],
				ATTR_GRAPH_BGCOLOR, ColorGraphOff);
		// to be complete about this, record current disabled state 
	// Critically Important : This is where GraphSelected is modified 
	// The control indicator is also updated to match 	
		ARFGraphs.GraphSelected = -1;
		errChk(SetCtrlVal (ARFGraphs.panel, ARF_GRAPHINDEX, -1));
		// Must do set arf interp after setting graph selected to -1 
		errChk(setArfInterp(dnaNoInterp,dnaTrue));	// Linear and dimmed=true
		errChk(disableXYValues());					// dim them
	}
	
	
	// Need to switch index on,,,, 
		
	if ((-1!=index) && (index!=old)) {
		// Critically Important : This is where GraphSelected is modified 
		// The control indicator is also updated to match 	
		ARFGraphs.GraphSelected = index;
		errChk(SetCtrlVal (ARFGraphs.panel, ARF_GRAPHINDEX, index));
		// highlight graph 
		GRAPH=ARFGraphs.GRAPH[index];
		SetCtrlAttribute (ARFGraphs.panel, GRAPH,
				ATTR_GRAPH_BGCOLOR, ColorGraphOn);
		// We need to load up the stuff 
		graph = GetGraph(index);
		nullChk(graph);
		errChk(dna_TouchARFGraph (graph));
		errChk(setArfInterp(graph->InterpType,dnaFalse)); //Params
		errChk(enableXYValues());							//Values
	}	
Error:
	return (err?-1:0);
}

//*********************************************************************
//																	   
//																	   
//                      Build Display functions						   
//																	   
//																	   
//*********************************************************************

int DuplicateGraph(int right,int down,int index)
{
	int err=0;
	int top,left;				// Position of the graph itself
	int ltop,lleft;				// Position of the graph label
	int newGraph;
	int newEnabledQ;

	// Get location of upper left, pre-existing graph 
	top = ARFGraphs.Top;		// Recorded previously for posterity
	ltop = ARFGraphs.LTop;		// Recorded previously for posterity
	left = ARFGraphs.Left;		// ditto
	
	// Calculate target location on panel for new graph 
	
	top += down*ARFGraphs.GraphHeight;
	left += right*ARFGraphs.GraphWidth;
	
	ltop += down*ARFGraphs.GraphHeight;
//	lleft += right*ARFGraphs.GraphWidth;
	
	// Make and configure the new graph 
// ALLOCATION of UI controls 	
	newGraph = DuplicateCtrl (ARFGraphs.panel, ARF_GRAPH, ARFGraphs.panel,
			GraphLabelPrefix[index],top, left);
	negChk(newGraph);
	// Save the handle to the new graph 
	ARFGraphs.GRAPH[index]=newGraph;	
// ALLOCATION of UI control 
	newEnabledQ = DuplicateCtrl (ARFGraphs.panel, ARF_ENABLEDQ, ARFGraphs.panel,
			"",ltop, left);
	negChk(newEnabledQ);
	ARFGraphs.ENABLEDQ[index]=newEnabledQ;

	errChk(SetCtrlAttribute (ARFGraphs.panel, newGraph, ATTR_LABEL_TOP, ltop));
							// Explicitly set the position of the top of the label
	errChk(SetCtrlAttribute (ARFGraphs.panel, newGraph,
			ATTR_LABEL_LEFT,VAL_AUTO_CENTER));	//Try the label centering feature
	errChk(SetCtrlAttribute (ARFGraphs.panel, newGraph,
			ATTR_CALLBACK_DATA, (void*)index));
	errChk(SetCtrlAttribute (ARFGraphs.panel, newEnabledQ,
			ATTR_CALLBACK_DATA, (void*)index));
	// Nothing to plug into yet so shut it off 
	errChk(disableGraph(index,dnaTrue));	// Totally shut off
Error:
	return (err?-1:0);
}

// Assumes stuff 
int BuildGraphs(void)
{
	int err=0;
	int i,j;
	// First setup universal attributes of upper left graph ARF_GRAPH, 
	// then they get copied to the others by Dulplicate Graphs. 

	for (i=0; i<ARFGraphs.NumGraphs; i++) {
		ARFGraphs.PLOT[i]=-1;
		ARFGraphs.IPLOT[i]=-1;
	}

	
	errChk(GetCtrlAttribute(ARFGraphs.panel,ARF_GRAPH,
			ATTR_TOP,&ARFGraphs.Top));
	errChk(GetCtrlAttribute(ARFGraphs.panel,ARF_GRAPH,
			ATTR_LABEL_TOP,&ARFGraphs.LTop));
	errChk(GetCtrlAttribute(ARFGraphs.panel,ARF_GRAPH,
			ATTR_LEFT,&ARFGraphs.Left));
	errChk(GetCtrlAttribute(ARFGraphs.panel,ARF_GRAPH,
			ATTR_WIDTH,&ARFGraphs.GraphWidth));
	ARFGraphs.GraphWidth+=4;
	// Height is harder to calculate 
	errChk(GetCtrlAttribute (ARFGraphs.panel, ARF_GRAPH, ATTR_HEIGHT,&j));
	ARFGraphs.GraphHeight = j+ARFGraphs.Top-ARFGraphs.LTop+4;
		// Top of label for the graph to bottom of graph itself
		// The 4 is an aesthetic factor.

	// Nothing to plug into yet, so dim them 
//	errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_ANALOGINDEX, ATTR_DIMMED, 1));
//	errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_RFINDEX, ATTR_DIMMED, 1));

	// Setup the existing graph (pre-loop)
	errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_GRAPH, ATTR_LABEL_TEXT,
						GraphLabelPrefix[0]));
	errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_GRAPH, ATTR_LABEL_LEFT,
							VAL_AUTO_CENTER));
	
	// Need to set this first handle here 
	ARFGraphs.GRAPH[0]=ARF_GRAPH;
	ARFGraphs.ENABLEDQ[0]=ARF_ENABLEDQ;
	// And disable it, which dims it and sets the label to default value 
	errChk(disableGraph(0,dnaTrue));	// Totally shutoff


	// Now we can generate handles for the other graphs 
	// This automatically calls disableGraph on the new Graphs 
	// There is some waste in that some setting are already setup by
	// the duplication process, but the waste is acceptable to me 
// ALLOCATION call of UI Controls 
#if WGFLAG == 2 
	errChk(DuplicateGraph(1,0,1));
	errChk(DuplicateGraph(2,0,2));
	errChk(DuplicateGraph(0,1,3));
	errChk(DuplicateGraph(1,1,4));
	errChk(DuplicateGraph(2,1,5));
	errChk(DuplicateGraph(0,2,6));
	errChk(DuplicateGraph(1,2,7));
	errChk(DuplicateGraph(2,2,8));
	errChk(DuplicateGraph(3,2,9));
	errChk(DuplicateGraph(3,0,10));		// RF Amplitude
	errChk(DuplicateGraph(3,1,11));		// RF Frequency
#elif WGFLAG == 3
	errChk(DuplicateGraph(1,0,1));
	errChk(DuplicateGraph(2,0,2));
	errChk(DuplicateGraph(0,1,3));
	errChk(DuplicateGraph(1,1,4));
	errChk(DuplicateGraph(2,1,5));
	errChk(DuplicateGraph(0,2,6));
	errChk(DuplicateGraph(1,2,7));
	errChk(DuplicateGraph(3,0,8));		// RF Amplitude
	errChk(DuplicateGraph(3,1,9));		// RF Frequency
#endif
Error:
	return (err?-1:0);
}


// Assumes stuff 
int BuildXYValues(void)
{
	int err=0;
	int i;

	int top;							// running top
	int height;							// typically 19
	int xleft,yleft;
	int panel;

	panel = ARFGraphs.panel;

	// Index of Graph being edited 
	ARFGraphs.GraphSelected=-1;
	errChk(SetCtrlVal (panel, ARF_GRAPHINDEX, -1));
//	errChk(SetCtrlAttribute(panel, ARF_GRAPHINDEX,ATTR_DIMMED, dnaTrue));
	
	errChk(GetCtrlAttribute (panel, ARF_XVALUE, ATTR_HEIGHT, &height));
	errChk(GetCtrlAttribute (panel, ARF_XVALUE, ATTR_TOP, &top));
	errChk(GetCtrlAttribute (panel, ARF_XVALUE, ATTR_LEFT, &xleft));
	errChk(GetCtrlAttribute (panel, ARF_YVALUE, ATTR_LEFT, &yleft));
	
	for (i=0; i<ARFGraphs.NumValues; i++) {
// ALLOCATION of UI Controls 
		negChk(ARFGraphs.XVALUE[i]=DuplicateCtrl(panel,
		 		ARF_XVALUE,ARFGraphs.panel, "", top,xleft));
		// Set Callback data 
		errChk(SetCtrlAttribute (panel, ARFGraphs.XVALUE[i],
				ATTR_CALLBACK_DATA,(void *) i));
		negChk(ARFGraphs.YVALUE[i]=DuplicateCtrl(panel,
				ARF_YVALUE,ARFGraphs.panel, "", top,yleft));
		errChk(SetCtrlAttribute (panel, ARFGraphs.YVALUE[i],
				ATTR_CALLBACK_DATA,(void *) i));
		top+=height;
	}
	errChk(SetCtrlAttribute (panel, ARF_XVALUE, ATTR_VISIBLE, 0));
	errChk(SetCtrlAttribute (panel, ARF_XVALUE, ATTR_DIMMED, 1));
	errChk(SetCtrlAttribute (panel, ARF_YVALUE, ATTR_VISIBLE, 0));
	errChk(SetCtrlAttribute (panel, ARF_YVALUE, ATTR_DIMMED, 1));

Error:
	return (err?-1:0);
}


int BuildParams(void)
{
	int err=0;
	int i;															   
	int top,left,height;
	
	// Just need to duplicate the controls and disable them all for now 
	
	errChk(GetCtrlAttribute (ARFGraphs.panel, ARF_PARAM, ATTR_TOP, &top));
	errChk(GetCtrlAttribute (ARFGraphs.panel, ARF_PARAM, ATTR_LEFT, &left));
	errChk(GetCtrlAttribute (ARFGraphs.panel, ARF_PARAM, ATTR_HEIGHT, &height));
	
	ARFGraphs.NumParams = dnaNumUIParams;	// GOTCHA from top of DNA.h
	// For tab order simplicity:
	ARFGraphs.PARAM[0]= DuplicateCtrl (ARFGraphs.panel, ARF_PARAM,
				ARFGraphs.panel, "", top, left);
	negChk(ARFGraphs.PARAM[0]); 
	for(i=1; i<ARFGraphs.NumParams; i++) {
		top+=height;
// ALLOCATION of UI controls 	
		ARFGraphs.PARAM[i] = DuplicateCtrl (ARFGraphs.panel, ARF_PARAM,
				ARFGraphs.panel, "", top, left);
		negChk(ARFGraphs.PARAM[i]);
	}
	// setup callback data index and dim them 
	for(i=0; i<ARFGraphs.NumParams; i++) {
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i],
				ATTR_CALLBACK_DATA, (void *)i));
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.PARAM[i], 
				ATTR_DIMMED,dnaTrue));
	}
	// hide and dim the template
	errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_PARAM, ATTR_VISIBLE, 0));
	errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_PARAM, ATTR_DIMMED, 1));
Error:
	return (err?-1:0);
}

int BuilMinMaxAnalog(void)
{
	int top;
	int err=0;
	int i;
	int panel;
	int left;
	int height;
	// Rest of default min.max are coded into painted FACE.uir file

	panel=ARFGraphs.panel;

	errChk(GetCtrlAttribute (panel, ARF_MINIMUMANALOG, ATTR_LEFT, &left));
	errChk(GetCtrlAttribute (panel, ARF_MINIMUMANALOG, ATTR_TOP, &top));
	errChk(GetCtrlAttribute (panel, ARF_MINIMUMANALOG, ATTR_HEIGHT, &height));

	for (i=0; i<faceGRAPHS;i++) {
		ARFGraphs.MINIMUMANALOG[i] = DuplicateCtrl (panel, ARF_MINIMUMANALOG,
				panel, GraphLabelPrefix[i], top,left);
		negChk(ARFGraphs.MINIMUMANALOG[i]);
		errChk(SetCtrlAttribute (panel, ARFGraphs.MINIMUMANALOG[i],
						  ATTR_CALLBACK_DATA, (void *)i));
		top+=height;
	}
	
//	// Since dnaAnalogUnipolar == dnaAnalogChannels == 10 now, this is not used
//	for (i=dnaAnalogUnipolar; i<dnaAnalogChannels; i++) {
//		errChk(SetCtrlAttribute (panel, ARFGraphs.INITIALANALOG[i],
//				ATTR_MIN_VALUE,min));
//	}

	// This turns off the painted control that we copied
	errChk(SetCtrlAttribute (panel, ARF_MINIMUMANALOG, ATTR_VISIBLE, 0));
	errChk(SetCtrlAttribute (panel, ARF_MINIMUMANALOG, ATTR_DIMMED, 1));

	errChk(GetCtrlAttribute (panel, ARF_MAXIMUMANALOG, ATTR_LEFT, &left));
	errChk(GetCtrlAttribute (panel, ARF_MAXIMUMANALOG, ATTR_TOP, &top));
	errChk(GetCtrlAttribute (panel, ARF_MAXIMUMANALOG, ATTR_HEIGHT, &height));

	for (i=0; i<faceGRAPHS;i++) {
		ARFGraphs.MAXIMUMANALOG[i] = DuplicateCtrl (panel, ARF_MAXIMUMANALOG,
				panel, GraphLabelPrefix[i], top,left);
		negChk(ARFGraphs.MAXIMUMANALOG[i]);
		errChk(SetCtrlAttribute (panel, ARFGraphs.MAXIMUMANALOG[i],
						  ATTR_CALLBACK_DATA, (void *)i));
		top+=height;
	}

	// This turns off the painted control that we copied
	errChk(SetCtrlAttribute (panel, ARF_MAXIMUMANALOG, ATTR_VISIBLE, 0));
	errChk(SetCtrlAttribute (panel, ARF_MAXIMUMANALOG, ATTR_DIMMED, 1));

Error:
	return(err?-1:0);
}

int BuildInitialAnalog(void)
{
	int top;
	int err=0;
	int i;
	int panel;
	int left;
	int height;
	double min = -10.0;		// GOTCHA kludge for bipolar minimum value
	// Rest of default min.max are coded into painted FACE.uir file

	panel=ARFGraphs.panel;

	errChk(GetCtrlAttribute (panel, ARF_INITIALANALOG, ATTR_LEFT, &left));
	errChk(GetCtrlAttribute (panel, ARF_INITIALANALOG, ATTR_TOP, &top));
	errChk(GetCtrlAttribute (panel, ARF_INITIALANALOG, ATTR_HEIGHT, &height));

	for (i=0; i<dnaAnalogChannels;i++) {
		ARFGraphs.INITIALANALOG[i] = DuplicateCtrl (panel, ARF_INITIALANALOG,
				panel, GraphLabelPrefix[i], top,left);
		negChk(ARFGraphs.INITIALANALOG[i]);
		errChk(SetCtrlAttribute (panel, ARFGraphs.INITIALANALOG[i],
						  ATTR_CALLBACK_DATA, (void *)i));
		top+=height;
	}
	// Added so that all the idle controls number like the user range controls
	ARFGraphs.INITIALANALOG[i]=ARF_IDLEAMPLITUDE;	
	ARFGraphs.INITIALANALOG[i+1]=ARF_IDLEFREQUENCY;	
	// Since dnaAnalogUnipolar == dnaAnalogChannels == 10 now, this is not used
	for (i=dnaAnalogUnipolar; i<dnaAnalogChannels; i++) {
		errChk(SetCtrlAttribute (panel, ARFGraphs.INITIALANALOG[i],
				ATTR_MIN_VALUE,min));
	}
	errChk(SetCtrlAttribute (panel, ARF_INITIALANALOG, ATTR_VISIBLE, 0));
	errChk(SetCtrlAttribute (panel, ARF_INITIALANALOG, ATTR_DIMMED, 1));

Error:
	return(err?-1:0);
}


int BuildARF(int panel, int NumValues)
{
	int err=0;
	int i;								// looping

	ARFGraphs.panel = panel;          
	ARFGraphs.NumValues = NumValues;

	// Build Graphs, setup FACES.c module array ARFGraphs 
	ARFGraphs.NumGraphs = faceGRAPHS;	// GOTCHA : taken from DNA.h define
	ARFGraphs.AnalogIndex = -1;
	ARFGraphs.RFIndex = -1;
	ARFGraphs.AnalogGroup = 0;
	ARFGraphs.RFGroup = 0;
	
	// This will have a tab order the same as the build order

 	errChk(BuildInitialAnalog());
	errChk(BuilMinMaxAnalog());
	errChk(BuildParams());
	errChk(BuildXYValues());
	errChk(BuildGraphs());

	// Now that they exist we need to shut them off ... 
	errChk(setArfInterp(dnaNoInterp,dnaTrue));	// Nothing and dimmed=true
	errChk(disableXYValues());					// dim it

	// lots of header stuff needs to be cleared up now 
	
	// This is for the controls for the analog group 
	errChk(SetCtrlAttribute(panel, ARF_ALABEL,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_AINDEX,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_ATICKS,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_ATIMEUNIT,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_ATICKSD,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_ATIMEUNITD,ATTR_DIMMED, dnaTrue));

	// Label of analog group being displayed 
	errChk(SetCtrlVal(panel, ARF_ALABEL,Unused));
	// Index of analog group being displayed 
	errChk(SetCtrlVal(panel, ARF_AINDEX,-1));
	// Timebase Ticks of analog group being displayed 
	errChk(SetCtrlVal(panel, ARF_ATICKS,-1));
	// Timebase Unit of analog group being displayed 
	errChk(SetCtrlVal(panel, ARF_ATIMEUNIT,dnaMSEC));
	// Duration Ticks of analog group being displayed 
	errChk(SetCtrlVal(panel, ARF_ATICKSD,-1));
	// Duration Unit of analog group being displayed 
	errChk(SetCtrlVal(panel, ARF_ATIMEUNITD,dnaMSEC));

	// This is for the controls for the RF group 
	errChk(SetCtrlAttribute(panel, ARF_RFLABEL,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_RFINDEX,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_RFTICKS,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNIT,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_RFTICKSD,ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlAttribute(panel, ARF_RFTIMEUNITD,ATTR_DIMMED, dnaTrue));
	
	// Label of RF group being displayed 
	errChk(SetCtrlVal(panel, ARF_RFLABEL,Unused));
	// Index of RF group being displayed 
	errChk(SetCtrlVal(panel, ARF_RFINDEX,-1));
	// Timebase Ticks of RF group being displayed 
	errChk(SetCtrlVal(panel, ARF_RFTICKS,-1));
	// Timebase Unit of RF group being displayed 
	errChk(SetCtrlVal(panel, ARF_RFTIMEUNIT,dnaMSEC));
	// Duration Ticks of RF group being displayed 
	errChk(SetCtrlVal(panel, ARF_RFTICKSD,-1));
	// Duration Unit of RF group being displayed 
	errChk(SetCtrlVal(panel, ARF_RFTIMEUNITD,dnaMSEC));

//TODO : setup the length of the NUMVALUES slider to match
Error:
	return (err?-1:0);
}


// CEK 2000/04/07 Adding vertical scroll area
/* This is for moving the WG_CLUSTER_OUTPUTLABEL, WG_CLUSTER_IDLE, WG_CANVAS to a
 * new child window
 */
int Buildvframe(void)
{   int err=0;
	int t1,l1;							// corner of output label on panel
	int t2,l2;							// corner of idle on panel
	int height,width;					// size of child panel
	int i,j;							// looping
	int scrollBarWidth=VAL_LARGE_SCROLL_BARS;	// GOTCHA : Magic number from Labwindows/CVI
	Rect R,RR;							// For drawing on canvas
	int panel;							// shortcut to ClusterArray.panel
	
	panel = ClusterArray.panel;
	// get the locations of the anchor elements, i.e. the ones at index 0 
	errChk(GetCtrlAttribute (panel, WG_CLUSTER_OUTPUTLABEL, ATTR_TOP, &t1));
	errChk(GetCtrlAttribute (panel, WG_CLUSTER_OUTPUTLABEL, ATTR_LEFT, &l1));
	errChk(GetCtrlAttribute (panel, WG_CLUSTER_IDLE, ATTR_TOP, &t2));
	errChk(GetCtrlAttribute (panel, WG_CLUSTER_IDLE, ATTR_LEFT, &l2));

	// Put current values into Top and Left, these will be replaced
	// at the end of this function with new values
	errChk(GetCtrlAttribute (panel, WG_CANVAS, ATTR_TOP, &ClusterArray.Top));
	errChk(GetCtrlAttribute (panel, WG_CANVAS, ATTR_LEFT, &ClusterArray.Left));

	// Calculate the full size of the scrolling area
	width = (ClusterArray.Left-l1)+scrollBarWidth+
			ClusterArray.DigitalWidth*ClusterArray.NumClusters;
	height = ClusterArray.DigitalHeight*32;		// GOTCHA: Magic Number
	
	// Create vframe and set its attributes
// ALLOCATION of UI panel
	ClusterArray.vframe=NewPanel (ClusterArray.panel, "", ClusterArray.Top, l1, height, width);
	GetPanelAttribute (ClusterArray.vframe, ATTR_SCROLL_BAR_SIZE,
					   &scrollBarWidth);
	negChk(ClusterArray.vframe);
	//errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_BACKCOLOR, VAL_MAGENTA));
	errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_FRAME_STYLE,
					   VAL_HIDDEN_FRAME));
	errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_SCROLL_BARS,
							  VAL_VERT_SCROLL_BAR));
	errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_TITLEBAR_VISIBLE,
					   dnaFalse));
	errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_MOVABLE, dnaFalse));
	errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_SIZABLE, dnaFalse));
	errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_SIZABLE, dnaFalse));
	errChk(SetPanelAttribute (ClusterArray.vframe, ATTR_VISIBLE, dnaTrue));
	
	
	// Correcting for top edge of vframe
	t1=t1-ClusterArray.Top;		// Usually t1=0
	t2=t2-ClusterArray.Top;		// Usually t2=0
	ClusterArray.Top=0; 		// ClusterArray.Top-ClusterArray.Top=0
	// Correcting for left edit of vframe						
	ClusterArray.Left=ClusterArray.Left-l1;
	l2=l2-l1;
	l1=0;						// l1-l1=0
	for (i=0; i<ClusterArray.NumberOfOutputs; i++) {
// ALLOCATION of UI Controls
		ClusterArray.OUTPUTLABEL[i] = DuplicateCtrl (panel,
				WG_CLUSTER_OUTPUTLABEL, ClusterArray.vframe, Unknown, t1,l1);
		negChk(ClusterArray.OUTPUTLABEL[i]);
		ClusterArray.IDLE[i] = DuplicateCtrl (panel,
				WG_CLUSTER_IDLE, ClusterArray.vframe, "", t2, l2);
		negChk(ClusterArray.IDLE[i]);
		// set the colors 
		// APC Fix 4 -> NumColors
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
				ATTR_TEXT_BGCOLOR, Colors[i%NumColors]));
		// APC Fix 4 -> NumColors
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.IDLE[i],
				ATTR_ON_COLOR, Colors[i%NumColors]));
		// turn off the IDLE control 
		errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.IDLE[i], 0));
		// set the callback data to the output line number 	
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
				ATTR_CALLBACK_DATA, (void *)i));
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.IDLE[i],
				ATTR_CALLBACK_DATA, (void *) i));
		// Increment the top edges by height of rectangle
		t1 += ClusterArray.DigitalHeight;
		t2 += ClusterArray.DigitalHeight;
	}
	for (; i<UIDigitalOutputs; i++) {
// ALLOCATION of UI Controls 
		ClusterArray.OUTPUTLABEL[i] = DuplicateCtrl (panel,
				WG_CLUSTER_OUTPUTLABEL, ClusterArray.vframe, Unused, t1,l1);
		negChk(ClusterArray.OUTPUTLABEL[i]);
		ClusterArray.IDLE[i] = DuplicateCtrl (panel,
				WG_CLUSTER_IDLE, ClusterArray.vframe, "", t2, l2);
		negChk(ClusterArray.IDLE[i]);
		// set the colors 
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
				ATTR_TEXT_BGCOLOR, Colors[i%NumColors]));
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.IDLE[i],
				ATTR_ON_COLOR,Colors[i%NumColors]));
		// turn off (dim) the controls 
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.IDLE[i], ATTR_DIMMED, 1));
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
				ATTR_DIMMED,1));
		// turn off the IDLE control 
		errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.IDLE[i], 0));
		// set the callback data to the output line number 	
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
				ATTR_CALLBACK_DATA, (void *)i));
		errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.IDLE[i],
				ATTR_CALLBACK_DATA, (void *) i));
		// move down by height of rectangle in canvas
		t1 += ClusterArray.DigitalHeight;
		t2 += ClusterArray.DigitalHeight;
	}
	// Blow away the hand drawn controls on panel
	errChk(DiscardCtrl (panel, WG_CLUSTER_OUTPUTLABEL));
	errChk(DiscardCtrl (panel, WG_CLUSTER_IDLE));

	// Setup the canvas and draw the black grid
// ALLOCATION of UI Control
	ClusterArray.CANVAS = DuplicateCtrl (panel,
				WG_CANVAS, ClusterArray.vframe, "", ClusterArray.Top, ClusterArray.Left);
	negChk(ClusterArray.CANVAS);
	errChk(DiscardCtrl (panel, WG_CANVAS));		// remove original canvas
	// Set the background color to FACE.c constant 
	errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.CANVAS, ATTR_PICT_BGCOLOR, ColorCanvas));
	// Set the size of the canvas to the active area 	
	errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.CANVAS, ATTR_HEIGHT,
					ClusterArray.NumberOfOutputs*ClusterArray.DigitalHeight));
	errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.CANVAS, ATTR_WIDTH,
						ClusterArray.NumClusters*ClusterArray.DigitalWidth));
	// setup the rectangle structure that canvas needs as a parameter 
	R.top=0;
	R.left=0;
	R.height=ClusterArray.DigitalHeight;
	R.width=ClusterArray.DigitalWidth;
	// set the color that the rectangle frame will use, from FACE.C constant 	
	errChk(SetCtrlAttribute (ClusterArray.vframe, ClusterArray.CANVAS, ATTR_PEN_COLOR, ColorBorder));
	// draw the array of rectangle frames 
	for(i=0; i<ClusterArray.NumClusters; i++) {	// iterate sideways
		// RR is the mobile rectangle, while R stays fixed at upper left 
		RR=R;
		RectOffset(&RR,RR.width*i,0);
		for(j=0; j<ClusterArray.NumberOfOutputs; j++) {  // iterate down
			errChk(CanvasDrawRect (ClusterArray.vframe, ClusterArray.CANVAS, RR, VAL_DRAW_FRAME));
			RectOffset (&RR, 0, RR.height);
		}
	}

	/* Fix up the horizontal scroll bar */
	// First make the drawn graph the correct width
	SetCtrlAttribute (ClusterArray.panel, WG_HSCROLL, ATTR_WIDTH,
					  ClusterArray.NumClusters*ClusterArray.DigitalWidth);
	ScrollBar_ConvertFromGraph (ClusterArray.panel, WG_HSCROLL, 0, 0);
	ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL, ATTR_SB_VALUE,
							0);
	ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,
							ATTR_SB_VIEW_SIZE, UIClusters);
	ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,
							ATTR_SB_DOC_MIN, 0);
	ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,
							ATTR_SB_DOC_MAX, UIClusters);
	ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,
							ATTR_SB_PROP_THUMB, dnaTrue);
		

Error:
	return (err?-1:0);
}


int BuildControls(void)					// Called from main
{
	int err=0;
	int i,j;							// looping
	int t1,l1,t2,l2;					// For making Cluster heading
	int *handle,*dest;					// For making Cluster heading
	const int items = 9;				// For making Cluster heading
	int panel;

	ClusterArray.panel = wg;	 // Global wg
	panel = ClusterArray.panel;

	// Learn these background colors from the hand drawn controls
	errChk(GetCtrlAttribute(arf,ARF_GRAPH,ATTR_GRAPH_BGCOLOR,&ColorGraphOff));
	errChk(GetCtrlAttribute(wg,WG_CANVAS,ATTR_PICT_BGCOLOR, &ColorCanvas));

	// build the analog / rf panel 
	errChk(BuildARF(arf,UIXYValues));	// GOTCHA : at top of FACES.c
	
	// Build Cluster Array 
	ClusterArray.NumClusters=UIClusters;	// GOTCHA : at top of FACE.c
	ClusterArray.NumberOfOutputs = UIUsedOutputs;	// GOTCHA : top of FACE.c
	
	// Global Offset for leftmost word index
	ClusterArray.GlobalOffset = -1;  // GOTCHA : -1 meaning disabled, while being built
	errChk(SetCtrlAttribute (panel, WG_GLOBALOFFSET, 
			ATTR_DIMMED, dnaTrue));
	errChk(SetCtrlVal (panel, WG_GLOBALOFFSET, -1));
	errChk(SetCtrlAttribute (ClusterArray.panel, WG_GLOBALOFFSET,
			ATTR_MAX_VALUE, 0));

	// OnlyEnabledQ 
	ClusterArray.OnlyEnabledQ = dnaFalse;
	errChk(SetCtrlVal (panel, WG_ONLYENABLEDQ, 
			ClusterArray.OnlyEnabledQ));
//  Finially turned this on
//	errChk(SetCtrlAttribute (panel, WG_ONLYENABLEDQ, 
//			ATTR_DIMMED, dnaTrue));

	// Size of cluster objects 
	errChk(GetCtrlAttribute (panel, WG_CLUSTER_LABEL, ATTR_HEIGHT,
			&ClusterArray.DigitalHeight));
	errChk(GetCtrlAttribute (panel, WG_CLUSTER_LABEL, ATTR_WIDTH,
			&ClusterArray.DigitalWidth));

	// Build digital channel labels, idle switches 
	// Start with the default drawn ones at ouput 0 
	// Save the handles in the array 
/*	ClusterArray.OUTPUTLABEL[0]=WG_CLUSTER_OUTPUTLABEL;
	ClusterArray.IDLE[0]=WG_CLUSTER_IDLE;
	// See if it is active or inactive 
	if (ClusterArray.NumberOfOutputs>0) {
		// turn off the IDLE control 
		errChk(SetCtrlVal (panel, ClusterArray.IDLE[0], 0));
		// set the callback data to the output line number 	
		errChk(SetCtrlAttribute (panel, ClusterArray.OUTPUTLABEL[0],
				ATTR_CALLBACK_DATA, 0));
		errChk(SetCtrlAttribute(panel,ClusterArray.IDLE[0],
				ATTR_CALLBACK_DATA,0));
		// set the colors 
		errChk(SetCtrlAttribute (panel, ClusterArray.OUTPUTLABEL[0],
				ATTR_TEXT_BGCOLOR, Colors[0]));
		errChk(SetCtrlAttribute (panel, ClusterArray.IDLE[0], ATTR_ON_COLOR,
				Colors[0]));
	}
*/
	//******** Setup the cluster headings **************

// ALLOCATION 	
	ClusterArray.MemoryUsed = 
		sizeof(faceClusterHandles)*ClusterArray.NumClusters;
	ClusterArray.Cluster = calloc (ClusterArray.NumClusters,
								   sizeof(faceClusterHandles));
	nullChk(ClusterArray.Cluster);
	// need to put all the drawn UI control handles into the array at index 0 
	ClusterArray.Cluster[0].INDEX		=WG_CLUSTER_INDEX;
	ClusterArray.Cluster[0].LABEL		=WG_CLUSTER_LABEL;
	ClusterArray.Cluster[0].ENABLEDQ	=WG_CLUSTER_ENABLEDQ;
	ClusterArray.Cluster[0].TICKS		=WG_CLUSTER_TICKS;
	ClusterArray.Cluster[0].TIMEUNIT	=WG_CLUSTER_TIMEUNIT;
	ClusterArray.Cluster[0].ANALOGRING	=WG_CLUSTER_ANALOGRING;
	ClusterArray.Cluster[0].ANALOGGROUP	=WG_CLUSTER_ANALOGGROUP;
	ClusterArray.Cluster[0].RFRING		=WG_CLUSTER_RFRING;
	ClusterArray.Cluster[0].RFGROUP		=WG_CLUSTER_RFGROUP;
	// no data to plug into yet so set pointer to zero 
	ClusterArray.Cluster[0].ClusterData=0;

	disableCluster(0,dnaFalse);
		
	// set the callback data to be the cluster index in ClusterArray 
	// GOTCHA Employ knowledge of structure layout 	
	handle = (int*)&ClusterArray.Cluster[0];	
	for (j=0; j<items; j++)
		errChk(SetCtrlAttribute (panel, handle[j],ATTR_CALLBACK_DATA,0));

	// get the left position of anchor cluster 
	errChk(GetCtrlAttribute (panel, WG_CLUSTER_LABEL, ATTR_LEFT, &l2));
	for (i=1; i<ClusterArray.NumClusters; i++) {
		// move over one width 
		l2 += ClusterArray.DigitalWidth;
		// get a clever pointer to handles to setup 
		dest = (int*)&ClusterArray.Cluster[i];
		for (j=0; j<items; j++) {
			// get to top position of source object 
			errChk(GetCtrlAttribute (panel, handle[j], ATTR_TOP, &t2));
// ALLOCATION of UI Controls 
			// GOTCHA Employ knowledge of structure layout 	
			dest[j] = DuplicateCtrl (panel, handle[j], panel, "", t2,l2);
			// set the callback data to be the cluster index in ClusterArray 
			errChk(SetCtrlAttribute (panel, dest[j],
					ATTR_CALLBACK_DATA,(void *)i));
			}
		disableCluster(i,dnaFalse);
	}
	errChk(Buildvframe());

	return 0;							// maybe useful value in future?
Error:
	abort();							// If we cannot build it...crash!
	return -1;							// here to avoid compiler warning;
}

//*********************************************************************
//																	   
//																	   
//    Used for the generic renaming operations the user requests	   
//																	   
//																	   
//*********************************************************************


// Returns 1 if did rename, 0 if it did not, -1 if error 

int EditTextMsg(int panel, int control,int maxLen)
{
	int err=0,answer;
	static const char Title[] = "Enter a New Label";
	char *buf,*message;
	
	
	errChk(GetCtrlAttribute (panel, control, ATTR_STRING_TEXT_LENGTH,
							 &answer));
	if (answer>maxLen)
		maxLen=answer;
// ALLOCATION 
	message=calloc(maxLen+1,1);
	buf=calloc(maxLen+1,1);
	errChk(GetCtrlVal (panel, control, message));
	answer = GenericMessagePopup (Title, message, "Accept",
								  "Cancel", "", buf, maxLen, 0,
								  VAL_GENERIC_POPUP_INPUT_STRING,
								  VAL_GENERIC_POPUP_BTN1,
								  VAL_GENERIC_POPUP_BTN2);
	if (answer==1)
		errChk(SetCtrlVal (panel, control, buf));
	else
		answer = 0;
	free (buf);
	free (message);
	return ((answer==1)?1:0);
Error:
	return -1;
}


// Returns 1 if did rename, 0 if it did not, -1 if error 

int EditLabelMsg(int panel, int control,int maxLen)
{
	int err=0,answer;
	static const char Title[] = "Enter a New Label";
	char *buf,*message;
	
	
	errChk(GetCtrlAttribute (panel, control, ATTR_LABEL_TEXT_LENGTH,
							&answer));
	if (answer>maxLen)
		maxLen=answer;
// ALLOCATION 
	message=calloc(maxLen+1,1);
	buf=calloc(maxLen+1,1);
	errChk(GetCtrlAttribute (panel, control, ATTR_LABEL_TEXT, message));
	answer = GenericMessagePopup (Title, message, "Accept",
								  "Cancel", "", buf, maxLen, 0,
								  VAL_GENERIC_POPUP_INPUT_STRING,
								  VAL_GENERIC_POPUP_BTN1,
								  VAL_GENERIC_POPUP_BTN2);
	if (answer==1)
		errChk(SetCtrlAttribute (panel, control, ATTR_LABEL_TEXT, buf));
	else
		answer =0;
	free (buf);
	free (message);
	return ((answer==1)?1:0);
Error:
	return -1;
}

//*********************************************************************
//																	   
//																	   
//				    Popup Menu and Dialog Box calls
//																	   
//																	   
//*********************************************************************


int popupCluster(int panel,int control, void * callback,
		int eventData1, int eventData2)
{
	int MenuItemID;
	POPData.MenuID=POPUP_CLUSTER;
	POPData.panel=panel;
	POPData.control=control;
	POPData.callback=callback;
	POPData.eventData1=eventData1;
	POPData.eventData2=eventData2;

	MenuItemID = RunPopupMenu (POPData.MENU, POPData.MenuID, POPData.panel,
			POPData.eventData1, POPData.eventData2, 0, 0, 0, 0);

	return ((MenuItemID<0)?-1:0);
}

int popupXYValue(int panel, int control, void * callback,
	int eventData1, int eventData2)
{
	int MenuItemID;
	POPData.MenuID=POPUP_XYVALUE;
	POPData.panel=panel;
	POPData.control=control;
	POPData.callback=callback;
	POPData.eventData1=eventData1;
	POPData.eventData2=eventData2;

	MenuItemID = RunPopupMenu (POPData.MENU, POPData.MenuID, POPData.panel,
			POPData.eventData1, POPData.eventData2, 0, 0, 0, 0);

	return ((MenuItemID<0)?-1:0);
}


//*********************************************************************
//																	   
//																	   
//                        FACETWO functions							   
//																	   
//																	   
//*********************************************************************

int faceSetInterpTypes(void)
{
	int i, err=0;
	
	errChk(ClearListCtrl (ARFGraphs.panel, ARF_INTERP));	// erase old values
	
	for (i=0; i<dnaNumInterps; i++) {
		errChk(InsertListItem (ARFGraphs.panel, ARF_INTERP, i,
				InterpData[i].Label, InterpData[i].ID));
	}

Error:
	return (err?-1:0);
}



int faceSetNumberOfLines(void)
{
	int i,err=0;

	// Make sure we don't try and display too many lines 

	if (UIDigitalOutputs>=8*Clusters.NumberOfPorts)
		ClusterArray.NumberOfOutputs = 8*Clusters.NumberOfPorts;
	else
		ClusterArray.NumberOfOutputs = UIDigitalOutputs;

	// Make sure we don't try and display too many lines 
		
	for (i=0; i<ClusterArray.NumberOfOutputs; i++) {
		errChk(SetCtrlAttribute(ClusterArray.vframe,ClusterArray.OUTPUTLABEL[i],
				ATTR_TEXT_BGCOLOR, Colors[i%NumColors]));
		errChk(SetCtrlAttribute(ClusterArray.vframe,ClusterArray.IDLE[i],
				ATTR_ON_COLOR,Colors[i%NumColors]));
	}
	for (; i<UIDigitalOutputs; i++) {
		errChk(SetCtrlAttribute(ClusterArray.vframe,ClusterArray.IDLE[i],
				ATTR_DIMMED, dnaTrue));
		errChk(SetCtrlAttribute(ClusterArray.vframe,ClusterArray.OUTPUTLABEL[i],
				ATTR_DIMMED,dnaTrue));
		errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.IDLE[i], 0));
		errChk(SetIdle(i, 0));
	}
Error:
	return (err?-1:0);
}

int faceSetNumberOfClusters(void) 
{
	int i,err=0;
	int n,offset;

#if defined BUGHUNT
	printf("faceSetNumberOfClusters{Invalidate cluster data}\n");
#endif
	// Invalidate the ClusterData pointers 
	for (i=0; i<ClusterArray.NumClusters;i++)
		ClusterArray.Cluster[i].ClusterData=0;

#if defined BUGHUNT
	printf("faceSetNumberOfClusters{Check Number Of Clusters>0}\n");
#endif
	n=Clusters.NumberOfClusters;		// Get BRAIN value
	if (n<0) {
		// This is a serious error condition 
		err=-1;
		goto Error;
	}
	
	// Now we need to take care of the GlobalOffset control/value 
	if (n==0) {
		// Need to turn off GlobalOffset
#if defined BUGHUNT
		printf("faceSetNumberOfClusters{n==0}\n");
#endif
		errChk(SetCtrlAttribute (ClusterArray.panel, WG_GLOBALOFFSET,
				ATTR_DIMMED, dnaTrue));
		offset=-1;
		// Disable scroll bar as well
		errChk(ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL, ATTR_SB_VALUE,-1));
		errChk(ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,ATTR_SB_DOC_MIN, -1));
		errChk(ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,ATTR_SB_DOC_MAX, -1));
	}
	else {
		// We know we have clusters so set the maximum offset 
#if defined BUGHUNT
		printf("faceSetNumberOfClusters{n>0}\n");
#endif
		errChk(SetCtrlAttribute (ClusterArray.panel, WG_GLOBALOFFSET,
				ATTR_MAX_VALUE, n-1));
		errChk(SetCtrlAttribute (ClusterArray.panel, WG_GLOBALOFFSET,
				ATTR_DIMMED, dnaFalse));
		if (ClusterArray.GlobalOffset<=-1)	// If < -1 then something is wrong
			offset=0;
		else {
			if (ClusterArray.GlobalOffset>n-1)
				// Need to lower the GlobalOffset
				offset=n-1;
			else
				offset=ClusterArray.GlobalOffset;
		}
		// Disable scroll bar as well
		errChk(ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL, ATTR_SB_VALUE,offset));
		errChk(ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,ATTR_SB_DOC_MIN, 0));
		errChk(ScrollBar_SetAttribute (ClusterArray.panel, WG_HSCROLL,ATTR_SB_DOC_MAX, n-1));
	}

	// Now I only have to call changedGlobalOffset 
#if defined BUGHUNT
	printf("faceSetNumberOfClusters{changedGlobalOffset}\n");
#endif
	err=changedGlobalOffset(offset);
#if defined BUGHUNT
	printf("faceSetNumberOfClusters{changedGlobalOffset err=%d}\n",err);
#endif
	errChk(err);
#if defined BUGHUNT
	printf("faceSetNumberOfClusters{Exiting Normally}\n");
#endif
Error:
	return (err?-1:0);
}

int faceSetOutputLabels(void)			// Names of the digital lines  
{
	int err=0;
	char name[32];   // GOTCHA FIXME  : Make this less ahrd coded
	char port[8]="ABCDEFGH";  // GOTCHA
	int i;
	
	for (i=0; i<ClusterArray.NumberOfOutputs; i++) {
		if (!Clusters.OutputLabels[i]) {
			negChk(Fmt(name,"%s<(%c%d) Line #%d",port[i/8],i%8,i));
			errChk(dna_SetOutputLabel (&Clusters, i, name));
		}
		
		if (Clusters.OutputLabels[i]) 
			errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
					Clusters.OutputLabels[i]));
		else
			errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
					Unknown));
	}
	for (;i<UIUsedOutputs; i++) {	// GOTCHA
		errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.OUTPUTLABEL[i],
				Unused));
	}
Error:
	return (err?-1:0);
}

// Has to be smart. ARGraphs.AnalogGroup may be "freed memory"
int faceSetNumberOfAGroups(void) 
{
	int err=0;
	int i,max;      					// max isnumber of groups
	int index,dummy;
	dnaARFGroupArray * groupArray;

	// Boy this function has ALOT to do, heh heh 
	
// Need to connect ARFGraphs UI Info with AGroups and RFGroups from BRAIN.c 

	// Need to setup the Index Controls for Analog and RF 
	// Then call changedAGroupIndex, changedRFGroupIndex 
	// These will then call loadGroup which will call a loadGraph loop 

	ARFGraphs.AnalogGroup=0;			// Invalidate
	dummy=0;							// GOTCHA
	groupArray=GetGroupArray(&dummy);
	nullChk(groupArray);
	max=groupArray->NumberOfGroups;
	if (max==0) {
		// Shut it down 
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_AINDEX,
				ATTR_DIMMED, dnaTrue));
		index = -1;
	}
	else {
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_AINDEX,
				ATTR_MAX_VALUE, max-1));	// Insure upper limit is correct
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_AINDEX,
				ATTR_DIMMED, dnaFalse));
		if (ARFGraphs.AnalogIndex<=-1)	// If < -1 then something is wrong
			// Need to turn on AnalogIndex
			index=0;
		else {
			if (ARFGraphs.AnalogIndex>max-1)
					// Need to lower the AnalogIndex
				index = max-1;
			else
				index = ARFGraphs.AnalogIndex; 
						// We need to nothing to AnalogIndex
		}
	}
	// Now that Analog Index is set we can call sub function to respond 		
	errChk(changedAnalogIndex(index));
	nullChk(ClusterArray.Cluster);
	for (i=0; i<ClusterArray.NumClusters; i++)
		SetCtrlAttribute(ClusterArray.panel,ClusterArray.Cluster[i].ANALOGGROUP,
				ATTR_MAX_VALUE,max-1);
Error:
	return (err?-1:0);
}   

// Has to be smart. ARGraphs.AnalogGroup may be "freed memory"
int faceSetNumberOfRFGroups(void)
{
	int err=0;
	int max,i,dummy;     				// max is number of groups
	int index;
	dnaARFGroupArray * groupArray;

	ARFGraphs.RFGroup=0;				// Invalidate
	dummy=dnaAnalogChannels;			// GOTCHA
	groupArray=GetGroupArray(&dummy);
	nullChk(groupArray);
	max=groupArray->NumberOfGroups;
	if (max==0) {
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_RFINDEX,
				ATTR_DIMMED, dnaTrue));
		index = -1;
	}
	else {
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_RFINDEX,
				ATTR_MAX_VALUE, max-1));
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARF_RFINDEX,
				ATTR_DIMMED, dnaFalse));
		if (ARFGraphs.RFIndex<=-1)  // If < -1 then something is wrong
			// Need to turn on RFIndex
			index = 0;
		else {
			if (ARFGraphs.RFIndex>max-1)
				// Need to lower the RFIndex
				index = max-1;
			else
				index = ARFGraphs.RFIndex;	// We need to nothing to RFIndex
		}
	}
	// Now that RF Index is set we can call sub function to respond 		
	errChk(changedRFIndex(index));
	nullChk(ClusterArray.Cluster);
	for (i=0; i<ClusterArray.NumClusters; i++)
		SetCtrlAttribute(ClusterArray.panel,ClusterArray.Cluster[i].RFGROUP,
				ATTR_MAX_VALUE,max-1);
Error:
	return (err?-1:0);
}

int faceSetIdleValues(void)
{
	int err=0;
	int i,max;

//#	errChk(SetCtrlVal (ClusterArray.panel, WG_IDLECONTROL, IdleOutput));
	max=ClusterArray.NumberOfOutputs;
	if (max>UIDigitalOutputs)
		max=UIDigitalOutputs;
	for (i=0; i<ClusterArray.NumberOfOutputs; i++) {
		errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.IDLE[i],
				InitIdle.IdleValues[i]));
	}
	for (;i<UIDigitalOutputs; i++) {	// GOTCHA
		errChk(SetCtrlVal (ClusterArray.vframe, ClusterArray.IDLE[i],0));

	}
Error:
	return (err?-1:0);
}

int faceSetInitAnalog(void)
{
	int i;
	int err=0;
	for (i=0; i<dnaAnalogChannels; i++) {
		errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.INITIALANALOG[i],
				InitIdle.InitAnalog[i]));
	}
Error:
	return (err?-1:0);
}

int faceSaveAuxillaryInfo(void)
{	// Any of these which are not kept updated at all times must be updated here
	// right before being saved to a file
	int err=0;
	
	// Cluster Panel
	AuxillaryInfo.GlobalOffset	=ClusterArray.GlobalOffset;
	AuxillaryInfo.OnlyEnabledQ	=ClusterArray.OnlyEnabledQ;
	AuxillaryInfo.RepeatRunQ	=RepeatRunQ();
	AuxillaryInfo.CameraTriggerQ=CameraTriggerQ();
	// These are needed for Execute Run:
	errChk(GetCtrlVal (ClusterArray.panel, WG_USERTICKS,
			&AuxillaryInfo.UserTicks));
	errChk(GetCtrlVal (ClusterArray.panel, WG_USERTIMEUNIT,
			&AuxillaryInfo.UserTimeUnit));
	errChk(GetCtrlVal (ClusterArray.panel, WG_TEST_GT_TEST, 
			&AuxillaryInfo.GTS));
	// Analog Panel
	AuxillaryInfo.AnalogIndex	=ARFGraphs.AnalogIndex;
	AuxillaryInfo.RFIndex		=ARFGraphs.RFIndex;			
	AuxillaryInfo.GraphSelected	=ARFGraphs.GraphSelected;
	AuxillaryInfo.PinRangeQ		=PinValuesQ();
Error:
	return (err?-1:0);
}

int faceSetARFRanges(int index)
// This takes AuxInfo ranges and applies them
// if index=-1, then all are applied
// otherwise just the indicated index is updated
// The UserMin and UserMax are not actually range checked by the widgets
// The range checking is done by BRAIN and DNA
{
	int err=0;
	int i;								// looping
	int first=0,last=faceGRAPHS; 		// range of loop for -1==index
	double value,newvalue;
	
	if (-1!=index) {					// All or One?
		// check range of index
		require(0<=index);				
		require(index<faceGRAPHS);
		// Do only i==index in the for() loop
		first=index;
		last=first+1;
	}
	for (i=first;i<last;i++) {
		// Setup Initial Analog controls and range
		// This will actively Update the controls if necessary
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.INITIALANALOG[i],
				ATTR_MIN_VALUE, RangeMin[AuxillaryInfo.Range[i]]));
		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.INITIALANALOG[i],
				ATTR_MAX_VALUE, AuxillaryInfo.UserMax[i]));
		errChk(GetCtrlVal (ARFGraphs.panel, ARFGraphs.INITIALANALOG[i], &value));
		newvalue = Pin(value,AuxillaryInfo.UserMin[i],AuxillaryInfo.UserMax[i]);
		if (newvalue!=value) {
			errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.INITIALANALOG[i], newvalue));
			printf("FYI: Initial Analog/RF value %i pinned to range.\n",index);
		}
		// Setup minimum control min,max and value
		//		first set value to something small
//		errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.MINIMUMANALOG[i],
//				RangeMin[AuxillaryInfo.Range[i]]));
		//		this stays the same
//		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.MINIMUMANALOG[i],
//				ATTR_MIN_VALUE, RangeMin[AuxillaryInfo.Range[i]]));
		//		this will change
//		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.MINIMUMANALOG[i],
//				ATTR_MAX_VALUE, AuxillaryInfo.UserMax[i]));
		//		now set to actual value
		errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.MINIMUMANALOG[i],
				AuxillaryInfo.UserMin[i]));
		// Setup maximum control min, max and value
//		errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.MAXIMUMANALOG[i],
//				RangeMax[AuxillaryInfo.Range[i]]));
//		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.MAXIMUMANALOG[i],
//				ATTR_MAX_VALUE, RangeMax[AuxillaryInfo.Range[i]]));
//		errChk(SetCtrlAttribute (ARFGraphs.panel, ARFGraphs.MAXIMUMANALOG[i],
//				ATTR_MIN_VALUE, AuxillaryInfo.UserMin[i]));
		errChk(SetCtrlVal (ARFGraphs.panel, ARFGraphs.MAXIMUMANALOG[i],
				AuxillaryInfo.UserMax[i]));
		// TODO NEED TO CHECK TO SEE IF i IS THE SELECTED GRAPH
		// AND REDISPLAY IT IF IT IS.
	};
Error:
	return (err?-1:0);
}

int faceLoadAuxillaryInfo(void)
{
	int err=0;
	int i;
	// Cluster Panel
	// This is done in needed order
	ClusterArray.OnlyEnabledQ=AuxillaryInfo.OnlyEnabledQ;
	errChk(SetCtrlVal (ClusterArray.panel, WG_ONLYENABLEDQ,
			AuxillaryInfo.OnlyEnabledQ));
	errChk(changedGlobalOffset(AuxillaryInfo.GlobalOffset));
	
	errChk(SetCtrlVal(ClusterArray.panel,WG_REPEATRUN,
			AuxillaryInfo.RepeatRunQ));
	errChk(SetCtrlVal(ClusterArray.panel,WG_TRIGGERRUN,
			AuxillaryInfo.CameraTriggerQ));
	errChk(SetCtrlVal (ClusterArray.panel, WG_USERTICKS,
			AuxillaryInfo.UserTicks));
	errChk(SetCtrlVal (ClusterArray.panel, WG_USERTIMEUNIT,
			AuxillaryInfo.UserTimeUnit));
	errChk(SetCtrlVal (ClusterArray.panel, WG_TEST_GT_TEST, 
			AuxillaryInfo.GTS));
	// Analog Panel
	errChk(changedAnalogIndex(AuxillaryInfo.AnalogIndex));
	errChk(changedRFIndex(AuxillaryInfo.RFIndex));
	errChk(SetCtrlVal(ARFGraphs.panel,ARF_PINRANGEQ,AuxillaryInfo.PinRangeQ));
	errChk(changedGraphSelection(AuxillaryInfo.GraphSelected));
	// Need to update Usermin and Usermax here, -1 to update all of them
	errChk(faceSetARFRanges(-1));
	errChk(setYValueRange());
	// These are needed for Execute Run:
Error:
	return (err?-1:0);
}

// This indicates that new pointers have been allocated in BRAIN 
// we need to erase bad pointers that face was holding on to 
int faceInvalidatePointers(void)
{
	int err=0;
	int i,index,max;
	
	ARFGraphs.AnalogGroup=0;
	ARFGraphs.RFGroup=0;
	nullChk(ClusterArray.Cluster);
	for(i=0;i<ClusterArray.NumClusters;i++)
		ClusterArray.Cluster[i].ClusterData=0;
Error:
	return (err?-1:0);
}

// The next set of functions may look at the globals, but only as read-only
int faceSendIdleRF(void)
{
	int err=0;
	double A,D,F;
	errChk(GetCtrlVal (ARFGraphs.panel, ARF_IDLEAMPLITUDE, &A));
	errChk(GetCtrlVal (ARFGraphs.panel, ARF_IDLEDCOFFSET, &D));
	errChk(GetCtrlVal (ARFGraphs.panel, ARF_IDLEFREQUENCY, &F));
	errChk(SetIdleRF(A,D,F));
Error:
	return (err?-1:0);
}
//*********************************************************************
//							Camera Trigger functions
//*********************************************************************

int faceAuxWaitOn(int line, int value)	// 0 ok, -1 error
{	// called from BRAIN WaitForAux
	int err=0;
	char msg[31];
	
	Fmt(msg,"%s<B%d",line+4);			// GOTCHA
	require (-1!=auxwait);				// FIXME : unneeded
	require(!auxwaitQ);
	errChk(SetCtrlVal (auxwait, AUXWAIT_LINE, msg));
	errChk(SetCtrlVal (auxwait, AUXWAIT_VALUE, value));
	errChk(InstallPopup (auxwait));	   // auxwait is set HERE !!!!!! window handle
	auxwaitQ=dnaTrue;					// Set auxwaitQ
Error:
	return (err?-1:0);
}

int faceAuxWaitStatus()					// 0 normal, 1 cancel, -1 error
{	// called from BRAIN WaitForAux in its loop
	if (-1==auxwait)					// FIXME : uneeded
		return -1;
	return (auxwaitQ?0:1);
}

int faceAuxWaitOff(void)				// 0 ok, 1 cancel, -1 error
{	// Called from aux wait panel callback for cancel button
	int err=0;
	
	require(-1!=auxwaitQ);				// FIXME : uneeded
	if (!auxwaitQ)
		return 1;
	errChk(RemovePopup (0));
	auxwaitQ=dnaFalse;					// Clear auxwaitQ
	return 0;
Error:
	return (err?-1:0);
}
//*********************************************************************
//							Running mode functions
//*********************************************************************


int	faceRunDim(int DQ)					// centralize this
{
	int err=0;
	// Turn off controls that user should not touch ! 
	errChk(SetCtrlAttribute(ClusterArray.panel,WG_RUN,ATTR_DIMMED,DQ));
	errChk(SetCtrlAttribute(ClusterArray.panel,WG_IDLEWORD,ATTR_DIMMED,DQ));
	errChk(SetCtrlAttribute(ClusterArray.panel,WG_IDLEZERO,ATTR_DIMMED,DQ));
	errChk(SetMenuBarAttribute (GetPanelMenuBar (ClusterArray.panel),
			WG2MENU_OPERATE_RUN, ATTR_DIMMED, DQ));
	errChk(SetMenuBarAttribute (GetPanelMenuBar (ARFGraphs.panel),
			ARFMENU_OPERATE_RUN, ATTR_DIMMED, DQ));
	errChk(SetMenuBarAttribute (POPData.MENU, 
			POPUP_CLUSTER_OUTPUT, ATTR_DIMMED,DQ));
	errChk(SetCtrlAttribute(ARFGraphs.panel,ARF_RUN_2,ATTR_DIMMED,DQ));
	errChk(SetCtrlAttribute(ARFGraphs.panel,ARF_IDLEANALOG,ATTR_DIMMED,DQ));
	errChk(SetCtrlAttribute(ARFGraphs.panel,ARF_IDLERF,ATTR_DIMMED,DQ));
Error:
	return(err?-1:0);
}


int faceBeginRunningMode(int Timebase)
// This is called by ExecuteRun to diabled controls and start update timer
{
	int err=0,CancelQ=0;
	int T=0,U=1;

	errChk(faceRunDim(dnaTrue));
	// Create and modify  progress dialog 
	RunStatusDialog = CreateProgressDialog ("Run in Progress",
			"Percent Complete", 0,VAL_FULL_MARKERS, "__Cancel");
	negChk(RunStatusDialog);
	errChk(SetPanelAttribute (RunStatusDialog, ATTR_FLOATING,
							  VAL_FLOAT_APP_ACTIVE));	// Always on top
	errChk(SetPanelAttribute (RunStatusDialog, ATTR_HAS_TASKBAR_BUTTON, 1));
	errChk(SetPanelAttribute (RunStatusDialog, ATTR_CAN_MINIMIZE, 1));
 	
	// Start progress dialup update timer going
	errChk(SetCtrlAttribute (ClusterArray.panel, WG_RUNTIMER, 
			ATTR_ENABLED, dnaTrue));

	T=Timebase;
	U=1;								// usec
	if (0==T%1000L) {					
		T/=1000;
		U*=1000;						// msec
	}
	if (0==T%1000L) {
		T/=1000;						// sec
		U*=1000;
	}
	errChk(SetCtrlVal (ClusterArray.panel, WG_USEDTICKS, T));
	errChk(SetCtrlVal (ClusterArray.panel, WG_USEDTIMEUNIT, U));

	// OLD TESTING 
//	SetCtrlAttribute (ClusterArray.panel, WG_RFTIMER, ATTR_ENABLED,
//					  dnaTrue);

	return (err?-1:0);					// normal return
Error:
	SetCtrlAttribute (ClusterArray.panel, WG_RUNTIMER, ATTR_ENABLED,dnaFalse);
	if (0<=RunStatusDialog) {
		DiscardProgressDialog (RunStatusDialog);
		RunStatusDialog = -1;
	}
	faceRunDim(dnaFalse);
	return (err?-1:0);
}

// Returns true if cancel requested or error encountered.
int faceShowProgress(int clusterIndex, int percentdone)	
// This can be called from anywhere to update the progress meter
{
	// may do more in the future 
	int err=0;
	if (0>RunStatusDialog)				// Now return an ok value here
		return dnaFalse;
	if (0==RunMode)						// process events if 0==RunMode
		return UpdateProgressDialog (RunStatusDialog, percentdone, 1);
	else								// else do not process events
		return UpdateProgressDialog (RunStatusDialog, percentdone, 0);
Error:
	return (err?dnaTrue:dnaFalse);
}

int faceEndRunningMode(void)			// Called by CheckRunStatus
// This can be called from anywhere, but in particular from RunTimer(1==RunDone)
// Thus the timer enabled in begin running mode will call end running mode later
// Calling this before begin running mode must be safe ! (See ExecuteRun Error)
{
	int err=0;
	// Stop the timer!
#if VERBOSE > 0
	printf("faceEndRunningMode{Entering function}");
#endif
	errChk(SetCtrlAttribute(ClusterArray.panel,WG_RUNTIMER,
			ATTR_ENABLED,dnaFalse));
	if (0<=RunStatusDialog) {			// Close the progress meter
		DiscardProgressDialog (RunStatusDialog);
		RunStatusDialog = -1;
	}

Error:
	faceRunDim(dnaFalse);
#if VERBOSE > 0
	printf("faceEndRunningMode{Leaving function with err=%d}",err);
#endif
	return (err?-1:0);
}

//*********************************************************************
//																	   
//																	   
//                            main									   
//																	   
//																	   
//*********************************************************************


int main (int argc, char *argv[])	// beginning of program
{
	int IsDup;
	int err=0;
	char buf[100];
	BOOL SuccessQ;
#if defined BUGHUNT
	printf("main Running\n");
#endif
	errChk(SetSystemAttribute (ATTR_ALLOW_MISSING_CALLBACKS, 1));
	errChk(SetSleepPolicy (VAL_SLEEP_MORE));
	if (InitCVIRTE (0, argv, 0) == 0)	
		// Needed if linking in external compiler; harmless otherwise 
		return -1;	// out of memory 
		
	if (CheckForDuplicateAppInstance (DO_NOT_ACTIVATE_OTHER_INSTANCE, &IsDup))
		return -1;
	if (IsDup) {
		Beep();
		errChk(MessagePopup ("Notification",
			"DuplicateAppInstance: This copy will not run the experiment"));
		ExperimentEnabled=dnaFalse;
	};
//	ExperimentEnabled=dnaFalse;
	
	if ((wg = LoadPanel (0, "FACE.uir", WG)) < 0)
		return -1;
	if ((arf = LoadPanel (0, "FACE.uir", ARF)) < 0)
		return -1;
	if ((auxwait = LoadPanel (0, "FACE.uir", AUXWAIT)) < 0)			// Load in panel
		return -1;

	POPData.MENU = LoadMenuBar (0, "FACE.uir", POPUP);
	Clipboard.Cluster=0;
	Clipboard.Graph=0;
	Clipboard.x=0.0;
	Clipboard.y=0.0;

#if defined BUGHUNT
//	printf("main Loaded panels and menu\n");
#endif

	errChk(BuildControls());   			// Build FACE first, get handles
	errChk(GetFileMenuList(ClusterArray.panel));	// Build recent file menu list

#if defined BUGHUNT
//	printf("main Built Controls\n");
#endif

	errChk(TurnMeOn());					// Now start BRAIN
										// This calls FACE2.h (FACE.c) routines
#if defined BUGHUNT
	printf("main Turned On\n");
#endif

	if (!ExperimentEnabled) {
		errChk(SetPanelAttribute (wg, ATTR_TITLE, "WG2 Disabled"));
		errChk(SetPanelAttribute (arf, ATTR_TITLE, "ARF Disabled"));
		errChk(faceRunDim(dnaTrue));
	};
 	
 	errChk(DisplayPanel (arf));			// duh       
	errChk(DisplayPanel (wg));			// duh

#if defined BUGHUNT
	printf("main Displayed\n");
#endif
#if defined THREADS
	InitializeCriticalSection(&critical);
	criticalready=1;
#endif	
#if defined BUGHUNT
	printf("main Going to RunUserInterface\n");
#endif

	if (ExperimentEnabled) errChk(Start_DDE());
//*********************************************
	RunUserInterface ();	// Let 'er rip!
//*********************************************	
	if (ExperimentEnabled) Stop_DDE();
	PutFileMenuList();	// Save recent file menu list


#if defined BUGHUNT
	printf("main From RunUserInterface\n");
#endif
	
#if defined THREADS
	if (0!=ThreadInfo)	{
		CloseHandle(ThreadInfo);	
		ThreadInfo=0;
	}
	DeleteCriticalSection(&critical);
	criticalready=0;
#endif
	
//	hand_ResetAll ();
	
	return 0;							// end of program
Error:
	if (ExperimentEnabled) Stop_DDE();
	Beep();
	return -1;							// Error => program over !!!!!!!!!!!!
	
}

//*********************************************************************
//																	   
//																	   
//                    User Interface Callbacks						   
//																	   
// 	Here is where the buck stops when it comes to error handling	   
//																	   
//*********************************************************************

int CVICALLBACK cbWG (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			QuitUserInterface (0);
			break;
		case WM_COMMAND:
			Breakpoint();
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterIdle (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,MyValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
			errChk(GetCtrlVal (panel,control, &MyValue));
			errChk(SetIdle(index,MyValue));
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterTicks (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,MyValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ClusterArray.Cluster);
 			nullChk(ClusterArray.Cluster[index].ClusterData);
			ClusterArray.Cluster[index].ClusterData->Ticks=MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterTimeUnit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,MyValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ClusterArray.Cluster);
 			nullChk(ClusterArray.Cluster[index].ClusterData);
			ClusterArray.Cluster[index].ClusterData->TimeUnit=MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}


int CVICALLBACK clusterEnabledQ (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,MyValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ClusterArray.Cluster);
 			nullChk(ClusterArray.Cluster[index].ClusterData);
 			ClusterArray.Cluster[index].ClusterData->EnabledQ=MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterRFRing (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,MyValue,Group;
	int H;								// Handle of correct RFGROUP
	
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ClusterArray.Cluster);
 			nullChk(ClusterArray.Cluster[index].ClusterData);
 			ClusterArray.Cluster[index].ClusterData->RFSelector=MyValue;
			H=ClusterArray.Cluster[index].RFGROUP;
			switch (MyValue) {
				case dnaARFStartGroup:
					if (RFGroups.NumberOfGroups) {
						errChk(SetCtrlAttribute(panel,H,ATTR_DIMMED,dnaFalse));
						Group=ClusterArray.Cluster[index].ClusterData->RFGroup;
						if (0>Group) {
							errChk(SetCtrlVal(panel, H, 0));
							ClusterArray.Cluster[index].ClusterData->RFGroup=0;
						}
					}
					else
						errChk(SetCtrlVal(panel, control, dnaARFContinue));
 					break;
				case dnaARFContinue:
					// Fall through 
				case dnaARFStop:
					errChk(SetCtrlAttribute (panel, H, ATTR_DIMMED, dnaTrue));
 					break;
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}


int CVICALLBACK clusterRFGroup (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,MyValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ClusterArray.Cluster);
			nullChk(ClusterArray.Cluster[index].ClusterData);
 			ClusterArray.Cluster[index].ClusterData->RFGroup=MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterAnalogRing (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{   
	int err=0,index,MyValue,Group;
	int H;								// Handle of correct ANALOGGROUP
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ClusterArray.Cluster);
			nullChk(ClusterArray.Cluster[index].ClusterData);
			ClusterArray.Cluster[index].ClusterData->AnalogSelector=MyValue;
 			H=ClusterArray.Cluster[index].ANALOGGROUP;
			switch (MyValue) {
				case dnaARFStartGroup:
					if (AGroups.NumberOfGroups) {
						errChk(SetCtrlAttribute(panel,H,ATTR_DIMMED,dnaFalse));
						Group=ClusterArray.Cluster[index].
								ClusterData->AnalogGroup;
						if (0>Group) { 
							errChk(SetCtrlVal(panel, H, 0));
							ClusterArray.Cluster[index].
									ClusterData->AnalogGroup=0;
						}
					}
					else 
						errChk(SetCtrlVal(panel, control, dnaARFContinue));
 					break;
				case dnaARFContinue:
					// Fall through 
				case dnaARFStop:
					errChk(SetCtrlAttribute (panel, H, ATTR_DIMMED, dnaTrue));
 					break;
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterAnalogGroup (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,MyValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ClusterArray.Cluster);
			nullChk(ClusterArray.Cluster[index].ClusterData);
 			ClusterArray.Cluster[index].ClusterData->AnalogGroup=MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfGraph (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index,len;
	int ans;
	char * MyValue;
	dnaARFGroupArray *group;

	switch (event) {
		case EVENT_COMMIT:
			break;
		case EVENT_LEFT_CLICK:
			index = (int)callbackData;
			errChk(changedGraphSelection(index));						
			break;
		case EVENT_RIGHT_DOUBLE_CLICK:
		case EVENT_LEFT_DOUBLE_CLICK:
			index = (int)callbackData;
// Returns 1 if did rename, 0 if it did not, -1 if error 
			ans=EditLabelMsg(panel,control,30);
			negChk(ans);
			if (ans) {
				group=GetGroupArray(&index);
				errChk(GetCtrlAttribute (panel, control, 
						ATTR_LABEL_TEXT_LENGTH, &len));
	// ALLOCATION 
				MyValue = calloc(len+1,sizeof(char));
				GetCtrlAttribute (panel, control, ATTR_LABEL_TEXT, MyValue);
				errChk(dna_SetGraphLabel (group, index, MyValue));
				errChk(SetCtrlAttribute (panel, control,
						ATTR_LABEL_LEFT,VAL_AUTO_CENTER));
				free(MyValue);
			}
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterLabel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int index,len,err=0;
	char * MyValue;


	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_RIGHT_DOUBLE_CLICK:
		case EVENT_LEFT_DOUBLE_CLICK:
			index = (int)callbackData;
			if (EditTextMsg(panel,control,11)) {
				errChk(GetCtrlAttribute (panel, control, 
						ATTR_STRING_TEXT_LENGTH, &len));
				index = (int)callbackData;	// Get UI index
// ALLOCATION 
				MyValue = calloc(len+1,1);
				nullChk(MyValue);
				errChk(GetCtrlVal (panel, control,MyValue));
				nullChk(ClusterArray.Cluster);
				errChk(dna_SetClusterLabel(
						ClusterArray.Cluster[index].ClusterData,MyValue));
				free(MyValue);
			}
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK canvas (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int Col,Row;						// This relative pixel clicked on
	int iC,iR;							// Index of the rectangle clicked on
	int index;							// Used to check validity
	int color,err;
	dnaByte *Value;
	Rect R;								// Used to draw on canvas
	// The index of the rectangle clicked on
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:
		    R.height=ClusterArray.DigitalHeight;
		    R.width=ClusterArray.DigitalWidth;
			R.top=0;
		    R.left=0;

			Col=eventData2-ClusterArray.Left;
			Row=eventData1-ClusterArray.Top;
			iC=Col/R.width;
			iR=Row/R.height;
			
			// Make sure the column clicked in has a valid header (not index==-1) 
			nullChk(ClusterArray.Cluster);
			errChk(GetCtrlVal (ClusterArray.panel,ClusterArray.Cluster[iC].INDEX,&index));
			if (index<0) return 0;

			// Set R to location around click
			RectOffset (&R, R.width*iC, R.height*iR);
			R.top++;
			R.left++;
			R.width-=2;
			R.height-=2;
//			errChk(CanvasGetPixel (panel, control, MakePoint(R.left+1,R.top+1),
//									&color);
//printf("%x\n",color);
//color=Colors[iR%NumColors];
//			if(color==Colors[iR%NumColors]) 
//				color = ColorCanvas;
//			else 
//				color=Colors[iR%NumColors];
		
			// Get the BRAIN.c data for this location and invert it 
			nullChk(ClusterArray.Cluster[iC].ClusterData)
			nullChk(ClusterArray.Cluster[iC].ClusterData->Digital)
			Value=&(ClusterArray.Cluster[iC].ClusterData->Digital[iR]);
			(*Value)=(*Value)?dnaFalse:dnaTrue; // True->False, False->True
			// Do our color calculation 
			if (*Value)
				color=Colors[iR%NumColors];	// static int[] from top of FACE.c
			else
				color=ColorCanvas;	// static int from top of FACE.c
			// Act on the canvas, so panel and control are already the right values.
			errChk(SetCtrlAttribute(panel,control,ATTR_PEN_FILL_COLOR,color));
			errChk(CanvasDrawRect (panel, control, R, VAL_DRAW_INTERIOR));
			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfRFIndex (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int MyValue;
	switch (event) {
		case EVENT_COMMIT:
			errChk(GetCtrlVal (panel,control, &MyValue));
  			if (0>=RFGroups.NumberOfGroups) {
 				// We should have been dimmed, this is an error state
				errChk(SetCtrlAttribute (panel, control, 
										ATTR_DIMMED, dnaTrue));
				
 				MyValue=-1;
 	 			errChk(SetCtrlVal (panel,control, MyValue));
				errChk(changedRFIndex(MyValue));
	 			return 0;
	 		}
			if (MyValue>=RFGroups.NumberOfGroups) {
				// Invalid state of control, this is an error state
				MyValue=RFGroups.NumberOfGroups-1;
 				errChk(SetCtrlVal (panel,control, MyValue));
 				errChk(changedRFIndex(MyValue));
				return 0;
			}
			// We know we have a valid value in MyValue now 
			// Now check to see if we have really changed or not 
 			if (MyValue!=ARFGraphs.RFIndex) {

				errChk(changedRFIndex(MyValue));
 			}
			// Normal termination leads to break
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterOnlyEnabledQ (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int MyValue;
	switch (event) {
		case EVENT_COMMIT:
			errChk(GetCtrlVal (panel,control, &MyValue));
			if (MyValue!=ClusterArray.OnlyEnabledQ) {
				// save effort by calling only if different 
				ClusterArray.OnlyEnabledQ=MyValue;
				changedGlobalOffset(ClusterArray.GlobalOffset);
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterOutputLabel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int index,len,err=0;
	char * MyValue;
	switch (event) {
		case EVENT_COMMIT:
			
			break;
		case EVENT_RIGHT_DOUBLE_CLICK:
		case EVENT_LEFT_DOUBLE_CLICK:
			index = (int)callbackData;
			if (EditTextMsg(panel,control,18)) {
				errChk(GetCtrlAttribute (panel, control, 
						ATTR_STRING_TEXT_LENGTH, &len));
				index = (int)callbackData;	// Get UI index
// ALLOCATION 
				MyValue = calloc(len+1,1);
				nullChk(MyValue);
				errChk(GetCtrlVal (panel, control,MyValue));
				errChk(dna_SetOutputLabel (&Clusters, index, MyValue));
				free(MyValue);
			}
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

// This is triggered by ressping the run button
int CVICALLBACK RunCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:
			// Load needed info into AuxillaryInfo
			errChk(faceSaveAuxillaryInfo());
			errChk(ExecuteRun(dnaTrue)); 
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
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resTicks (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resTimeUnit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resAskTicks (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resAskTimeUnit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK cbSetup (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupNumberOfLines (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupNumberOfGraphs (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupNumberOfRFGroups (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupNumberOfAGroups (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupNumberOfClusters (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupResetCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupAcceptCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resRFTicks (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resRFTimeUnit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK cbTiming (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resATicks (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK resATimeUnit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK timingAcceptCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK timingResetCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK timingCancelCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK setupCancelCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfRFTimeUnitD (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ARFGraphs.RFGroup);
 			ARFGraphs.RFGroup->TimeUnitD = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfRFTimeUnit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ARFGraphs.RFGroup);
 			ARFGraphs.RFGroup->TimeUnit = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfATimeUnitD (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ARFGraphs.AnalogGroup);
 			ARFGraphs.AnalogGroup->TimeUnitD = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfRFTicksD (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ARFGraphs.RFGroup);
 			ARFGraphs.RFGroup->TicksD = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfATimeUnit (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
 			nullChk(ARFGraphs.AnalogGroup);
 			ARFGraphs.AnalogGroup->TimeUnit = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfATicks (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ARFGraphs.AnalogGroup);
 			ARFGraphs.AnalogGroup->Ticks = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfATicksD (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
 			nullChk(ARFGraphs.AnalogGroup);
 			ARFGraphs.AnalogGroup->TicksD = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfAIndex (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int MyValue;
	switch (event) {
		case EVENT_COMMIT:
			errChk(GetCtrlVal (panel,control, &MyValue));
  			if (0>=AGroups.NumberOfGroups) {
 				// We should have been dimmed, this is an error state
				errChk(SetCtrlAttribute (panel, control, 
										ATTR_DIMMED, dnaTrue));
				
 				MyValue=-1;
 	 			errChk(SetCtrlVal (panel,control, MyValue));
				errChk(changedAnalogIndex(MyValue));
	 			return 0;
	 		}
			if (MyValue>=AGroups.NumberOfGroups) {
				// Invalid state of control, this is an error state
				MyValue=AGroups.NumberOfGroups-1;
 				errChk(SetCtrlVal (panel,control, MyValue));
 				errChk(changedAnalogIndex(MyValue));
				return 0;
			}
			// We know we have a valid value in MyValue now 
			// Now check to see if we have really changed or not 
 			if (MyValue!=ARFGraphs.AnalogIndex) {
				errChk(changedAnalogIndex(MyValue));
 			}
			// Normal termination leads to break
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfYValue (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index;
	double MyValue;
	int swap;
	dnaARFGraph *graph;
	
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;  // whick am i
 			errChk(GetCtrlVal (panel,control, &MyValue));	// what am i
			graph=GetGraph(ARFGraphs.GraphSelected);	
										// what am I controlling a part of
			nullChk(graph);
			nullChk(graph->YValues);
			graph->YValues[index]=MyValue;	// save my value
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:
			errChk(popupXYValue(panel,control,callbackData,
					eventData1,eventData2));
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfXValue (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index;
	double MyValue;
	int swap;
	dnaARFGraph *graph;
	
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;  // whick am i
 			errChk(GetCtrlVal (panel,control, &MyValue));	// what am i
			graph=GetGraph(ARFGraphs.GraphSelected);
										// what am controlling a part of
			nullChk(graph);
			nullChk(graph->XValues);
			graph->XValues[index]=MyValue;	// save my value
//			swap=SortValues(graph,&index);	// re-order if necessary
//			negChk(swap);
//			if (swap) {					
//				errChk(enableXYValues());	// We need to redisplay     
//				errChk(SetActiveCtrl (panel, ARFGraphs.XVALUE[index]));
//			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:
			errChk(popupXYValue(panel,control,callbackData,
					eventData1,eventData2));
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfNumValues (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int MyValue;
	int temp=-1;
	int old,i;
	dnaARFGraph *graph; 
		
	switch (event) {
		case EVENT_COMMIT:
   			errChk(GetCtrlVal (panel,control, &MyValue));
 			graph=GetGraph(ARFGraphs.GraphSelected);
			nullChk(graph);
			nullChk(graph->XValues);
			nullChk(graph->YValues);
			if (MyValue!=graph->NumberOfValues) {
				old=graph->NumberOfValues;
				dna_SetGraphValues (graph, MyValue);
				if (0<old) 
					for (i=old; i<MyValue; i++) 
						if (0.0==graph->XValues[i]) 
							graph->XValues[i]=graph->XValues[old-1];
				errChk(dna_CalcXYValuesMinMax (graph, 3));
				negChk(SortValues(graph,&temp));
				errChk(enableXYValues());
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfRFLabel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int index,len;
	char* MyValue;
	
	switch (event) {
		case EVENT_COMMIT:
			errChk(GetCtrlAttribute (panel, control,
					ATTR_STRING_TEXT_LENGTH, &len));
// ALLOCATION 
			MyValue = calloc(len+1,sizeof(char));
			errChk(GetCtrlVal (panel, control,MyValue));
			errChk(GetCtrlVal (panel, ARF_AINDEX, &index));
			errChk(dna_SetGroupLabel (ARFGraphs.RFGroup,MyValue));
			free(MyValue);
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfALabel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int index,len;
	char* MyValue;
	
	switch (event) {
		case EVENT_COMMIT:
			errChk(GetCtrlAttribute (panel, control,
					ATTR_STRING_TEXT_LENGTH, &len));
// ALLOCATION 
			MyValue = calloc(len+1,sizeof(char));
			errChk(GetCtrlVal (panel, control,MyValue));
			errChk(GetCtrlVal (panel, ARF_AINDEX, &index));
			errChk(dna_SetGroupLabel (ARFGraphs.AnalogGroup,MyValue));
			free(MyValue);
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfGraphIndex (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfInterp (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int MyValue;
	
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));	// what am i
			errChk(setArfInterp(MyValue,dnaFalse));
			errChk(enableXYValues());
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfPARAM (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index;
	double MyValue;
	dnaARFGraph *graph;
	
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;  // whick am i
 			errChk(GetCtrlVal (panel,control, &MyValue));	// what am i
			if ((0==index) & (0.0==MyValue)) {
				MyValue = 1.0;
	 			errChk(SetCtrlVal (panel,control, MyValue));
	 		}
			graph=GetGraph(ARFGraphs.GraphSelected);
										// what am controlling a part of
			nullChk(graph);
			nullChk(graph->P);			// stupid to check right now
			graph->P[index]=MyValue;	// save my value
			if (index < dnaReservedParams)
				setYValueRange();
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK BreakpointCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:
			Breakpoint() ;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfRFTicks (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
			nullChk(ARFGraphs.RFGroup);
 			ARFGraphs.RFGroup->Ticks = MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfEnabledQ (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	// We need some magic to happen here 
	int err=0;
	int index,MyValue;
	int BrainIndex;
	dnaARFGroupArray * groupArray;
	switch (event) {
		case EVENT_COMMIT:
			index = (int) callbackData;
  			errChk(GetCtrlVal (panel,control, &MyValue));
			BrainIndex=index;
			groupArray=GetGroupArray(&BrainIndex);
			nullChk(groupArray);
			nullChk(groupArray->EnabledQ)
			groupArray->EnabledQ[BrainIndex]=MyValue; 
			if (MyValue)
			 	enableGraph(index);
			 else  {
				if (ARFGraphs.GraphSelected==index)
					errChk(changedGraphSelection(-1));
			 	errChk(disableGraph(index,dnaFalse));
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfUpdateGraph (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int index;
	dnaARFGraph * graph;
	switch (event) {
		case EVENT_COMMIT:
			index = ARFGraphs.GraphSelected;
			if (-1!=index) {
				graph = GetGraph(index);
				nullChk(graph);
				errChk(dna_TouchARFGraph (graph));
				errChk(enableGraph(index));
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfPinRangeQ (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:
			errChk(setYValueRange());
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}


int CVICALLBACK RunTimer (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_TIMER_TICK:
#if VERBOSE > 3		// 1
			printf("RunTimer{EVENT_TIMER_TICK, RunDone %d, RunMode %d}\n",RunDone, RunMode);
#endif
			switch (RunDone) {
				case -1:				// happens during before start of a run
					break;
				case 0:					// Only check status if in background
					if (RunMode<2)
						errChk(CheckRunStatus());
					break;
				case 1:					// Some function saw the run is over
#if VERBOSE > 0
					printf("RunTimer{EVENT_TIMER_TICK when 1==RunDone}\n");
#endif
					errChk(SetCtrlAttribute (panel, control, 
							ATTR_ENABLED, dnaFalse)); // do not tick again!
					errChk(faceEndRunningMode());
					break;
			}
			break;
	}
Error:
	if (err<0) {}						// cannnot risking beeping at this rate
	return 0;
}

int CVICALLBACK arfInitialAnalog (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index;
	double MyValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
 			InitIdle.InitAnalog[index]=MyValue;
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfIdleAnalog (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	int i;
	switch (event) {
		case EVENT_COMMIT:
			errChk(SetAnalogIdle ());
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfIdleRF (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;

	switch (event) {
		case EVENT_COMMIT:
			err==(faceSendIdleRF());
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterIndex (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:
		case EVENT_RIGHT_CLICK:
 			errChk(popupCluster(panel,control,callbackData,
 					eventData1,eventData2));	
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}


int CVICALLBACK CommentText (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int MyState,err=0;
	switch (event) {
		case EVENT_COMMIT:

			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:
			GetCtrlAttribute (panel, control, ATTR_NO_EDIT_TEXT, &MyState);
			if (MyState) {
				errChk(SetCtrlAttribute (panel, control,
						ATTR_NO_EDIT_TEXT, dnaFalse));
				errChk(SetCtrlAttribute(panel,WG_ACCEPT,ATTR_DIMMED,dnaFalse));
				errChk(SetCtrlAttribute(panel,WG_CANCEL,ATTR_DIMMED,dnaFalse));
			}
			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK commentCancelCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int MyState,err=0;
	switch (event) {
		case EVENT_COMMIT:
			errChk(GetCtrlAttribute(panel,WG_COMMENTTEXT,
					ATTR_NO_EDIT_TEXT,&MyState));
			if (!MyState) {
				errChk(ResetTextBox (panel, WG_COMMENTTEXT, 
						AuxillaryInfo.CommentS));
				errChk(SetCtrlAttribute (panel, WG_COMMENTTEXT,
						ATTR_NO_EDIT_TEXT, dnaTrue));
				errChk(SetCtrlAttribute(panel,WG_ACCEPT,ATTR_DIMMED,dnaTrue));
				errChk(SetCtrlAttribute(panel,WG_CANCEL,ATTR_DIMMED,dnaTrue));
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK commentAcceptCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{   
	int MyState,length,err=0;
	switch (event) {
		case EVENT_COMMIT:
			errChk(GetCtrlAttribute(panel,WG_COMMENTTEXT,
					ATTR_NO_EDIT_TEXT,&MyState));
			if (!MyState) {
				if (AuxillaryInfo.CommentS)
					free(AuxillaryInfo.CommentS);
				AuxillaryInfo.CommentS=0;
				errChk(GetCtrlAttribute(panel,WG_COMMENTTEXT,
						ATTR_STRING_TEXT_LENGTH,&length));
// ALLOCATION
				AuxillaryInfo.CommentS=calloc(length+1,sizeof(char));
				nullChk(AuxillaryInfo.CommentS);
				errChk(GetCtrlVal(panel,WG_COMMENTTEXT,AuxillaryInfo.CommentS));
				errChk(SetCtrlAttribute(panel,WG_COMMENTTEXT,
						ATTR_NO_EDIT_TEXT, dnaTrue));
				errChk(SetCtrlAttribute(panel,WG_ACCEPT,ATTR_DIMMED,dnaTrue));
				errChk(SetCtrlAttribute(panel,WG_CANCEL,ATTR_DIMMED,dnaTrue));
			}
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK AuxCancel (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			if (-1==faceAuxWaitOff())
				Beep();
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
	return 0;
}

//******************** MENU CALLBACKS *************************

void CVICALLBACK arfEDITInsertValue (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	// Adds a value pair below the last select XY point in the current graph
	// (Moves the big yellow pointer down)
	int err=0;
	int n,max,i;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		nullChk(graph->XValues);
		nullChk(graph->YValues);
		n=graph->NumberOfValues;
		max=ARFGraphs.NumValues;		// GOTCHA
		if (n<max) {
			for (i=max-1; i>n; i--) {
				graph->XValues[i]=graph->XValues[i-1];
				graph->YValues[i]=graph->YValues[i-1];
			}
			if (n>0) {
				graph->XValues[n]=graph->XValues[n-1];
				graph->YValues[n]=graph->YValues[n-1];
			}
			else {
				graph->XValues[n]=0.0;
				graph->YValues[n]=0.0;
			}
			graph->NumberOfValues++;
			errChk(enableXYValues());
		}
	}
Error:
	if (err) {Beep();}
}

void CVICALLBACK arfEDITDeleteValue (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	// Deletes the last selected 
	int err=0;
	int n,max,i;
	dnaARFGraph *graph;

	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		nullChk(graph->XValues);
		nullChk(graph->YValues);
		n=graph->NumberOfValues;
		max=ARFGraphs.NumValues;		// GOTCHA
		if (n>0) {
			for (i=n; i<max-1; i++) {
				graph->XValues[i-1]=graph->XValues[i];
				graph->YValues[i-1]=graph->YValues[i];
			}
			graph->NumberOfValues--;
			errChk(enableXYValues());
		}
	}
Error:
	if (err) {Beep();}
}

int CVICALLBACK SaveIt(char *filename)
{
	int err=0;
	char *title;
	
	AuxillaryInfo.FileS=StrDup(filename);				
	nullChk(AuxillaryInfo.FileS);
	errChk(SaveIni());
	errChk(SetCtrlVal (ClusterArray.panel, WG_FILETIME,
			AuxillaryInfo.TimeS));
	errChk(SetCtrlVal (ClusterArray.panel, WG_FILEDATE,
			AuxillaryInfo.DateS));
#if WGFLAG == 2	
	title=StrDup("WG2 ");
#elif WGFLAG == 3
	title=StrDup("WG2 ");
#endif
	AppendString (&title, AuxillaryInfo.FileS, -1);
	errChk(SetPanelAttribute (ClusterArray.panel, ATTR_TITLE, 
			AuxillaryInfo.FileS));
	free(title);
	errChk(AddRecentFilename(filename));
Error:
	return (err?-1:0);
};


void CVICALLBACK menuFILESaveIni (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int answer;
	int status;
	int file=-1;
	char filename[MAX_PATHNAME_LEN];
	char message[255];
	int err=0;
	status = FileSelectPopup ("D:\\Project\\Development", "*.wg",
							  "*.wg;*.ini;*.*",
							  "Save Word Generator II Settings",
							  VAL_SAVE_BUTTON, 0, 0, 1, 1, filename);
	negChk(status);
	commentAcceptCommand(ClusterArray.panel,EVENT_COMMIT,WG_ACCEPT,0,0,0);
	switch (status) {
		case VAL_EXISTING_FILE_SELECTED :
			Fmt(message, "%s<Do you want to overwrite\n%s ?", filename);
			answer = ConfirmPopup ("You are about to delete data",message);
			negChk(answer);
			if (dnaTrue==answer) {
				if (AuxillaryInfo.FileS) {
					free(AuxillaryInfo.FileS);
					AuxillaryInfo.FileS=0;
				}
			}
			errChk(SaveIt(filename));
			break;
		case VAL_NEW_FILE_SELECTED :
			if (AuxillaryInfo.FileS) {
				free(AuxillaryInfo.FileS);
				AuxillaryInfo.FileS=0;
			}
			errChk(SaveIt(filename));
			break;
	}

Error:
	if (err<0) {Beep();}
}		  
		  

/* This is the function that tells BRAIN.c to open a file */
/* all file open requests should go through this function */
/* filename is referenced, but no allocation or free-ing is done here */
/* Also, filename ought to be valid, to aid in debugging */
int CVICALLBACK OpenIt(char * filename)
{
	int err=0;
	
//	printf("open it\n");  
	errChk(LoadIni(filename));
//	printf("opened ok\n");
	errChk(SetCtrlVal (ClusterArray.panel, WG_FILETIME, 
			AuxillaryInfo.TimeS));
	errChk(SetCtrlVal (ClusterArray.panel, WG_FILEDATE, 
			AuxillaryInfo.DateS));
	errChk(ResetTextBox(ClusterArray.panel,WG_COMMENTTEXT,
			AuxillaryInfo.CommentS));
	errChk(faceInvalidatePointers());	// clear old pointers	
	errChk(faceSetNumberOfLines());		// Tell FACE to set itself up
	errChk(faceSetNumberOfClusters());	// Tell FACE to set itself up
	errChk(faceSetOutputLabels());		// Tell FACE to set itself up
#ifdef BUGHUNT2
	printf("Will faceSetNumberOfRFGroups");
#endif
	errChk(faceSetNumberOfRFGroups ());	// Tell FACE to set itself up
#ifdef BUGHUNT2
	printf("did faceSetNumberOfRFGroups");
#endif
#ifdef BUGHUNT2
	printf("Will faceSetNumberOfAGroups");
#endif
 	errChk(faceSetNumberOfAGroups ());	// Tell FACE to set itself up
#ifdef BUGHUNT2
	printf("Did faceSetNumberOfAGroups");
#endif
	errChk(faceSetIdleValues());		// Tell FACE to set itself up      
 	errChk(faceSetInitAnalog());		// Tell FACE to set itself up      
	errChk(faceLoadAuxillaryInfo());
	errChk(SetPanelAttribute (ClusterArray.panel, ATTR_TITLE, 
			AuxillaryInfo.FileS));
	errChk(AddRecentFilename(filename));
Error:
//	printf("open it err:%i\n",err);  
	return(err?-1:0);
}

void CVICALLBACK MsgOpenIt(void * filename)
{
	int err=0;
	err=OpenIt(filename);
	if (0!=err)
		Beep();
}

void CVICALLBACK MsgSaveIt(void * filename)
{
	int err=0;
	err=SaveIt(filename);
	if (0!=err)
		Beep();
}

void CVICALLBACK menuFILEOpenIni (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int status;
	int answer;
	int file=-1;
	char filename[MAX_PATHNAME_LEN];
	char pathname[MAX_PATHNAME_LEN];
	char message[255];
	Fmt(message, "%s<Do you want to overwrite\nthe data in memory?");
	answer = ConfirmPopup ("You are about to delete data",message);
	negChk(answer);
	if (dnaTrue!=answer) {
		goto Error;
	}
	if (AuxillaryInfo.FileS) {
		SplitPath (AuxillaryInfo.FileS, NULL, pathname, NULL);
	} else {
		errChk(GetDir (pathname));
	}
	status = FileSelectPopup (pathname, "*.wg", "*.wg;*.ini;*.*",
							  "Load Wordgenerator Settings",
							  VAL_LOAD_BUTTON, 0, 0, 1, 1, filename);
	negChk(status)
	commentAcceptCommand(ClusterArray.panel,EVENT_COMMIT,WG_ACCEPT,0,0,0);
	switch (status) {
		case VAL_EXISTING_FILE_SELECTED :
			errChk(OpenIt(filename));
			break;
		case VAL_NEW_FILE_SELECTED :
			errChk(MessagePopup ("Error Loading Settings",
						  "You selected a file that does not exist!"));
			break;
	}
//	errChk(SetPanelAttribute (ClusterArray.panel, ATTR_TITLE, AuxillaryInfo.FileS));
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK wgOPERTATEEnableAll (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,i;
	dnaCluster *cluster;
	
	i=0;
	while (cluster=GetCluster(i)) {
		cluster->EnabledQ=dnaTrue;
		i++;
	}
	errChk(changedGlobalOffset(ClusterArray.GlobalOffset));
	
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK wgOPERTATEDisableAll (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,i;
	dnaCluster *cluster;
	
	i=0;
	while (cluster=GetCluster(i)) {
		cluster->EnabledQ=dnaFalse;
		i++;
	}
	errChk(changedGlobalOffset(ClusterArray.GlobalOffset));

Error:
	if (err<0) {Beep();}
}

void CVICALLBACK menuWINDOWARF (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	errChk(SetActivePanel (ARFGraphs.panel));
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK menuWINDOWDigital (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	errChk(SetActivePanel (ClusterArray.panel));
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK menuFILEQuit (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	QuitUserInterface(0);
}


void CVICALLBACK arfEDITDuplicateAnalog (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,ConfirmQ;
	int i,dummy,max;
	dnaARFGraph *graph;
	dnaARFGroup *group;
	dnaARFGroupArray *groupArray;
	ConfirmQ = ConfirmPopup ("Confirm Menu Operation",
   			"Duplicate This Analog Group?");
   	negChk(ConfirmQ);
   	if (ConfirmQ) {
		dummy=0;
		nullChk(groupArray=GetGroupArray(&dummy));
		max=(groupArray->NumberOfGroups);           
		// Values=0 parameter in the mext line will not release any data memory
		// It will leave all current memory intact. Of course done in copy mode
		errChk(dna_ReInitARFGroupArray (groupArray, 1+max,
										groupArray->NumberOfGraphs, 1, 1));
		nullChk(groupArray=GetGroupArray(&dummy));
		ARFGraphs.AnalogGroup=&groupArray->ARFGroups[ARFGraphs.AnalogIndex];
		nullChk(group=GetGroup(&dummy));
		nullChk(graph=GetGraph(dummy));
																			
		// We Need to Fix RangeIndex in destination graphs
		errChk(dna_CopyARFGroup (&groupArray->ARFGroups[max],group));
		for(i=0; i<groupArray->NumberOfGraphs; i++) {
// TODO : Remove this printf
//			printf("FYI: RangeIndex: %i -> %i\n",groupArray->ARFGroups[max].ARFGraphs[i].RangeIndex,i);
			groupArray->ARFGroups[max].ARFGraphs[i].RangeIndex=i;
		}
		errChk(dna_SetGroupLabel (&groupArray->ARFGroups[max], 0));
	}
	
	errChk(faceSetNumberOfAGroups());
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK arfEDITDuplicateRF (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,ConfirmQ;
	int i,dummy,max;
	dnaARFGraph *graph;
	dnaARFGroup *group;
	dnaARFGroupArray *groupArray;
	ConfirmQ = ConfirmPopup ("Confirm Menu Operation",
   			"Duplicate This RF Group?");
   	negChk(ConfirmQ);
   	if (ConfirmQ) {
		dummy=dnaAnalogChannels;
		nullChk(groupArray=GetGroupArray(&dummy));
		max=(groupArray->NumberOfGroups);           
		// Values=0 paramter in the mext line will not release any data memory
		// It will leave all current memory intact. Of course done in copy mode
		errChk(dna_ReInitARFGroupArray (groupArray, 1+max,
										groupArray->NumberOfGraphs, 1, 1));
		dummy=dnaAnalogChannels;
		nullChk(groupArray=GetGroupArray(&dummy));
		ARFGraphs.RFGroup=&groupArray->ARFGroups[ARFGraphs.RFIndex];
		dummy=dnaAnalogChannels;
		nullChk(group=GetGroup(&dummy));
		dummy=dnaAnalogChannels;
		nullChk(graph=GetGraph(dummy));
									
		// Need to fix destination RangeIndex																			
		errChk(dna_CopyARFGroup (&groupArray->ARFGroups[max],group));
		for(i=0; i<groupArray->NumberOfGraphs; i++) {
// TODO : Remove this printf
//			printf("FYI: RangeIndex: %i -> %i\n",groupArray->ARFGroups[max].ARFGraphs[i].RangeIndex,i+dnaAnalogChannels);
			groupArray->ARFGroups[max].ARFGraphs[i].RangeIndex=i+dnaAnalogChannels;
		}
		errChk(dna_SetGroupLabel (&groupArray->ARFGroups[max], 0));
	}
	errChk(faceSetNumberOfRFGroups());
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK arfEDITEraseAnalog (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	// NOT IMPLIMENTED
	int err=0,ConfirmQ;
	int i,dummy,max;
	dnaARFGraph *graph;
	dnaARFGroup *group;
	dnaARFGroupArray *groupArray;
	ConfirmQ = ConfirmPopup ("Confirm Menu Operation",
							 "DESTROY THIS ANALOG GROUP?");
   	negChk(ConfirmQ);
   	if (ConfirmQ) {
	}
	err=-1;
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK arfEDITEraseRF (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	// NOT IMPLIMENTED
	int err=0;
	err-1;
Error:
	if (err<0) {Beep();}
}

void CVICALLBACK arfEDITCopyGraph (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		if (0==Clipboard.Graph) {
			Clipboard.Graph=calloc(1,sizeof(dnaARFGraph));
			nullChk(Clipboard.Graph);
			errChk(dna_InitARFGraph (Clipboard.Graph));
		}
		// Copying ot clipboard, do not need to fix RangeIndex
		errChk(dna_CopyARFGraph(Clipboard.Graph,graph));
	}
	else 
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK arfEDITPasteGraph (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	
	dnaARFGraph *graph;
	
	if ((-1!=ARFGraphs.GraphSelected) && (Clipboard.Graph)) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		errChk(dna_ReInitARFGraph (graph, 0, 0));
		// Need to fix RangeIndex
		errChk(dna_CopyARFGraph(graph,Clipboard.Graph));
		graph->RangeIndex=ARFGraphs.GraphSelected;
		errChk(changedGraphSelection(-1));
		errChk(changedGraphSelection(ARFGraphs.GraphSelected));
	}
	else 
		err=-1;
Error:
	if (err) {Beep();}
}
void CVICALLBACK arfOPERATEZoomGraph (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,i,max;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		if (!graph->InterpReadyQ) {		// No scaled values to copy
			MessagePopup ("Menu Operation Message", 
					"You must update the graph first");
			return;
		}
		nullChk(graph->InterpXValues);
		nullChk(graph->InterpYValues);
		errChk(XYGraphPopup ("Zoom Graph", graph->InterpXValues,
					  graph->InterpYValues, graph->NumberOfInterps,
					  VAL_DOUBLE, VAL_DOUBLE));
 	}	
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK arfOPERATEFillX (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,i,max;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		nullChk(graph->XValues);
		max=graph->ValueMemoryUsed/sizeof(double);
		// Fill from zero, counting by one
		for(i=0;i<max;i++)
			graph->XValues[i]=i*1.0;
		errChk(enableXYValues());
	}	
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK arfOPERATEReplaceX (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,i,max;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		if (!graph->InterpReadyQ) {		// No scaled values to copy
			MessagePopup ("Menu Operation Message", 
					"You must update the graph first");
			return;
		}
		nullChk(graph->XValues);
		nullChk(graph->XScaled);
		max=graph->ValueMemoryUsed/sizeof(double);
		for(i=0;i<max;i++)
			graph->XValues[i]=graph->XScaled[i];
		errChk(enableXYValues());
	}	
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK arfOPERATEReplaceY (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,i,max,ConfirmQ;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		if (!graph->InterpReadyQ) {		// No scaled values to copy
			MessagePopup ("Menu Operation Message", 
					"You must update the graph first");
			return;
		}
		nullChk(graph->YValues);
		nullChk(graph->YScaled);
		max=graph->ValueMemoryUsed/sizeof(double);
		for(i=0;i<max;i++)
			graph->YValues[i]=graph->YScaled[i];
		errChk(enableXYValues());
		ConfirmQ = ConfirmPopup ("Menu Operation Question",
								 "Reset Scaling Parameters?");
		negChk(ConfirmQ);
		if (dnaTrue==ConfirmQ) {
			graph->P[0]=1.0;
			graph->P[1]=0.0;
			// Now update the display of the parameters
			setArfInterp(graph->InterpType,dnaFalse);
		}			
	}	
	else
		err=-1;
Error:
	if (err) {Beep();}
}


void CVICALLBACK menuOPERATERun (int menuBar, int menuItem, void *callbackData,
		int panel)
{   
	RunCommand(-1,-1,EVENT_COMMIT,0,0,0);
}

void CVICALLBACK menuFILEPrint (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	errChk(SetPrintAttribute (ATTR_COLOR_MODE, VAL_BW));
	errChk(SetPrintAttribute (ATTR_PRINT_AREA_HEIGHT, VAL_USE_ENTIRE_PAPER));
	errChk(SetPrintAttribute (ATTR_PRINT_AREA_WIDTH, VAL_USE_ENTIRE_PAPER));
	err=PrintPanel (panel, "", 1, VAL_FULL_PANEL, 1);
	if (VAL_USER_CANCEL==err)
		err=0;
	errChk(err);
Error:
	if (err) {Beep();}
}

//*********************************************************************
//																	   
//																	   
//			popupCLUSTER menu callbacks are implimented here		   
//		They need to access BRAIN globals then call FACETWO functions  
//																	   
//*********************************************************************



void CVICALLBACK popupCLUSTEROutput (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int UIIndex,BrainIndex;
	
	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (0>BrainIndex) {
		errChk(MessagePopup ("Bad Index", "You cannot use this cluster"));
		return;
	};
	errChk(SetIdleWord(BrainIndex));
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupCLUSTERInsert (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int UIIndex,BrainIndex;
	
	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (-1==BrainIndex)						// Insert at end
		BrainIndex=Clusters.NumberOfClusters;
	errChk(dna_InsertCluster (&Clusters,BrainIndex));
	Clusters.Clusters[BrainIndex].EnabledQ=dnaTrue;	// Assume they want it !
	errChk(faceSetNumberOfClusters());
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupCLUSTERDuplicate (int menuBar, int menuItem, void *callbackData,
		int panel)
// Duplicate does not use the Clipboard.
{
	int err=0;
	int UIIndex,BrainIndex;
	
	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (0>BrainIndex) {
		errChk(MessagePopup ("Bad Index", "You cannot use this cluster"));
		return;
	};
	errChk(dna_InsertCluster (&Clusters,BrainIndex));
	errChk(dna_CopyClusterData(GetCluster(BrainIndex),
			GetCluster(BrainIndex+1)));
	errChk(faceSetNumberOfClusters());
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupCLUSTERCopy (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int UIIndex,BrainIndex;
	
	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (0>BrainIndex) {
		errChk(MessagePopup ("Bad Index", "You cannot use this cluster"));
		return;
	};
	if (Clipboard.Cluster) {				// Clear clipboard item
		errChk(dna_ReInitCluster (Clipboard.Cluster, 0, 0));
	}
	else {									// Get clipboard item
// ALLOCATION
		Clipboard.Cluster=calloc(1,sizeof(dnaCluster));
		nullChk(Clipboard.Cluster);
		errChk(dna_InitCluster (Clipboard.Cluster, 0));
	}
	errChk(dna_CopyClusterData(Clipboard.Cluster,GetCluster(BrainIndex)));
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupCLUSTERCut (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int UIIndex,BrainIndex;
	
	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (0>BrainIndex) {
		errChk(MessagePopup ("Bad Index", "You cannot use this cluster"));
		return;
	};
	if (Clipboard.Cluster) {				// Clear clipboard item
		errChk(dna_ReInitCluster (Clipboard.Cluster, 0, 0));
	}
	else {									// Get clipboard item
// ALLOCATION
		Clipboard.Cluster=calloc(1,sizeof(dnaCluster));
		nullChk(Clipboard.Cluster);
		errChk(dna_InitCluster (Clipboard.Cluster, 0));
	}
	errChk(dna_CopyClusterData(Clipboard.Cluster,GetCluster(BrainIndex)));
	errChk(dna_DeleteCluster (&Clusters, BrainIndex));
	errChk(faceSetNumberOfClusters());
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupCLUSTERPaste (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int UIIndex,BrainIndex;
	//	NOTE FIXME GOTCHA BUG : Nice error if clipbaord is empty
	nullChk(Clipboard.Cluster);				// must have something ot insert
	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (-1==BrainIndex)						// Insert at end
		BrainIndex=Clusters.NumberOfClusters;
	errChk(dna_InsertCluster (&Clusters,BrainIndex));
	errChk(dna_CopyClusterData(GetCluster(BrainIndex),Clipboard.Cluster));
	errChk(faceSetNumberOfClusters());
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupCLUSTERReplace (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int UIIndex,BrainIndex;
	
	nullChk(Clipboard.Cluster);				// must have something ot insert
	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (0>BrainIndex) {
		errChk(MessagePopup ("Bad Index", "You cannot use this cluster"));
		return;
	};
	errChk(dna_ReInitCluster (GetCluster(BrainIndex), 0, 0));
	errChk(dna_CopyClusterData(GetCluster(BrainIndex),Clipboard.Cluster));
	errChk(faceSetNumberOfClusters());
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupCLUSTERDelete (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0;
	int UIIndex,BrainIndex;

	UIIndex=(int)POPData.callback;
	BrainIndex=GetClusterIndex(UIIndex);	// Handy function
	if (0>BrainIndex) {
		errChk(MessagePopup ("Bad Index", "You cannot use this cluster"));
		return;
	};
   	errChk(dna_DeleteCluster (&Clusters, BrainIndex));
	errChk(faceSetNumberOfClusters());
Error:
	if (err) {Beep();}
}

//*********************************************************************
//			popupXYVALUE menu callbacks are implimented here		   
//		They need to access BRAIN globals then call FACETWO functions  
//*********************************************************************

void CVICALLBACK popupXYVALUESort (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,dummy=-1;
	int SortQ;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph)
		SortQ=SortValues(graph,&dummy);
		negChk(SortQ);
		if(SortQ)
			errChk(enableXYValues());
	}
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupXYVALUEDuplicate (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,index;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		index=(int)POPData.callback;
		errChk(dna_DuplicateGraphValue(graph,index));
		errChk(enableXYValues());
	}
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupXYVALUECopy (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,index;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		index=(int)POPData.callback;
		nullChk(graph->XValues);
		nullChk(graph->YValues);
		Clipboard.x=graph->XValues[index];
		Clipboard.y=graph->YValues[index];
	}
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupXYVALUECut (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,index;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		index=(int)POPData.callback;
		nullChk(graph->XValues);
		nullChk(graph->YValues);
		Clipboard.x=graph->XValues[index];
		Clipboard.y=graph->YValues[index];
		errChk(dna_DeleteGraphValue(graph,index));
		errChk(enableXYValues());
	}
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupXYVALUEPaste (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,index;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		index=(int)POPData.callback;
		nullChk(graph->XValues);
		nullChk(graph->YValues);
		errChk(dna_DuplicateGraphValue(graph,index));
		graph->XValues[index]=Clipboard.x;
		graph->YValues[index]=Clipboard.y;
		errChk(enableXYValues());
	}
	else
		err=-1;
Error:
	if (err) {Beep();}
}

void CVICALLBACK popupXYVALUEDelete (int menuBar, int menuItem, void *callbackData,
		int panel)
{
	int err=0,index;
	dnaARFGraph *graph;
	
	if (-1!=ARFGraphs.GraphSelected) {
		graph=GetGraph(ARFGraphs.GraphSelected);
		nullChk(graph);
		index=(int)POPData.callback;
		errChk(dna_DeleteGraphValue(graph,index));
		errChk(enableXYValues());
	}
	else
		err=-1;
Error:
	if (err) {Beep();}
}

//*********************************************************************



int CVICALLBACK IdleZeroCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:
			errChk(SetIdleOutput(dnaFalse));
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK IdleWordCommand (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0;
	switch (event) {
		case EVENT_COMMIT:
			errChk(SetIdleOutput(dnaTrue));
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfMinimumAnalog (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index;
	double MyValue,NewValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			errChk(SetRangeMin(index,MyValue,&NewValue));
			if (MyValue!=NewValue)
				printf("Min Index %i value changed\n",index);
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK arfMaximumAnalog (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,index;
	double MyValue,NewValue;
	switch (event) {
		case EVENT_COMMIT:
			index = (int)callbackData;
 			errChk(GetCtrlVal (panel,control, &MyValue));
			errChk(SetRangeMax(index,MyValue,&NewValue));
			if (MyValue!=NewValue)
				printf("Max Index %i value changed\n",index);
			// easier to just turn off the graph selection
			if (-1!=ARFGraphs.GraphSelected)
				errChk(changedGraphSelection(-1));						
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}


/* CEK 2000/04/07 Events from horiz. scroll bar */
/* The example scrldemo.prj was critical for learning these events */
int CVICALLBACK hscroll (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	/* Special handling for scroll bar events, look at eventData1 */
	MyValue=eventData1;
	switch (event)
		{
		case EVENT_SB_THUMB_CHANGE:
			/* Update the displayed number for precise feedback while user drags scrollbar */
 			errChk(SetCtrlVal (panel,WG_GLOBALOFFSET, MyValue));
			break;
		case EVENT_SB_COMMIT:
			errChk(changedGlobalOffset(MyValue));
			break;
		}
Error:
	if (err<0) {Beep();}
	return 0;
}

int CVICALLBACK clusterGlobalOffset (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	int err=0,MyValue;
	switch (event) {
		case EVENT_COMMIT:
 			errChk(GetCtrlVal (panel,control, &MyValue));
			errChk(changedGlobalOffset(MyValue));
			break;
		case EVENT_LEFT_CLICK:

			break;
		case EVENT_RIGHT_CLICK:

			break;
	}
Error:
	if (err<0) {Beep();}
	return 0;
}

