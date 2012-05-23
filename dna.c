#include <userint.h>
#include "face.h"
#include "dna.h"   
//
//This is the DNA of Word Generator 2
//
//This contains implimentations of data structures needed to make
//the program work.
//
//These are used in the BRAIN of Word Generator 2 to instantiate global
//variables.
//
//The FACE of WG2 (user interface source code) uses the BRAIN and hence this DNA
//

// reminder from DNA.h
// typedef enum {dnaNoRange,dnaUnipolar,dnaBipolar,
//					dnaAmplitude,dnaFrequency} dnaRanges;
	    
// RangeMax[dnaAmplitude] for the SRS is 5.0 (volts) but for the amplifier
// after the SRS we need to limit this to 1.0 volt. This could be done
// as a user selection, but to simplify things, we are doing it here
// Obviously, these are all GOTCHA values. The digitizing function used
// for the analog channels was prodded and tested to come up with them.
const double RangeMax[dnaNumRanges]=
		{brainMaxDouble, 9.998779296874,  9.99755859374, 10.0, 30.0};
const double RangeMin[dnaNumRanges]=
		{brainMinDouble,-0.001220703124,-10.00244140624, 0.0,  0.0};
// FROM BRAIN.c
int GetYMin(dnaARFGraph *graph, double *value);
int GetYMax(dnaARFGraph *graph, double *value);

extern brainInterpData InterpData[dnaNumInterps];

int CVIFUNC dna_InitClusterArray (dnaClusterArray *clusterArray,
								   int numberOfClusters, int ports)
{   // Assumes that Clusters is not already allocated

	int i,err=0;
	int outputs;
	dnaCluster *cp;

	clusterArray->NumberOfClusters = numberOfClusters;
	clusterArray->NumberOfPorts = ports;
	outputs=ports*8;					// GOTCHA : hand coded 8, ha ha
	for (i=0; i<outputs; i++) {
		clusterArray->OutputLabelSize[i]=0;
		clusterArray->OutputLabels[i]=0;
	}
// ALLOCATION 
	clusterArray->MemoryUsed = sizeof(dnaCluster)*numberOfClusters;
	clusterArray->Clusters = calloc (numberOfClusters, sizeof(dnaCluster));
	nullChk(clusterArray->Clusters);
	for (i=0,cp=clusterArray->Clusters; i<numberOfClusters; i++,cp++) {
		errChk(dna_InitCluster (cp, 8*ports));  // GOTCHA...
	};
	return 0;
Error:
	return -1;
}

// option = 0 to clear, 1 to copy old cluster data 
int CVIFUNC dna_ReInitClusterArray (dnaClusterArray *clusterArray,
									 int numberOfClusters, int ports, int option)
{
	int i,err=0;
	int outputs;
	int N,M;
	dnaCluster *cp;
	
	nullChk(clusterArray);
	if ((0==option) || (0==numberOfClusters)) {
		outputs=clusterArray->NumberOfPorts*8; 
		// Need to release all the old memory 
		for (i=0; i<outputs; i++) {
			if (clusterArray->OutputLabels[i])
				free(clusterArray->OutputLabels[i]);
			clusterArray->OutputLabelSize[i]=0;
			clusterArray->OutputLabels[i]=0;
		}
		for (i=0; i<clusterArray->NumberOfClusters;i++) {
			// frees all memory used by the cluster 
			errChk(dna_ReInitCluster (&clusterArray->Clusters[i], 0, 0));
		}
		free(clusterArray->Clusters);
		clusterArray->Clusters=0;
		clusterArray->MemoryUsed=0;
// ALLOCATION call 
		errChk(dna_InitClusterArray(clusterArray,numberOfClusters,ports));
	}
	else {
		nullChk(clusterArray->Clusters);
		N=clusterArray->MemoryUsed/sizeof(dnaCluster);	// In memory
		M=clusterArray->NumberOfClusters;				// In use
		if (0==N)
			require(0==clusterArray->Clusters);
		else
			require(0!=clusterArray->Clusters);
		outputs=8*clusterArray->NumberOfPorts;
		for (i=8*ports; i<outputs; i++) {		// free unneeded labels
			if (clusterArray->OutputLabels[i])
				free(clusterArray->OutputLabels[i]);
			clusterArray->OutputLabelSize[i]=0;
			clusterArray->OutputLabels[i]=0;
		}
		if (N<numberOfClusters) {				// Need more memory
// ALLOCATION realloc
			clusterArray->Clusters=realloc(clusterArray->Clusters,
				numberOfClusters*sizeof(dnaCluster));
			nullChk(clusterArray->Clusters);
			clusterArray->MemoryUsed=numberOfClusters*sizeof(dnaCluster);
			for (i=N; i<numberOfClusters; i++)	// Clear new memory area
				errChk(dna_InitCluster (&clusterArray->Clusters[i], 8*ports));
			N=numberOfClusters;
		}
		else if (0==N) {
// ALLOCATION
			clusterArray->Clusters=calloc(numberOfClusters,sizeof(dnaCluster));
			nullChk(clusterArray->Clusters);
			clusterArray->MemoryUsed=numberOfClusters*sizeof(dnaCluster);
			for (i=N; i<numberOfClusters; i++)	// Clear new memory area
				errChk(dna_InitCluster (&clusterArray->Clusters[i], 0));
			N=numberOfClusters;
		}
		//  Now we have at least as much memory as needed
		for (i=numberOfClusters; i<M; i++) // free memory used by old clusters
			errChk(dna_ReInitCluster (&clusterArray->Clusters[i], 0, 0));
		clusterArray->NumberOfClusters=numberOfClusters;
		// calls on old data may be redundant
		for (i=0; i<numberOfClusters; i++)	// adjust ports on new clusters
			errChk(dna_ReInitCluster (&clusterArray->Clusters[i], 8*ports, 1));
		clusterArray->NumberOfPorts=ports;
	}
	return 0;
Error:
	return -1;
}

int CVIFUNC dna_SetOutputLabel (dnaClusterArray *clusterArray, int index,
								char *outputName)
{
	int err=0;
	if (clusterArray->OutputLabels[index]){
		free(clusterArray->OutputLabels[index]);
		clusterArray->OutputLabels[index]=0;
		clusterArray->OutputLabelSize[index]=0;
	}
	if (outputName) {
// ALLOCATION call 
		clusterArray->OutputLabels[index]=StrDup(outputName);
		nullChk(clusterArray->OutputLabels[index]);
		clusterArray->OutputLabelSize[index]=strlen(outputName);
	}
	return 0;
Error:
	return -1;
}

int CVIFUNC dna_InsertCluster (dnaClusterArray *clusterArray, int index)
{
	int err=0;
	int i,N,M;								// Number allocated
	dnaCluster Temp;

	nullChk(clusterArray);
	// make sure graph is in consistant state
	N=clusterArray->MemoryUsed/sizeof(dnaCluster);
	require(N>=0);
	if (0==N)
		require(0==clusterArray->Clusters);
	else
		require(0!=clusterArray->Clusters);
	// make index conform to size of array
	if (index<0)
		index=0;
	M=clusterArray->NumberOfClusters;
	require(M<=N);
	if (index>M)
		index=M;
	// Now do trivial case
	if (0==clusterArray->Clusters) {	// easy case
// ALLOCATION call
		// Allocate space for 4 clusters in clear mode
		errChk(dna_ReInitClusterArray (clusterArray, 4, 
				clusterArray->NumberOfPorts,0));
		// Reducing to one cluster in copy mode keeps memory allocated
		errChk(dna_ReInitClusterArray (clusterArray, 1, 
				clusterArray->NumberOfPorts,1));
		N=4;
		M=1;
		return 0;
	}
	if (N==M) {							// Need more memory
// ALLOCATION realloc
		// Allocate space for 4 clusters in copy mode
		errChk(dna_ReInitClusterArray (clusterArray, N+4, 
				clusterArray->NumberOfPorts,1));
		// Reducing clusters in copy mode keeps memory allocated
		errChk(dna_ReInitClusterArray (clusterArray, M+1, 
				clusterArray->NumberOfPorts,1));
		N+=4;
		M+=1;
		// new piece of memory need not be cleared, probably
	}
	else {	// Initialize extra memory
		errChk(dna_ReInitClusterArray (clusterArray, M+1, 
				clusterArray->NumberOfPorts,1));
		M+=1;
	}
	Temp=clusterArray->Clusters[M-1];		// End one is blank  ?
	// move over 		
	for (i=M-2; i>=index; i--)				// move data over
		clusterArray->Clusters[i+1]=clusterArray->Clusters[i];
	// clear the fields of the "inserted" cluster
	// Clusters[index] no longer "owns" the memory it points to so we
	// can cheat and call InitCluster
	clusterArray->Clusters[index]=Temp;
Error:
	return (err?-1:0);
}

int CVIFUNC dna_DeleteCluster (dnaClusterArray *clusterArray, int index)
{
 	int err=0;   
	int i,N,M;								// Number allocated

	nullChk(clusterArray);
	N=clusterArray->MemoryUsed/sizeof(dnaCluster);
	M=clusterArray->NumberOfClusters;
	// Block of paranoia code
	require(N>0);
	require(M<=N);
	require(index>=0);
	require(index<M);
	require(0!=clusterArray->Clusters);
	// Free memory
	errChk(dna_ReInitCluster (&clusterArray->Clusters[index], 0, 0));
	// Move over data
	for (i=index; i<M-1; i++)				// move data over
		clusterArray->Clusters[i]=clusterArray->Clusters[i+1];
	// Last one now does not own its pointers, so cheat and call init
	errChk(dna_InitCluster (&clusterArray->Clusters[M-1], 0));
	// Now we can truncate the last cluster
	errChk(dna_ReInitClusterArray (clusterArray, M-1,
			clusterArray->NumberOfPorts, 1));
Error:
	return (err?-1:0);
}

int CVIFUNC dna_InitCluster (dnaCluster *clusterPointer,
							  int numberofDigitalValues)
{
	int err=0;
	// Assumes that Digital is not already allocated
	// Some nice default values:
	clusterPointer->LabelSize = 0; 	
	clusterPointer->Label = 0; 
	clusterPointer->Ticks = 1; 
	clusterPointer->TimeUnit = dnaSEC;
	clusterPointer->EnabledQ = dnaFalse;
	clusterPointer->AnalogSelector = dnaARFContinue;
	clusterPointer->AnalogGroup = -1;
	clusterPointer->RFSelector = dnaARFContinue;
	clusterPointer->RFGroup = -1;
	clusterPointer->NumberOfValues = numberofDigitalValues;
// ALLOCATION 
	clusterPointer->MemoryUsed = numberofDigitalValues;
	if (numberofDigitalValues) {
		clusterPointer->Digital = calloc(numberofDigitalValues,sizeof(dnaByte));
		nullChk(clusterPointer->Digital);
	}
	else {
		clusterPointer->Digital=0;
	}
	return 0;
Error:
	return -1;
}

int CVIFUNC dna_ReInitCluster (dnaCluster *clusterPointer,
							   int numberofDigitalValues, int option)
{
	int err=0;
	int i;
	if (0==option) {
		if (clusterPointer->Label) {
			free(clusterPointer->Label);
			clusterPointer->Label=0;
			clusterPointer->LabelSize=0;
		}
		else {
			clusterPointer->LabelSize=0;
		}
		clusterPointer->Ticks=1;
		clusterPointer->TimeUnit = dnaSEC;
		clusterPointer->EnabledQ = dnaFalse;
		clusterPointer->AnalogSelector = dnaARFContinue;
		clusterPointer->AnalogGroup = -1;
		clusterPointer->RFSelector = dnaARFContinue;
		clusterPointer->RFGroup = -1;
		clusterPointer->NumberOfValues = numberofDigitalValues;
		if (clusterPointer->Digital) {
			free(clusterPointer->Digital);
			clusterPointer->Digital=0;
			clusterPointer->MemoryUsed=0;
		}
// ALLOCATION 
		clusterPointer->MemoryUsed = numberofDigitalValues;
		if (numberofDigitalValues) {
			clusterPointer->Digital = calloc(numberofDigitalValues,sizeof(dnaByte));
			nullChk(clusterPointer->Digital);
		}
	}
	else {
		// Need to save the old data 
		if (clusterPointer->Digital) {
			if (numberofDigitalValues>clusterPointer->MemoryUsed) {
// ALLOCATION realloc 
				realloc(clusterPointer->Digital,
						numberofDigitalValues*sizeof(dnaByte));
				// need to initialize the new memory 
				for (i=clusterPointer->MemoryUsed; i<numberofDigitalValues; i++)
					clusterPointer->Digital[i]=0;
				clusterPointer->NumberOfValues = numberofDigitalValues;
				clusterPointer->MemoryUsed = numberofDigitalValues;
				nullChk(clusterPointer->Digital);
			}
			else {
				clusterPointer->NumberOfValues = numberofDigitalValues;
			}
		}
		else {
			if (numberofDigitalValues) {
// ALLOCATION 
				clusterPointer->NumberOfValues = numberofDigitalValues;
				clusterPointer->MemoryUsed = numberofDigitalValues;
				clusterPointer->Digital = calloc(numberofDigitalValues,
						sizeof(dnaByte));
				nullChk(clusterPointer->Digital);
			}
			else {
				clusterPointer->NumberOfValues = numberofDigitalValues;
				clusterPointer->MemoryUsed = numberofDigitalValues;
				clusterPointer->Digital=0;
			}
		}
	}
	return 0;
Error:
	return -1;
}


int CVIFUNC dna_SetClusterLabel (dnaCluster *clusterPointer, char *clusterName)
{
	int err=0;
	// This copies the string of clusterName to fresh memory space
	if (0==clusterPointer) {
		err-1;
		goto Error;
	}
	if (clusterPointer->Label) {
// ALLOCATION 
		free (clusterPointer->Label);
		clusterPointer->LabelSize=0;
	};
	if (clusterName) {
		clusterPointer->LabelSize = strlen (clusterName);     
// ALLOCATION 
		clusterPointer->Label = calloc (clusterPointer->LabelSize+1, 1);
		nullChk(clusterPointer->Label);
		strcpy (clusterPointer->Label, clusterName);
	};

	return 0;
Error:
	return -1;
}

int CVIFUNC dna_SetClusterLength (dnaCluster *clusterPointer,
								   int numberofDigitalValues, int option)
{
	return 0;
}

int CVIFUNC dna_CopyClusterData (dnaCluster *target, dnaCluster *source)
{
 	int i,err=0;
 	
	errChk(dna_ReInitCluster (target, source->NumberOfValues, 0));

	if (source->Label) {
		target->LabelSize=source->LabelSize;
		target->Label=StrDup(source->Label);
	}
	target->Ticks=source->Ticks;
	target->TimeUnit=source->TimeUnit;
	target->EnabledQ=source->EnabledQ;
	target->AnalogSelector=source->AnalogSelector;
	target->AnalogGroup=source->AnalogGroup;
	target->RFSelector=source->RFSelector;
	target->RFGroup=source->RFGroup;
	target->NumberOfValues=source->NumberOfValues;
	target->MemoryUsed=source->MemoryUsed;
	for(i=0;i<source->NumberOfValues;i++)
		target->Digital[i]=source->Digital[i];

Error:
	return (err?-1:0);
}

int CVIFUNC dna_InitARFGroupArray (dnaARFGroupArray *groupArray, int groups,
									dnaByte graphsinaGroup)
{	
	int err=0;
	int i;								// looping;
	
	groupArray->NumberOfGroups=groups;
	groupArray->NumberOfGraphs=graphsinaGroup;
// ALLOCATION 
	if(graphsinaGroup) {
		groupArray->EnabledQ=calloc(graphsinaGroup,sizeof(dnaByte));
		nullChk(groupArray->EnabledQ);
	}
	else {
		groupArray->EnabledQ=0;
	}
	// GOTCHA : dnaAnalogChannels from DNA.h define
	for (i=0; i<dnaAnalogChannels; i++) {
		groupArray->LabelSize[i]=0;
		groupArray->Labels[i]=0;
	}
// ALLOCATION 
	if (groups) {
		groupArray->MemoryUsed = groups*sizeof(dnaARFGroup);
		groupArray->ARFGroups = calloc(groups,sizeof(dnaARFGroup));
 		nullChk(groupArray->ARFGroups);
 	}
 	else {
		groupArray->MemoryUsed = 0;
		groupArray->ARFGroups = 0;
 	}

	for (i=0; i<groups; i++) {
		errChk(dna_InitGroup (&groupArray->ARFGroups[i], 0, graphsinaGroup));
	}
	return 0;
Error:
	return -1;
}

// option = 0 to clear old data, option = 1 to save old data 
int CVIFUNC dna_ReInitARFGroupArray (dnaARFGroupArray *groupArray, int groups,
									 dnaByte graphsinaGroup, int values,
									 int option)
{
	int err=0;
	int i,j;
	dnaByte *B;

	// As a GOTCHA, if graphsinagroup==2 we are RF, otherwise Analog

	if (0==groups)
		graphsinaGroup=0;
	if (0==graphsinaGroup)
		values=0;
	
	if (0==option) {
		// need to free old allocation 
		if (groupArray->EnabledQ) {
			free(groupArray->EnabledQ);
			groupArray->EnabledQ=0;
		}
		for (i=0; i<dnaAnalogChannels; i++) {
			if (groupArray->Labels[i])
				free(groupArray->Labels[i]);
			groupArray->Labels[i]=0;
			groupArray->LabelSize[i]=0;
		}
		if (groupArray->ARFGroups) {
			// Need to erase old group/graph data 
			for (i=0; i<groupArray->NumberOfGroups; i++) {
				dna_ReInitGroup (&groupArray->ARFGroups[i], 0, 0, 0, 0);
			}
			// Now free the group memory 
			free(groupArray->ARFGroups);
			groupArray->MemoryUsed = 0;
			groupArray->ARFGroups = 0;
		}
		if (groups) {
			if(graphsinaGroup) {
				groupArray->EnabledQ=calloc(graphsinaGroup,sizeof(dnaByte));
				nullChk(groupArray->EnabledQ);
			}
			groupArray->MemoryUsed = groups*sizeof(dnaARFGroup);
// ALLOCATION 
			groupArray->ARFGroups = calloc(groups,sizeof(dnaARFGroup));
	 		nullChk(groupArray->ARFGroups);
	 	}
		for (i=0; i<groups; i++) {
// ALLOCATION calls 
			errChk(dna_InitGroup (&groupArray->ARFGroups[i],
					0, graphsinaGroup));
			if (values)					// Add allocation for values
					dna_ReInitGroup (&groupArray->ARFGroups[i], 0,
							graphsinaGroup, values, 0);
		}
	}
	else {  // SAVE WHAT WE CAN
		if (0==groups) {				// Kill them all!
			if (groupArray->EnabledQ) {
				free(groupArray->EnabledQ);
				groupArray->EnabledQ=0;
			}
			for (i=0; i<dnaAnalogChannels; i++) {
				if (groupArray->Labels[i])
					free(groupArray->Labels[i]);
				groupArray->Labels[i]=0;
				groupArray->LabelSize[i]=0;
			}
			if (groupArray->ARFGroups) {
				// Need to erase old group/graph data 
				for (i=0; i<groupArray->NumberOfGroups; i++) {
					dna_ReInitGroup (&groupArray->ARFGroups[i], 0, 0, 0, 0);
				}
				// Now free the group memory 
				free(groupArray->ARFGroups);
				groupArray->MemoryUsed = 0;
				groupArray->ARFGroups = 0;
			}
			groupArray->NumberOfGroups=0;
			groupArray->NumberOfGraphs=0;
		}
		else {							// Save what we can
			if (groupArray->ARFGroups) {	// We have existing memory
				if (groups*sizeof(dnaARFGroup)>groupArray->MemoryUsed) {
					// we need more memory for groups 
// ALLOCATION realloc 
					groupArray->ARFGroups=realloc(groupArray->ARFGroups,
							groups*sizeof(dnaARFGroup));
					nullChk(groupArray->ARFGroups);
					groupArray->MemoryUsed=groups*sizeof(dnaARFGroup);
					// Need to initialize the newly allocated bit of memory 
					// Note : we assume anything we overwrite does not have
					//	any allocated pointers in it 
					for (i=groupArray->NumberOfGroups; i<groups; i++) {
// ALLOCATION call    
						errChk(dna_InitGroup (&groupArray->ARFGroups[i],
								0, graphsinaGroup));
					}
				}			
				else {
					if (groups<groupArray->NumberOfGraphs) {
						// we need less memory for groups 
						// very important we free the excess groups 
						for (i=groups; i<groupArray->NumberOfGroups; i++) {
// ALLOCATION call 		
							errChk(dna_ReInitGroup (&groupArray->ARFGroups[i],
									0, 0, 0, 0));
						}
					}
				}
			}
			else {						// We need fresh memory
// ALLOCATION 
				groupArray->ARFGroups = calloc(groups,sizeof(dnaARFGroup));
		 		nullChk(groupArray->ARFGroups);
				groupArray->MemoryUsed = groups*sizeof(dnaARFGroup);
				for (i=0; i<groups; i++) {
// ALLOCATION call 	// Need to initialize the new memory 
					errChk(dna_InitGroup (&groupArray->ARFGroups[i],
							0, graphsinaGroup));
				}
			}	
			for (i=0; i<groups; i++) {
				errChk(dna_ReInitGroup (&groupArray->ARFGroups[i], 0,
						graphsinaGroup, values, option));
			}
		}

		// We now have the right amount of groups ready 
		// Each with the right number of graphs 
		// And with at least as many values are requested 	
		if (0==graphsinaGroup) {
			if (groupArray->EnabledQ) {
				free(groupArray->EnabledQ);
				groupArray->EnabledQ=0;
			}
		}
		else {
			if (groupArray->EnabledQ) {
// ALLOCATION realloc 
				groupArray->EnabledQ=realloc(groupArray->EnabledQ,
						graphsinaGroup*sizeof(dnaByte));
				nullChk(groupArray->EnabledQ);
				// May need to initilize extra memory, if allocated 
				for (i=groupArray->NumberOfGraphs; i<graphsinaGroup; i++)
					groupArray->EnabledQ[i]=0;
			}
			else {
// ALLOCATION groupArray->EnabledQ=calloc(graphsinaGroup,sizeof(dnaByte));
				nullChk(groupArray->EnabledQ);
			}
		}
		for (i=graphsinaGroup; i<groupArray->NumberOfGroups; i++) {
			if (groupArray->Labels[i])
				free(groupArray->Labels[i]);
			groupArray->Labels[i]=0;
			groupArray->LabelSize[i]=0;
		}			
	}
	// The last thing done in this function : 
	groupArray->NumberOfGroups=groups;
	groupArray->NumberOfGraphs=graphsinaGroup;
	return 0;	
Error:
	return -1;
}

int CVIFUNC dna_DeleteGroup (dnaARFGroupArray *groupArray, int index)
{
	int err=0;
	int i;

	require(index<groupArray->NumberOfGroups);
	// Kill the target
	errChk(dna_ReInitGroup (&groupArray->ARFGroups[index], 0, 0, 0, 0));
	for (i=index; i<groupArray->NumberOfGroups-1;i++)
		groupArray->ARFGroups[i]=groupArray->ARFGroups[i+1];
	// Kill the duplicate
	errChk(dna_ReInitGroup (&groupArray->ARFGroups[groupArray->NumberOfGroups-1],
			0, 0, 0, 0));
	--groupArray->NumberOfGroups;
Error:
	return (err?-1:0);
}

int CVIFUNC dna_SetGraphLabel (dnaARFGroupArray *groupArray, int graphIndex,
							   char *graphName)
{
	int err=0;
	char * buf;
	
	if (graphName) {
		buf = StrDup(graphName);
		nullChk(buf);
		groupArray->LabelSize[graphIndex]=strlen(buf);
		groupArray->Labels[graphIndex]=buf;
	}
	else {
		groupArray->LabelSize[graphIndex]=0;
		groupArray->Labels[graphIndex]=0;
	}
	return 0;
Error:
	return -1;
}

int CVIFUNC dna_InitGroup (dnaARFGroup *groupPointer, char *groupName,
						   dnaByte graphs)

{
	int err=0;
	int i;
	
	if (groupName==0) {
		groupPointer->LabelSize=0;
		groupPointer->Label=0;
	}
	else {
// ALLOCATION   
		groupPointer->LabelSize=strlen(groupName);
		groupPointer->Label=calloc(groupPointer->LabelSize+1,sizeof(char));
	}
	groupPointer->Ticks=1;
	groupPointer->TimeUnit=dnaMSEC;
	groupPointer->TicksD=1;
	groupPointer->TimeUnitD=dnaSEC;
	groupPointer->NumberOfGraphs=graphs;
// ALLOCATION 
	groupPointer->MemoryUsed = graphs * sizeof(dnaARFGraph);
	groupPointer->ARFGraphs = calloc(graphs,sizeof(dnaARFGraph));
	nullChk(groupPointer->ARFGraphs);

	for (i=0; i<graphs; i++) {
		errChk(dna_InitARFGraph(&groupPointer->ARFGraphs[i]));
		groupPointer->ARFGraphs[i].RangeIndex=i;
		if (2==groupPointer->NumberOfGraphs)	// Must be RF
			groupPointer->ARFGraphs[i].RangeIndex+=dnaAnalogChannels;
	}
	return 0;
Error:
	return -1;
}


// option =0 to clear old data, option =1 to save old data 
// graphs=0 frees memory 
// values is number of XY pairs to allocate in the graphs
// values must be !=0 to reallocate graphs
int CVIFUNC dna_ReInitGroup (dnaARFGroup *groupPointer, char *groupName,
							 dnaByte graphs, int values, int option)
{
	int err=0;
	int i;
	
	if (0==graphs)
		values=0;
	
	if (0==option) {
		if (groupPointer->Label) {
			free(groupPointer->Label);
			groupPointer->Label=0;
			groupPointer->LabelSize=0;
		}
		if (groupName) {
			groupPointer->LabelSize=strlen(groupName);
	// ALLOCATION call 
			groupPointer->Label=StrDup(groupName);
		}
		groupPointer->Ticks=1;
		groupPointer->TimeUnit=dnaMSEC;
		groupPointer->TicksD=1;
		groupPointer->TimeUnitD=dnaSEC;
		if (groupPointer->ARFGraphs) {
			// free the memory that the graphs allocated 
			for (i=0; i<groupPointer->NumberOfGraphs; i++) {
				errChk(dna_ReInitARFGraph (&groupPointer->ARFGraphs[i],
						0, option));	// 0==values IMPLIES Free Memory
			}
			// then lose the memory of the graphs themselves 
			free(groupPointer->ARFGraphs);
			groupPointer->ARFGraphs=0;
			groupPointer->MemoryUsed=0;
		}
		if (graphs) {
			groupPointer->MemoryUsed = graphs * sizeof(dnaARFGraph);
// ALLOCATION 
			groupPointer->ARFGraphs = calloc(graphs,sizeof(dnaARFGraph));
			nullChk(groupPointer->ARFGraphs);
		}
		groupPointer->NumberOfGraphs=graphs;
		for (i=0; i<graphs; i++) {
			errChk(dna_InitARFGraph(&groupPointer->ARFGraphs[i]));
			groupPointer->ARFGraphs[i].RangeIndex=i;
			if (2==graphs)	// Must be RF
				groupPointer->ARFGraphs[i].RangeIndex+=dnaAnalogChannels;
		}
	}
	else {
		if (groupName) {
			if (groupPointer->Label) {
				free(groupPointer->Label);
				groupPointer->Label=0;
				groupPointer->LabelSize=0;
			}
			groupPointer->LabelSize=strlen(groupName);
	// ALLOCATION call 
			groupPointer->Label=StrDup(groupName);
		}
		if (0==graphs) {				// blow away existing data
			if (groupPointer->ARFGraphs) {
				// Need to free existing graphs 
				for (i=0; i<groupPointer->NumberOfGraphs; i++) {
					errChk(dna_ReInitARFGraph(&groupPointer->ARFGraphs[i],
							0,0));
				}
				free(groupPointer->ARFGraphs);
				groupPointer->ARFGraphs=0;
			}
			groupPointer->MemoryUsed=0;
			groupPointer->NumberOfGraphs=0;
		}
		else {
			if (groupPointer->ARFGraphs) {
				if (graphs*sizeof(dnaARFGraph)>groupPointer->MemoryUsed) {
					// Need more memory 
// ALLOCATION realloc 
					groupPointer->ARFGraphs=realloc(groupPointer->ARFGraphs,
							graphs*sizeof(dnaARFGraph));
					nullChk(groupPointer->ARFGraphs);
					groupPointer->MemoryUsed=graphs*sizeof(dnaARFGraph);
					// Must initialize the newly allocated piece of memory 
					// Note : everything beyond NumberOfGraphs but that was
					//	part of old memory block is assumed to have been
					//	NOT holding only any allocated pointers 
					for (i=groupPointer->NumberOfGraphs; i<graphs; i++) {
						errChk(dna_InitARFGraph(&groupPointer->ARFGraphs[i]));
						groupPointer->ARFGraphs[i].RangeIndex=i;
						if (2==graphs)	// Must be RF
							groupPointer->ARFGraphs[i].RangeIndex+=dnaAnalogChannels;
					}								  
				}
				else {
					if (graphs<groupPointer->NumberOfGraphs) {
						// Need fewer graphs, destroy extra ones 
						for (i=graphs; i<groupPointer->NumberOfGraphs; i++) {
							errChk(dna_ReInitARFGraph(
									&groupPointer->ARFGraphs[i],0,0));
							groupPointer->ARFGraphs[i].RangeIndex=i;
							if (2==graphs)	// Must be RF
								groupPointer->ARFGraphs[i].RangeIndex+=dnaAnalogChannels;
						}		
					}
				}
			}
			else {						// We get fresh memory
				groupPointer->MemoryUsed = graphs * sizeof(dnaARFGraph);
// ALLOCATION 
				groupPointer->ARFGraphs = calloc(graphs,sizeof(dnaARFGraph));
				nullChk(groupPointer->ARFGraphs);
			}
			// Now we can manipulate the graphs 
			for (i=0; i<graphs; i++) {
				errChk(dna_ReInitARFGraph(&groupPointer->ARFGraphs[i],
						values,option));
				groupPointer->ARFGraphs[i].RangeIndex=i;
				if (2==graphs)	// Must be RF
					groupPointer->ARFGraphs[i].RangeIndex+=dnaAnalogChannels;
			}
		}
		// Lastly we can set the NumberOfGraphs 
		groupPointer->NumberOfGraphs=graphs;
	}
	return 0;
Error:
	return -1;
}


// HOW THE HELL CAN I FIX THIS WITH RESPECT TO RANGEINDEX?
// ANSWER : LIKE RANGE BEFORE, DO NOT COPY RANGEINDEX
int CVIFUNC dna_CopyARFGroup (dnaARFGroup *destination, dnaARFGroup *source)
{
	int err=0;
	int i,max;							// max is total graphs allocated
	nullChk(destination);
	nullChk(source);
	errChk(dna_ReInitGroup (destination, 0, 0, 0, 0));	// erase destination
// ALLOCATION call
	errChk(dna_ReInitGroup (destination, 0, source->NumberOfGraphs, 0, 0));
// ALLOCATION call
	errChk(dna_SetGroupLabel (destination, source->Label));
	destination->Ticks=source->Ticks;
	destination->TimeUnit=source->TimeUnit;
	destination->TicksD=source->TicksD;
	destination->TimeUnitD=source->TimeUnitD;
	destination->MemoryUsed=source->MemoryUsed;
	// copy the graphs
	max=(destination->MemoryUsed)/sizeof(dnaARFGraph);
	if (0<max)
		nullChk(destination->ARFGraphs);
	for (i=0;i<max;i++) {
// ALLOCATION call
		errChk(dna_CopyARFGraph(&destination->ARFGraphs[i],
				&source->ARFGraphs[i]));
	}
Error:
	return (err?-1:0);
}

int CVIFUNC dna_SetGroupLabel (dnaARFGroup *groupPointer, char *groupName)
{
	int err=0;
	nullChk(groupPointer);
	if (groupPointer->Label)
// ALLOCATION free 
		free(groupPointer->Label);
	groupPointer->LabelSize=0;
	if (groupName) {
		groupPointer->Label = StrDup (groupName);
		nullChk(groupPointer->Label);
		groupPointer->LabelSize=strlen(groupName);
	}
	else {
		groupPointer->Label=0;
		groupPointer->LabelSize=0;
	}
	
	return 0;
Error :
	return -1;
}


// NOT IMPLIMENTED
int CVIFUNC dna_SetGroupGraphs (dnaARFGroup *groupPointer, dnaByte graphs)
{
	int err=0;
	require(0);	// die if you try....
Error:
	return -1;	
}

int CVIFUNC dna_DuplicateGraphValue (dnaARFGraph *graphPointer, int index)
{
	int err=0,i;
	int max;
	nullChk(graphPointer);
  	max = graphPointer->ValueMemoryUsed/sizeof(double);

  	// Paranoia state verification
	require(0<max);
	nullChk(graphPointer->XValues);
	nullChk(graphPointer->YValues);
  	require(0<=graphPointer->NumberOfValues);
  	require(graphPointer->NumberOfValues<=max);
  	require(0<=index);
  	require(index<=graphPointer->NumberOfValues);
	// Now move data
   	for (i=max-1; i>index; i--) {
   		graphPointer->XValues[i]=graphPointer->XValues[i-1];
   		graphPointer->YValues[i]=graphPointer->YValues[i-1];
   	}
   	if (graphPointer->NumberOfValues<max)
   		++graphPointer->NumberOfValues;
Error:
	return (err?-1:0);
}


int CVIFUNC dna_DeleteGraphValue (dnaARFGraph *graphPointer, int index)
{
	int err=0,i;
	int max;
	nullChk(graphPointer);
  	max = graphPointer->ValueMemoryUsed/sizeof(double);
 
 	// Paranoia state verification
	require(0<max);
	nullChk(graphPointer->XValues);
	nullChk(graphPointer->YValues);
  	require(0<=graphPointer->NumberOfValues);
  	require(graphPointer->NumberOfValues<=max);
 	require(0<=index);
  	require(index<=graphPointer->NumberOfValues);
	// Now move data
   	for (i=index+1; i<max; i++) {
   		graphPointer->XValues[i-1]=graphPointer->XValues[i];
   		graphPointer->YValues[i-1]=graphPointer->YValues[i];
   	}
   	if (0<graphPointer->NumberOfValues)
   		--graphPointer->NumberOfValues;
	
Error:
	return (err?-1:0);
}

// i had to make this a more paranoid function 
int CVIFUNC dna_CalcXYValuesMinMax (dnaARFGraph *graphPointer, int which)
{
  	int i,err=0;
  	double min,max;
  	
  	nullChk(graphPointer);
	if (!InterpData[graphPointer->InterpType].ValuesEnabledQ) {
			graphPointer->XMin = 0.0;  			
			graphPointer->XMax = 0.0;  			
			graphPointer->YMin = 0.0;  			
			graphPointer->YMax = 0.0;
	}
	else {
	  	if (graphPointer->NumberOfValues)	{
	 		if (which && 1) {
		  		min = graphPointer->XValues[0];
		  		max = graphPointer->XValues[0];
		  		for (i=1; i<graphPointer->NumberOfValues; i++) {
		  			if (graphPointer->XValues[i]<min)
				  		min = graphPointer->XValues[i];
		  			if (graphPointer->XValues[i]>max)
				  		max = graphPointer->XValues[i];
				}
				graphPointer->XMin = min;  			
				graphPointer->XMax = max;  			
		  	}
		  	if (which && 2) {
		  		min = graphPointer->YValues[0];
		  		max = graphPointer->YValues[0];
		  		for (i=1; i<graphPointer->NumberOfValues; i++) {
		  			if (graphPointer->YValues[i]<min)
				  		min = graphPointer->YValues[i];
		  			if (graphPointer->YValues[i]>max)
				  		max = graphPointer->YValues[i];
				}
				graphPointer->YMin = min;  			
				graphPointer->YMax = max; 
			}
		}
		else {
			// Note that in this case which is ignored and both are cleared 
			graphPointer->XMin = 0.0;  			
			graphPointer->XMax = 0.0;  			
			graphPointer->YMin = 0.0;  			
			graphPointer->YMax = 0.0; 
		}
	}
Error:
	return (err?-1:0);
}

// This assumes stuff 
//
//The XMin,XMax,Param[0],Param[1],Duration,NumberOfValues,XValues,YValues
//must all be correct.
//
// Afterwards XScaled minimum is 0.0 and maximum is Duration/1000.0

int CVIFUNC dna_ScaleGraphValues (dnaARFGraph *graphPointer)
{
	int err=0;
	int i;
	double d;							// duration
	double s;							// x axis scale factor;
	
	// destroy any old scaled data 
// ALLOCATION free 
	if (graphPointer->XScaled)
		free(graphPointer->XScaled);
	graphPointer->XScaled = 0;
	if (graphPointer->YScaled)
		free(graphPointer->YScaled);
	graphPointer->YScaled = 0;

	if (0==graphPointer->NumberOfValues) {
		// Nothing to do 
		return 0;
	}

// ALLOCATION 
	graphPointer->XScaled = calloc(graphPointer->ValueMemoryUsed,1);
	nullChk(graphPointer->XScaled);
	graphPointer->YScaled = calloc(graphPointer->ValueMemoryUsed,1);
	nullChk(graphPointer->YScaled);

	if (0.0!=graphPointer->Duration) {
		// We can scale X values 		
		if (0.0==graphPointer->XMax-graphPointer->XMin)
			s=0.0;
		else
			s=graphPointer->Duration/
					(graphPointer->XMax-graphPointer->XMin); // Scale Factor;
		// Note that if XMin==XMax then all XScaled are zero 
		for (i=0; i<graphPointer->NumberOfValues; i++) {
			graphPointer->XScaled[i]=
					s*(graphPointer->XValues[i]-graphPointer->XMin);
		}
	}
	else {
		// We cannot scale X values 
		for (i=0; i<graphPointer->NumberOfValues; i++) {
			graphPointer->XScaled[i]=0.0;
		}
	}
	
	// Scaling YValues is always possible 
	for (i=0; i<graphPointer->NumberOfValues; i++) {
		graphPointer->YScaled[i]=graphPointer->P[0]
				*graphPointer->YValues[i]+graphPointer->P[1];
	}
Error:
	return (err?-1:0);
}

// Quite paranoid function 
int CVIFUNC dna_PinGraphValues (dnaARFGraph *graphPointer)
{
	int err=0;
	int i,n;
	double min,max;
	
	nullChk(graphPointer);
	require(0!=graphPointer->InterpReadyQ);
	n=graphPointer->NumberOfInterps;
	require(n>=0);							// !!!!!!!!!!!!!!!!!!
	if (0==n)
		return 0;
	nullChk(graphPointer->InterpYValues);
//  USERPIN need to impliment user min/max here?
	errChk(GetYMin(graphPointer,&min));	// look up min
	errChk(GetYMax(graphPointer,&max));	// look up max
	for(i=0;i<n;i++) {
		graphPointer->InterpYValues[i] = 
				Pin (graphPointer->InterpYValues[i],min,max);
	}

Error:
	return (err?-1:0);
}

// Note: destination->RangeIndex is set to -1
int CVIFUNC dna_InitARFGraph (dnaARFGraph *graphPointer)
{
	int i;										// Looping
	graphPointer->NumberOfValues = 0;
//	graphPointer->ClearAfterQ = dnaFalse;
	graphPointer->P[0]=1.0;			// Scale Factor=1.0
	for (i=1; i<dnaNumParams; i++)		// GOTCHA
		graphPointer->P[i]=0.0;
	graphPointer->XMin=0.0;
	graphPointer->XMax=0.0;
	graphPointer->YMin=0.0;
	graphPointer->YMax=0.0;
	graphPointer->RangeIndex=-1;			// RANGEFLAG
	graphPointer->ValueMemoryUsed = 0;
	graphPointer->XValues=0;
	graphPointer->YValues=0;
	graphPointer->InterpType=dnaNoInterp;	// default

	// This is the break between user and interpolation fields 
	
	graphPointer->NumberOfInterps = 0;
	graphPointer->InterpReadyQ = dnaFalse;
	graphPointer->Interp = 0;
	graphPointer->Duration = 0.0;
	graphPointer->XScaled = 0;
	graphPointer->YScaled = 0;
	graphPointer->SplineValues=0;
	graphPointer->InterpMemoryUsed = 0;
	graphPointer->InterpXValues=0;
	graphPointer->InterpYValues=0;
	return 0;
Error:
	return -1;
}

// option =0 to clear old data, option =1 to save old data 
// values=0 frees memory 
// Note: destination->RangeIndex is set to -1
int CVIFUNC dna_ReInitARFGraph (dnaARFGraph *graphPointer, int values, 
		int option)
{
	int err=0;
	int i;

	// Always blow away the interpolation state 
	errChk(dna_TouchARFGraph (graphPointer));						    
	if (0==option) {
		graphPointer->NumberOfValues = values;
//		graphPointer->ClearAfterQ = dnaFalse;
		for (i=0; i<dnaNumParams; i++)	// GOTCHA
			graphPointer->P[i]=0.0;
		graphPointer->XMin=0.0;
		graphPointer->XMax=0.0;
		graphPointer->YMin=0.0;
		graphPointer->YMax=0.0;
		graphPointer->RangeIndex=-1;			// RANGEFLAG
		graphPointer->InterpType=dnaNoInterp;	// default
		if (graphPointer->XValues) {
			free(graphPointer->XValues);
			graphPointer->XValues=0;
		}
		if (graphPointer->YValues) {
			free(graphPointer->YValues);
			graphPointer->YValues=0;
		}
		graphPointer->ValueMemoryUsed = values * sizeof(double);
		if(values) {
// ALLOCATION 
			graphPointer->XValues=calloc(values,sizeof(double));
			nullChk(graphPointer->XValues);
			graphPointer->YValues=calloc(values,sizeof(double));
			nullChk(graphPointer->YValues);
		}
	}
	else {
		if (0==values) {			// We do not want memory allocated
			if (graphPointer->XValues) {
				free(graphPointer->XValues);
				graphPointer->XValues=0;
			}
			if (graphPointer->YValues) {
				free(graphPointer->YValues);
				graphPointer->YValues=0;
			}
			graphPointer->ValueMemoryUsed=0;
			graphPointer->NumberOfValues=0;
		}
		else {
			if (graphPointer->XValues) {	// See if we have memory allocated
				// Now see if we need more memory 
				if (values*sizeof(double)>graphPointer->ValueMemoryUsed) {
	// ALLOCATION realloc 
					graphPointer->XValues=
						realloc(graphPointer->XValues,values*sizeof(double));				
					nullChk(graphPointer->XValues);
					graphPointer->YValues=
						realloc(graphPointer->YValues,values*sizeof(double));				
					nullChk(graphPointer->XValues);
					graphPointer->ValueMemoryUsed=values*sizeof(double);
					// Have to initialize the extra memory we now have 
					for (i=values; i<graphPointer->NumberOfValues; i++) {
						graphPointer->XValues[i]=
							graphPointer->XValues[graphPointer->NumberOfValues];
						graphPointer->YValues[i]=0.0;
					}
				}
				// We now know we have enough memory or more allocated 
				// We do not change the number of values unless we have to:
				//	Thus option=1 will not truncate existing graphs unless
				//	all the memory is freed. The next statement is only
				//	here for paranoia. 
				if (graphPointer->NumberOfValues*sizeof(double) >
						graphPointer->ValueMemoryUsed) 
					graphPointer->NumberOfValues=values;
			}
			else {							// We have no memory to work with
// ALLOCATION 
				graphPointer->XValues=calloc(values,sizeof(double));
				nullChk(graphPointer->XValues);
				graphPointer->YValues=calloc(values,sizeof(double));
				nullChk(graphPointer->YValues);
				graphPointer->ValueMemoryUsed=values*sizeof(double);
				graphPointer->NumberOfValues=values;
			}					
		}
	}
	return 0;
Error:
	return -1;
}

// Note: destination->RangeIndex is set to -1
// HOW THE HELL CAN I FIX THIS WITH RESPECT TO RANGEINDEX?
// ANSWER : LIKE RANGE BEFORE
int CVIFUNC dna_CopyARFGraph (dnaARFGraph *destination, dnaARFGraph *source)
{
 	int err=0; 
	int i,max;
 	
 	nullChk(destination);
 	nullChk(source);
	// Clear out any memory held by destination
	max=source->ValueMemoryUsed/sizeof(double);
	errChk(dna_ReInitARFGraph (destination, max, 0));
	destination->NumberOfValues=source->NumberOfValues;
//	destination->ClearAfterQ=source->ClearAfterQ;
	for (i=0;i<dnaNumParams;i++)
		destination->P[i]=source->P[i];
	destination->RangeIndex=-1;			// RANGEFLAG
	destination->InterpType=source->InterpType;
	destination->ValueMemoryUsed=source->ValueMemoryUsed;
	if (source->XValues) {
		nullChk(destination->XValues);
		for (i=0;i<max;i++)
			destination->XValues[i]=source->XValues[i];
	}
	if (source->YValues) {
		nullChk(destination->YValues);
			for (i=0;i<max;i++)
					destination->YValues[i]=source->YValues[i];
	}
	errChk(dna_TouchARFGraph(destination)); 	
Error:
	return (err?-1:0);
}

int CVIFUNC dna_TouchARFGraph (dnaARFGraph *graphPointer)
{
	// No error handling, since it cannot fail
 	graphPointer->NumberOfInterps = 0;
	graphPointer->InterpReadyQ = dnaFalse;
	// .InterpType/Interp are left alone!
	graphPointer->Duration = 0.0;
	if (graphPointer->XScaled)
		free(graphPointer->XScaled);
	graphPointer->XScaled = 0;
	if (graphPointer->YScaled)
		free(graphPointer->YScaled);
	graphPointer->YScaled = 0;
	if (graphPointer->SplineValues);
		free(graphPointer->SplineValues);
	graphPointer->SplineValues=0;
	graphPointer->InterpMemoryUsed = 0;
	if (graphPointer->InterpXValues)
		free(graphPointer->InterpXValues);
	graphPointer->InterpXValues=0;
	if (graphPointer->InterpYValues)
		free(graphPointer->InterpYValues);
	graphPointer->InterpYValues=0;
	return 0;
}


// if values - 0 it will NOT free the memory , otherwise it will see if it
//has enough memory already, if not it will allocate or reallocate the
//memory to meet the demand 

int CVIFUNC dna_SetGraphValues (dnaARFGraph *graphPointer, int values)
{
	int err=0;
	int i;
	int old,mem;
	double d;
	
	old = graphPointer->NumberOfValues;
	graphPointer->NumberOfValues = values;
	// OLD behavior : see if we need to free all memory here 
	// NEW behavior : if no values, we are done
	if (0==values)  {
		err=0;
		goto Error;
/*		
		if (graphPointer->XValues) {
			free(graphPointer->XValues);
		}
		graphPointer->XValues = 0;
		if (graphPointer->YValues)
			free(graphPointer->YValues);
		graphPointer->YValues = 0;
		err=0;
		goto Error;
*/
	}
	mem = values * sizeof(double);		// Amount of memory required
	// See if any memory already exists, if not get fresh memory 
	if(0==graphPointer->ValueMemoryUsed) {
// ALLOCATION 
		graphPointer->ValueMemoryUsed = mem;
		graphPointer->XValues=calloc(values,sizeof(double));
		nullChk(graphPointer->XValues);
		graphPointer->YValues=calloc(values,sizeof(double));
		nullChk(graphPointer->YValues);
		err=0;
		goto Error;
	}		
	// Now we want to preserve the contents of the memory and reallocate
	// if necessary to hold more values 
	if (mem > graphPointer->ValueMemoryUsed) {
// ALLOCATION realloc 
		graphPointer->ValueMemoryUsed = mem;
		graphPointer->XValues=realloc(graphPointer->XValues,mem);
		nullChk(graphPointer->XValues);
		graphPointer->YValues=realloc(graphPointer->YValues,mem);
		nullChk(graphPointer->YValues);
		for (i=old; i<values; i++) {
			graphPointer->XValues[i]=graphPointer->XMax;
			graphPointer->YValues[i]=0.0;
		}
		err=0;
	}
Error:
	return (err?-1:0);
}

// This assumes it is acting on an uninitialized structure
// Do not call directly except at top of program.
// This is one we use. Is called from SetBufferRF, when ok.
int CVIFUNC dna_InitBufferRF (dnaBufferRF *buffer)
{
	int i;
	buffer->CriticalTime=0;
	buffer->TargetTime=0;
	buffer->NumberOfStages=0;
	buffer->CommandCounter=0;
	for(i=0;i<dnaRFStages;i++) {
		buffer->StartTimes[i]=0;
		buffer->RFTimebase[i]=0;
		buffer->Commands[i]=0;
	}
	buffer->CallTimebase=0;
	buffer->AmpIndex=0;
	buffer->FreqIndex=1;
	buffer->PosByte=0;
	buffer->PosCommand=0;
	buffer->NumberOfCommands=0;
	buffer->MemoryUsed=0;
	buffer->Buffer=0;
	return 0;	
}

// Only call this if dna_InitBufferRF has not been called yet
// Pass this zero bytes to clear the buffer
int CVIFUNC dna_SetupBufferRF (dnaBufferRF *buffer, int bytes)
{
	int err=0;
	
	if (buffer->Buffer) {
		free(buffer->Buffer);
	}
	dna_InitBufferRF (buffer);
	if (bytes) {
// ALLOCATAION 
		buffer->Buffer=calloc(bytes,1);
		nullChk(buffer->Buffer);
		buffer->MemoryUsed=bytes;
	}
Error:
	return (err?-1:0);
}

// Number of NON null bytes stating at current position 
// If current position is null, will return zero 
int CVIFUNC dna_LengthBufferRF (dnaBufferRF *buffer)
{
	int l;
	l=StringLength (buffer->Buffer+buffer->PosByte);
	return l;
}

// if pointing at a null value, will advance by one byte 
// if pointing a null terminated string, will advance 1 byte past 1st null 
int CVIFUNC dna_AdvanceBufferRF (dnaBufferRF *buffer)
{
	int l,err=0;
	require((buffer->PosByte<buffer->MemoryUsed) && 
			(buffer->PosCommand<buffer->NumberOfCommands))
	l=dna_LengthBufferRF(buffer);
	++l;
	buffer->PosByte+=l;
	++buffer->PosCommand;
	++buffer->Commands[buffer->CurrentStage];
Error:
	return (err?-1:0);						// Cannot advance
}

int CVIFUNC dna_AdvanceRunBufferRF (dnaBufferRF *buffer)
{
 	int l,err=0;
	require((buffer->PosByte<buffer->MemoryUsed) && 
			(buffer->PosCommand<buffer->NumberOfCommands))
	l=dna_LengthBufferRF(buffer);
	++l;
	buffer->PosByte+=l;
	++buffer->PosCommand;
	--buffer->CommandCounter;
	buffer->TargetTime+=buffer->CallTimebase;
Error:
	return (err?-1:0);						// Cannot advance
}

int CVIFUNC dna_AdvanceStage (dnaBufferRF *buffer, unsigned int startTime,
							  unsigned int timeBase)
{
	++buffer->NumberOfStages;
	buffer->StartTimes[buffer->CurrentStage]=startTime;
	buffer->RFTimebase[buffer->CurrentStage]=timeBase;
	++buffer->CurrentStage;
	return 0;
}

int CVIFUNC dna_AdvanceRunStage (dnaBufferRF *buffer)
{
	int err=0;
	require(buffer->CurrentStage<buffer->NumberOfStages);
	buffer->CurrentStage++;
	if (buffer->CurrentStage<buffer->NumberOfStages) {
		buffer->TargetTime		=	buffer->StartTimes[buffer->CurrentStage];
		buffer->CommandCounter	=	buffer->Commands[buffer->CurrentStage];
		buffer->CallTimebase	=	buffer->RFTimebase[buffer->CurrentStage];
	}
Error:
	return (err?-1:0);						// Cannot advance
}

int CVIFUNC dna_InitBufferDigital (dnaBufferDigital *buffer)
{
	// This just clears the buffer fields 
	buffer->Pos = 0;
	buffer->TimeBase = 0;
	buffer->NumberOfLines = 0;
	buffer->NumberOfPorts = 0;
	buffer->NumberOfWords = 0;
	buffer->NumberOfBytes = 0;
	buffer->MemoryUsed = 0;
	buffer->Buffer.asVoid = 0;
	return 0;
}

// This can coerce the right number of ports from the lines value 
int CVIFUNC dna_SetupBufferDigital (dnaBufferDigital *buffer, int lines,
									 int words)
{
	int err=0;
// Assumes that non zero buffer->Buffer.asVoid implies previous allocation 
	if ((lines>0)&&(words>0)) {
		buffer->Pos = 0;
		buffer->TimeBase = 0;
		buffer->NumberOfLines = lines;
		buffer->NumberOfPorts = lines / 8;
		if (lines % 8) 
			buffer->NumberOfPorts++;
		buffer->NumberOfWords = words;
// ALLOCATION 
// TODO : Make more intelligent. Don't reallocate if we have enough room
		if (buffer->Buffer.asVoid) 
			free(buffer->Buffer.asVoid);	// free old buffer
		buffer->NumberOfBytes = buffer->NumberOfPorts * words;
		buffer->MemoryUsed = buffer->NumberOfBytes;
		buffer->Buffer.asVoid = calloc(words,buffer->NumberOfPorts);
		nullChk(buffer->Buffer.asVoid);
	}
	else {
		buffer->Pos = 0;
		buffer->TimeBase = 0;
		buffer->NumberOfLines = 0;
		buffer->NumberOfPorts = 0;
		buffer->NumberOfWords = 0;
		buffer->NumberOfBytes = 0;
		buffer->MemoryUsed = 0;
		if (buffer->Buffer.asVoid) 
			free(buffer->Buffer.asVoid);	// free old buffer
		buffer->Buffer.asVoid = 0;
	}
	return 0;
Error:
	return -1;
}

// call this only once to erase the fields 
// do not call again, as it does not free allocated memory and could cause a leak.
int CVIFUNC dna_InitBufferAnalog (dnaBufferAnalog *buffer)
{
	int i;
	buffer->Pos=0;
	buffer->NumberOfOutputs=0;
	for (i=0;i<dnaAnalogChannels;i++) {
		buffer->ChannelVector[i]=0;
	}
	buffer->NumberOfWords=0;
	buffer->NumberOfBytes=0;
	buffer->MemoryUsed=0;
	buffer->Buffer=0;
	return 0;	
}

// Call this only after dna_InitBufferAnalog puts buffer in a good state
// Call with output or words as zero to erase and shut off
int CVIFUNC dna_SetupBufferAnalog (dnaBufferAnalog *buffer, int outputs,
									int words)
{
	int err=0;
	int i;
	
	require(0<=outputs);
	require(0<=words);
	if (0==(outputs*words)) {			// erase buffer
		buffer->Pos=0;
		buffer->NumberOfOutputs=0;
		for (i=0;i<dnaAnalogChannels;i++) {
			buffer->ChannelVector[i]=0;
		}
		buffer->NumberOfWords=0;
		buffer->NumberOfBytes=0;
		if (buffer->Buffer) {
			free(buffer->Buffer);
			buffer->Buffer=0;
		}
		buffer->MemoryUsed=0;
		return 0;						// just return
	}
	buffer->Pos=0;
	buffer->NumberOfOutputs=outputs;
	for (i=0;i<dnaAnalogChannels;i++) {
		buffer->ChannelVector[i]=0;
	}
	buffer->NumberOfWords=words;
	buffer->NumberOfBytes=outputs*words*sizeof(dnaAnalogValue);
// ALLOCATION 
	if (buffer->Buffer) {
		if (buffer->MemoryUsed>=buffer->NumberOfBytes) {
			// re-use memory block, just need to clear it 
			for (i=0;i<words*outputs;i++) {
				buffer->Buffer[i].bipolar=0;
			}
		}
		else {
			// need bigger, empty block so free and calloc 
			free(buffer->Buffer);
			buffer->Buffer=0;
			buffer->MemoryUsed=0;
			if (buffer->NumberOfBytes) {
				buffer->Buffer=calloc(buffer->NumberOfBytes,1);
				nullChk(buffer->Buffer);
				buffer->MemoryUsed=buffer->NumberOfBytes;
			}
		}
	}
	else {
		// need a fresh empty block
		buffer->MemoryUsed=0;
		if (buffer->NumberOfBytes) {
			buffer->Buffer=calloc(buffer->NumberOfBytes,1);
			nullChk(buffer->Buffer);
			buffer->MemoryUsed=buffer->NumberOfBytes;
		}
	}
	return 0;	
Error:
	return -1;
}

int CVIFUNC dna_ResetInitIdle (dnaInitIdle *idle)
// Very simple
{
	int i;	// looping
	for (i=0; i<dnaDigitalOutputs; i++) {	// zero digital idle
		idle->IdleValues[i] = dnaFalse;
	}
	for (i=0; i<dnaAnalogChannels; i++) {	// zero analog idle
		idle->InitAnalog[i]=0.0;
	}
	idle->RFAmplitude=0.0;
	idle->RFOffset=0.0;
	idle->RFFrequency=0.0;
	return 0;    
}

// Sets up the default values
// This used to be done in BRAIN.c
// numGraphs should be the #define faceGRAPHS 12
// dnaAnalogChannels
int CVIFUNC dna_InitAuxInfo (dnaAuxillaryInfo *auxInfo)
{
	int err=0;
 	int i;	// for looping
 	
	//HEADER
	auxInfo->FileVersion=0;
	auxInfo->FileS=0;
	auxInfo->ClassS=0;
	auxInfo->Index=-1;
	auxInfo->DateS=0;
	auxInfo->TimeS=0;
// ALLOCATION CALL
	auxInfo->CommentS=StrDup("User Comment Area");
	nullChk(auxInfo->CommentS)
// ALLOCATION CALL
	auxInfo->AutoCommentS=StrDup("Automated Comment");
	nullChk(auxInfo->AutoCommentS)
	
	//CLUSTER
	auxInfo->GlobalOffset=0;
	auxInfo->OnlyEnabledQ=dnaFalse;
	auxInfo->RepeatRunQ=dnaFalse;
	auxInfo->CameraTriggerQ=dnaFalse;
	auxInfo->UserTicks=100;
	auxInfo->UserTimeUnit=1;
	auxInfo->GTS=0;
	
	//ANALOG/RF
	auxInfo->AnalogIndex=0;
	auxInfo->RFIndex=0;
	auxInfo->GraphSelected=-1;
	auxInfo->PinRangeQ=dnaTrue;
	// range stuff, this violates DNA's lack of knowledge
	// could move this into BRAIN TurnMeOn calling SetRanges/SetDefaults/etc...
	for (i=0; i<dnaAnalogUnipolar; i++)
		auxInfo->Range[i]=dnaUnipolar;
	for (;i<dnaAnalogChannels; i++)
		auxInfo->Range[i]=dnaBipolar;
	// GOTCHA, setup 10 and 11 as the RF controls
	auxInfo->Range[i]=dnaAmplitude;
	auxInfo->Range[i+1]=dnaFrequency;
	// minimum and maximum
	for (i=0; i<faceGRAPHS; i++) {
		auxInfo->UserMin[i]=RangeMin[auxInfo->Range[i]];
		auxInfo->UserMax[i]=RangeMax[auxInfo->Range[i]];
	}
	return 0;	// no way to bomb out until memory is dynamic
Error:
	return -1;
}

int CVIFUNC dna_ResetAuxInfo (dnaAuxillaryInfo *auxInfo)
// Only call this if Initialize has already been called
{
 	int i;	// for looping
	int err=0;
 	
	//HEADER
	auxInfo->FileVersion=0;
	auxInfo->FileS=0;
	auxInfo->ClassS=0;
	auxInfo->Index=-1;
	auxInfo->DateS=0;
	auxInfo->TimeS=0;
	if (auxInfo->CommentS) {
		free(auxInfo->CommentS);
		auxInfo->CommentS=0;
	}
// ALLOCATION CALL
	auxInfo->CommentS=StrDup("User Comment Area");
	nullChk(auxInfo->CommentS)
	if (auxInfo->AutoCommentS) {
		free(auxInfo->AutoCommentS);
		auxInfo->AutoCommentS=0;
	}
// ALLOCATION CALL
	auxInfo->AutoCommentS=StrDup("Automated Comment");
	nullChk(auxInfo->AutoCommentS);
	
	//CLUSTER
	auxInfo->GlobalOffset=0;
	auxInfo->OnlyEnabledQ=dnaFalse;
	auxInfo->RepeatRunQ=dnaFalse;
	auxInfo->CameraTriggerQ=dnaFalse;
	auxInfo->UserTicks=100;
	auxInfo->UserTimeUnit=1;
	auxInfo->GTS=0;
	
	//ANALOG/RF
	auxInfo->AnalogIndex=0;
	auxInfo->RFIndex=0;
	auxInfo->GraphSelected=-1;
	auxInfo->PinRangeQ=dnaTrue;
	// range
	for (i=0; i<dnaAnalogUnipolar; i++)
		auxInfo->Range[i]=dnaUnipolar;
	for (;i<dnaAnalogChannels; i++)
		auxInfo->Range[i]=dnaBipolar;
	// GOTCHA, setup 10 and 11 as the RF controls
	auxInfo->Range[i]=dnaAmplitude;
	auxInfo->Range[i+1]=dnaFrequency;
	// minimum and maximum
	for (i=0; i<faceGRAPHS; i++) {
		auxInfo->UserMin[i]=RangeMin[auxInfo->Range[i]];
		auxInfo->UserMax[i]=RangeMax[auxInfo->Range[i]];
	}
	return 0;	// no way to bomb out until memory is dynamic
Error:
	return -1;
}


double CVIFUNC dna_SetRangeMin (dnaAuxillaryInfo *auxInfo, int index,
								double value)
// returns the value assigned, which may differ from that passed
{
	int err=0;
	// parameter check
	require((0<=index)&&(index<faceGRAPHS));
	// sanity check
	require(auxInfo->UserMin[index]>=RangeMin[auxInfo->Range[index]]);
	require(auxInfo->UserMax[index]<=RangeMax[auxInfo->Range[index]]);
	require(auxInfo->UserMin[index]<=auxInfo->UserMax[index]);
	// value replacement
	value = Pin (value, RangeMin[auxInfo->Range[index]],auxInfo->UserMax[index]);
	auxInfo->UserMin[index]=value;
	return value;
Error:
	printf("SERIOUS ERROR, %i OUT OF RANGE !\n",index);
	return 0.0;
}

double CVIFUNC dna_SetRangeMax (dnaAuxillaryInfo *auxInfo, int index,
								double value)
// returns the value assigned, which may differ from that passed
{
	int err=0;
	// parameter check
	require((0<=index)&&(index<faceGRAPHS));
	// sanity check
	require(auxInfo->UserMin[index]>=RangeMin[auxInfo->Range[index]]);
	require(auxInfo->UserMax[index]<=RangeMax[auxInfo->Range[index]]);
	require(auxInfo->UserMin[index]<=auxInfo->UserMax[index]);
	// value replacement
	value = Pin (value,auxInfo->UserMin[index], RangeMax[auxInfo->Range[index]]);
	auxInfo->UserMax[index]=value;
	return value;
Error:
	printf("SERIOUS ERROR, %i OUT OF RANGE !\n",index);
	return 0.0;
}

