// Reconstructed cached INI settings. Original private names are unknown.
#ifndef KAILLERA_CONFIG_H
#define KAILLERA_CONFIG_H

class Config {
public:
    void flush();
    ~Config();
    Config(char *filename=0,char *section=0);
    void writeInt(char *name,int value,int dirty=1);
    int readInt(char *name,int defaultValue);
    char *writeString(char *name,char *value,int dirty=1);
    char *readString(char *name,char *defaultValue);
private:
    struct IntEntry { char name[16]; int value; int dirty; };
    struct StringEntry { char name[16]; char *value; int dirty; };
    IntEntry *integers;
    StringEntry *strings;
    int integerCount,integerCapacity;
    int stringCount,stringCapacity;
    char filename[260];
    char section[32];
};
#endif
