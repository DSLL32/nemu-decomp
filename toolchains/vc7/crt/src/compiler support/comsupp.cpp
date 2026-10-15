/***
* comsupp.cpp - Native C++ compiler COM support - IDispatch::Invoke wrapper
*
*   Copyright (C) 1996-2001 Microsoft Corporation
*   All rights reserved.
*
****/

#include <comdef.h>

#pragma hdrstop

#include <stdarg.h>
#include <malloc.h>

#include "invkprxy.h"

#pragma intrinsic(memset)

#pragma warning(disable:4290)

/////////////////////////////////////////////////////////////////////////////

typedef void (__stdcall *__errorPfnType)(HRESULT hr, IErrorInfo* perrinfo);

// throw exceptions by default
__errorPfnType __errorPfn = &_com_raise_error;

/////////////////////////////////////////////////////////////////////////////

void __stdcall 
_set_com_error_handler(void (__stdcall *pHandler)(HRESULT hr, IErrorInfo* perrinfo)) {
    __errorPfn = pHandler;
}

void __stdcall
_com_issue_error(HRESULT hr) throw(_com_error)
{
    __errorPfn(hr, NULL);
}

void __stdcall
_com_issue_errorex(HRESULT hr, IUnknown* punk, REFIID riid) throw(_com_error)
{
    IErrorInfo* perrinfo = NULL;
    if (punk == NULL) {
        goto exeunt;
    }
    ISupportErrorInfo* psei;
    if (FAILED(punk->QueryInterface(__uuidof(ISupportErrorInfo),
               (void**)&psei))) {
        goto exeunt;
    }
    HRESULT hrSupportsErrorInfo;
    hrSupportsErrorInfo = psei->InterfaceSupportsErrorInfo(riid);
    psei->Release();
    if (hrSupportsErrorInfo != S_OK) {
        goto exeunt;
    }
    if (GetErrorInfo(0, &perrinfo) != S_OK) {
        perrinfo = NULL;
    }
exeunt:
    __errorPfn(hr, perrinfo);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT __cdecl
_com_dispatch_method(IDispatch* pDispatch,
                     DISPID dwDispID,
                     WORD wFlags,
                     VARTYPE vtRet,
                     void* pvRet,
                     const wchar_t* pwParamInfo,
                     ...) throw(_com_error)
{
    va_list argList;
    va_start(argList, pwParamInfo);

    IErrorInfo* perrinfo;
    HRESULT hr = _com_invoke_helper(pDispatch,
                                    dwDispID,
                                    wFlags,
                                    vtRet,
                                    pvRet,
                                    pwParamInfo,
                                    argList,
                                    &perrinfo);
    if (FAILED(hr)) {
        __errorPfn(hr, perrinfo);
    }

    va_end(argList);
    return hr;
}

HRESULT __stdcall
_com_dispatch_propget(IDispatch* pDispatch,
                      DISPID dwDispID,
                      VARTYPE vtProp,
                      void* pvProp) throw(_com_error)
{
    return _com_dispatch_method(pDispatch,
                                dwDispID,
                                DISPATCH_PROPERTYGET,
                                vtProp,
                                pvProp,
                                NULL);
}

HRESULT __cdecl
_com_dispatch_propput(IDispatch* pDispatch,
                      DISPID dwDispID,
                      VARTYPE vtProp,
                      ...) throw(_com_error)
{
    va_list argList;
    va_start(argList, vtProp);
#ifdef _MAC
    argList -= 2;
#endif

    wchar_t rgwParams[2];
    rgwParams[0] = vtProp;
    rgwParams[1] = 0;

    WORD wFlags = (vtProp == VT_DISPATCH || vtProp == VT_UNKNOWN)
                    ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT;

    IErrorInfo* perrinfo;
    HRESULT hr = _com_invoke_helper(pDispatch,
                                    dwDispID,
                                    wFlags,
                                    VT_EMPTY,
                                    NULL,
                                    rgwParams,
                                    argList,
                                    &perrinfo);
    if (FAILED(hr)) {
        __errorPfn(hr, perrinfo);
    }

    va_end(argList);
    return hr;
}
