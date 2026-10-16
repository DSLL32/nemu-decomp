#include "StdAfx.h"
#include <stdlib.h>
#include <string.h>
#include "RW_INI_files.h"

char pszBrowseFolder[MAX_PATH] = {0};
char pszIniDir[MAX_PATH] = {0};
char pszProfileName[MAX_PATH] = {0};
char *inifilename = 0;

int NemuGetProfileSection(const char *section, char *buffer, unsigned long size)
{
    return GetPrivateProfileSection(section, buffer, size, pszProfileName);
}

void NemuGetProfileString(const char *section, const char *entry,
                         const char *defaultValue, char *buffer)
{
    GetPrivateProfileString(section, entry, defaultValue, buffer, 256, pszProfileName);
}

int NemuGetProfileInt(const char *section, const char *entry, int defaultValue)
{
    char defaultText[32];
    char valueText[32];
    int value;
    int count;

    itoa(defaultValue, defaultText, 10);
    count = GetPrivateProfileString(section, entry, defaultText,
                                   valueText, 256, pszProfileName);
    value = atoi(valueText);
    return value;
}

int NemuWriteProfileString(const char *section, const char *entry, char *value)
{
    return WritePrivateProfileString(section, entry, value, pszProfileName);
}

int NemuWriteProfileInt(const char *section, const char *entry, int value)
{
    char valueText[32];
    itoa(value, valueText, 10);
    return WritePrivateProfileString(section, entry, valueText, pszProfileName);
}

void NemuIniSetDirectory()
{
    memset(pszProfileName, 0, 260);
    lstrcat(pszProfileName, inifilename);
}
