#ifndef RW_INI_FILES_H
#define RW_INI_FILES_H

extern char pszProfileName[260];
extern char *inifilename;
extern char pszIniDir[260];
extern char pszBrowseFolder[260];

int NemuGetProfileSection(const char *section, char *buffer, unsigned long size);
void NemuGetProfileString(const char *section, const char *entry,
                         const char *defaultValue, char *buffer);
int NemuGetProfileInt(const char *section, const char *entry, int defaultValue);
int NemuWriteProfileString(const char *section, const char *entry, char *value);
int NemuWriteProfileInt(const char *section, const char *entry, int value);
void NemuIniSetDirectory();

#endif
