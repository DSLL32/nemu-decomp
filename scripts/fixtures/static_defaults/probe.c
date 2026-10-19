/* Tests the actual static archive, using synthetic disk overrides only. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "StaticFiles.h"
extern unsigned char __cdecl NemuStaticInitialize(HINSTANCE);
extern void __cdecl NemuStaticShutdown(void);
extern int __cdecl ProbeReadText(const char *, unsigned int *, unsigned int *);
extern int __cdecl ProbeIniValue(const char *, const char *, const char *);
extern void __cdecl ProbeIniWrite(const char *, const char *, const char *, int);
extern void __cdecl ProbeLoadDatFile(void);
extern char *ProbeDatFilename;
typedef struct { unsigned int Length, lo, hi, extras; unsigned int commands[40][7]; } Script;
typedef struct { unsigned int Count; Script *Script; } Scripts;
extern Scripts *ProbeScripts;
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ExitProcess(5); } } while(0)
static void writefile(const char *name, const char *data, unsigned int size) {
 FILE *file=fopen(name,"wb");REQUIRE(file!=NULL);
 REQUIRE(fwrite(data,1,size,file)==size);REQUIRE(fclose(file)==0);
}
static void compare(const char *file,const char *section,const char *key,unsigned int capacity) {
 char a[65536],b[65536];DWORD na,nb;
 memset(a,0x55,sizeof(a));memset(b,0x55,sizeof(b));
 na=NemuStaticGetPrivateProfileStringA(section,key,"fallback  ",a,capacity,file);
 nb=GetPrivateProfileStringA(section,key,"fallback  ",b,capacity,".\\reference.ini");
 if(na!=nb || memcmp(a,b,capacity)) {
  fprintf(stderr,"INI mismatch file=%s section=%s key=%s capacity=%u lengths=%lu/%lu\n",file,section?section:"(null)",key?key:"(null)",capacity,na,nb);
  fprintf(stderr,"values='%s'/'%s'\n",a,b);ExitProcess(6);
 }
}
static void ini(const char *name) {
 const char *data,*section,*key;unsigned int size,capacity;
 char sections[65536],keys[65536];
 data=NemuStaticDefaultData(name,&size);REQUIRE(data!=NULL);
 writefile("reference.ini",data,size);
 REQUIRE(GetPrivateProfileStringA(NULL,NULL,NULL,sections,sizeof(sections),".\\reference.ini")>0);
 compare(name,NULL,NULL,sizeof(sections));
 for(section=sections;*section;section+=strlen(section)+1) {
  compare(name,section,NULL,sizeof(keys));
  GetPrivateProfileStringA(section,NULL,NULL,keys,sizeof(keys),".\\reference.ini");
  for(key=keys;*key;key+=strlen(key)+1) {
   compare(name,section,key,65536);
   for(capacity=0;capacity<8;++capacity)compare(name,section,key,capacity);
  }
 }
 compare(name,"absent section","absent key",32);
 REQUIRE(GetFileAttributesA(name)==INVALID_FILE_ATTRIBUTES);
 DeleteFileA("reference.ini");
}
static void text(const char *name) {
 NemuStaticText *file;unsigned int hash=2166136261u,lines=0,ph,pl,i;char line[4096];
 file=NemuStaticTextOpen(name);REQUIRE(file!=NULL);
 while(NemuStaticTextLine(file,line,sizeof(line))) {
  unsigned int n=strlen(line);if(n&&line[n-1]=='\n')--n;if(n&&line[n-1]=='\r')--n;
  for(i=0;i<n;++i)hash=(hash^(unsigned char)line[i])*16777619u;
  hash=(hash^10)*16777619u;++lines;
 }
 NemuStaticTextClose(file);
 REQUIRE(ProbeReadText(name,&ph,&pl));REQUIRE(ph==hash&&pl==lines);
 printf("Text %s: lines=%u hash=%08X\n",name,lines,hash);
}
static unsigned int combiners(void) {
 unsigned int hash=2166136261u,i,count;
 strcpy(ProbeDatFilename,".\\LemD3DCombine.dat");ProbeLoadDatFile();
 count=ProbeScripts->Count;
 for(i=0;i<count;++i) {
  Script *s=&ProbeScripts->Script[i];
  hash=(hash^s->lo)*16777619u;hash=(hash^s->hi)*16777619u;
  hash=(hash^s->extras)*16777619u;hash=(hash^s->Length)*16777619u;
 }
 printf("Combiners: count=%u hash=%08X\n",count,hash);
 free(ProbeScripts->Script);ProbeScripts->Script=NULL;
 return hash;
}
int main(void) {
 const char *names[]={"config.ini","nemu64.ini","LemD3DCombine.dat","LemD3DCombine.new"};
 const char *data;unsigned int size,i,embedded;char value[256];int order[7];
 setvbuf(stdout,NULL,_IONBF,0);
 REQUIRE(NemuStaticInitialize(GetModuleHandleA(NULL)));
 ini(".\\config.ini");ini(".\\nemu64.ini");
 REQUIRE(ProbeIniValue(".\\config.ini","Configuration","RDRAMSize")==8);
 REQUIRE(ProbeIniValue(".\\nemu64.ini","1080 SNOWBOARDING","CompilerMode")==3);
 REQUIRE(NemuStaticGetPrivateProfileStructA("Configuration","ColumnOrder",order,sizeof(order),".\\config.ini"));
 for(i=0;i<7;++i)REQUIRE(order[i]==i);
 for(i=0;i<4;++i) {
  REQUIRE(GetFileAttributesA(names[i])==INVALID_FILE_ATTRIBUTES);
  text(names[i]);
  data=NemuStaticDefaultData(names[i],&size);REQUIRE(data!=NULL);
  writefile(names[i],data,size);text(names[i]);
  REQUIRE(NemuStaticDefaultData(names[i],&size)==NULL);
  DeleteFileA(names[i]);
 }
 embedded=combiners();REQUIRE(ProbeScripts->Count>100);
 data=NemuStaticDefaultData("LemD3DCombine.dat",&size);writefile("LemD3DCombine.dat",data,size);
 REQUIRE(combiners()==embedded);DeleteFileA("LemD3DCombine.dat");
 data="[0000010000000200000000]\r\nNewPass 5, 6\r\n";
 writefile("LemD3DCombine.dat",data,strlen(data));
 REQUIRE(combiners()!=embedded);REQUIRE(ProbeScripts->Count==1);text("LemD3DCombine.dat");
 writefile("LemD3DCombine.dat","",0);combiners();REQUIRE(ProbeScripts->Count==0);text("LemD3DCombine.dat");
 DeleteFileA("LemD3DCombine.dat");REQUIRE(combiners()==embedded);
 /* Empty and partial disk INIs replace the entire embedded file. */
 writefile("config.ini","",0);
 NemuStaticGetPrivateProfileStringA("Configuration","RDRAMSize","empty",value,sizeof(value),".\\config.ini");REQUIRE(!strcmp(value,"empty"));
 data="[Configuration]\r\nRDRAMSize=4\r\n";writefile("config.ini",data,strlen(data));
 NemuStaticGetPrivateProfileStringA("Configuration","RDRAMSize","",value,sizeof(value),".\\config.ini");REQUIRE(!strcmp(value,"4"));
 NemuStaticGetPrivateProfileStringA("GFX","Width","missing",value,sizeof(value),".\\config.ini");REQUIRE(!strcmp(value,"missing"));
 REQUIRE(ProbeIniValue(".\\config.ini","Configuration","RDRAMSize")==4);
 REQUIRE(ProbeIniValue(".\\config.ini","GFX","Width")==-99);
 REQUIRE(NemuStaticWritePrivateProfileStringA("Configuration","Resolution","1024",".\\config.ini"));
 NemuStaticGetPrivateProfileStringA("GFX","Width","missing",value,sizeof(value),".\\config.ini");REQUIRE(!strcmp(value,"missing"));
 DeleteFileA("config.ini");
 /* A first write preserves the rest of the built-in defaults. */
 ProbeIniWrite(".\\config.ini","Configuration","Resolution",900);
 REQUIRE(GetPrivateProfileIntA("GFX","Width",0,".\\config.ini")==800);
 REQUIRE(GetPrivateProfileIntA("Configuration","Resolution",0,".\\config.ini")==900);
 DeleteFileA("config.ini");
 REQUIRE(NemuStaticWritePrivateProfileStringA("Configuration","Resolution","1024",".\\config.ini"));
 REQUIRE(GetPrivateProfileIntA("Configuration","Resolution",0,".\\config.ini")==1024);
 REQUIRE(GetPrivateProfileIntA("GFX","Width",0,".\\config.ini")==800);
 REQUIRE(GetPrivateProfileIntA("Configuration","RDRAMSize",0,".\\config.ini")==8);
 DeleteFileA("config.ini");order[0]=6;order[6]=0;
 REQUIRE(NemuStaticWritePrivateProfileStructA("Configuration","ColumnOrder",order,sizeof(order),".\\config.ini"));
 REQUIRE(GetPrivateProfileIntA("GFX","Width",0,".\\config.ini")==800);
 DeleteFileA("config.ini");
 for(i=0;i<4;++i)REQUIRE(GetFileAttributesA(names[i])==INVALID_FILE_ATTRIBUTES);
 NemuStaticShutdown();puts("PASS: embedded defaults, Windows INI parity, Delphi/C text, real combiner parser, disk/empty overrides, seeded writes");
 return 0;
}
