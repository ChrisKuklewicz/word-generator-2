
// Memory.c for saving and loading files
// This will impliment the hdf interface for saving files
#include "dna.h"
#include "memory.h"
#include "brain.h"
#include "inifile.h"
//#define BUGHUNT2


/* Warning : GOTCHA :
 * The Ini_GetData call calloc internally
 * This could trick me into making a memory leak
 */

static IniText Ini;						// Holds the handle for the current action
typedef const unsigned char * PutIniData;
typedef unsigned char * * GetIniData;

// Number indicates which version of the file structure is in use
static const int CurrentVersion = 2;
// formatIndex is used in adding an underscore and integer index to a Tag
static const char formatIndex[] = "%s<%s_%d";
static const char formatAppend[] = "%s[a]<_%s";
// These denote dna structures
// Section Headings have their names in theses constants:
static const char TagAuxillaryInfo[] = "AuxillaryInfo";
static const char TagClusterArray[] = "ClusterArray";
static const char TagCluster[] = "Cluster";
static const char TagAGroupArray[] = "AGroupArray";
static const char TagRFGroupArray[] = "RFGroupArray";
static const char TagGroup[] = "Group";
static const char TagGraph[] = "Graph";
static const char TagInitIdle[] = "InitIdle";

int Put_Aux(const char PreTag[],int index, const dnaAuxillaryInfo* Aux)
{	// BRAIN.c Auxillaryinfo structure
	int err=0;
	int i;
	char Tag[40];
	char Tag2[40];
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	// File header information
	errChk(Ini_PutInt(Ini,Tag,"FileVerson",Aux->FileVersion));
	errChk(Ini_PutString(Ini,Tag,"FileS",Aux->FileS));
	errChk(Ini_PutString(Ini,Tag,"ClassS",Aux->ClassS));
	errChk(Ini_PutInt(Ini,Tag,"Index",Aux->Index));
	errChk(Ini_PutString(Ini,Tag,"DateS",Aux->DateS));
	errChk(Ini_PutString(Ini,Tag,"TimeS",Aux->TimeS));
	errChk(Ini_PutString(Ini,Tag,"CommentS",Aux->CommentS));
	errChk(Ini_PutString(Ini,Tag,"AutoCommentS",Aux->AutoCommentS));
	// Cluster Window Settings
	errChk(Ini_PutInt(Ini,Tag,"GlobalOffset",Aux->GlobalOffset));
	errChk(Ini_PutInt(Ini,Tag,"OnlyEnabledQ",Aux->OnlyEnabledQ));
	errChk(Ini_PutInt(Ini,Tag,"RepeatRunQ",Aux->RepeatRunQ));
	errChk(Ini_PutInt(Ini,Tag,"CameraTriggerQ",Aux->CameraTriggerQ));
	errChk(Ini_PutInt(Ini,Tag,"UserTicks",Aux->UserTicks));
	errChk(Ini_PutInt(Ini,Tag,"UserTimeUnit",Aux->UserTimeUnit));
	errChk(Ini_PutInt(Ini,Tag,"GTS",Aux->GTS));
	// ARF Window Settings
	errChk(Ini_PutInt(Ini,Tag,"AnalogIndex",Aux->AnalogIndex));
	errChk(Ini_PutInt(Ini,Tag,"RFIndex",Aux->RFIndex));
	errChk(Ini_PutInt(Ini,Tag,"GraphSelected",Aux->GraphSelected));
	errChk(Ini_PutInt(Ini,Tag,"PinRangeQ",Aux->PinRangeQ));
	// And Now put the arrays.....GOTCHA
	// WE PUT THE HARDWARE DEPENDENT Range[] ARRAY INTO THE FILE
	// BUT WE WILL NOT READ IT BACK.
	errChk(Ini_PutInt(Ini,Tag,"NumberOfAnalogRF",faceGRAPHS));
	for (i=0; i<faceGRAPHS; i++) {
		negChk(Fmt(Tag2,formatIndex,"Range",i));
		errChk(Ini_PutDouble(Ini,Tag,Tag2,Aux->Range[i]));
	}
	for (i=0; i<faceGRAPHS; i++) {
		negChk(Fmt(Tag2,formatIndex,"UserMin",i));
		errChk(Ini_PutDouble(Ini,Tag,Tag2,Aux->UserMin[i]));
	}
	for (i=0; i<faceGRAPHS; i++) {
		negChk(Fmt(Tag2,formatIndex,"UserMax",i));
		errChk(Ini_PutDouble(Ini,Tag,Tag2,Aux->UserMax[i]));
	}
Error:
	return (err?-1:0);
}

int Get_Aux(const char PreTag[],int index, dnaAuxillaryInfo* Aux)
{	// Very Easy....Simply a reversal of PutAux.
	// BRAIN.c Auxillaryinfo structure
	int err=0;
	int a,i;
	char Tag[40];
	char Tag2[40];
	// First we will reset to default values
	// This is neccessary since alot of saved files will
	// still be FileVersion==1 and not have UserMin and UserMax
	errChk(dna_ResetAuxInfo (&AuxillaryInfo));
	// Now we can begin
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	// File header information
	err=(Ini_GetInt (Ini, Tag, "FileVersion", &(Aux->FileVersion)));
	negChk(err);
	if (Aux->FileS) free(Aux->FileS);
	negChk(Ini_GetStringCopy(Ini,Tag,"FileS",&Aux->FileS));
	if (Aux->ClassS) free(Aux->ClassS);
	negChk(Ini_GetStringCopy(Ini,Tag,"ClassS",&Aux->ClassS));
	negChk(Ini_GetInt(Ini,Tag,"Index",&Aux->Index));
	if (Aux->DateS) free(Aux->DateS);
	negChk(Ini_GetStringCopy(Ini,Tag,"DateS",&Aux->DateS));
	if (Aux->TimeS) free(Aux->TimeS);
	negChk(Ini_GetStringCopy(Ini,Tag,"TimeS",&Aux->TimeS));
	if (Aux->CommentS) free(Aux->CommentS);
	negChk(Ini_GetStringCopy(Ini,Tag,"CommentS",&Aux->CommentS));
	if (Aux->AutoCommentS) free(Aux->AutoCommentS);
	negChk(Ini_GetStringCopy(Ini,Tag,"AutoCommentS",&Aux->AutoCommentS));
	// Cluster Window Settings
	negChk(Ini_GetInt(Ini,Tag,"GlobalOffset",&Aux->GlobalOffset));
	negChk(Ini_GetInt(Ini,Tag,"OnlyEnabledQ",&Aux->OnlyEnabledQ));
	negChk(Ini_GetInt(Ini,Tag,"RepeatRunQ",&Aux->RepeatRunQ));
	negChk(Ini_GetInt(Ini,Tag,"CameraTriggerQ",&Aux->CameraTriggerQ));
	negChk(Ini_GetInt(Ini,Tag,"UserTicks",&Aux->UserTicks));
	negChk(Ini_GetInt(Ini,Tag,"UserTimeUnit",&Aux->UserTimeUnit));
	negChk(Ini_GetInt(Ini,Tag,"GTS",&Aux->GTS));
	// ARF Window Settings
	negChk(Ini_GetInt(Ini,Tag,"AnalogIndex",&Aux->AnalogIndex));
	negChk(Ini_GetInt(Ini,Tag,"RFIndex",&Aux->RFIndex));
	negChk(Ini_GetInt(Ini,Tag,"GraphSelected",&Aux->GraphSelected));
	negChk(Ini_GetInt(Ini,Tag,"PinRangeQ",&Aux->PinRangeQ));
	// And now get the arrays....GOTCHA
	// Do not bother to try if the FileVersion is old
	if (2<=Aux->FileVersion) {			// GOTCHA with the FileVersion
		// double check.....
		a = Ini_ItemExists (Ini, Tag, "NumberOfAnalogRF");
		if (1!=a) {
			printf("Expected Saved parameter missing\n");
		}
		else {
			// Now we have the info we need to proceed safely
			negChk(Ini_GetInt(Ini,Tag,"NumberOfAnalogRF",&a));
			if (a>faceGRAPHS)			// protect array addressing
				a=faceGRAPHS;			// TODO : DYNAMIC
			// if a<faceGRAPHS; the ResetAuxInfo already setup other info
			// WE PUT THE HARDWARE DEPENDENT Range[] ARRAY INTO THE FILE
			// BUT WE WILL NOT READ IT BACK.
			for (i=0;i<a;i++) {
				negChk(Fmt(Tag2,formatIndex,"UserMin",i));
				negChk(Ini_GetDouble (Ini, Tag, Tag2,&Aux->UserMin[i]));
			}			
			for (i=0;i<a;i++) {
				negChk(Fmt(Tag2,formatIndex,"UserMax",i));
				negChk(Ini_GetDouble (Ini, Tag, Tag2,&Aux->UserMax[i]));
			}			
		}
	}
	// If 1==FileVersion the ResetAuxInfo already setup what we needed.
Error:
	return (err?-1:0);
}

int Put_Cluster(const char PreTag[],int index, const dnaCluster* Cluster)
{
	int err=0,bytes,temp;
	char Tag[40];
	negChk(Fmt(Tag,formatIndex,PreTag,index));
//	if (Cluster->Label)
	errChk(Ini_PutString(Ini,Tag,"Label",Cluster->Label));
	errChk(Ini_PutInt(Ini,Tag,"Ticks",Cluster->Ticks));
	temp=Cluster->TimeUnit;
	errChk(Ini_PutInt(Ini,Tag,"TimeUnit",temp));
	errChk(Ini_PutBoolean(Ini,Tag,"EnabledQ",Cluster->EnabledQ));
	errChk(Ini_PutInt(Ini,Tag,"AnalogSelector",Cluster->AnalogSelector));
	errChk(Ini_PutInt(Ini,Tag,"AnalogGroup",Cluster->AnalogGroup));
	errChk(Ini_PutInt(Ini,Tag,"RFSelector",Cluster->RFSelector));
	errChk(Ini_PutInt(Ini,Tag,"RFGroup",Cluster->RFGroup));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfValues",Cluster->NumberOfValues));
	bytes=sizeof(dnaByte)*Cluster->NumberOfValues;
	errChk(Ini_PutData(Ini,Tag,"Digital",Cluster->Digital,bytes));
Error:
	return (err?-1:0);
}

int Get_Cluster(const char PreTag[],int index, dnaCluster* Cluster)
{
	int err=0,bytes,temp;
	char Tag[40];
	int a;  // Temporary number
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	negChk(Ini_GetStringCopy(Ini,Tag,"Label",&Cluster->Label));
	Cluster->LabelSize=((Cluster->Label)?strlen(Cluster->Label):0);
	
	negChk(Ini_GetInt(Ini,Tag,"Ticks",&Cluster->Ticks));
	negChk(Ini_GetInt(Ini,Tag,"TimeUnit",&temp));
	Cluster->TimeUnit=temp;
	negChk(Ini_GetBoolean(Ini,Tag,"EnabledQ",&a));
	Cluster->EnabledQ=a;
	negChk(Ini_GetInt(Ini,Tag,"AnalogSelector",&a));
	Cluster->AnalogSelector=a;
	negChk(Ini_GetInt(Ini,Tag,"AnalogGroup",&Cluster->AnalogGroup));
	negChk(Ini_GetInt(Ini,Tag,"RFSelector",&a));
	Cluster->RFSelector=a;
	negChk(Ini_GetInt(Ini,Tag,"RFGroup",&Cluster->RFGroup));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfValues",&a));

	/* APC / CEK added 2000-03-28 
	 * dna_ReInitCluster added to fix possible allocation bug
	 * if someone had editied the saved file */
	if (a != Cluster->NumberOfValues ) {
	 	printf("MEMORY.c Get_Cluster reports for Ini %s and Tag %s:\n",Ini,Tag);
	 	printf("New number of digital lines %i\n",a);
	 	printf("Expected number of digital lines %i\n",Cluster->NumberOfValues);
		errChk(dna_ReInitCluster (Cluster, a, dnaTrue));
	}
	/* Now we need to un-allocate the buffer */
	//require(0==Cluster->Digital);
	free(Cluster->Digital);
	Cluster->Digital=0;
// ALLOCATION : Ini_GetData calls calloc and later we must free	Cluster->Digital
	negChk(Ini_GetData(Ini,Tag,"Digital",&Cluster->Digital,&bytes));
	//expected that (bytes==sizeof(dnaByte)*Cluster->NumberOfValues;)
	if (bytes != sizeof(dnaByte)*Cluster->NumberOfValues ) {
		/* This should never happen, unless the file is corrupted */
	 	printf("MEMORY.c Get_Cluster reports for Ini %s and Tag %s:\n",Ini,Tag);
	 	printf("Loaded number of digital lines %i\n",bytes);
	 	printf("Previously expected number of digital lines %i\n",Cluster->NumberOfValues);
	 	printf("Trying to recover, but unlikely to work\n");
		Cluster->NumberOfValues=bytes;
		Cluster->MemoryUsed=bytes;
	}
Error:
	return (err?-1:0);
}

int Put_ClusterArray(const char PreTag[],int index, const dnaClusterArray* ClusterArray)
{
	int err=0,i;
	char Tag[40],Tag2[40];
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfClusters",ClusterArray->NumberOfClusters));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfPorts",ClusterArray->NumberOfPorts));
	// NOTE : Extra parameter to denote length
	errChk(Ini_PutInt(Ini,Tag,"NumberOfOutputLabels",8*ClusterArray->NumberOfPorts));
	for (i=0; i<8*ClusterArray->NumberOfPorts;i++) {
		negChk(Fmt(Tag2,formatIndex,"OutputLabels",i));
//		if (ClusterArray->OutputLabels[i])
		errChk(Ini_PutString (Ini, Tag, Tag2, ClusterArray->OutputLabels[i]));
	}
	negChk(Fmt(Tag,formatAppend,TagCluster));
	for (i=0; i<ClusterArray->NumberOfClusters; i++)
		errChk(Put_Cluster(Tag,i,&ClusterArray->Clusters[i]));
Error:
	return (err?-1:0);
}

int Get_ClusterArray(const char PreTag[],int index, dnaClusterArray* ClusterArray)
{
	int err=0,i;
	char Tag[40],Tag2[40];
	int a,b;
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfClusters",&a));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfPorts",&b));
	errChk(dna_ReInitClusterArray(&Clusters, a, b, 0));	// clear old and repop
	// NOTE : Extra parameter to denote length
	negChk(Ini_GetInt(Ini,Tag,"NumberOfOutputLabels",&a));
	for (i=0; i<a;i++) {
		negChk(Fmt(Tag2,formatIndex,"OutputLabels",i));
		negChk(Ini_GetStringCopy (Ini, Tag, Tag2,&ClusterArray->OutputLabels[i]));
		ClusterArray->OutputLabelSize[i]=((ClusterArray->OutputLabels[i])?
			strlen(ClusterArray->OutputLabels[i]):0);
	}
	negChk(Fmt(Tag,formatAppend,TagCluster));
	for (i=0; i<ClusterArray->NumberOfClusters; i++)
		errChk(Get_Cluster(Tag,i,&ClusterArray->Clusters[i]));
Error:
	return (err?-1:0);
}

int Put_Graph(const char PreTag[],int index, const dnaARFGraph* Graph)
{
	int err=0,i,bytes;
	char Tag[40],Tag2[40];
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfValues",Graph->NumberOfValues));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfParameters",dnaNumParams));
	for (i=0; i<dnaNumParams;i++) {
		negChk(Fmt(Tag2,formatIndex,"P",i));
		errChk(Ini_PutDouble (Ini, Tag, Tag2, Graph->P[i]));
	}
// This has been superceded by RangeIndex as of 2==FileVersion
//	errChk(Ini_PutInt(Ini,Tag,"Range",Graph->Range));
// Anyway...Range is hardware dependent and should not be restored...
// SANITY CHECK before saving.
	if (Graph->RangeIndex<dnaAnalogChannels)
		require(index==Graph->RangeIndex);	// Analog
	else
		require(index+dnaAnalogChannels==Graph->RangeIndex);	// RF
	errChk(Ini_PutInt(Ini,Tag,"RangeIndex",Graph->RangeIndex));
	errChk(Ini_PutInt(Ini,Tag,"InterpType",Graph->InterpType));
	bytes = sizeof(double)*Graph->NumberOfValues;
	errChk(Ini_PutInt(Ini,Tag,"ValueMemoryUsed",bytes));
	errChk(Ini_PutData (Ini, Tag, "XValues", (PutIniData)Graph->XValues,bytes));
	errChk(Ini_PutData (Ini, Tag, "YValues", (PutIniData)Graph->YValues,bytes));
Error:
	return (err?-1:0);
}

int Get_Graph(const char PreTag[],int index, dnaARFGraph* Graph)
{
	int err=0,i,bytes;
	char Tag[40],Tag2[40];
	int a;
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfValues",&Graph->NumberOfValues));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfParameters",&a));	// dnaNumParams
#ifdef BUGHUNT2
	printf("Get_Graph{Tag=%s}\n",Tag);
	printf("Get_Graph{Ini_GetInt,Graph->NumberOfValues=%d}\n",Graph->NumberOfValues);
	printf("Get_Graph{Ini_GetInt,NumberOfParameters=%d}\n",a);
#endif
	if (a>dnaNumParams)
		a=dnaNumParams;
	for (i=0; i<a;i++) {
		negChk(Fmt(Tag2,formatIndex,"P",i));
		negChk(Ini_GetDouble (Ini, Tag, Tag2,&Graph->P[i]));
#ifdef BUGHUNT2
//		printf("Get_Graph{Ini_GetInt,Graph->P[%d]=%f}\n",i,Graph->P[i]);
#endif
	}
// This has been superceded by RangeIndex as of 2==FileVersion
//	negChk(Ini_GetInt(Ini,Tag,"Range",(int*)&Graph->Range));
// Anyway...Range is hardware dependent and should not be restored...
	a = Ini_ItemExists (Ini, Tag, "RangeIndex");
	if (a!=1) {
		// Must be FileVersion==1
		// We do not know if we are an Analog or an RF graph
		Graph->RangeIndex=-1;		
	}
	else {
		negChk(Ini_GetInt(Ini,Tag,"RangeIndex",(int*)&Graph->RangeIndex));
// SANITY CHECK, TODO: make non-fatal, just set to index
		if (Graph->RangeIndex<dnaAnalogChannels)	// GOTCHA
			require(index==Graph->RangeIndex);	// Analog
		else
			require(index+dnaAnalogChannels==Graph->RangeIndex);	// RF
	}
	negChk(Ini_GetInt(Ini,Tag,"InterpType",(int*)&Graph->InterpType));
#ifdef BUGHUNT2      	
	printf("Get_Graph{Ini_GetInt,Graph->Range=%d}\n",Graph->Range);
	printf("Get_Graph{Ini_GetInt,Graph->InterpType=%d}\n",Graph->InterpType);
#endif
	// TODO : Remove this check if nothing is caught (added 2000-03-28)
	require(0==Graph->XValues);
// ALLOCATION
	negChk(Ini_GetData (Ini, Tag, "XValues", (GetIniData)&(Graph->XValues),&bytes));
	if (bytes) nullChk(Graph->XValues);
	// expect that (bytes==sizeof(double)*Graph->NumberOfValues); 
	require(bytes==sizeof(double)*Graph->NumberOfValues);
	// TODO : Remove this check if nothing is caught (added 2000-03-28)
	require(0==Graph->YValues);
// ALLOCATION
	negChk(Ini_GetData (Ini, Tag, "YValues", (GetIniData)&(Graph->YValues),&bytes));
	if (bytes) nullChk(Graph->YValues);
	// expect that (bytes==sizeof(double)*Graph->NumberOfValues); 
	require(bytes==sizeof(double)*Graph->NumberOfValues);
	Graph->ValueMemoryUsed=bytes;
Error:
	return (err?-1:0);
}

int Put_Group(const char PreTag[],int index, const dnaARFGroup* Group)
{
	int err=0,i,bytes;
	char Tag[40];
	negChk(Fmt(Tag,formatIndex,PreTag,index));
//	if (Group->Label)
	errChk(Ini_PutString (Ini, Tag, "Label", Group->Label));
	errChk(Ini_PutInt(Ini,Tag,"Ticks",Group->Ticks));
	errChk(Ini_PutInt(Ini,Tag,"TimeUnit",Group->TimeUnit));
	errChk(Ini_PutInt(Ini,Tag,"TicksD",Group->TicksD));
	errChk(Ini_PutInt(Ini,Tag,"TimeUnitD",Group->TimeUnitD));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfGraphs",Group->NumberOfGraphs));
	negChk(Fmt(Tag,formatAppend,TagGraph));
	for (i=0; i<Group->NumberOfGraphs; i++)
		errChk(Put_Graph(Tag,i,&Group->ARFGraphs[i]));
Error:
	return (err?-1:0);
}

int Get_Group(const char PreTag[],int index, dnaARFGroup* Group)
{
	int err=0,i,bytes;
	char Tag[40];
	int a;
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	negChk(Ini_GetStringCopy(Ini,Tag,"Label",&Group->Label));
	Group->LabelSize=((Group->Label)?strlen(Group->Label):0);
	negChk(Ini_GetInt(Ini,Tag,"Ticks",&Group->Ticks));
	negChk(Ini_GetInt(Ini,Tag,"TimeUnit",(int*)&Group->TimeUnit));
	negChk(Ini_GetInt(Ini,Tag,"TicksD",&Group->TicksD));
	negChk(Ini_GetInt(Ini,Tag,"TimeUnitD",(int*)&Group->TimeUnitD));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfGraphs",&a));
	Group->NumberOfGraphs=a;
#ifdef BUGHUNT2
	printf("Get_Group{Tag=%s}\n",Tag);
	printf("Get_Group{Ini_GetStringCopy,Group->Label=%s}\n",Group->Label);
	printf("Get_Group{Ini_GetInt,Group->Ticks=%d}\n",Group->Ticks);
	printf("Get_Group{Ini_GetInt,Group->TimeUnit=%d}\n",Group->TimeUnit);
	printf("Get_Group{Ini_GetInt,Group->TicksD=%d}\n",Group->TicksD);
	printf("Get_Group{Ini_GetInt,Group->TimeUnitD=%d}\n",Group->TimeUnitD);
	printf("Get_Group{Ini_GetData,Group->NumberOfGraphs, bytes = %d}\n",Group->NumberOfGraphs);
#endif
	negChk(Fmt(Tag,formatAppend,TagGraph));
	for (i=0; i<Group->NumberOfGraphs; i++)
		errChk(Get_Graph(Tag,i,&Group->ARFGraphs[i]));
Error:
	return (err?-1:0);
}

int Put_GroupArray(const char PreTag[],int index, const dnaARFGroupArray* GroupArray)
{
	int err=0,i,bytes;
	char Tag[40],Tag2[40];
	negChk(Fmt(Tag,formatIndex,PreTag,index));
//	if (Group->Label)
	errChk(Ini_PutInt(Ini,Tag,"NumberOfGroups",GroupArray->NumberOfGroups));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfGraphs",GroupArray->NumberOfGraphs));
	bytes=sizeof(dnaByte)*GroupArray->NumberOfGraphs;
	errChk(Ini_PutData (Ini, Tag, "EnabledQ",(PutIniData)GroupArray->EnabledQ,bytes));
#ifdef BUGHUNT2
	printf("Put_GroupArray(%s){Ini_PutData,GroupArray->EnabledQ, bytes = %d}\n",
		Tag,bytes);
#endif
	for (i=0; i<GroupArray->NumberOfGraphs;i++) {
		negChk(Fmt(Tag2,formatIndex,"Labels",i));
//		if (GroupArray->LabelSize[i])
		errChk(Ini_PutString (Ini, Tag, Tag2, GroupArray->Labels[i]));
	}
	negChk(Fmt(Tag,formatAppend,TagGroup));
	for (i=0; i<GroupArray->NumberOfGroups; i++)
		errChk(Put_Group(Tag,i,&GroupArray->ARFGroups[i]));
Error:
	return (err?-1:0);
}

int Get_GroupArray(const char PreTag[],int index, dnaARFGroupArray* GroupArray)
{
	int err=0,i,bytes;
	unsigned char * ptr;
	char Tag[40],Tag2[40];
	int a,b;
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfGroups",&a));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfGraphs",&b));
	errChk(dna_ReInitARFGroupArray (GroupArray, a, b, 0, 0));
	if (GroupArray->EnabledQ)
		free(GroupArray->EnabledQ);
	GroupArray->EnabledQ=0;
	ptr=0;
// ALLOCATION
	negChk(Ini_GetData (Ini, Tag, "EnabledQ", &ptr,&bytes));
	if (bytes) nullChk(ptr);
	GroupArray->EnabledQ=ptr;
	// expected that (bytes==sizeof(dnaByte)*GroupArray->NumberOfGraphs)
	require(bytes==sizeof(dnaByte)*GroupArray->NumberOfGraphs);
#ifdef BUGHUNT2
	printf("Get_GroupArray{Tag=%s}\n",Tag);
	printf("Get_GroupArray{Ini_GetInt,GroupArray->NumberOfGroups=%d}\n",a);
	printf("Get_GroupArray{Ini_GetInt,GroupArray->NumberOfGraphs=%d}\n",b);
	printf("Get_GroupArray{Ini_GetData,GroupArray->EnabledQ, bytes = %d}\n",bytes);
#endif
	for (i=0; i<GroupArray->NumberOfGraphs;i++) {
		negChk(Fmt(Tag2,formatIndex,"Labels",i));
		negChk(Ini_GetStringCopy(Ini,Tag,Tag2,&GroupArray->Labels[i]));
		GroupArray->LabelSize[i]=((GroupArray->Labels[i])?
				strlen(GroupArray->Labels[i]):0);
#ifdef BUGHUNT2
		printf("Get_GroupArray{Ini_GetStringCopy,GroupArray->Labels[%d]=%s}\n",i,GroupArray->Labels[i]);
#endif
	}
	negChk(Fmt(Tag,formatAppend,TagGroup));
	for (i=0; i<GroupArray->NumberOfGroups; i++)
		errChk(Get_Group(Tag,i,&GroupArray->ARFGroups[i]));
Error:
	return (err?-1:0);
}

int Put_InitIdle(const char PreTag[],int index, const dnaInitIdle * InitIdle)
{
	int err=0,bytes;
	char Tag[40];
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfIdleValues",dnaDigitalOutputs));
	bytes = sizeof(dnaByte)*dnaDigitalOutputs;
	errChk(Ini_PutData(Ini,Tag,"IdleValues",(PutIniData)InitIdle->IdleValues,bytes));
	errChk(Ini_PutInt(Ini,Tag,"NumberOfInitAnalog",dnaAnalogChannels));
	bytes = sizeof(double)*dnaAnalogChannels;
	errChk(Ini_PutData(Ini,Tag,"InitAnalog",(PutIniData)InitIdle->InitAnalog,bytes));
	errChk(Ini_PutDouble(Ini,Tag,"RFAmplitude",InitIdle->RFAmplitude));
	errChk(Ini_PutDouble(Ini,Tag,"RFOffset",InitIdle->RFOffset));
	errChk(Ini_PutDouble(Ini,Tag,"RFFrequency",InitIdle->RFFrequency));
Error:
	return (err?-1:0);
}


int Get_InitIdle(const char PreTag[],int index, dnaInitIdle * InitIdle)
{
	int err=0,i,bytes;
	char Tag[40];
	int a;
	dnaByte * IV=0;
	double * IA=0;
	negChk(Fmt(Tag,formatIndex,PreTag,index));
	negChk(Ini_GetInt(Ini,Tag,"NumberOfIdleValues",&a));
	// expect (a==dnaDigitalOutputs);
	negChk(Ini_GetData(Ini,Tag,"IdleValues",(GetIniData)&IV,&bytes));
	if (bytes) nullChk(IV);
	//expect (bytes == sizeof(dnaByte)*dnaDigitalOutputs)
	if (a>dnaDigitalOutputs)
		a=dnaDigitalOutputs;
	for (i=0;i<a;i++)
		InitIdle->IdleValues[i]=IV[i];
	// expect (a==dnaAnalogChannels);
	negChk(Ini_GetInt(Ini,Tag,"NumberOfInitAnalog",&a));
	negChk(Ini_GetData(Ini,Tag,"InitAnalog",(GetIniData)&IA,&bytes));
	if (bytes) nullChk(IA);
	// expect (bytes == sizeof(double)*dnaAnalogChannels)
	if (a>dnaAnalogChannels)
		a=dnaAnalogChannels;
	for (i=0;i<a;i++)
		InitIdle->InitAnalog[i]=IA[i];
	negChk(Ini_GetDouble(Ini,Tag,"RFAmplitude",&InitIdle->RFAmplitude));
	negChk(Ini_GetDouble(Ini,Tag,"RFOffset",&InitIdle->RFOffset));
	negChk(Ini_GetDouble(Ini,Tag,"RFFrequency",&InitIdle->RFFrequency));
Error:
	return (err?-1:0);
}

int Save(int Index)					// Filename
{
	int err=0;
	nullChk(Ini = Ini_New (0));
	AuxillaryInfo.FileVersion=CurrentVersion;
	errChk(Put_Aux(TagAuxillaryInfo,Index,&AuxillaryInfo));
	errChk(Put_InitIdle(TagInitIdle,Index,&InitIdle));
	errChk(Put_ClusterArray(TagClusterArray,Index,&Clusters));
	errChk(Put_GroupArray(TagAGroupArray,Index,&AGroups)); 
	errChk(Put_GroupArray(TagRFGroupArray,Index,&RFGroups)); 
	errChk(Ini_WriteToFile (Ini, AuxillaryInfo.FileS));
Error:
	Ini_Dispose (Ini);
	return (err?-1:0);
}

// TODO : This next function is not working yet
int CVICALLBACK FilterIndex(IniText theIniText,void *callbackData,char *sectionName)
{
	int err=0,index,i;
	char buf[80];
	index=(int)callbackData;
	Scan (sectionName, "%s>%s_%d", buf,i);
	return (i==index);
Error:
	return (err?-1:0);
}

// This is modified from BRAIN.c
int SetARFRanges(void)
// called from TurnMeOn only.
// adjusts the allowed output hardware range settings (card jumpers)
{
	int i,j;
// USERPIN
	// GOTCHA : very ugly code to set bipolar mode
	// see also HAND.c hand_ResetAll and hand_ResetAnalog
	// see also BRAIN.c SetAnalogIdle
	for (i=0; i<AGroups.NumberOfGroups; i++) {
		for (j=0; j<dnaAnalogChannels; j++) {
			AGroups.ARFGroups[i].ARFGraphs[j].RangeIndex=j;
		}
	}
	for (i=0; i<RFGroups.NumberOfGroups; i++) {
		RFGroups.ARFGroups[i].ARFGraphs[0].RangeIndex=dnaAnalogChannels;
		RFGroups.ARFGroups[i].ARFGraphs[1].RangeIndex=dnaAnalogChannels+1;
	}
	return 0;
}

int Load(int Index, const char filename[])
{
	int err=0;
	Ini=Ini_New(0);
	nullChk(Ini);
//	errChk(Ini_SetSectionFilter (Ini, FilterIndex, (void*)Index));
//	printf("Ini_ReadFromFile (Ini,%s)",filename);
	errChk(Ini_ReadFromFile (Ini, filename));
//	errChk(Ini_SetSectionFilter (Ini, 0, 0));
	Ini_Sort (Ini);
	errChk(Get_Aux(TagAuxillaryInfo,Index,&AuxillaryInfo));
	// expect (AuxillaryInfo.FileVersion<=CurrentVersion)
	errChk(Get_InitIdle(TagInitIdle,Index,&InitIdle));
	errChk(Get_ClusterArray(TagClusterArray,Index,&Clusters));
	errChk(Get_GroupArray(TagAGroupArray,Index,&AGroups)); 
	errChk(Get_GroupArray(TagRFGroupArray,Index,&RFGroups)); 

	// We can only tell the difference between Analog and RF
	// groups and graphs in this function, so here is where
	// we have to set this up
	if (2>AuxillaryInfo.FileVersion) {
		errChk(SetARFRanges());
	}
Error:
	Ini_Dispose (Ini);
	return (err?-1:0);
}


/*

//*********************************************************************
//
//			This section is for the old all digital format
//
//*********************************************************************
//


// Save routines have error return capability 
// Load routines just return the object 

int SaveLenString(int file,const char *S)
{
	int err=0;
	int l;
	if (S)
		l = strlen(S);
	else
		l=0;
	FmtFile (file,"%i<%i", l);
	if (l) FmtFile(file,"%*c<%*c[i0]",l,l,S);
Error:
	return (0);
}   

// note that length is size byte on outside 
char * LoadLenString(int file,dnaByte *length)
{   
	int l=0;
	char * S=0;
	ScanFile (file,"%i>%i", &l);
	(*length) = l;
	if (l) {
// ALLOCATION 
		S = calloc(l+1,sizeof(char));
		ScanFile(file,"%*c>%*c[i0]",l,l,S);
	}
	else
		S=0;
	return S;
}   

int SaveInt(int file, int I)
{   FmtFile (file,"%i<%i",I);
	return 0;
}

int LoadInt(int file)
{	int I=0.0;
	ScanFile (file,"%i>%i",&I);
	return I;
}

int SaveDouble(int file, double D)
{   FmtFile (file,"%f<%f",D);
	return 0;
}

double LoadDouble(int file)
{	double D=0.0;
	ScanFile (file,"%f>%f",&D);
	return D;
}

int SaveByteArray(int file, int count, dnaByte *B)
{
	if (count)
		FmtFile (file, "%*i<%*i[b1i0]",count,count,B);
	return 0;
}

dnaByte * LoadByteArray(int file, int count)
{
	dnaByte *B=0;
	if (count) {
// ALLOCATION 
		B=calloc(count,sizeof(dnaByte));
		if (B) ScanFile (file, "%*i>%*i[b1i0]",count,count,B);
	}
	return B;
}

int SaveIntArray(int file, int count, int* I)
{   
	if (count)
		FmtFile (file, "%*i<%*i[i0]",count,count,I);
	return 0;
}

int * LoadIntArray(int file, int count)
{   
	int * I=0;
	if (count) {
// ALLOCATION 
		I=calloc(count,sizeof(int));
		if (I) ScanFile (file, "%*i>%*i[i0]",count,count,I);
	}
	return I;
}

int SaveDoubleArray(int file, int count, double* D)
{   
	if (count)
	FmtFile (file, "%*f<%*f[i0]",count,count,D);
	return 0;
}

double * LoadDoubleArray(int file, int count)
{   
	double * D=0;
	if (count) {
// ALLOCATION 
		D=calloc(count,sizeof(double));
		if (D) ScanFile (file, "%*f>%*f[i0]",count,count,D);
	}
	return D;
}

int SaveAll(int file)
{
	int err=0;
	int i,j,k;							// Looping (all 3 needed)

	errChk(faceSaveAuxillaryInfo());	// Get all the extra data
// Get time and date this file was created 
	AuxillaryInfo.DateS=DateStr();
	AuxillaryInfo.TimeS=TimeStr();
	
// begin the process 
	SaveLenString(file,AuxillaryInfo.DateS);
	SaveLenString(file,AuxillaryInfo.TimeS);
	SaveLenString(file,AuxillaryInfo.CommentS);

	SaveInt(file,0);					// was idleoutput
	SaveInt(file,dnaDigitalOutputs);
	SaveByteArray(file,dnaDigitalOutputs,InitIdle.IdleValues);
	SaveInt(file,dnaAnalogChannels);
	SaveDoubleArray(file,dnaAnalogChannels,InitIdle.InitAnalog);
	
	SaveInt(file,Clusters.NumberOfClusters);
	SaveInt(file,Clusters.NumberOfPorts);
	
	for (i=0; i<dnaDigitalOutputs;i++)
		SaveLenString(file,Clusters.OutputLabels[i]);
	
	for (i=0; i<Clusters.NumberOfClusters; i++) {
		SaveLenString(file,Clusters.Clusters[i].Label);
		SaveInt(file,Clusters.Clusters[i].Ticks);
		SaveInt(file,Clusters.Clusters[i].TimeUnit);
		SaveInt(file,Clusters.Clusters[i].EnabledQ);
		SaveInt(file,Clusters.Clusters[i].AnalogSelector);
		SaveInt(file,Clusters.Clusters[i].AnalogGroup);
		SaveInt(file,Clusters.Clusters[i].RFSelector);
		SaveInt(file,Clusters.Clusters[i].RFGroup);
		SaveInt(file,Clusters.Clusters[i].NumberOfValues);
		SaveByteArray(file,Clusters.Clusters[i].NumberOfValues,
				Clusters.Clusters[i].Digital);
	}
	
	SaveInt(file,AGroups.NumberOfGroups);
	SaveInt(file,AGroups.NumberOfGraphs);
	SaveByteArray(file,AGroups.NumberOfGraphs,AGroups.EnabledQ);
	for (i=0; i<dnaAnalogChannels; i++) {
		SaveLenString(file,AGroups.Labels[i]);
	}
	for (i=0; i<AGroups.NumberOfGroups; i++) {	
		SaveLenString(file,AGroups.ARFGroups[i].Label);
		SaveInt(file,AGroups.ARFGroups[i].Ticks);
		SaveInt(file,AGroups.ARFGroups[i].TimeUnit);
		SaveInt(file,AGroups.ARFGroups[i].TicksD);
		SaveInt(file,AGroups.ARFGroups[i].TimeUnitD);
		SaveInt(file,AGroups.ARFGroups[i].NumberOfGraphs);
		for (j=0; j<AGroups.ARFGroups[i].NumberOfGraphs; j++) {
			SaveInt(file,AGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues);
//			SaveInt(file,AGroups.ARFGroups[i].ARFGraphs[j].ClearAfterQ);
			SaveInt(file,0);
			SaveInt(file,dnaNumParams);	// GOTCHA
			for (k=0; k<dnaNumParams; k++) {
				SaveDouble(file,AGroups.ARFGroups[i].ARFGraphs[j].P[k]);
			}
			SaveDouble(file,AGroups.ARFGroups[i].ARFGraphs[j].XMin);
			SaveDouble(file,AGroups.ARFGroups[i].ARFGraphs[j].XMax);
			SaveDouble(file,AGroups.ARFGroups[i].ARFGraphs[j].YMin);
			SaveDouble(file,AGroups.ARFGroups[i].ARFGraphs[j].YMax);
			SaveInt(file,AGroups.ARFGroups[i].ARFGraphs[j].Range);
			// Note next item is out of order, in some sense 
			SaveInt(file,AGroups.ARFGroups[i].ARFGraphs[j].InterpType);
			SaveDoubleArray(file,
					AGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues,
					AGroups.ARFGroups[i].ARFGraphs[j].XValues);
			SaveDoubleArray(file,
					AGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues,
					AGroups.ARFGroups[i].ARFGraphs[j].YValues);
		}
	}

	SaveInt(file,RFGroups.NumberOfGroups);
	SaveInt(file,RFGroups.NumberOfGraphs);
	SaveByteArray(file,RFGroups.NumberOfGraphs,RFGroups.EnabledQ);
	for (i=0; i<dnaAnalogChannels; i++) {
		SaveLenString(file,RFGroups.Labels[i]);
	}
	for (i=0; i<RFGroups.NumberOfGroups; i++) {	
		SaveLenString(file,RFGroups.ARFGroups[i].Label);
		SaveInt(file,RFGroups.ARFGroups[i].Ticks);
		SaveInt(file,RFGroups.ARFGroups[i].TimeUnit);
		SaveInt(file,RFGroups.ARFGroups[i].TicksD);
		SaveInt(file,RFGroups.ARFGroups[i].TimeUnitD);
		SaveInt(file,RFGroups.ARFGroups[i].NumberOfGraphs);
		for (j=0; j<RFGroups.ARFGroups[i].NumberOfGraphs; j++) {
			SaveInt(file,RFGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues);
//			SaveInt(file,RFGroups.ARFGroups[i].ARFGraphs[j].ClearAfterQ);
			SaveInt(file,0);
			SaveInt(file,dnaNumParams);	// GOTCHA
			for (k=0; k<dnaNumParams; k++) {
				SaveDouble(file,RFGroups.ARFGroups[i].ARFGraphs[j].P[k]);
			}
			SaveDouble(file,RFGroups.ARFGroups[i].ARFGraphs[j].XMin);
			SaveDouble(file,RFGroups.ARFGroups[i].ARFGraphs[j].XMax);
			SaveDouble(file,RFGroups.ARFGroups[i].ARFGraphs[j].YMin);
			SaveDouble(file,RFGroups.ARFGroups[i].ARFGraphs[j].YMax);
			SaveInt(file,RFGroups.ARFGroups[i].ARFGraphs[j].Range);
			// Note next item is out of order, in some sense 
			SaveInt(file,RFGroups.ARFGroups[i].ARFGraphs[j].InterpType);
			SaveDoubleArray(file,
					RFGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues,
					RFGroups.ARFGroups[i].ARFGraphs[j].XValues);
			SaveDoubleArray(file,
					RFGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues,
					RFGroups.ARFGroups[i].ARFGraphs[j].YValues);
		}
	}

	SaveInt(file,AuxillaryInfo.GlobalOffset);
	SaveInt(file,AuxillaryInfo.OnlyEnabledQ);
	SaveInt(file,AuxillaryInfo.AnalogIndex);
	SaveInt(file,AuxillaryInfo.RFIndex);
	SaveInt(file,AuxillaryInfo.GraphSelected);
	SaveInt(file,AuxillaryInfo.PinRangeQ);
	SaveInt(file,AuxillaryInfo.RepeatRunQ);

Error:
	return (err?-1:0);
}

int LoadAll(int file)
{
	int err=0;
	int i,j,k;							// Looping (all 3 needed)
	int a,b,c;							// temporary values
	dnaByte l;								// temporary length
	dnaByte *B;
	double *D;
	
// begin the process 
	AuxillaryInfo.DateS=LoadLenString(file,&l);
	AuxillaryInfo.TimeS=LoadLenString(file,&l);
	AuxillaryInfo.CommentS=LoadLenString(file,&l);

	LoadInt(file);						// was idle output
	c=LoadInt(file);					// GOTCHA : dnaDigitalOutputs
	B=LoadByteArray(file,c);
	for (i=0; i<c; i++)
		InitIdle.IdleValues[i]=B[i];
	free(B);
	c=LoadInt(file);					// GOTCHA : dnaAnalogChannels
	D=LoadDoubleArray(file,c);
	for (i=0; i<c; i++)
		InitIdle.InitAnalog[i]=D[i];
	free(D);

	a=LoadInt(file);					// Clusters.NumberOfClusters
	b=LoadInt(file);					// Clusters.NumberOfPorts
// ALLOCATION CALL 
	errChk(dna_ReInitClusterArray(&Clusters, a, b, 0));	// clear old and repop
	for (i=0; i<dnaDigitalOutputs;i++)
		Clusters.OutputLabels[i]=LoadLenString(file,
				&Clusters.OutputLabelSize[i]);
	for (i=0; i<Clusters.NumberOfClusters; i++) {
		Clusters.Clusters[i].Label			=LoadLenString(file,
				&Clusters.Clusters[i].LabelSize);
		Clusters.Clusters[i].Ticks			=LoadInt(file);
		Clusters.Clusters[i].TimeUnit		=LoadInt(file);
		Clusters.Clusters[i].EnabledQ		=LoadInt(file);
		Clusters.Clusters[i].AnalogSelector	=LoadInt(file);
		Clusters.Clusters[i].AnalogGroup	=LoadInt(file);
		Clusters.Clusters[i].RFSelector		=LoadInt(file);
		Clusters.Clusters[i].RFGroup		=LoadInt(file);
		Clusters.Clusters[i].NumberOfValues	=LoadInt(file);
		Clusters.Clusters[i].Digital		=LoadByteArray(file,
				Clusters.Clusters[i].NumberOfValues);
		Clusters.Clusters[i].MemoryUsed=Clusters.Clusters[i].NumberOfValues*
			sizeof(dnaByte);
	}
	
	a=LoadInt(file);					// AGroups.NumberOfGroups
	b=LoadInt(file);					// AGroups.NumberOfGraphs
	errChk(dna_ReInitARFGroupArray (&AGroups, a, b, 0, 0));
	if (AGroups.EnabledQ)
		free(AGroups.EnabledQ);
	AGroups.EnabledQ=LoadByteArray(file,AGroups.NumberOfGraphs);
	for (i=0; i<dnaAnalogChannels; i++) {
		AGroups.Labels[i]=LoadLenString(file,&AGroups.LabelSize[i]);
	}
	for (i=0; i<AGroups.NumberOfGroups; i++) {	
		AGroups.ARFGroups[i].Label			=LoadLenString(file,
				&AGroups.ARFGroups[i].LabelSize);
		AGroups.ARFGroups[i].Ticks			=LoadInt(file);
		AGroups.ARFGroups[i].TimeUnit		=LoadInt(file);
		AGroups.ARFGroups[i].TicksD			=LoadInt(file);
		AGroups.ARFGroups[i].TimeUnitD		=LoadInt(file);
		AGroups.ARFGroups[i].NumberOfGraphs	=LoadInt(file);
		for (j=0; j<AGroups.ARFGroups[i].NumberOfGraphs; j++) {
			AGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues=LoadInt(file);
//			AGroups.ARFGroups[i].ARFGraphs[j].ClearAfterQ	=LoadInt(file);
			c=LoadInt(file);			// GOTCHA : ClearAfterQ
			c=LoadInt(file);			// GOTCHA : dnaNumParams
			for (k=0; k<c; k++) {
				AGroups.ARFGroups[i].ARFGraphs[j].P[k]	=LoadDouble(file);
			}
			AGroups.ARFGroups[i].ARFGraphs[j].XMin=LoadDouble(file);
			AGroups.ARFGroups[i].ARFGraphs[j].XMax=LoadDouble(file);
			AGroups.ARFGroups[i].ARFGraphs[j].YMin=LoadDouble(file);
			AGroups.ARFGroups[i].ARFGraphs[j].YMax=LoadDouble(file);
			AGroups.ARFGroups[i].ARFGraphs[j].Range=LoadInt(file);
			// Note next item is out of order, in some sense 
			AGroups.ARFGroups[i].ARFGraphs[j].InterpType=LoadInt(file);
			AGroups.ARFGroups[i].ARFGraphs[j].XValues	=LoadDoubleArray(file,
					AGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues);
			AGroups.ARFGroups[i].ARFGraphs[j].YValues	=LoadDoubleArray(file,
					AGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues);
			AGroups.ARFGroups[i].ARFGraphs[j].ValueMemoryUsed=
				AGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues*sizeof(double);
		}
	}

	a=LoadInt(file);					// RFGroups.NumberOfGroups
	b=LoadInt(file);					// RFGroups.NumberOfGraphs
	errChk(dna_ReInitARFGroupArray (&RFGroups, a, b,0,0));
	if (RFGroups.EnabledQ)
		free(RFGroups.EnabledQ);
	RFGroups.EnabledQ=LoadByteArray(file,RFGroups.NumberOfGraphs);
	for (i=0; i<dnaAnalogChannels; i++) {
		RFGroups.Labels[i]=LoadLenString(file,&RFGroups.LabelSize[i]);
	}
	for (i=0; i<RFGroups.NumberOfGroups; i++) {	
		RFGroups.ARFGroups[i].Label			=LoadLenString(file,
				&RFGroups.ARFGroups[i].LabelSize);
		RFGroups.ARFGroups[i].Ticks			=LoadInt(file);
		RFGroups.ARFGroups[i].TimeUnit		=LoadInt(file);
		RFGroups.ARFGroups[i].TicksD			=LoadInt(file);
		RFGroups.ARFGroups[i].TimeUnitD		=LoadInt(file);
		RFGroups.ARFGroups[i].NumberOfGraphs	=LoadInt(file);
		for (j=0; j<RFGroups.ARFGroups[i].NumberOfGraphs; j++) {
			RFGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues=LoadInt(file);
//			RFGroups.ARFGroups[i].ARFGraphs[j].ClearAfterQ	=LoadInt(file);
			c=LoadInt(file);			// GOTCHA : ClearAfterQ
			c=LoadInt(file);			// GOTCHA : dnaNumParams
			for (k=0; k<c; k++) {
				RFGroups.ARFGroups[i].ARFGraphs[j].P[k]	=LoadDouble(file);
			}
			RFGroups.ARFGroups[i].ARFGraphs[j].XMin=LoadDouble(file);
			RFGroups.ARFGroups[i].ARFGraphs[j].XMax=LoadDouble(file);
			RFGroups.ARFGroups[i].ARFGraphs[j].YMin=LoadDouble(file);
			RFGroups.ARFGroups[i].ARFGraphs[j].YMax=LoadDouble(file);
			RFGroups.ARFGroups[i].ARFGraphs[j].Range=LoadInt(file);
			// Note next item is out of order, in some sense 
			RFGroups.ARFGroups[i].ARFGraphs[j].InterpType=LoadInt(file);
			RFGroups.ARFGroups[i].ARFGraphs[j].XValues	=LoadDoubleArray(file,
					RFGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues);
			RFGroups.ARFGroups[i].ARFGraphs[j].YValues	=LoadDoubleArray(file,
					RFGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues);
			RFGroups.ARFGroups[i].ARFGraphs[j].ValueMemoryUsed=
			   RFGroups.ARFGroups[i].ARFGraphs[j].NumberOfValues*sizeof(double);
		}
	}
	AuxillaryInfo.GlobalOffset=LoadInt(file);
	AuxillaryInfo.OnlyEnabledQ=LoadInt(file);
	AuxillaryInfo.AnalogIndex=LoadInt(file);
	AuxillaryInfo.RFIndex=LoadInt(file);
	AuxillaryInfo.GraphSelected=LoadInt(file);
	AuxillaryInfo.PinRangeQ=LoadInt(file);
	AuxillaryInfo.RepeatRunQ=LoadInt(file);

	errChk(SetRanges());
Error:
	return (err?-1:0);
}

*/
