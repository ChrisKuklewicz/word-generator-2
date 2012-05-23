#include "dna.h"
#include <utility.h>
#include "menuutil.h"
#include "face.h"
#include "facetwo.h"
#include "recent.h"

static menuList sFileMenuListHandle = 0;		// Hold description of menu file list
static IniText sIniTextHandle = 0;				// Holds description of menu file list for saving
static int maxFileEntries=10;					// Limit to number of entries in the list



/************************** Begin Trying to get recent file list working *****************************/

void CVICALLBACK FILE_Recent(menuList list,
      int menuIndex, int event, void *callbackData)
{
	long temp;
	int err=0;
	int answer;
	char message[255];
	char *filename=(char*)callbackData;
	switch (event) {
		case EVENT_COMMIT:
			/* Use GetFileSize to test if the filename is valid */
			err=GetFileSize (filename, &temp);
			/* Now test to see if the file was found */
			if (-1==err) {
				errChk(MessagePopup ("Error", "File or directory in path not found"));
				err=0;
				goto Error;
			}
			/* Now check for fatal errors */
			errChk(err);
			/* open the file */
			Fmt(message, "%s<Do you want to overwrite\nthe data in memory?");
			answer = ConfirmPopup ("You are about to delete data",message);
			negChk(answer);
			if (dnaTrue!=answer) {
				goto Error;
			}
			errChk(OpenIt(filename));
			break;
		case EVENT_DISCARD:
			if (filename) {
				free(filename);
				filename=0;
			}
			break;
	}
Error:
	if (err<0) {Beep();}
	return;
}

/* This create and loads the recent file list */
/* It should be run once, at the beginning of the the program */
int GetFileMenuList(int panel)
{
	int err=0;
	int menuBar;								// Insert into this menu bar
	int menuID;									// Insert into this menu
	int menuItem;								// Insert before this item
	
	negChk(menuBar = GetPanelMenuBar (panel));
	menuID=WG2MENU_FILE;
	menuItem=WG2MENU_FILE_SEPARATOR2;
	
	if (0==sIniTextHandle) {
		sIniTextHandle=Ini_New (0);
		nullChk(sIniTextHandle);
	}
	
	err=(MU_ReadRegistryInfo (sIniTextHandle, REGISTRY));
//	require(1==err);
	if (0==sFileMenuListHandle) {
		sFileMenuListHandle = MU_CreateMenuList (menuBar, menuID, menuItem,
												 maxFileEntries, FILE_Recent);
		negChk(sFileMenuListHandle);
		err=(MU_SetMenuListAttribute (sFileMenuListHandle, 0,
								ATTR_MENULIST_ALLOW_DUPLICATE_ITEMS,dnaFalse));
//		require(1==err);
		err=(MU_GetFileListFromIniFile (sFileMenuListHandle, sIniTextHandle,
								   "FILEMenuList", "Filename", 1));
//		require(1==err);
	}
Error:
	return 0;
//	return (err?-1:0);
}

/* This should be run once, before the program exits */
/* It save the list into the registry */
int PutFileMenuList(void)
{
	int err=0;
	nullChk(sFileMenuListHandle);
	nullChk(sIniTextHandle);
	err=(MU_PutFileListInIniFile (sFileMenuListHandle, sIniTextHandle,
				   "FILEMenuList", "Filename", 1));
//	require(1==err);
	err=(MU_DeleteMenuList (sFileMenuListHandle));
//	require(1==err);
	sFileMenuListHandle=0;
	err=(MU_WriteRegistryInfo (sIniTextHandle, REGISTRY));
//	require(1==err);
	Ini_Dispose(sIniTextHandle);
	sIniTextHandle=0;
Error:
	return 0;
//	return (err?-1:0);
}

/* AddRecentFilename is called from OpenIt(filename) if there are no other errors */
int AddRecentFilename(char * filename)
{
	int err=0;
	nullChk(sFileMenuListHandle);
	nullChk(filename);
	require(filename[0]!='\0');		
// ALLOCATION : The callbackdata is passed a copy of the filename in a new buffer
// This is freed in the Event_DISCARD of the FILE_Recent callback function
	err=(MU_AddItemToMenuList (sFileMenuListHandle, FRONT_OF_LIST,
			MU_MakeShortFileName (NULL, filename, 32),StrDup(filename)));
//	require(1==err);
Error:
	return 0;
//	return (err?-1:0);
}

/************************** End Trying to get recent file list working *****************************/
