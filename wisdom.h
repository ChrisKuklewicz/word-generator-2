// WISDOM.h

//	This is the New Development documentation area.
	
/*	Sub-projects
		Change preprocessor directives to be more flexible
$			Add #define NODOBOARD to DNA.h and support that in HAND		(Done 2000/3/24)
$			Add #define NOAOBOARD to DNA.h and support that in HAND		(Done 2000/3/24)
$			Add #define WGFLAG 2 or 3 and support that. (Done? 2000/3/28)			
		Add support for second digital board
$			Alter data structures to hold second board info (expressed DNA) (No Need to do this)
$			Add to the save/load routines (MEMORY) to handle second board (audited 2000/03/28)
$			Alter initial Build routines to make interface for second board (FACE) (Done 2000/3/28)
			Audit HAND to ensure access to second board (Done 2000/3/28) (Test for just 1 board)
$			Audit code in BRAIN/HAND that sets up the hardware (timing) (seems to work 2000/3/28)
			Alter code in BRAIN to provide Idle access to second board (Done 2000/3/28) (Test for just 1 board)
$			Add code in BRAIN to build buffer for second board (Done 2000/3/28)
			Test the second board's output (simple test: it looks right) (Next test with oscilloscape)
		CEK->APC Walk through the analog graph handling code
			Demonstrate code path for displaying graph
			Demonstrate code path for filling buffer
		Fix RTSI connections from DOBoard to DIOBoard and AOBoard in BRAIN/HAND (Done 2000/3/24) 
$			Add ClearRTSI function (Done 2000/03/28)
		Fix up analog code for 6713
$			Alter FACE.uir layout to use only 8, not 10 analog graphs  (Done 2000/03/28)
$			Alter initial Build routines for analog interface (FACE) (Done 2000/03/28)
			Fix min/max analog range to account for being bipolar sometimes!!!~!!!!
			Alter code that builds analog buffer since channels need not be consecutive
				(See the scanner code as an example)
			Audit the analog idle code (Compare with scanner code)
			Audit buffer building
			Test the analog output
		Miscellaneous changes
			Rename hand_ConfigDIO to not use "DIO" in that sense
			Replace pop up progess window with widget on main window 
			Per analog channel toggle between auto scale voltage axis and min-max scale
			Make sub-panel to scroll words up and down (with their labels of course)
$			Add feature to output idle analog values when cancel is pressed (Done 2000/03/30)
			Add button to idle ZERO analog values (and zero RF ?)
			Add feature to RUN OLD buffer, to avoid time to rebuild it
			For RunDone and RunMode, replace magic number values with static constants
$			Darken the orange/green (I forget which) to make it visible when printing Black&White (Done 2000/03/30)
			Request new horizontal scroll interface for words/clusters
		Enhancements to Automation and Control:
			Auto save filename selection based on date (put into automated comment)
			Slave Camera / Master WG instead of other way around.
			The above two together imply a possibly used logging ability:
				If so, then one could synchonize saving the WG3 program with recording the image data.
				The filenames could be related
				Comments could be added to the image and/or WG3 data automatically
				It still won't fix your coffee for you, though.  (Cream? Sugar?)
			Note: One model of controlling the camera is to have WinView run alongside a Visual Basic
				program that controls it.  This VB control program could take order from the WG3 so
				one could tell the WG3 to tell the VB program to tell WinView to setup the Camera
				in a given mode.  Sort of "Preamble" to the experiment setup, or as a pallete of
				commands.  The ability to send commands back and forth between the WG and Camera computer(s)
				offers many possibilies.
*/	

/*	Documentation keywords: These are always in all capital letters.
	
	APC : things changed for WG3 (with ananth), put before the line changed
	
	WG2->3 : Difference between WG2 and WG3 here // TODO : mark analog stuff
	
	ALLOCATION : Dynamic memory makes the program much more flexible and
	powerful.  Dynamic memory allows evil memory leaks and the most evil
	of all, derefencing bad or (shudder) null pointers.  Thus all code that 
	calls calloc (not malloc), realloc, and free should be commented with this keyword so
	it sticks out like a sore thumb and so it can be efficiently searched
	for.  Why? Because!  Also: library functions that return allocated memory
	must be tagged as well.
	
	Note: All calls to free should be followed by assigning zero (i.e. null) to the pointer variable.

	TODO : Use liberally to write down what you need to do / should have done.
	Only files which are finished (ahem, very few indeed) should be free of this
	keyword.

	GOTCHA : Use liberally to indicate breach of best practice programming, this
	is usually use of 'magic numbers' or 'fixed sized arrays' or 'ugly pointer
	manipulation' or anything that could cause problems later if you are not
	careful as hell.
	
	FIXME : This is a TODO / GOTCHA situation that is more serious.  The comment
	ought to provide an idea for a better implimentation.
	
	BUG : These are the most serious.  Use to tag code that reports errors,
	so you do not lose track of where shit happens.
*/

/*	Journal

	4/7/2000 - CEK
	Made sub panel for vertical scrolling and added external horizontal scroll widget.
	Added the scroll.fp function panel to implement horiz. scroll bar.
	Made recent file list work in the menu. It is saved in the windows registry.
	Added menuutil.fp and menuutil.c and advapi32.lib to make this work.
	
	
	C:\cvi\samples\custctrl\scrolbar\scrldemo.prj was critical to learning horiz. scroll
	C:\cvi\samples\userint\panels.prj was critical to learning vert. scroll	
	C:\cvi\samples\toolbox\menudemo.prj was critical to learning the menu list trick

	3/30/2000 - CEK
	Copy change fron WG2 to send idle analog values when run is cancelled
	
	3/28/2000-c - CEK and APC
	Got all digital output working with second card.  RTSI scheme is to share the clock
	from board 1 to board 2.  And share the REQ1 update signal from board 1 to board 2.
	Pattern generation for the first board has INTERNAL request source.
	Pattern generation for the second board has EXTERNAL request source.
	The order of the next two calls is critical.
	The board 2 buffer is sent to DIG_Block_Out first, so it then listens for REQ1.
	The board 1 buffer is sent to DIG_Block_Out second, so it then sends REQ1.
	
	Several interfaces to hand were updated to have dBuf1 and dBuf2 sent.
	This should be tested running a single card, since the new interface may break
	if dBuf2 is NOT used.

	3/28/2000-b - CEK
	Added a ClearRTSI function
	Added #define WGFLAG 2 or 3 to help with "cross-platform" compiling
	Altered the startup code to show 64 lines correctly and 8 analog graphs
	Fixed a memory leak bug in loding digital word data from disk
	Fixed the idle for 64 digital outputs (changed function panel interface)
	Fixed a few magic numbers that were hand coded (4->NumColors,32->dnaXXX)
	Made plan to put digital labels, idle toggle, and canvas into a scrollable 
	 	child-panel of the main window .I expect that this could be done entirely
	 	in the program start-up code, without changing the uir file.  The problem
	 	will come in with assumptions that the parent panel==wg and not child.

	3/24/2000 - APC
	Added RTSI connections from DOBoard to DIOBoard and AOBoard
	Added PCI6713 section to RTSI code. Should work with both AT-AO-10 and 6713.
	

	1/17/2000 - Chris Kuklewicz
	Note that now the latest version is letter Z,  The next stage will
	be to create an unstable branch for WG3 development by Ananth.  The
	best plan is to allow the eventually stable WG3 code to run in both
	the old and new labs, by use of compile time options.  Perhaps
	#define LAB_A or #define LAB_C can be used to control source code.
	A notebook containing the current version Z code and a cflow mappings
	(depth 3 forward, and a backward map) has been printed and is on
	Ananth's bookshelf.
	This comment is being written on Kenobi, which now has the Labwindows
	development enviroenment installed.
	The unstable branch is hosted in the "Active Development" folder.
	
	1/14/2000 - Chris Kuklewicz
	The WG2 network card was not working due to BIOS assigning IRQ 11 to the
	video and ethernet cards.  This was fixed with Johnny's help.
	
	Release X and X2 functioned, but SeeMolasses caused an erroneous
	RF sweep to occur when first run.
	More obviously correct coding has been implimented to hopefully
	fix this behavior.
	Also the negchk in faceShowProgress (line 2576) was occasionally
	triggered. No ill behvior resulted, however; thus this will become
	a silent condition.
	This debugging was done on bimodal.

	5/26/99
	Attempt to make RF execute failure harmless instead of fatal.
	Existing code does not look fatal as is, but try to fix anyway.
	Real problem may have been elsewhere.
	Also add error recovery to Idle RF to try mouth_InitRF, so
	user can turn SRS on and off as neccessary.
*/
	

//******************** OLD COMMENTS FROM TOP OF DNA.h **********************

// After version "WordGenW.exe" I split off into the "New Development"
// directory to work on implimenting user selectable limits on analog
// and RF channel voltages, in order to help protect connected equiptment.

// Up till now, the dnaARFGraph structure held no reference to the index
// of the channel it corresponded to. This forced the code to be
// more organaized about its local knowledge. This minimalistic goal is now an
// obstacle to the best code for implimenting the user selectable limits
// in a clean way. Thus dnaARFGraphs now need to know their index.

// Before a UI graph object had a graph object, now there is a more
// egalitarian correspondance. This may help with cleaning up other bits
// of code, if the index is put to use in pre-existing functions.

// As a matter of convenience, the user selectable ranges will stored
// in the Auxillary info structure. There will be 12 entries, one
// for each analog and rf graph. The usual RF Amp=10, RF Freq=11 mapping
// will be applied.

// Query functions will be built to hide the details. The UI controls will
// be added last.

//******************** 

//	WG2 Change log
//	* : Tried to fix some error handling
//	W : Changed to 10 unipolar outputs instead of 6 + 4 bipolar
//	V : Fixed cancel of printer dialog error reporting. Catch small rfbuf error.
//		Will now check for cancel reuqested while waiting for RF, kind of.
//	U : IBDEV 1024 limit error fixed. "Cancel Requested" is now graceful
//	T : Error reporting includes err value now, DuplicateApp->Expt Off
//		Now will idle analog when run is pressed (not after each repetition)
//	S : F5->Run shortcut, fixed graph label centering
//	R : altered ARF "Run" to dim during run. ValidateChannels() added.
//		fixed error handling in DNA.c
//	Q : Replaced some require checks with message box errors,
//		altered UI right and left clicks, dimmed idle controls for run.
//	P : Fixed a bug in the analog buffer building calls
//	O : Some minor tweaks
//	N : Added a check for already executing copies of the program
//	M : DDE callback uses MsgOpenIt as a deferred call
//	L : DDE file open works for *.WG files
//	K : changed wati for aux window attributes and run progress attributes
//	J : changed WaitForAux to set Aux A0 to 1 when waiting for B4
//	I : changed title bar to filename
//	H : changed wait for aux window attributes
//	G : changed to idle word while waiting for aux (altered ExecuteRun)
//	F : changed to idle word after cancel and after run over
//	... lost in the mists of time ...

//******************** 

