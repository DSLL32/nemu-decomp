#include <windows.h>
#include <stdio.h>
#include "StaticRuntime.h"
extern "C" void __cdecl RunVclCrash(void);
int main(void)
{
    if(!NemuStaticInitialize(GetModuleHandleA(NULL))) {
        fprintf(stderr,"Static init failed: RAM=%p error=%lu\n",NemuStaticRdram(),GetLastError());return 3;
    }
    RunVclCrash();return 4;
}
