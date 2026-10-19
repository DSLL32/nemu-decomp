#include <windows.h>
#include <stdlib.h>
#include "StaticCrash.h"
static volatile unsigned observed;
extern "C" __declspec(noinline) unsigned CrashLeaf(unsigned mode)
{
    volatile unsigned locals[24];
    for (unsigned i=0;i<24;++i)locals[i]=i+mode;
    observed=locals[mode%24];
    *(volatile unsigned *)0=observed;
    return locals[3];
}
extern "C" __declspec(noinline) unsigned CrashMiddle(unsigned mode)
{
    volatile unsigned locals[9];
    for(unsigned i=0;i<9;++i)locals[i]=mode+i;
    unsigned value=CrashLeaf(mode);
    return value+locals[mode%9];
}
extern "C" __declspec(naked) void BrokenFrame(void)
{
    __asm {
        mov ebp, 0deadbeefh
        xor eax,eax
        mov dword ptr [eax],1
        ret
    }
}
extern "C" __declspec(noinline) DWORD WINAPI CrashWorker(void *) { return CrashMiddle(2); }
int main(int argc,char **argv)
{
    NemuStaticCrashInitialize(GetModuleHandleA(NULL));
    if(argc>1 && !lstrcmpA(argv[1],"caught")) {
        __try { *(volatile unsigned *)0=1; }
        __except(EXCEPTION_EXECUTE_HANDLER) { observed=1; }
        NemuStaticCrashShutdown();return observed==1 ? 0 : 1;
    }
    if(argc>1 && !lstrcmpA(argv[1],"badbp"))BrokenFrame();
    if(argc>1 && !lstrcmpA(argv[1],"worker")) {
        HANDLE thread=CreateThread(NULL,0,CrashWorker,NULL,0,NULL);
        WaitForSingleObject(thread,INFINITE);
    } else CrashMiddle(1);
    return 2;
}
