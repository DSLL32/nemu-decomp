#include <windows.h>
#include <stdio.h>
#pragma pack(push,1)
typedef struct { unsigned short priority; void (__cdecl *function)(void); } Init;
#pragma pack(pop)
extern Init _delphi_init_system[], _delphi_init_SysInit[], _delphi_init_RuntimeProbe[], _delphi_init_HelperProbe[];
extern Init _delphi_fini_system[], _delphi_fini_SysInit[], _delphi_fini_RuntimeProbe[], _delphi_fini_HelperProbe[];
extern void __cdecl VclInit(unsigned char,unsigned char,int,unsigned char);
extern void __cdecl VclExit(void);
extern int __cdecl Values(int), Strings(void), Callback(void*,int), FinallyTest(int), ThreadValue(int), CrossUnit(int);
extern int __cdecl ExceptionTest(int);
extern unsigned int TlsIndex;
extern unsigned long _tls_index;
__declspec(thread) volatile int nativeTls = 99;
static DWORD WINAPI thread(void *unused) {
 return ThreadValue(123)==0 && ThreadValue(321)==123 ? 0:1;
}
static int __cdecl callback(int a) { return a*7; }
#define REQUIRE(x) do { if(!(x)) { printf("FAIL line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
 HANDLE h;DWORD code;
 VclInit(0,0,(int)GetModuleHandleA(NULL),0);
 TlsIndex = _tls_index;
 _delphi_init_SysInit[0].function();
 _delphi_init_system[0].function();
 _delphi_init_HelperProbe[0].function();
 _delphi_init_RuntimeProbe[0].function();
 REQUIRE(nativeTls==99);
 REQUIRE(Values(0)==42);
 REQUIRE(Strings()==23);
 REQUIRE(CrossUnit(2)==18);
 REQUIRE(CrossUnit(2)==20);
 REQUIRE(Callback(callback,4)==39);
 REQUIRE(FinallyTest(9)==32);
 REQUIRE(ExceptionTest(1)==-776);
 REQUIRE(ExceptionTest(0)==19);
 REQUIRE(Strings()==23);
 REQUIRE(ThreadValue(7)==0);
 h=CreateThread(NULL,0,thread,NULL,0,NULL);REQUIRE(h!=NULL);
 REQUIRE(WaitForSingleObject(h,10000)==WAIT_OBJECT_0);
 REQUIRE(GetExitCodeThread(h,&code) && code==0);CloseHandle(h);
 REQUIRE(ThreadValue(8)==7);
 _delphi_fini_RuntimeProbe[0].function();
 REQUIRE(Values(0)==-88);
 _delphi_fini_HelperProbe[0].function();
 _delphi_fini_system[0].function();
 _delphi_fini_SysInit[0].function();
 VclExit();
 puts("PASS: globals, cross-unit data imports, callbacks, initialization/finalization, managed strings, exception unwinding/finally, native TLS and worker thread");
 return 0;
}
