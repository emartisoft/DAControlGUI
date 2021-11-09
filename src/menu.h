#ifndef MENU_H
#define MENU_H

//-------------------------------
#define DAControlGUIMenu 0
#define DAControlGUIMenuAbout 0
#define DAControlGUIMenuBar1 1
#define DAControlGUIMenuIconify 2
#define DAControlGUIMenuBar2 3
#define DAControlGUIMenuQuit 4
//-------------------------------
#define Controls 1
#define ControlsLoadChange 0
#define ControlsCreate 1
#define ControlsBar2 2
#define ControlsEject 3
#define ControlsEjectAll 4
#define ControlsBar3 5
#define ControlsRefresh	6
//-------------------------------
#define HelpMenu 2
#define HelpMenuManual 0

struct NewMenu dacMenuNewMenu[] =
{
    NM_TITLE, (STRPTR)"DAControlGUI"               	,  NULL , 0, NULL, (APTR)~0,
    NM_ITEM , (STRPTR)"About"               		,  NULL , 0, 0L, (APTR)~0,
    NM_ITEM , NM_BARLABEL                          	,  NULL , 0, 0L, (APTR)~0,
    NM_ITEM , (STRPTR)"Iconify"                     ,  "I" , 0, 0L, (APTR)~0,
    NM_ITEM , NM_BARLABEL                          	,  NULL , 0, 0L, (APTR)~0,
    NM_ITEM , (STRPTR)"Quit"                      	,  "Q" , 0, 0L, (APTR)~0,
	
    NM_TITLE, (STRPTR)"Controls"              		,  NULL , 0, NULL, (APTR)~0,
    NM_ITEM , (STRPTR)"Load / Change ADF" 			,  "L" , 0, 0L, (APTR)~0,
    NM_ITEM , (STRPTR)"Create ADF"                  ,  "C" , 0, 0L, (APTR)~0,
	NM_ITEM , NM_BARLABEL                          	,  NULL , 0, 0L, (APTR)~0,
	NM_ITEM , (STRPTR)"Eject ADF"                 	,  "E" , 0, 0L, (APTR)~0,
    NM_ITEM , (STRPTR)"Eject All"            		,  "A" , 0, 0L, (APTR)~0,
	NM_ITEM , NM_BARLABEL                          	,  NULL , 0, 0L, (APTR)~0,
	NM_ITEM , (STRPTR)"Refresh"                 	,  "R" , 0, 0L, (APTR)~0,
    
    NM_TITLE, (STRPTR)"Help"                       ,  NULL , 0, NULL, (APTR)~0,
    NM_ITEM , (STRPTR)"Manual"                      ,  "M" , 0, 0L, (APTR)~0,
	
    NM_END  , NULL                                 ,  NULL , 0, 0L, (APTR)~0
};

ULONG dacMenuTags[] =
{
    (GT_TagBase+67), TRUE,
    (TAG_DONE)
};

#endif