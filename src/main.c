;/*
; To compile from the Shell, type: 'Execute main.c'
delete /bin/DAControlGUI
delete #?.o
delete /bin/#?.lnk
sc #?.c link LIB functions.lib LIB:sc.lib LIB:amiga.lib LIB:reaction.lib TO /bin/DAControlGUI
delete #?.o
delete /bin/#?.lnk
; copy files to bin drawer
copy /guide/DAControlGUI.guide /bin
copy /guide/DAControlGUI.guide.info /bin
copy /info/DAControlGUI.info /bin
; copy bin drawer to da64 floppy driver (adf)
DAControl eject device da64: safeeject=yes timeout 15 quiet
DAControl load device da64: writeprotected=no /adf/DAControlGUI.adf
copy /bin/#? da64:
quit
*/

/*
'scoptions' file
================
DATA=FAR
CODE=FAR
PARAMETERS=REGISTERS
NOSTACKCHECK
COMMENTNEST
OPTIMIZERSIZE
VERBOSE
STRIPDEBUG
UTILITYLIBRARY
NOERRORHIGHLIGHT
BATCH
OPTIMIZERALIAS
INCLUDEDIR=INCLUDE:
IGNORE=73
IGNORE=225
IGNORE=51
IGNORE=100
NOSTanDardIO
*/

#include "includes.h"
#include "menu.h"
#include "functions.h"
#include "version.h"

BASEDEF(Intuition);
BASEDEF(Utility);
BASEDEF(GadTools);
BASEDEF(Icon);
BASEDEF(Workbench);
BASEDEF(Window);
BASEDEF(Layout);
BASEDEF(CheckBox);

enum {
    IDCREATE = 0x01,
    IDLOADCHANGE,
    IDEJECT,
    IDEJECTALL,
	IDREFRESH,
    IDLISTBROWSER
};

struct DosBase*       DosBase           = NULL;
struct Screen*        ScreenPtr         = NULL;
struct Object*        WindowObjectPtr   = NULL;
struct Gadget* 		  hParent           = NULL;
struct Gadget* 		  bLoadChange       = NULL;
struct Gadget* 		  bCreate           = NULL;
struct Gadget*		  bEject            = NULL;
struct Gadget*		  bEjectAll         = NULL;
struct Gadget*		  bRefresh          = NULL;
struct Gadget* 		  vParent           = NULL;
struct Gadget*		  lbAdfInfoVolumes  = NULL;
struct VisualInfo*    VisualInfoPtr     = NULL;
struct Window*        WindowPtr         = NULL;
struct Menu*          dacMenu           = NULL;

signed char done;
APTR winVisualInfo;
ULONG signal;

// prototypes
void ProcessMenuIDCMPdacMenu(UWORD MenuNumber);
void CloseLibs(void);
int makeMenu(APTR MenuVisualInfo);
int appMain(void);
BOOL makeList(struct List* list, char* labels1[], char* labels2[], char* labels3[], char* labels4[], char* labels5[]);
void freeList(struct List*);
void About(void);
void AppTerminate(void);
void Eject(char* device);
void clickEject(void);
void clickEjectAll(void);
void loadChangeAdfWin(void);
void createAdfWin(void);
void setDisable(struct Gadget* gad, BOOL value);
void createADFList(void);
void buttonsDisable(BOOL b);
void iconify(void);

#define PREFFILEPATH	"SYS:Prefs/Env-Archive/DAControlGUI.prefs"
void SavePrefs(void);
void LoadPrefs(void);
char loadChangeFileReqPath[BUFFERSIZE];
char createFileReqPath[BUFFERSIZE];
UWORD appLeft, appTop, appWidth, appHeight;

/*
// Hook is used to add device to list for IDCMP_DISKINSERTED and IDCMP_DISKREMOVED but it is NOT stable.
ULONG HookFunc(struct Hook *h, VOID *o, VOID *msg);
ULONG ASM hookEntry(REG(a0) struct Hook *h, REG(a2) VOID *o, REG(a1) VOID *msg);
void InitHook(struct Hook* hook, ULONG (*func)(), void* data);
*/

STRPTR disktype[] = {"DD","HD", NULL};

char* col1[MAX_LISTED_ADF];
char* col2[MAX_LISTED_ADF];
char* col3[MAX_LISTED_ADF];
char* col4[MAX_LISTED_ADF];
char* col5[MAX_LISTED_ADF];

struct ColumnInfo ci[] =
{
  { 80, "Device"                            , 0 },
  { 60, "Volume Name                     "  , 0 },
  { 90, "  Type  "                          , 0 },
  { 90, "   Access   "                      , 0 },
  { 60, "File"                              , 0 },
  { -1, (STRPTR)~0, -1 }
};

int selectedIndex;
WORD count;
struct List adfList;

char appPath[PATH_MAX+NAME_MAX];
char curPath[PATH_MAX];
char manualPath[PATH_MAX+NAME_MAX];

struct MsgPort*     AppPort = NULL;
struct DiskObject*  dobj    = NULL;
struct AppIcon*     appicon = NULL;
struct AppMenuItem* appmenuitem = NULL;
//struct Hook HookStruct;

int selectedDeviceNo;

int main(void)
{
	int ret = 0;
	
	if(!fileExist("SYS:C/DAControl"))  // do you have os 3.2?
    {
       AppTerminate();
       return --ret;
    }

	if(!fileExist(PREFFILEPATH))
	{
		strcpy(loadChangeFileReqPath, "RAM:Empty.adf");
		strcpy(createFileReqPath, "RAM:Empty.adf");
		appLeft = 50;
		appTop = 50;
		appWidth = 450;
		appHeight = 250;
		SavePrefs();
	}
		
	LoadPrefs();
	
	OPENLIB(Window,     "window.class");
    OPENLIB(Layout,     "gadgets/layout.gadget");	
	OPENLIB(CheckBox,   "gadgets/checkbox.gadget");
	OPENLIB(Dos,        "dos.library");
	OPENLIB(Icon,       "icon.library");
	OPENLIB(Workbench,  "workbench.library");
	OPENLIB(GadTools,   "gadtools.library");
	OPENLIB(Intuition,  "intuition.library");
	OPENLIB(Utility,    "utility.library");

	ret = appMain();
	
	CloseLibs();
	return ret;
}

int makeMenu(APTR MenuVisualInfo)
{
    if (NULL == (dacMenu = CreateMenusA( dacMenuNewMenu, NULL))) return( 1L );
    LayoutMenusA( dacMenu, MenuVisualInfo, (struct TagItem *)(&dacMenuTags[0]));
    return( 0L );
}

int appMain()
{
	ULONG result;
	UWORD code;

	ScreenPtr = LockPubScreen(NULL);
    VisualInfoPtr = GetVisualInfoA(ScreenPtr, NULL);
	//InitHook(&HookStruct, HookFunc, NULL);
	
	
	GetCurrentDirName(curPath, PATH_MAX);
	strcpy(appPath, curPath);
	strcat(appPath, "/DAControlGUI");
	
	strcpy(manualPath, "SYS:Utilities/MultiView ");
	strcat(manualPath, appPath);
	strcat(manualPath, ".guide"); // DAControlGUI.guide
	
	selectedIndex = -1;

	dobj=GetDiskObject(appPath);
    if(dobj!=0)
	{
		dobj->do_Type=0;
	}

    if (!(WindowObjectPtr = NewObject
    (   WINDOW_GetClass(),
        NULL,
        WA_PubScreen,             ScreenPtr,
        WA_ScreenTitle,           "DAControlGUI by emarti, Murat Ozdemir",
        WA_Title,                 "DAControlGUI",
        WA_Activate,              TRUE,
        WA_DepthGadget,           TRUE,
        WA_DragBar,               TRUE,
        WA_CloseGadget,           TRUE,
		WINDOW_IconifyGadget,     TRUE,
        WA_SizeGadget,            TRUE,
        WA_SmartRefresh,          TRUE,
        WA_MinWidth,              400,
        WA_MinHeight,             180,
        WA_MaxWidth,              -1,
        WA_MaxHeight,             -1,
        WA_Width,                 appWidth,
        WA_Height,                appHeight,
        WA_Left,                  appLeft,
        WA_Top,                   appTop,
        WA_SizeBRight,            FALSE,
        WA_SizeBBottom,           TRUE,
        WA_NewLookMenus,          TRUE,
		WINDOW_IconTitle,         "DAControlGUI",
        WA_IDCMP,                 IDCMP_DISKINSERTED | IDCMP_DISKREMOVED | IDCMP_MENUPICK,
        /* 
	    WINDOW_IDCMPHookBits,     IDCMP_DISKINSERTED | IDCMP_DISKREMOVED ,
		WINDOW_IDCMPHook,         &HookStruct,
		*/
        WINDOW_AppPort,           AppPort,
		
		//WINDOW_Position,          WPOS_CENTERSCREEN,
        WINDOW_ParentGroup,       vParent = VGroupObject,
												LAYOUT_SpaceOuter, TRUE,
												LAYOUT_BevelStyle, BVS_GROUP,
												LAYOUT_DeferLayout, TRUE,
												LAYOUT_AddChild, HGroupObject,
														LAYOUT_SpaceOuter, FALSE,
														LAYOUT_BevelStyle, BVS_SBAR_VERT,
														LAYOUT_Label, " List of devices ",
														LAYOUT_AddChild, lbAdfInfoVolumes = ListBrowserObject,
														GA_ID, IDLISTBROWSER,
														LISTBROWSER_Labels, &adfList,
														GA_RelVerify , TRUE,
														LISTBROWSER_Labels, (ULONG)&adfList,
														LISTBROWSER_ColumnInfo, (ULONG)&ci,
														LISTBROWSER_ColumnTitles, TRUE,
														LISTBROWSER_MultiSelect, FALSE,
														LISTBROWSER_Separators, TRUE,
														LISTBROWSER_ShowSelected, TRUE,
														LISTBROWSER_Editable, FALSE,
														LISTBROWSER_HorizontalProp, TRUE,
														LISTBROWSER_VerticalProp, TRUE,
														LISTBROWSER_AutoFit, TRUE,
														LISTBROWSER_Selected, -1,
													End,
													End,

												LAYOUT_AddChild, HGroupObject,
														LAYOUT_BevelStyle, BVS_SBAR_VERT,
														LAYOUT_Label, " Controls ",
														LAYOUT_AddChild, bLoadChange = ButtonObject,
																GA_Text, "_Load/Change",
																GA_RelVerify, TRUE,
																GA_ID, IDLOADCHANGE,
															End,

														LAYOUT_AddChild, bCreate = ButtonObject,
																GA_Text, "_Create",
																GA_RelVerify, TRUE,
																GA_ID, IDCREATE,
															End,

														LAYOUT_AddChild, bEject = ButtonObject,
																GA_Text, "_Eject",
																GA_RelVerify, TRUE,
																GA_ID, IDEJECT,
															End,

														LAYOUT_AddChild, bEjectAll = ButtonObject,
																GA_Text, "Eject _All",
																GA_RelVerify, TRUE,
																GA_ID, IDEJECTALL,
															End,
														LAYOUT_AddChild, bRefresh = ButtonObject,
																GA_Text, "_Refresh",
																GA_RelVerify, TRUE,
																GA_ID, IDREFRESH,
															End,
													End,
													CHILD_WeightedHeight,10,
													CHILD_MinWidth, 300,
													CHILD_MinHeight, 25,
													CHILD_MaxHeight, 25,

												End
	)))
    {   
        done=TRUE;
    }
	
    UnlockPubScreen(NULL, ScreenPtr);
    ScreenPtr = NULL;

    if (!(WindowPtr = (struct Window *) DoMethod(WindowObjectPtr, WM_OPEN, NULL)))
    {   
        done=TRUE;
    }

	
    makeMenu(VisualInfoPtr);
    SetMenuStrip(WindowPtr, dacMenu);
	createADFList();
	
	GetAttr(WINDOW_SigMask, WindowObjectPtr, &signal);

    while(!done)
    {   
		Wait(signal | (1 << WindowPtr->UserPort->mp_SigBit));

        while ((result = DoMethod(WindowObjectPtr, WM_HANDLEINPUT, &code)) != WMHI_LASTMSG)
        {   switch (result & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    done = TRUE;
                break;

                case WMHI_GADGETUP:
                    switch(result & WMHI_GADGETMASK)
                    {
                        
                        case IDLOADCHANGE:
                            loadChangeAdfWin();
                        break;

                        case IDCREATE:
                            createAdfWin();
                        break;

                        case IDEJECT:
                            clickEject();
                        break;
						
						case IDEJECTALL:
							clickEjectAll();
                        break;

						case IDREFRESH:
							createADFList();
						break;

                        case IDLISTBROWSER:
							selectedIndex = code;
                        break;

                        default:
                        break;
                    }
                break;
                
                case WMHI_ICONIFY: /* iconify / uniconify */
					iconify();		
                break;
				
				case WMHI_MENUPICK:
					ProcessMenuIDCMPdacMenu(code);
				break;
				
				case WMHI_RAWKEY:
					switch(code)
					{
						case 0x45: // press ESC to quit
							done=TRUE;
						break;
						
						default:
						break;
					}
					break;
					
                
                default:
                break;
			}   
		}   
	}
	
	appTop = WindowPtr->TopEdge;
	appLeft = WindowPtr->LeftEdge;
	appWidth = WindowPtr->Width;
	appHeight = WindowPtr->Height;
	
	SavePrefs();
	freeList(&adfList);
    ClearMenuStrip(WindowPtr);
	FreeVisualInfo(VisualInfoPtr);
	
	// delete log file
	Execute("Delete RAM:dacgui.log >NIL:", 0, 0); 

	return 0;
}

/*
ULONG HookFunc(struct Hook *h, VOID *o, VOID *msg)
{
    UWORD code;
    ULONG class;
    class  = ((struct IntuiMessage *) msg)->Class;
    code   = ((struct IntuiMessage *) msg)->Code;
	

    switch(class)
    {
        case IDCMP_DISKINSERTED:
		case IDCMP_DISKREMOVED:
			//Printf("Disk insert/removed ...\n");
			//createADFList();
        break;

        default:
        break;
    }

    return 1;
}


ULONG ASM hookEntry(REG(a0) struct Hook *h, REG(a2) VOID *o, REG(a1) VOID *msg)
{  
    return ((*(ULONG (*)(struct Hook *, VOID *, VOID *))(*h->h_SubEntry))(h, o, msg));
}

void InitHook(struct Hook* hook, ULONG (*func)(), void* data)
{ 
    if (hook)
    {  
        hook->h_Entry    = (ULONG (*)()) hookEntry;
        hook->h_SubEntry = func;
        hook->h_Data     = data;
    } 
    else
    {   
        done=TRUE;
	}   
}
*/

void CloseLibs(void)
{
  	if (ScreenPtr)
    {   
		UnlockPubScreen(NULL, ScreenPtr);
        ScreenPtr = NULL;
    }
	
    if (WindowObjectPtr)
    {   
		DisposeObject(WindowObjectPtr);
        WindowObjectPtr = NULL;
    }
	
	if (dobj) FreeDiskObject(dobj);
	
	if (AppPort) DeleteMsgPort(AppPort);
	
	CLOSELIB(Intuition);
	CLOSELIB(Utility);
	CLOSELIB(GadTools);
	CLOSELIB(Workbench);
	CLOSELIB(Icon);
    CLOSELIB(Layout);
	CLOSELIB(CheckBox);
    CLOSELIB(Window);
    CLOSELIB(Dos);
}

void ProcessMenuIDCMPdacMenu(UWORD MenuNumber)
{
  UWORD MenuNum;
  UWORD ItemNumber;
  struct MenuItem *item;
  while (MenuNumber != MENUNULL)
	{
		item = ItemAddress( dacMenu, MenuNumber);
		MenuNum = MENUNUM(MenuNumber);
		ItemNumber = ITEMNUM(MenuNumber);
		switch ( MenuNum )
		{
			case NOMENU :
				break;

			case DAControlGUIMenu :
				switch ( ItemNumber )
				{
					case NOITEM :
						break;

					case DAControlGUIMenuAbout :
						About();
						break;
					case DAControlGUIMenuIconify :
						iconify();
						break;
					case DAControlGUIMenuQuit :
                        done=TRUE;
						break;
				}
				break;

			case Controls :
				switch ( ItemNumber )
				{
					case NOITEM :
						break;

					case ControlsLoadChange :
							loadChangeAdfWin();
						break;
					case ControlsCreate :
							createAdfWin();
						break;
					case ControlsEject :
							clickEject();
						break;
					case ControlsEjectAll :
							clickEjectAll();
						break;		
					case ControlsRefresh :
							createADFList();
						break;
				}
				break;

			case HelpMenu :
				switch ( ItemNumber )
				{
					case NOITEM :
						break;

					case HelpMenuManual :
						Execute(manualPath, 0, 0); 
						break;
				}
				break;
				

				default:
				break;
		}
		MenuNumber = item->NextSelect;
    }
}

void About(void)
{
    struct EasyStruct aboutReq =
    {
        sizeof(struct EasyStruct),
        0,
        "About",
        ABOUT,
		"Ok"
    };
    EasyRequest(WindowPtr, &aboutReq, NULL, NULL);
}

void AppTerminate(void)
{
    struct EasyStruct appTerReq =
    {
        sizeof(struct EasyStruct),
        0,
        "Error - DAControlGUI",
        "SYS:C/DAControl not found.\nSYS:Devs/trackfile.device not found.\nDo you have OS 3.2?\nThe application will be terminated.\n",
        "Ok"
    };
    EasyRequest(WindowPtr, &appTerReq, NULL, NULL);
}

BOOL makeList(struct List* list, char* labels1[], char* labels2[], char* labels3[], char* labels4[], char* labels5[])
{
    struct Node *node;
    WORD i = 0;

    NewList(list);

    while (labels1[i] != '\0')
    {
        if (node = AllocListBrowserNode(5,
            LBNA_Column, 0,
                LBNCA_Justification, LCJ_CENTER,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, labels1[i],
                LBNCA_MaxChars, 8,
            LBNA_Column, 1,
                LBNCA_Justification, LCJ_LEFT,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, labels2[i],
                LBNCA_MaxChars, 32,
            LBNA_Column, 2,
                LBNCA_Justification, LCJ_CENTER,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, labels3[i],
                LBNCA_MaxChars, 8,
            LBNA_Column, 3,
                LBNCA_Justification, LCJ_CENTER,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, labels4[i],
                LBNCA_MaxChars, 11,
            LBNA_Column, 4,
                LBNCA_Justification, LCJ_LEFT,
                LBNCA_CopyText, TRUE,
                LBNCA_Text, labels5[i],
                LBNCA_MaxChars, PATH_MAX,
            TAG_DONE))
        {
            AddTail(list, node);
        }
        else
            break;
        i++;
    }
    return(TRUE);
}

void freeList(struct List* list)
{
    struct Node *node, *nextnode;

    node = list->lh_Head;
    while (nextnode = node->ln_Succ)
    {
        FreeListBrowserNode(node);
        node = nextnode;
    }
    NewList(list);
}

void createADFList(void) 
{
	//int x;
	BPTR fp;
	BOOL ejectDisable = FALSE;
    RunDAControl("INFO SHOWVOLUMES >RAM:dacgui.log");
    fp = Open("RAM:dacgui.log", MODE_OLDFILE);
    count=0;
	
    if(fp)
    {
        UBYTE buffer[BUFFERSIZE];
        while(FGets(fp, buffer, BUFFERSIZE))
        {
            if((count>0)&&(strlen(buffer)>0))
            {
                col1[count-1]= fulltrim(substring(buffer, 1, 7)); // device
                col3[count-1]= fulltrim(substring(buffer, 9, 8)); // type
                col2[count-1]= fulltrim(substring(buffer, 18, 32)); // volume name
                col4[count-1]= fulltrim(substring(buffer, 79, 11)); // access
                col5[count-1]= fulltrim(substring(buffer, 91, strlen(buffer)-90)); // file
                free(buffer);
            }
            count++;
        }
        Close(fp);

        if(count==1) // no mounted device(s)
        {
            col1[0]=" ";
            col2[0]=" ";
            col3[0]=" ";
            col4[0]=" ";
            col5[0]=" ";
			ejectDisable = TRUE;
		}	
		
		/*
		for(x=0;x<count-1;x++)
		{
			Printf("%s|%s|%s|%s|%s\n",col1[x],col2[x],col3[x],col4[x],col5[x]);
		}
		*/
		
        setDisable(bEject, ejectDisable);
		setDisable(bEjectAll, ejectDisable);	

        freeList(&adfList);
        makeList(&adfList,col1,col2,col3,col4,col5);
        SetGadgetAttrs(lbAdfInfoVolumes, WindowPtr, NULL,
                            LISTBROWSER_ColumnInfo, (ULONG)&ci,
							LISTBROWSER_Labels, (ULONG)&adfList,
							LISTBROWSER_AutoFit, TRUE,					
                            TAG_DONE);
    }
	count--;
	//Printf("List count=%d\n", count);
	
}

void Eject(char* device)
{
	char cmd[BUFFERSIZE];
    sprintf(cmd, "EJECT DEVICE ""%s"" SAFEEJECT=YES TIMEOUT 15 QUIET >NIL:", device);
    RunDAControl(cmd);
}

void clickEject(void)
{
	if(selectedIndex>=0) Eject(col1[selectedIndex]);
	createADFList();
}

void clickEjectAll(void)
{
	int m;
	for(m=0; m<count; m++)
	{
		Eject(col1[m]);
	}
	createADFList();
}

void loadChangeAdfWin(void)
{
	enum {
		IDFIRST=0x00,
		// gadget id starts
		IDBOK,
		IDBCANCEL,
		IDGETFILE,
		IDINTEGER,
		IDCHECKBOX,	
		// end of gadget id
		IDLAST
	};
	
	struct Gadget *o[IDLAST-1];
	struct Gadget *vLoadAdfParent = NULL;
	Object *loadAdfWinObj = NULL;
	struct Window *loadAdfWin = NULL;
	ULONG signalLoadAdf, resultLoadAdf;
	signed char doneLoadAdf;
	UWORD codeLoadAdf;
	
	int deviceNo;
	ULONG writeProtected;
	STRPTR gFullFilename;
	ULONG resGetFile = 0;
	char cmd[BUFFERSIZE];
	char currentDeviceStr[8];
	int m;
	BOOL mounted = FALSE;

	char strNo[6];
	strcpy(strNo, &col1[selectedIndex][2]);
	selectedDeviceNo = atoi(strNo);

	if (!(loadAdfWinObj = NewObject
    (   WINDOW_GetClass(),
        NULL,
		WA_PubScreen,             ScreenPtr,
        WA_Title,           	  "Load ADF File",
        WA_Activate,              TRUE,
        WA_DragBar,               TRUE,
        WA_CloseGadget,           TRUE,
		WA_SizeGadget, 			  TRUE,
        WA_SmartRefresh,          TRUE,
        WA_MinWidth,              350,
        WA_MinHeight,             30,
        WA_MaxWidth,              -1,
        WA_MaxHeight,             30,
        WA_Width,                 350,
        WA_Height,                30,
        WA_Left,                  ScreenPtr->Width/20,
        WA_Top,                   (ScreenPtr->Height-200)/2,
        WA_SizeBRight,            FALSE,
        WA_SizeBBottom,           TRUE,
        WA_NewLookMenus,          TRUE,
        WINDOW_Position,          WPOS_CENTERSCREEN,
		WINDOW_ParentGroup,       vLoadAdfParent = VGroupObject,
												LAYOUT_SpaceOuter, TRUE,
												LAYOUT_DeferLayout, TRUE,
												
												LAYOUT_AddChild, HGroupObject,
																  LAYOUT_Label, "",
																  LAYOUT_Orientation, 0,
																  LAYOUT_HorizAlignment, LALIGN_LEFT,
																  LAYOUT_VertAlignment, LALIGN_TOP,
																  LAYOUT_LabelPlace, BVJ_TOP_CENTER,
																  LAYOUT_BevelState, IDS_NORMAL,
																  LAYOUT_BevelStyle, 4,
																  LAYOUT_AddChild, o[3] = (struct Gadget *) IntegerObject,
																				   GA_ID, IDINTEGER,
																				   GA_RelVerify, TRUE,
																				   INTEGER_Number, selectedDeviceNo,
																				   INTEGER_MaxChars, 4,
																				   INTEGER_Minimum, 0,
																				   INTEGER_Maximum, 9999,
																				   INTEGER_Arrows, TRUE,
																				   STRINGA_Justification, GACT_STRINGRIGHT,
																  End,
																  CHILD_Label, LabelObject,
																				   LABEL_Justification, 0,
																				   LABEL_Text, "Unit:",
																  End,
																  LAYOUT_AddChild, o[4] = (struct Gadget *) CheckBoxObject,
																				   GA_ID, IDCHECKBOX,
																				   GA_RelVerify, TRUE,
																				   GA_Selected, TRUE,
																				   GA_Text, "     Write protected ",
																				   CHECKBOX_TextPen, 1,
																				   CHECKBOX_FillTextPen, 1,
																				   CHECKBOX_BackgroundPen, 0,
																				   CHECKBOX_TextPlace, PLACETEXT_LEFT,
																  End,
												 End,
									
												
												LAYOUT_AddChild, o[2] = (struct Gadget *) GetFileObject,
													  GA_ID, IDGETFILE,
													  GA_RelVerify, TRUE,
													  GETFILE_TitleText, "Load ADF File ...",
													  GETFILE_Pattern, "#?.adf",
													  GETFILE_FullFile, loadChangeFileReqPath, 
													  GETFILE_DoPatterns, TRUE,
													  GETFILE_RejectIcons, TRUE,
												 End,
												 
												 CHILD_Label, LabelObject,
													  LABEL_Justification, 0,
													  LABEL_Text, "File:",
												 End,
												 
												
												LAYOUT_AddChild, HGroupObject,
													LAYOUT_AddChild, o[0] = ButtonObject,
															GA_Text, "_Ok",
															GA_RelVerify, TRUE,
															GA_ID, IDBOK,
														End,

													LAYOUT_AddChild, o[1] = ButtonObject,
															GA_Text, "_Cancel",
															GA_RelVerify, TRUE,
															GA_ID, IDBCANCEL,
														End,
												End,

											End
	)))
	{
		doneLoadAdf = TRUE;
	}
		
	loadAdfWin = RA_OpenWindow(loadAdfWinObj);
	GetAttr(WINDOW_SigMask, loadAdfWinObj, &signalLoadAdf);	
	
	doneLoadAdf=FALSE;
	buttonsDisable(TRUE);
	
	while(!doneLoadAdf)
    {   
		Wait(signalLoadAdf | (1 << loadAdfWin->UserPort->mp_SigBit));

        while ((resultLoadAdf = DoMethod(loadAdfWinObj, WM_HANDLEINPUT, &codeLoadAdf)) != WMHI_LASTMSG)
        {   switch (resultLoadAdf & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    doneLoadAdf = TRUE;
                break;
				
				case WMHI_GADGETUP:
                    switch(resultLoadAdf & WMHI_GADGETMASK)
                    {                     
                        case IDBOK:
							GetAttr(INTEGER_Number, o[3], &deviceNo); 
							GetAttr(GA_Selected, o[4], &writeProtected);
							GetAttr(GETFILE_FullFile, o[2], (ULONG*)&gFullFilename);
							GetAttr(GETFILE_Drawer, o[2], (ULONG*)&loadChangeFileReqPath);
							
							if(!isContainsAdfExt(gFullFilename)) break;

							sprintf(loadChangeFileReqPath, "%s", gFullFilename);

							sprintf(currentDeviceStr, "DA%d", deviceNo);
							for(m=0; m<count;m++)
							{
								if(STREQUAL(col1[m], currentDeviceStr)) mounted = TRUE;
							}

							if(!mounted)
							{
								sprintf(cmd, "LOAD \"%s\" DEVICE DA%d: WRITEPROTECTED=""%s"" QUIET IGNORE >NIL:", gFullFilename, deviceNo, (writeProtected) ? "YES" : "NO");
								RunDAControl(cmd);
							}
							else
							{
								sprintf(cmd, "EJECT DEVICE DA%d: SAFEEJECT=YES TIMEOUT 15 QUIET >NIL:", deviceNo);
								RunDAControl(cmd);
								sprintf(cmd, "CHANGE DEVICE DA%d: WRITEPROTECTED=""%s"" \"%s\" QUIET >NIL:", deviceNo, (writeProtected) ? "YES" : "NO", gFullFilename);
								RunDAControl(cmd);
							}
							doneLoadAdf = TRUE;
                        break;
						
						case IDBCANCEL:
                            doneLoadAdf = TRUE;
                        break;
						
						case IDGETFILE:
							resGetFile = gfRequestFile(o[2], loadAdfWin);
						break;
						
                        break;
                    }
                break;
				
				default:
                break;
			}   
		}   
	}
	
	RA_CloseWindow(loadAdfWinObj);
	loadAdfWin=NULL;
	if (loadAdfWinObj)
    {   
		DisposeObject(loadAdfWinObj);
        loadAdfWinObj = NULL;
    }
	
	createADFList();
	buttonsDisable(FALSE);
}

void createAdfWin(void)
{

	enum {
		IDFIRST=0x00,
		// gadget id starts
		IDBOK,
		IDBCANCEL,
		IDGETFILE,
		IDINTEGER,
		IDCHECKBOX,
		IDCHOOSER,
		IDSTRING,
		// end of gadget id
		IDLAST
	};
	
	struct Gadget *co[IDLAST-1];
	struct Gadget *vCreateAdfParent = NULL;
	Object *createAdfWinObj = NULL;
	struct Window *createAdfWin = NULL;
	ULONG signalCreateAdf, resultCreateAdf;
	signed char doneCreateAdf;
	UWORD codeCreateAdf;
	
	int cdeviceNo;
	ULONG cwriteProtected;
	ULONG cresGetFile = 0;
	STRPTR cgFullFilename;
	char cmd[BUFFERSIZE];
	STRPTR labelstr;
	ULONG idx;
	struct List *chooserlist;
	char strNo[6];

	strcpy(strNo, &col1[selectedIndex][2]);
	selectedDeviceNo = atoi(strNo);

	chooserlist = ChooserLabels( "DD","HD", NULL );
	
	if (!(createAdfWinObj = NewObject
    (   WINDOW_GetClass(),
        NULL,
		WA_PubScreen,             ScreenPtr,
        WA_Title,           	  "Create ADF File",
        WA_Activate,              TRUE,
        WA_DragBar,               TRUE,
        WA_CloseGadget,           TRUE,
		WA_SizeGadget, 			  TRUE,
        WA_SmartRefresh,          TRUE,
        WA_MinWidth,              350,
        WA_MinHeight,             30,
        WA_MaxWidth,              -1,
        WA_MaxHeight,             30,
        WA_Width,                 350,
        WA_Height,                30,
        WA_Left,                  ScreenPtr->Width/20,
        WA_Top,                   (ScreenPtr->Height-200)/2,
        WA_SizeBRight,            FALSE,
        WA_SizeBBottom,           TRUE,
        WA_NewLookMenus,          TRUE,
        WINDOW_Position,          WPOS_CENTERSCREEN,
		WINDOW_ParentGroup,       vCreateAdfParent = VGroupObject,
												LAYOUT_SpaceOuter, TRUE,
												LAYOUT_DeferLayout, TRUE,
												
												LAYOUT_AddChild, co[6] = (struct Gadget *) StringObject,
																  GA_ID, IDSTRING,
																  GA_RelVerify, TRUE,
																  STRINGA_TextVal, "Empty",
																  STRINGA_MaxChars, 32,
																  STRINGA_Justification, GACT_STRINGLEFT,
																  STRINGA_HookType, SHK_CUSTOM,
												 End,
												 CHILD_Label, LabelObject,
																  LABEL_Justification, 0,
																  LABEL_Text, "Label:",
												 End,
												
												LAYOUT_AddChild, HGroupObject,
																  LAYOUT_Label, "",
																  LAYOUT_Orientation, 0,
																  LAYOUT_HorizAlignment, LALIGN_LEFT,
																  LAYOUT_VertAlignment, LALIGN_TOP,
																  LAYOUT_LabelPlace, BVJ_TOP_CENTER,
																  LAYOUT_BevelState, IDS_NORMAL,
																  LAYOUT_BevelStyle, 4,
																  LAYOUT_AddChild, co[3] = (struct Gadget *) IntegerObject,
																				   GA_ID, IDINTEGER,
																				   GA_RelVerify, TRUE,
																				   INTEGER_Number, selectedDeviceNo,
																				   INTEGER_MaxChars, 4,
																				   INTEGER_Minimum, 0,
																				   INTEGER_Maximum, 9999,
																				   INTEGER_Arrows, TRUE,
																				   STRINGA_Justification, GACT_STRINGRIGHT,
																  End,
																  CHILD_Label, LabelObject,
																				   LABEL_Justification, 0,
																				   LABEL_Text, "Unit:",
																  End,
																  LAYOUT_AddChild, co[5] = (struct Gadget *) ChooserObject,
																				   GA_ID, IDCHOOSER,
																				   GA_RelVerify, TRUE,
																				   CHOOSER_PopUp, TRUE,
																				   CHOOSER_MaxLabels, 10,
																				   CHOOSER_Offset, 0,
																				   CHOOSER_Selected, 0,
																				   CHOOSER_Labels, chooserlist,
																  End,
																  CHILD_Label,  LabelObject,
																				   LABEL_Justification, 0,
																				   LABEL_Text, " Disk Type: ",
																  End,
																  LAYOUT_AddChild, co[4] = (struct Gadget *) CheckBoxObject,
																				   GA_ID, IDCHECKBOX,
																				   GA_RelVerify, TRUE,
																				   GA_Selected, FALSE,
																				   GA_Text, "     Write protected ",
																				   CHECKBOX_TextPen, 1,
																				   CHECKBOX_FillTextPen, 1,
																				   CHECKBOX_BackgroundPen, 0,
																				   CHECKBOX_TextPlace, PLACETEXT_LEFT,
																  End,
												 End,
									
												
												LAYOUT_AddChild, co[2] = (struct Gadget *) GetFileObject,
													  GA_ID, IDGETFILE,
													  GA_RelVerify, TRUE,
													  GETFILE_TitleText, "Save ADF File ...",
													  GETFILE_Pattern, "#?.adf",
													  GETFILE_DoPatterns, TRUE,
													  GETFILE_RejectIcons, TRUE,
													  GETFILE_FullFile, createFileReqPath,
												 End,
												 
												 CHILD_Label, LabelObject,
													  LABEL_Justification, 0,
													  LABEL_Text, "File:",
												 End,
												 
												
												LAYOUT_AddChild, HGroupObject,
													LAYOUT_AddChild, co[0] = ButtonObject,
															GA_Text, "_Ok",
															GA_RelVerify, TRUE,
															GA_ID, IDBOK,
														End,

													LAYOUT_AddChild, co[1] = ButtonObject,
															GA_Text, "_Cancel",
															GA_RelVerify, TRUE,
															GA_ID, IDBCANCEL,
														End,
												End,

											End
	)))
	{
		doneCreateAdf = TRUE;
	}
		
	createAdfWin = RA_OpenWindow(createAdfWinObj);
	GetAttr(WINDOW_SigMask, createAdfWinObj, &signalCreateAdf);	
	
	doneCreateAdf=FALSE;
	buttonsDisable(TRUE);
	
	while(!doneCreateAdf)
    {   
		Wait(signalCreateAdf | (1 << createAdfWin->UserPort->mp_SigBit));

        while ((resultCreateAdf = DoMethod(createAdfWinObj, WM_HANDLEINPUT, &codeCreateAdf)) != WMHI_LASTMSG)
        {   switch (resultCreateAdf & WMHI_CLASSMASK)
            {
                case WMHI_CLOSEWINDOW:
                    doneCreateAdf = TRUE;
                break;
				
				case WMHI_GADGETUP:
                    switch(resultCreateAdf & WMHI_GADGETMASK)
                    {                     
                        case IDBOK:
							GetAttr(INTEGER_Number, co[3], &cdeviceNo); 
							GetAttr(GA_Selected, co[4], &cwriteProtected);
							GetAttr(GETFILE_FullFile, co[2], (ULONG*)&cgFullFilename);
							GetAttr(STRINGA_TextVal, co[6], (ULONG*)&labelstr);
							GetAttr(CHOOSER_Selected, co[5], (ULONG*)&idx);

							if(!isContainsAdfExt(cgFullFilename)) break;
								
							sprintf(createFileReqPath, "%s", cgFullFilename);
							// creating adf...
							sprintf(cmd, "CREATE LABEL=\"%s\" DISKTYPE ""%s"" DEVICE DA%d: \"%s\" QUIET >NIL:", labelstr, disktype[idx], cdeviceNo, cgFullFilename);
							RunDAControl(cmd);
							// to display in list (because volumename column value is "-")
							// eject + change
							sprintf(cmd, "EJECT DEVICE DA%d: SAFEEJECT=YES TIMEOUT 15 QUIET >NIL:", cdeviceNo);
							RunDAControl(cmd);
							sprintf(cmd, "CHANGE DEVICE DA%d: \"%s\" WRITEPROTECTED=""%s"" QUIET >NIL:", cdeviceNo, cgFullFilename, (cwriteProtected) ? "YES" : "NO");
							RunDAControl(cmd);
                            doneCreateAdf = TRUE;
                        break;
						
						case IDBCANCEL:
                            doneCreateAdf = TRUE;
                        break;

						case IDGETFILE:
							cresGetFile = gfRequestFile(co[2], createAdfWin);
						break;
						
                        default:
                        break;
                    }
                break;
				
				default:
                break;
			}   
		}   
	}

	FreeChooserLabels(chooserlist);
	RA_CloseWindow(createAdfWinObj);
	createAdfWin=NULL;
	if (createAdfWinObj)
    {   
		DisposeObject(createAdfWinObj);
        createAdfWinObj = NULL;
    }

	createADFList();																																	
	buttonsDisable(FALSE);
}

void setDisable(struct Gadget* gad, BOOL value)
{
	SetGadgetAttrs(gad, WindowPtr, NULL,
						GA_Disabled, value,
						TAG_DONE);
}

void buttonsDisable(BOOL b)
{
	setDisable(bLoadChange, b);
	setDisable(bCreate, b);
	setDisable(bEject, b);
	setDisable(bEjectAll, b);
	setDisable(bRefresh, b);
	if(b) ClearMenuStrip(WindowPtr); else SetMenuStrip(WindowPtr, dacMenu);
}

/*
### SYS:Prefs/Env-Archive/DAControlGUI.prefs ###
LeftEdge
TopEdge
Width
Height
Load/Change File Req. full filename
Create File Req. full filename
*/
BPTR fp;
char fpStr[BUFFERSIZE];

void LoadPrefs(void)
{
	fp = Open(PREFFILEPATH, MODE_OLDFILE);
	if (fp)
	{
		UBYTE buffer[BUFFERSIZE];
        FGets(fp, buffer, BUFFERSIZE);
		appLeft = atoi(buffer);
		FGets(fp, buffer, BUFFERSIZE);
		appTop = atoi(buffer);
		FGets(fp, buffer, BUFFERSIZE);
		appWidth = atoi(buffer);
		FGets(fp, buffer, BUFFERSIZE);
		appHeight = atoi(buffer);
		FGets(fp, buffer, BUFFERSIZE);
		sprintf(loadChangeFileReqPath, "%s", buffer);
		loadChangeFileReqPath[strlen(loadChangeFileReqPath)-1] = '\0';
		FGets(fp, buffer, BUFFERSIZE);
		sprintf(createFileReqPath, "%s", buffer);
		createFileReqPath[strlen(createFileReqPath)-1] = '\0';
		
		Close(fp);
	}
}

void SavePrefs(void)
{
	fp = Open(PREFFILEPATH, MODE_NEWFILE);
	if (fp)
	{
		sprintf(fpStr, "%d\n%d\n%d\n%d\n%s\n%s\n", appLeft, appTop, appWidth, appHeight, loadChangeFileReqPath, createFileReqPath);
		FPuts(fp, fpStr);
		Close(fp);
	}
}

void iconify(void)
{
	AppPort = CreateMsgPort();
	appicon=AddAppIconA(0L, 0L, "DAControlGUI", AppPort, NULL, dobj, NULL);
	appmenuitem=AddAppMenuItemA(0L, 0L, "DAControlGUI", AppPort, NULL);
	RA_CloseWindow(WindowObjectPtr);
	WindowPtr = NULL;
	WaitPort(AppPort);
	RemoveAppIcon(appicon);
	RemoveAppMenuItem(appmenuitem);
	WindowPtr = (struct Window *) RA_OpenWindow(WindowObjectPtr);

	if (WindowPtr)
	{
		SetMenuStrip(WindowPtr, dacMenu);
		GetAttr(WINDOW_SigMask, WindowObjectPtr, &signal);
	}   
	else
	{
		done = TRUE;
	}
}
