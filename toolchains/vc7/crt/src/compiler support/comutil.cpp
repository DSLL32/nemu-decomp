/***
* comutil.cpp - Native C++ compiler COM support - conversion helpers
*
*   Copyright (C) 1996-2001 Microsoft Corporation
*   All rights reserved.
*
****/
//
// The definitions of functions and data declared in comutil.h
//

#include <comdef.h>

#pragma hdrstop

#include <malloc.h>

#pragma warning(disable:4290)

_variant_t vtMissing(DISP_E_PARAMNOTFOUND, VT_ERROR);

namespace _com_util {
    //
    // Convert char * to BSTR
    //
    BSTR __stdcall ConvertStringToBSTR(const char* pSrc) throw(_com_error)
    {
        if (pSrc == NULL) {
            return NULL;
        }
        else {
            int size = lstrlenA(pSrc) + 1;
            BSTR pDest = (size < 0x1000) 
                         ? static_cast<BSTR>(::_alloca(size * sizeof(wchar_t)))
                         : static_cast<BSTR>(::malloc(size * sizeof(wchar_t)));

            if (pDest == NULL) {
                _com_issue_error(E_OUTOFMEMORY);
            }

            pDest[0] = '\0';

            if (::MultiByteToWideChar(CP_ACP, 0, pSrc, -1, pDest, size) == 0) {
                _com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
            }

            BSTR pRet = ::SysAllocString(pDest);
            if (size >= 0x1000) {
                free(pDest);
            }
            return pRet;
        }
    }

    // Convert BSTR to char *
    //
    char* __stdcall ConvertBSTRToString(BSTR pSrc) throw(_com_error)
    {
        if (pSrc == NULL) {
            return NULL;
        }
        else {
            int size = (int)((wcslen(pSrc) + 1) * sizeof(wchar_t));
            char* pDest;
            try {
                pDest = ::new char[size];
            } catch (...) {
                pDest = NULL;
            }

            if (pDest == NULL) {
                _com_issue_error(E_OUTOFMEMORY);
            }

            pDest[0] = '\0';

            if (::WideCharToMultiByte(CP_ACP, 0, pSrc, -1, pDest, size, NULL, NULL) == 0) {
                delete [] pDest;
                pDest = 0;
                _com_issue_error(HRESULT_FROM_WIN32(GetLastError()));
            }

            return pDest;
        }
    }
}
