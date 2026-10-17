#ifndef NEMU64_FRONTEND_INI_H
#define NEMU64_FRONTEND_INI_H

extern char NemuIniPath[260];
extern char ConfigurationIniPath[260];
extern char CheatsIniPath[260];
extern char InitialDirectory[260];

void ReadIniString(const char* section, const char* key, const char* defaultValue,
                   char* destination, const char* filename);
int ReadIniInteger(const char* section, const char* key, int defaultValue,
                   const char* filename);
void WriteIniString(const char* section, const char* key, const char* value,
                    const char* filename);
void WriteIniInteger(const char* section, const char* key, int value,
                     const char* filename);
void InitializeIniPaths();
void ReplaceExtension(char* filename, const char* extension);
int ReadRomInteger(const char* key, int defaultValue);
void ReadRomString(const char* key, const char* defaultValue, char* destination);
void ReadRomConfiguration();
void SaveBrowserSettings();

#endif
