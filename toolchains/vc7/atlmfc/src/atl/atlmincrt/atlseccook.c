// This is a part of the Active Template Library.
// Copyright (C) 2000-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

// Copied from CRT source file seccook.c
// Modified to not depend on the rest of the CRT

#include <windows.h>

#define _CRTALLOC(seg)	__declspec( allocate( seg ) )
typedef void(*_PVFV)();
/*
 * The global security cookie.  This name is known to the compiler.
 * Initialize to a garbage non-zero value just in case we have a buffer overrun
 * in any code that gets run before __security_init_cookie() has a chance to
 * initialize the cookie to the final value.
 */

DWORD_PTR __security_cookie = 0xBB40E64E;

/*
 * Trigger initialization of the global security cookie on program startup.
 * Force initialization before any #pragma init_seg() inits by using .CRT$XCAA
 * as the startup funcptr section.
 */

#pragma data_seg(".CRT$XCAA")
extern void __cdecl __security_init_cookie(void);
static _CRTALLOC(".CRT$XCAA") _PVFV init_cookie = __security_init_cookie;
#pragma data_seg()

static void __cdecl report_failure(void);

#if !defined (_SYSCRT) || !defined (CRTDLL)
/*
 * The routine called if a cookie check fails.
 */
#define REPORT_ERROR_HANDLER    __security_error_handler
#else  /* !defined (_SYSCRT) || !defined (CRTDLL) */
/*
 * When using an older system CRT, use a local cookie failure reporting
 * routine, with a default implementation that calls __security_error_handler
 * if available, otherwise displays a default message box.
 */
#define REPORT_ERROR_HANDLER    __local_security_error_handler
#endif  /* !defined (_SYSCRT) || !defined (CRTDLL) */

extern void __cdecl REPORT_ERROR_HANDLER(int, void *);

/***
*__security_check_cookie(cookie) - check for buffer overrun
*
*Purpose:
*       Compiler helper.  Check if a local copy of the security cookie still
*       matches the global value.  If not, then report the fatal error.
*
*       The actual reporting is split out into static helper report_failure,
*       since the cookie check routine must be minimal code that preserves
*       any registers used in returning the callee's result.
*
*Entry:
*       DWORD_PTR cookie - local security cookie to check
*
*Exit:
*       Returns immediately if the local cookie matches the global version.
*       Otherwise, calls the failure reporting handler and exits.
*
*Exceptions:
*
*******************************************************************************/

#pragma warning(disable:4100)

#ifndef _M_IX86

void __fastcall __security_check_cookie(DWORD_PTR cookie)
{
    /* Immediately return if the local cookie is OK. */
    if (cookie == __security_cookie)
        return;

    /* Report the failure */
    report_failure();
}

#else  /* _M_IX86 */

void __declspec(naked) __fastcall __security_check_cookie(DWORD_PTR cookie)
{
    /* x86 version written in asm to preserve all regs */
    __asm {
        cmp ecx, __security_cookie
        jne failure
        ret
failure:
        jmp report_failure
    }
}

#endif  /* _M_IX86 */

static void __cdecl report_failure(void)
{
    /* Report the failure */
    __try {
        REPORT_ERROR_HANDLER(_SECERR_BUFFER_OVERRUN, NULL);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        /* nothing */
    }

    ExitProcess(3);
}
