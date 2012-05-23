/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2000. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  ARF                             1       /* callback function: cbARF */
#define  ARF_RUN_2                       2       /* callback function: RunCommand */
#define  ARF_BREAKPOINT                  3       /* callback function: BreakpointCommand */
#define  ARF_UPDATEGRAPH                 4       /* callback function: arfUpdateGraph */
#define  ARF_PINRANGEQ                   5       /* callback function: arfPinRangeQ */
#define  ARF_IDLEANALOG                  6       /* callback function: arfIdleAnalog */
#define  ARF_IDLEAMPLITUDE               7
#define  ARF_IDLEDCOFFSET                8
#define  ARF_IDLEFREQUENCY               9
#define  ARF_IDLERF                      10      /* callback function: arfIdleRF */
#define  ARF_GRAPHINDEX                  11      /* callback function: arfGraphIndex */
#define  ARF_ALABEL                      12      /* callback function: arfALabel */
#define  ARF_AINDEX                      13      /* callback function: arfAIndex */
#define  ARF_ATICKS                      14      /* callback function: arfATicks */
#define  ARF_ATIMEUNIT                   15      /* callback function: arfATimeUnit */
#define  ARF_ATICKSD                     16      /* callback function: arfATicksD */
#define  ARF_ATIMEUNITD                  17      /* callback function: arfATimeUnitD */
#define  ARF_RFLABEL                     18      /* callback function: arfRFLabel */
#define  ARF_RFINDEX                     19      /* callback function: arfRFIndex */
#define  ARF_RFTICKS                     20      /* callback function: arfRFTicks */
#define  ARF_RFTIMEUNIT                  21      /* callback function: arfRFTimeUnit */
#define  ARF_RFTICKSLT                   22
#define  ARF_RFTIMEUNITLT                23      /* callback function: arfRFTimeUnitLT */
#define  ARF_RFTICKSD                    24      /* callback function: arfRFTicksD */
#define  ARF_RFTIMEUNITD                 25      /* callback function: arfRFTimeUnitD */
#define  ARF_INTERP                      26      /* callback function: arfInterp */
#define  ARF_NUMVALUES                   27      /* callback function: arfNumValues */
#define  ARF_XVALUE                      28      /* callback function: arfXValue */
#define  ARF_YVALUE                      29      /* callback function: arfYValue */
#define  ARF_PARAM                       30      /* callback function: arfPARAM */
#define  ARF_GRAPH                       31      /* callback function: arfGraph */
#define  ARF_MAXIMUMANALOG               32      /* callback function: arfMaximumAnalog */
#define  ARF_MINIMUMANALOG               33      /* callback function: arfMinimumAnalog */
#define  ARF_INITIALANALOG               34      /* callback function: arfInitialAnalog */
#define  ARF_ENABLEDQ                    35      /* callback function: arfEnabledQ */
#define  ARF_DECORATION                  36
#define  ARF_DECORATION_2                37
#define  ARF_TEXTMSG                     38
#define  ARF_TEXTMSG_2                   39
#define  ARF_XVALUELABEL                 40
#define  ARF_YVALUELABEL                 41
#define  ARF_INITANALOGTEXT              42
#define  ARF_MINIMUMANALOGTEXT           43
#define  ARF_MAXIMUMANALALOGTEXT         44

#define  AUXWAIT                         2
#define  AUXWAIT_VALUE                   2
#define  AUXWAIT_CANCELBUTTON            3       /* callback function: AuxCancel */
#define  AUXWAIT_LINE                    4
#define  AUXWAIT_TEXTWAIT1               5

#define  DB_COMMENT                      3
#define  DB_COMMENT_COMMENTTEXT          2
#define  DB_COMMENT_CANCEL               3       /* callback function: commentCancelCommand */
#define  DB_COMMENT_RESET                4       /* callback function: commentResetCommand */
#define  DB_COMMENT_ACCEPT               5       /* callback function: commentAcceptCommand */
#define  DB_COMMENT_FILEDATE             6
#define  DB_COMMENT_FILETIME             7

#define  SETUP                           4       /* callback function: cbSetup */
#define  SETUP_NUMBEROFLINES             2       /* callback function: setupNumberOfLines */
#define  SETUP_NUMBEROFGRAPHS            3       /* callback function: setupNumberOfGraphs */
#define  SETUP_NUMBEROFRFGROUPS_2        4       /* callback function: setupNumberOfRFGroups */
#define  SETUP_NUMBEROFAGROUPS           5       /* callback function: setupNumberOfAGroups */
#define  SETUP_NUMBEROFCLUSTERS          6       /* callback function: setupNumberOfClusters */
#define  SETUP_SETUPCANCEL               7       /* callback function: setupCancelCommand */
#define  SETUP_SETUPRESET                8       /* callback function: setupResetCommand */
#define  SETUP_SETUPACCEPT               9       /* callback function: setupAcceptCommand */
#define  SETUP_TEXTMSG                   10

#define  TIMING                          5       /* callback function: cbTiming */
#define  TIMING_RESGCFTICKS              2
#define  TIMING_RESGCFTIMEUNIT           3
#define  TIMING_RESATIMEUNIT             4       /* callback function: resATimeUnit */
#define  TIMING_RESRFTIMEUNIT            5       /* callback function: resRFTimeUnit */
#define  TIMING_RESASKTICKS              6       /* callback function: resAskTicks */
#define  TIMING_TIMINGCANCEL             7       /* callback function: timingCancelCommand */
#define  TIMING_TIMINGRESET              8       /* callback function: timingResetCommand */
#define  TIMING_TIMINGACCEPT             9       /* callback function: timingAcceptCommand */
#define  TIMING_RESTICKS                 10
#define  TIMING_RESASKTIMEUNIT           11      /* callback function: resAskTimeUnit */
#define  TIMING_RESATICKS                12      /* callback function: resATicks */
#define  TIMING_RESTIMEUNIT              13
#define  TIMING_RESRFTICKS               14      /* callback function: resRFTicks */
#define  TIMING_TEXTBOX                  15

#define  WG                              6       /* callback function: cbWG */
#define  WG_RUN                          2       /* callback function: RunCommand */
#define  WG_FILETIME                     3
#define  WG_FILEDATE                     4
#define  WG_AUTOCOMMENTTEXT              5
#define  WG_COMMENTTEXT                  6       /* callback function: CommentText */
#define  WG_ACCEPT                       7       /* callback function: commentAcceptCommand */
#define  WG_CANCEL                       8       /* callback function: commentCancelCommand */
#define  WG_USEDTICKS                    9
#define  WG_USERTICKS                    10
#define  WG_TEST_GT_TEST                 11
#define  WG_BREAKPOINT                   12      /* callback function: BreakpointCommand */
#define  WG_GLOBALOFFSET                 13      /* callback function: clusterGlobalOffset */
#define  WG_ONLYENABLEDQ                 14      /* callback function: clusterOnlyEnabledQ */
#define  WG_TRIGGERRUN                   15
#define  WG_REPEATRUN                    16
#define  WG_CLUSTER_IDLE                 17      /* callback function: clusterIdle */
#define  WG_CANVAS                       18      /* callback function: canvas */
#define  WG_CLUSTER_INDEX                19      /* callback function: clusterIndex */
#define  WG_CLUSTER_ENABLEDQ             20      /* callback function: clusterEnabledQ */
#define  WG_USEDTIMEUNIT                 21
#define  WG_CLUSTER_TICKS                22      /* callback function: clusterTicks */
#define  WG_USERTIMEUNIT                 23
#define  WG_CLUSTER_TIMEUNIT             24      /* callback function: clusterTimeUnit */
#define  WG_CLUSTER_ANALOGRING           25      /* callback function: clusterAnalogRing */
#define  WG_CLUSTER_ANALOGGROUP          26      /* callback function: clusterAnalogGroup */
#define  WG_CLUSTER_RFRING               27      /* callback function: clusterRFRing */
#define  WG_CLUSTER_RFGROUP              28      /* callback function: clusterRFGroup */
#define  WG_IDLEWORD                     29      /* callback function: IdleWordCommand */
#define  WG_IDLEZERO                     30      /* callback function: IdleZeroCommand */
#define  WG_HSCROLL                      31      /* callback function: hscroll */
#define  WG_TEXTMSG_4                    32
#define  WG_TEXTMSG_5                    33
#define  WG_TEXTMSG_6                    34
#define  WG_TEXTMSG_7                    35
#define  WG_TEXTMSG_2                    36
#define  WG_TEXTMSG                      37
#define  WG_TEXTMSG_3                    38
#define  WG_TEXTMSG_9                    39
#define  WG_CLUSTER_OUTPUTLABEL          40      /* callback function: clusterOutputLabel */
#define  WG_TEXTMSG_8                    41
#define  WG_RUNTIMER                     42      /* callback function: RunTimer */
#define  WG_CLUSTER_LABEL                43      /* callback function: clusterLabel */
#define  WG_TEXTMSG_11                   44
#define  WG_TEXTMSG_10                   45


     /* Menu Bars, Menus, and Menu Items: */

#define  ARFMENU                         1
#define  ARFMENU_FILE                    2
#define  ARFMENU_FILE_SAVEAS             3       /* callback function: menuFILESaveIni */
#define  ARFMENU_FILE_OPEN               4       /* callback function: menuFILEOpenIni */
#define  ARFMENU_FILE_PRINT              5       /* callback function: menuFILEPrint */
#define  ARFMENU_FILE_SEPARATOR_4        6
#define  ARFMENU_FILE_QUIT               7       /* callback function: menuFILEQuit */
#define  ARFMENU_EDIT                    8
#define  ARFMENU_EDIT_DUPLICATEANALOG    9       /* callback function: arfEDITDuplicateAnalog */
#define  ARFMENU_EDIT_DUPLICATERF        10      /* callback function: arfEDITDuplicateRF */
#define  ARFMENU_EDIT_ERASEANALOG        11      /* callback function: arfEDITEraseAnalog */
#define  ARFMENU_EDIT_ERASERF            12      /* callback function: arfEDITEraseRF */
#define  ARFMENU_EDIT_SEPARATOR          13
#define  ARFMENU_EDIT_COPYGRAPH          14      /* callback function: arfEDITCopyGraph */
#define  ARFMENU_EDIT_PASTEGRAPH         15      /* callback function: arfEDITPasteGraph */
#define  ARFMENU_EDIT_SEPARATOR_2        16
#define  ARFMENU_EDIT_INSERTVALUE        17      /* callback function: arfEDITInsertValue */
#define  ARFMENU_EDIT_DELETEVALUE        18      /* callback function: arfEDITDeleteValue */
#define  ARFMENU_OPERATE                 19
#define  ARFMENU_OPERATE_RUN             20      /* callback function: menuOPERATERun */
#define  ARFMENU_OPERATE_SEPARATOR_5     21
#define  ARFMENU_OPERATE_ZOOMGRAPH       22      /* callback function: arfOPERATEZoomGraph */
#define  ARFMENU_OPERATE_SEPARATOR_3     23
#define  ARFMENU_OPERATE_FILLX           24      /* callback function: arfOPERATEFillX */
#define  ARFMENU_OPERATE_REPLACEX        25      /* callback function: arfOPERATEReplaceX */
#define  ARFMENU_OPERATE_REPLACEY        26      /* callback function: arfOPERATEReplaceY */
#define  ARFMENU_WINDOW                  27
#define  ARFMENU_WINDOW_DIGITAL          28      /* callback function: menuWINDOWDigital */

#define  POPUP                           2
#define  POPUP_CLUSTER                   2
#define  POPUP_CLUSTER_OUTPUT            3       /* callback function: popupCLUSTEROutput */
#define  POPUP_CLUSTER_INSERT            4       /* callback function: popupCLUSTERInsert */
#define  POPUP_CLUSTER_DUPLICATE         5       /* callback function: popupCLUSTERDuplicate */
#define  POPUP_CLUSTER_COPY              6       /* callback function: popupCLUSTERCopy */
#define  POPUP_CLUSTER_CUT               7       /* callback function: popupCLUSTERCut */
#define  POPUP_CLUSTER_PASTE             8       /* callback function: popupCLUSTERPaste */
#define  POPUP_CLUSTER_SEPARATOR         9
#define  POPUP_CLUSTER_REPLACE           10      /* callback function: popupCLUSTERReplace */
#define  POPUP_CLUSTER_SEPARATOR_2       11
#define  POPUP_CLUSTER_DELETE            12      /* callback function: popupCLUSTERDelete */
#define  POPUP_XYVALUE                   13
#define  POPUP_XYVALUE_SORTPAIR          14      /* callback function: popupXYVALUESort */
#define  POPUP_XYVALUE_DUPLICATEPAIR     15      /* callback function: popupXYVALUEDuplicate */
#define  POPUP_XYVALUE_COPYPAIR          16      /* callback function: popupXYVALUECopy */
#define  POPUP_XYVALUE_CUTPAIR           17      /* callback function: popupXYVALUECut */
#define  POPUP_XYVALUE_PASTEPAIR         18      /* callback function: popupXYVALUEPaste */
#define  POPUP_XYVALUE_SEPARATOR_3       19
#define  POPUP_XYVALUE_DELETEPAIR        20      /* callback function: popupXYVALUEDelete */

#define  WG2MENU                         3
#define  WG2MENU_FILE                    2
#define  WG2MENU_FILE_SAVEAS             3       /* callback function: menuFILESaveIni */
#define  WG2MENU_FILE_OPEN               4       /* callback function: menuFILEOpenIni */
#define  WG2MENU_FILE_PRINT              5       /* callback function: menuFILEPrint */
#define  WG2MENU_FILE_SEPARATOR          6
#define  WG2MENU_FILE_SEPARATOR2         7
#define  WG2MENU_FILE_QUIT               8       /* callback function: menuFILEQuit */
#define  WG2MENU_OPERATE                 9
#define  WG2MENU_OPERATE_RUN             10      /* callback function: menuOPERATERun */
#define  WG2MENU_OPERATE_SEPARATOR_2     11
#define  WG2MENU_OPERATE_ENABLEALL       12      /* callback function: wgOPERTATEEnableAll */
#define  WG2MENU_OPERATE_DISABLEALL      13      /* callback function: wgOPERTATEDisableAll */
#define  WG2MENU_WINDOW                  14
#define  WG2MENU_WINDOW_ARF              15      /* callback function: menuWINDOWARF */


     /* Callback Prototypes: */ 

int  CVICALLBACK arfAIndex(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfALabel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfATicks(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfATicksD(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfATimeUnit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfATimeUnitD(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK arfEDITCopyGraph(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfEDITDeleteValue(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfEDITDuplicateAnalog(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfEDITDuplicateRF(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfEDITEraseAnalog(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfEDITEraseRF(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfEDITInsertValue(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfEDITPasteGraph(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK arfEnabledQ(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfGraph(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfGraphIndex(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfIdleAnalog(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfIdleRF(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfInitialAnalog(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfInterp(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfMaximumAnalog(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfMinimumAnalog(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfNumValues(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK arfOPERATEFillX(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfOPERATEReplaceX(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfOPERATEReplaceY(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK arfOPERATEZoomGraph(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK arfPARAM(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfPinRangeQ(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfRFIndex(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfRFLabel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfRFTicks(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfRFTicksD(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfRFTimeUnit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfRFTimeUnitD(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfRFTimeUnitLT(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfUpdateGraph(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfXValue(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK arfYValue(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK AuxCancel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK BreakpointCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK canvas(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbARF(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbSetup(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbTiming(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK cbWG(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterAnalogGroup(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterAnalogRing(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterEnabledQ(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterGlobalOffset(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterIdle(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterIndex(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterLabel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterOnlyEnabledQ(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterOutputLabel(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterRFGroup(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterRFRing(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterTicks(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK clusterTimeUnit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK commentAcceptCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK commentCancelCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK commentResetCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK CommentText(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK hscroll(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK IdleWordCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK IdleZeroCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK menuFILEOpenIni(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuFILEPrint(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuFILEQuit(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuFILESaveIni(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuOPERATERun(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuWINDOWARF(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuWINDOWDigital(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTERCopy(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTERCut(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTERDelete(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTERDuplicate(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTERInsert(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTEROutput(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTERPaste(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupCLUSTERReplace(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupXYVALUECopy(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupXYVALUECut(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupXYVALUEDelete(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupXYVALUEDuplicate(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupXYVALUEPaste(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK popupXYVALUESort(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK resAskTicks(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK resAskTimeUnit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK resATicks(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK resATimeUnit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK resRFTicks(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK resRFTimeUnit(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RunCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK RunTimer(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupAcceptCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupCancelCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupNumberOfAGroups(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupNumberOfClusters(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupNumberOfGraphs(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupNumberOfLines(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupNumberOfRFGroups(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK setupResetCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK timingAcceptCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK timingCancelCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK timingResetCommand(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK wgOPERTATEDisableAll(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK wgOPERTATEEnableAll(int menubar, int menuItem, void *callbackData, int panel);


#ifdef __cplusplus
    }
#endif
