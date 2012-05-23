/************************** Begin Trying to get recent file list working *****************************/

#ifdef WIN32
	#if WGFLAG == 2
		#define REGISTRY "Software\\National Instruments\\Word Generator\\WG2"
	#elif WGFLAG == 3
    	#define REGISTRY "Software\\National Instruments\\Word Generator\\WG3"
  	#endif // WGFLAG
#else // WIN32
  	#if WGFLAG == 2
    	#define REGISTRY "C:\WG2Master.ini"
  	#elif WGFLAG == 3
    	#define REGISTRY "C:\WG3Master.ini"
  	#endif // WGFLAG
#endif // WIN32

/************************** End Trying to get recent file list working *****************************/

int GetFileMenuList(int panel);
int PutFileMenuList(void);

int AddRecentFilename(char * filename);

