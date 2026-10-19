#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

void Config::flush()
{
    int i;
    char name[17];
    char value[32];
    if (integers) {
        for (i=0;i<integerCount;i++) {
            if (integers[i].dirty) {
                strncpy(name,integers[i].name,16);
                name[16]=0;
                wsprintf(value,"%d",integers[i].value);
                WritePrivateProfileString(section,name,value,filename);
                integers[i].dirty=0;
            }
        }
    }
    if (strings) {
        for (i=0;i<stringCount;i++) {
            if (strings[i].value && strings[i].dirty) {
                strncpy(name,strings[i].name,16);
                name[16]=0;
                WritePrivateProfileString(section,name,strings[i].value,filename);
                strings[i].dirty=0;
            }
        }
    }
}

Config::~Config()
{
    flush();
    if (integers) free(integers);
    if (strings) {
        for (int i=0;i<stringCount;i++) {
            if (strings[i].value) free(strings[i].value);
        }
        free(strings);
    }
}

Config::Config(char *file,char *sect)
{
    if (sect) {
        strncpy(section,sect,31);
        section[31]=0;
    } else strcpy(section,"config");
    if (file) strncpy(filename,file,259);
    else {
        GetModuleFileName(GetModuleHandle(0),filename,sizeof(filename));
        char *p=filename;
        while (*p) p++;
        while (p>filename && *p!='.' && *p!='\\') p--;
        if (*p=='.') strcpy(p+1,"ini");
        else {
            if (*p=='\\') strcpy(++p,sect);
            else strcpy(filename,sect);
            strcat(p,".ini");
        }
    }
    integers=0;
    StringEntry *emptyStrings;
    memset(&emptyStrings,0,sizeof(emptyStrings));
    strings=emptyStrings;
    integerCount=integerCapacity=0;
    stringCount=stringCapacity=0;
    filename[259]=0;
}

void Config::writeInt(char *name,int value,int dirty)
{
    int i;
    for (i=0;i<integerCount;i++) {
        if (!strncmp(name,integers[i].name,16)) {
            integers[i].dirty=1;
            integers[i].value=value;
            return;
        }
    }
    if (integerCount>=integerCapacity || !integers) {
        integerCapacity=integerCount+8;
        integers=(IntEntry *)realloc(integers,integerCapacity*sizeof(IntEntry));
    }
    strncpy(integers[integerCount].name,name,16);
    integers[integerCount].dirty=dirty;
    integers[integerCount].value=value;
    integerCount++;
}

int Config::readInt(char *name,int defaultValue)
{
    for (int i=0;i<integerCount;i++) {
        if (!strncmp(name,integers[i].name,16)) return integers[i].value;
    }
    int value=GetPrivateProfileInt(section,name,defaultValue,filename);
    writeInt(name,value,0);
    return value;
}

char *Config::writeString(char *name,char *value,int dirty)
{
    for (int i=0;i<stringCount;i++) {
        if (!strncmp(name,strings[i].name,16)) {
            if (strings[i].value && strlen(value)<strlen(strings[i].value)) {
                strcpy(strings[i].value,value);
            } else {
                if (strings[i].value) free(strings[i].value);
                strings[i].value=strdup(value);
            }
            strings[i].dirty=dirty;
            return strings[i].value;
        }
    }
    if (stringCount>=stringCapacity || !strings) {
        stringCapacity=stringCount+8;
        strings=(StringEntry *)realloc(strings,stringCapacity*sizeof(StringEntry));
    }
    strncpy(strings[stringCount].name,name,16);
    strings[stringCount].dirty=dirty;
    strings[stringCount].value=strdup(value);
    return strings[stringCount++].value;
}

char *Config::readString(char *name,char *defaultValue)
{
    for (int i=0;i<stringCount;i++) {
        if (!strncmp(name,strings[i].name,16)) return strings[i].value;
    }
    char value[2048];
    GetPrivateProfileString(section,name,defaultValue,value,sizeof(value),filename);
    return writeString(name,value,0);
}
