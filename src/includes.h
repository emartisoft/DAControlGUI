#ifndef INCLUDES_H
#define INCLUDES_H

#define STREQUAL(s1, s2) (strcmp(s1, s2) == 0)

#include <stdlib.h>
#include <stdio.h>

#include <exec/types.h>
#include <libraries/gadtools.h>
#include <intuition/intuition.h>
#include <exec/execbase.h>

#include <classes/window.h>
#include <gadgets/layout.h>
#include <proto/layout.h>
#include <proto/window.h>

#include <gadgets/button.h>
#include <proto/listbrowser.h>
#include <gadgets/listbrowser.h>
#include <proto/integer.h>
#include <gadgets/integer.h>
#include <proto/checkbox.h>
#include <gadgets/checkbox.h>
#include <proto/label.h>
#include <images/label.h>
#include <proto/getfile.h>
#include <gadgets/getfile.h>
#include <proto/chooser.h>
#include <gadgets/chooser.h>
#include <proto/string.h>
#include <gadgets/string.h>

#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/icon.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/wb.h>

#include <clib/alib_protos.h>
#include <clib/reaction_lib_protos.h>

#define  ALL_REACTION_MACROS
#define  ALL_REACTION_CLASSES
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#define ASM             __asm
#define REG(x)          register __ ## x

#define BASEDEF(base)      struct Library *base##Base = NULL
#define OPENLIB(base,name) if (!((base##Base) = OpenLibrary ((name),0))) CloseLibs()
#define CLOSELIB(base)     if (base##Base) {CloseLibrary (base##Base); base##Base=NULL;}

#define NAME_MAX 		0X020
#define PATH_MAX    	0X100
#define MAX_LISTED_ADF  0X200
#define BUFFERSIZE  	0X200

#include <string.h>

#endif
