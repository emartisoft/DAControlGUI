#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <exec/types.h>
#include <proto/dos.h>

#define BUFFERSIZE  	0X200

extern BOOL fileExist(STRPTR filePath);
extern void RunDAControl(char parameter[BUFFERSIZE]);
extern char* fulltrim(char* str);
extern char* substring(char*, int , int);
extern BOOL isContainsAdfExt(STRPTR filePath);

#endif
