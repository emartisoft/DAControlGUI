#include "functions.h"

/*===============================
 fileExist
===============================*/
BOOL fileExist(STRPTR filePath)
{
    BPTR fp;
    fp = Open(filePath, MODE_OLDFILE);
    if(fp)
    {
        Close(fp);
        return 1;
    }
    else
    {
        return 0;
    }
}

/*===============================
 RunDAControl
===============================*/
void RunDAControl(char parameter[BUFFERSIZE])
{
    char dacontrolPATH[BUFFERSIZE];
    strcpy(dacontrolPATH, "SYS:C/DAControl ");
    strcat(dacontrolPATH, parameter);
    Execute(dacontrolPATH, NULL, NULL); 
}

/*===============================
 substring
===============================*/
char* substring(char* string, int position, int length)
{
   char *p;
   int c;

   p = malloc(length+1);

   if (p != NULL)
   {
        for (c = 0; c < length; c++)
        {
            *(p+c) = *(string+position-1);
            string++;
        }

        *(p+c) = '\0';
   }

   return p;
}

/*===============================
 fulltrim
===============================*/
char* fulltrim(char* str)
{
    int index, i;
    index = -1;
    i = 0;
    while(str[i] == ' ')
    {
        str++;
        i++;
    }

    while(str[i] != '\0')
    {
        if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
        {
            index= i;
        }
        i++;
    }
    str[index + 1] = '\0';
    return str;
}

extern BOOL isContainsAdfExt(STRPTR filePath)
{
    int i;
    char * isPointAdfStr;
    char fPath[BUFFERSIZE];
    const char padf[5] = ".adf";
    
    sprintf(fPath, "%s", filePath);
    i=0;
    while(fPath[i]) { fPath[i] = tolower(fPath[i]); i++; }

    isPointAdfStr = strstr(fPath, padf);
    if(isPointAdfStr) return 1; else return 0;
}