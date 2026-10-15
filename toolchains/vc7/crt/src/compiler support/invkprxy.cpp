/***
* invkprxy.cpp - Native C++ compiler COM support - non-exception handling IDispatch::Invoke
*
*   Copyright (C) 1996-2001 Microsoft Corporation
*   All rights reserved.
*
****/

#include <comdef.h>
#pragma hdrstop

#include <stdarg.h>

#include "invkprxy.h"

#pragma intrinsic(memset)

#pragma warning(disable:4290)

/////////////////////////////////////////////////////////////////////////////

#define VT_OPTIONAL 0x0800

struct FLOAT_ARG  { BYTE floatBits[sizeof(float)]; };
struct DOUBLE_ARG { BYTE doubleBits[sizeof(double)]; };

/////////////////////////////////////////////////////////////////////////////

HRESULT __cdecl
_com_invoke_helper(IDispatch* pDispatch,
                   DISPID dwDispID,
                   WORD wFlags,
                   VARTYPE vtRet,
                   void* pvRet,
                   const wchar_t* pwParamInfo,
                   va_list argList,
                   IErrorInfo** pperrinfo) 
{
    *pperrinfo = NULL;

    if (pDispatch == NULL) {
        return E_POINTER;
    }

    DISPPARAMS dispparams;
    VARIANT* rgvarg;
    rgvarg = NULL;
    memset(&dispparams, 0, sizeof dispparams);

    // determine number of arguments
    if (pwParamInfo != NULL) {
        dispparams.cArgs = lstrlenW(pwParamInfo);
    }

    DISPID dispidNamed;
    dispidNamed = DISPID_PROPERTYPUT;
    if (wFlags & (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF)) {
        if (dispparams.cArgs <= 0) {
            return E_INVALIDARG;
        }
        dispparams.cNamedArgs = 1;
        dispparams.rgdispidNamedArgs = &dispidNamed;
    }

    if (dispparams.cArgs != 0) {
        // allocate memory for all VARIANT parameters
        rgvarg = (VARIANT*)_alloca(dispparams.cArgs * sizeof(VARIANT));
        memset(rgvarg, 0, sizeof(VARIANT) * dispparams.cArgs);
        dispparams.rgvarg = rgvarg;

        // get ready to walk vararg list
        const wchar_t* pw = pwParamInfo;
        VARIANT* pArg;
        pArg = rgvarg + dispparams.cArgs - 1;   // params go in opposite order

        while (*pw != 0) {
            V_VT(pArg) = (VARTYPE) (*pw & ~VT_OPTIONAL); // set the variant type
            switch (V_VT(pArg)) {
            case VT_I2:
#ifdef _MAC
                V_I2(pArg) = (short)va_arg(argList, int);
#else
                V_I2(pArg) = va_arg(argList, SHORT);
#endif
                break;
            case VT_I4:
                V_I4(pArg) = va_arg(argList, LONG);
                break;
            case VT_R4:
                // Note: All float arguments to vararg functions are passed
                //  as doubles instead.  That's why they are passed as VT_R8
                //  instead of VT_R4.
                V_VT(pArg) = VT_R8;
                *(DOUBLE_ARG*)&V_R8(pArg) = va_arg(argList, DOUBLE_ARG);
                break;
            case VT_R8:
                *(DOUBLE_ARG*)&V_R8(pArg) = va_arg(argList, DOUBLE_ARG);
                break;
            case VT_DATE:
                *(DOUBLE_ARG*)&V_DATE(pArg) = va_arg(argList, DOUBLE_ARG);
                break;
            case VT_CY:
                V_CY(pArg) = *va_arg(argList, CY*);
                break;
            case VT_BSTR:
                V_BSTR(pArg) = va_arg(argList, BSTR);
                break;
            case VT_DISPATCH:
                V_DISPATCH(pArg) = va_arg(argList, IDispatch*);
                break;
            case VT_ERROR:
            case VT_HRESULT:
                V_ERROR(pArg) = va_arg(argList, SCODE);
                break;
            case VT_BOOL:
#ifdef _MAC
                V_BOOL(pArg) = (VARIANT_BOOL)va_arg(argList, int)
                                    ? VARIANT_TRUE : VARIANT_FALSE;
#else
                V_BOOL(pArg) = va_arg(argList, VARIANT_BOOL)
                                    ? VARIANT_TRUE : VARIANT_FALSE;
#endif
                break;
            case VT_VARIANT:
                *pArg = *va_arg(argList, VARIANT*);
                break;
            case VT_UNKNOWN:
                V_UNKNOWN(pArg) = va_arg(argList, IUnknown*);
                break;
            case VT_DECIMAL:
                V_DECIMAL(pArg) = *va_arg(argList, DECIMAL*);
                V_VT(pArg) = VT_DECIMAL;
                break;

            case VT_I1:
#ifdef _MAC
                V_I1(pArg) = (CHAR)va_arg(argList, int);
#else
                V_I1(pArg) = va_arg(argList, CHAR);
#endif
                break;
            case VT_UI1:
#ifdef _MAC
                V_UI1(pArg) = (BYTE)va_arg(argList, int);
#else
                V_UI1(pArg) = va_arg(argList, BYTE);
#endif
                break;
            case VT_UI2:
#ifdef _MAC
                V_UI2(pArg) = (USHORT)va_arg(argList, int);
#else
                V_UI2(pArg) = va_arg(argList, USHORT);
#endif
                break;
            case VT_UI4:
                V_UI4(pArg) = va_arg(argList, ULONG);
                break;
            case VT_INT:
                V_INT(pArg) = va_arg(argList, INT);
                break;
            case VT_UINT:
                V_UINT(pArg) = va_arg(argList, UINT);
                break;
            case VT_I8:
                V_I8(pArg) = va_arg(argList, LONGLONG);
                break;
            case VT_UI8:
                V_UI8(pArg) = va_arg(argList, ULONGLONG);
                break;
            case VT_RECORD:
            case VT_RECORD|VT_BYREF:
                V_RECORD(pArg) = va_arg(argList, PVOID);
                break;

            case VT_I2|VT_BYREF:
                V_I2REF(pArg) = va_arg(argList, SHORT*);
                break;
            case VT_I4|VT_BYREF:
                V_I4REF(pArg) = va_arg(argList, LONG*);
                break;
            case VT_R4|VT_BYREF:
                V_R4REF(pArg) = va_arg(argList, FLOAT*);
                break;
            case VT_R8|VT_BYREF:
                V_R8REF(pArg) = va_arg(argList, DOUBLE*);
                break;
            case VT_DATE|VT_BYREF:
                V_DATEREF(pArg) = va_arg(argList, DATE*);
                break;
            case VT_CY|VT_BYREF:
                V_CYREF(pArg) = va_arg(argList, CY*);
                break;
            case VT_BSTR|VT_BYREF:
                V_BSTRREF(pArg) = va_arg(argList, BSTR*);
                break;
            case VT_DISPATCH|VT_BYREF:
                V_DISPATCHREF(pArg) = va_arg(argList, IDispatch**);
                break;
            case VT_ERROR|VT_BYREF:
            case VT_HRESULT|VT_BYREF:
                V_ERRORREF(pArg) = va_arg(argList, SCODE*);
                break;
            case VT_BOOL|VT_BYREF:
                V_BOOLREF(pArg) = va_arg(argList, VARIANT_BOOL*);
                break;
            case VT_VARIANT|VT_BYREF:
                V_VARIANTREF(pArg) = va_arg(argList, VARIANT*);
                break;
            case VT_UNKNOWN|VT_BYREF:
                V_UNKNOWNREF(pArg) = va_arg(argList, IUnknown**);
                break;
            case VT_DECIMAL|VT_BYREF:
                V_DECIMALREF(pArg) = va_arg(argList, DECIMAL*);
                break;

            case VT_I1|VT_BYREF:
                V_I1REF(pArg) = va_arg(argList, CHAR*);
                break;
            case VT_UI1|VT_BYREF:
                V_UI1REF(pArg) = va_arg(argList, BYTE*);
                break;
            case VT_UI2|VT_BYREF:
                V_UI2REF(pArg) = va_arg(argList, USHORT*);
                break;
            case VT_UI4|VT_BYREF:
                V_UI4REF(pArg) = va_arg(argList, ULONG*);
                break;
            case VT_INT|VT_BYREF:
                V_INTREF(pArg) = va_arg(argList, INT*);
                break;
            case VT_UINT|VT_BYREF:
                V_UINTREF(pArg) = va_arg(argList, UINT*);
                break;
            case VT_I8|VT_BYREF:
                V_I8REF(pArg) = va_arg(argList, LONGLONG*);
                break;
            case VT_UI8|VT_BYREF:
                V_UI8REF(pArg) = va_arg(argList, ULONGLONG*);
                break;

            default:
                if (V_VT(pArg) & VT_ARRAY) {
                    if (V_VT(pArg) & VT_BYREF) {
                        V_ARRAYREF(pArg) = va_arg(argList, SAFEARRAY**);
                    } else {
                        V_ARRAY(pArg) = va_arg(argList, SAFEARRAY*);
                    }
                    break;
                }
                // unknown type!
                return E_INVALIDARG;
            }

            --pArg; // get ready to fill next argument
            ++pw;
        }

        // Check for missing optional unnamed args at the end of the arglist,
        // and remove them from the DISPPARAMS.  This permits calling servers
        // which modify their action depending on the actual number of args.
        // E.g. Excel95 Application.Workbooks returns a Workbooks* if called
        // with no args, a Workbook* if called with one arg - this shouldn't
        // be necessary, but Excel95 doesn't appear to check for missing
        // args indicated by VT_ERROR/DISP_E_PARAMNOTFOUND.
        pArg = rgvarg + dispparams.cNamedArgs;
        pw = pwParamInfo + dispparams.cArgs - dispparams.cNamedArgs - 1;
        unsigned int cMissingArgs = 0;

        // Count the number of missing arguments
        while (pw >= pwParamInfo) {
            // Optional args must be VARIANT or VARIANT*
            if ((*pw & ~VT_BYREF) != (VT_VARIANT|VT_OPTIONAL)) {
                break;
            }

            VARIANT* pVar;
            pVar = (*pw & VT_BYREF) ? pArg->pvarVal : pArg;
            if (V_VT(pVar) != VT_ERROR ||
                V_ERROR(pVar) != DISP_E_PARAMNOTFOUND)
            {
                break;
            }

            ++cMissingArgs;
            ++pArg;
            --pw;
        }

        // Move the named args up next to the remaining unnamed args and
        // adjust the DISPPARAMS struct.
        if (cMissingArgs > 0) {
            for (unsigned int c = 0; c < dispparams.cNamedArgs; ++c) {
                rgvarg[c + cMissingArgs] = rgvarg[c];
            }
            dispparams.cArgs -= cMissingArgs;
            dispparams.rgvarg += cMissingArgs;
        }
    }

    // initialize return value
    VARIANT* pvarResult;
    VARIANT vaResult;
    VariantInit(&vaResult);
    pvarResult = (vtRet != VT_EMPTY) ? &vaResult : NULL;

    // initialize EXCEPINFO struct
    EXCEPINFO excepInfo;
    memset(&excepInfo, 0, sizeof excepInfo);

    UINT nArgErr;
    nArgErr = (UINT)-1;  // initialize to invalid arg

    // make the call
    HRESULT hr = pDispatch->Invoke(dwDispID, __uuidof(NULL), 0, wFlags,
                                   &dispparams, pvarResult, &excepInfo,
                                   &nArgErr);

    if (FAILED(hr)) {
        VariantClear(&vaResult);
        if (hr != DISP_E_EXCEPTION) {
            // non-exception error code
            return hr;
        }

        return _com_handle_excepinfo(excepInfo, pperrinfo);
    }

    if (vtRet != VT_EMPTY) {
        // convert return value unless already correct
        if (vtRet != VT_VARIANT && vtRet != vaResult.vt) {
            hr = VariantChangeType(&vaResult, &vaResult, 0, vtRet);
            if (FAILED(hr)) {
                VariantClear(&vaResult);
                return hr;
            }
        }

        if (pvRet == 0) {
            return E_POINTER;
        } else {

            // copy return value into return spot!
            switch (vtRet) {
            case VT_I2:
                *(SHORT*)pvRet = V_I2(&vaResult);
                break;
            case VT_I4:
                *(LONG*)pvRet = V_I4(&vaResult);
                break;
            case VT_R4:
                *(FLOAT*)pvRet = V_R4(&vaResult);
                break;
            case VT_R8:
                *(DOUBLE*)pvRet = V_R8(&vaResult);
                break;
            case VT_DATE:
                *(DATE*)pvRet = V_DATE(&vaResult);
                break;
            case VT_CY:
                *(CY*)pvRet = V_CY(&vaResult);
                break;
            case VT_BSTR:
                *(BSTR*)pvRet = V_BSTR(&vaResult);
                break;
            case VT_DISPATCH:
                *(IDispatch**)pvRet = V_DISPATCH(&vaResult);
                break;
            case VT_ERROR:
            case VT_HRESULT:
                *(SCODE*)pvRet = V_ERROR(&vaResult);
                break;
            case VT_BOOL:
                *(VARIANT_BOOL*)pvRet = V_BOOL(&vaResult);
                break;
            case VT_VARIANT:
                *(VARIANT*)pvRet = vaResult;
                break;
            case VT_UNKNOWN:
                *(IUnknown**)pvRet = V_UNKNOWN(&vaResult);
                break;
            case VT_DECIMAL:
                *(DECIMAL*)pvRet = V_DECIMAL(&vaResult);
                break;

            case VT_I1:
                *(CHAR*)pvRet = V_I1(&vaResult);
                break;
            case VT_UI1:
                *(BYTE*)pvRet = V_UI1(&vaResult);
                break;
            case VT_UI2:
                *(USHORT*)pvRet = V_UI2(&vaResult);
                break;
            case VT_UI4:
                *(ULONG*)pvRet = V_UI4(&vaResult);
                break;
            case VT_INT:
                *(INT*)pvRet = V_INT(&vaResult);
                break;
            case VT_UINT:
                *(UINT*)pvRet = V_UINT(&vaResult);
                break;
            case VT_I8:
                *(LONGLONG*)pvRet = V_I8(&vaResult);
                break;
            case VT_UI8:
                *(ULONGLONG*)pvRet = V_UI8(&vaResult);
                break;
            case VT_RECORD:
            case VT_RECORD|VT_BYREF:
                *(PVOID*)pvRet = V_RECORD(&vaResult);
                break;

            case VT_I2|VT_BYREF:
                *(SHORT**)pvRet = V_I2REF(&vaResult);
                break;
            case VT_I4|VT_BYREF:
                *(LONG**)pvRet = V_I4REF(&vaResult);
                break;
            case VT_R4|VT_BYREF:
                *(FLOAT**)pvRet = V_R4REF(&vaResult);
                break;
            case VT_R8|VT_BYREF:
                *(DOUBLE**)pvRet = V_R8REF(&vaResult);
                break;
            case VT_DATE|VT_BYREF:
                *(DATE**)pvRet = V_DATEREF(&vaResult);
                break;
            case VT_CY|VT_BYREF:
                *(CY**)pvRet = V_CYREF(&vaResult);
                break;
            case VT_BSTR|VT_BYREF:
                *(BSTR**)pvRet = V_BSTRREF(&vaResult);
                break;
            case VT_DISPATCH|VT_BYREF:
                *(IDispatch***)pvRet = V_DISPATCHREF(&vaResult);
                break;
            case VT_ERROR|VT_BYREF:
            case VT_HRESULT|VT_BYREF:
                *(SCODE**)pvRet = V_ERRORREF(&vaResult);
                break;
            case VT_BOOL|VT_BYREF:
                *(VARIANT_BOOL**)pvRet = V_BOOLREF(&vaResult);
                break;
            case VT_VARIANT|VT_BYREF:
                *(VARIANT**)pvRet = V_VARIANTREF(&vaResult);
                break;
            case VT_UNKNOWN|VT_BYREF:
                *(IUnknown***)pvRet = V_UNKNOWNREF(&vaResult);
                break;
            case VT_DECIMAL|VT_BYREF:
                *(DECIMAL**)pvRet = V_DECIMALREF(&vaResult);
                break;
            
            case VT_I1|VT_BYREF:
                *(CHAR**)pvRet = V_I1REF(&vaResult);
                break;
            case VT_UI1|VT_BYREF:
                *(BYTE**)pvRet = V_UI1REF(&vaResult);
                break;
            case VT_UI2|VT_BYREF:
                *(USHORT**)pvRet = V_UI2REF(&vaResult);
                break;
            case VT_UI4|VT_BYREF:
                *(ULONG**)pvRet = V_UI4REF(&vaResult);
                break;
            case VT_INT|VT_BYREF:
                *(INT**)pvRet = V_INTREF(&vaResult);
                break;
            case VT_UINT|VT_BYREF:
                *(UINT**)pvRet = V_UINTREF(&vaResult);
                break;
            case VT_I8|VT_BYREF:
                *(LONGLONG**)pvRet = V_I8REF(&vaResult);
                break;
            case VT_UI8|VT_BYREF:
                *(ULONGLONG**)pvRet = V_UI8REF(&vaResult);
                break;

            default:
                if (vtRet & VT_ARRAY) {
                    if (vtRet & VT_BYREF) {
                        *(SAFEARRAY***)pvRet = V_ARRAYREF(&vaResult);
                        break;
                    } else {
                        *(SAFEARRAY**)pvRet = V_ARRAY(&vaResult);
                        break;
                    }
                }
                // invalid return type!
                VariantClear(&vaResult);
                return E_INVALIDARG;
            }
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT __cdecl
_com_dispatch_raw_method(IDispatch* pDispatch,
                         DISPID dwDispID,
                         WORD wFlags,
                         VARTYPE vtRet,
                         void* pvRet,
                         const wchar_t* pwParamInfo,
                         ...) 
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
        SetErrorInfo(0, perrinfo);
    }

    va_end(argList);
    return hr;
}

HRESULT __stdcall 
_com_dispatch_raw_propget(IDispatch* pDispatch,
                          DISPID dwDispID,
                          VARTYPE vtProp,
                          void* pvProp)
{
    return _com_dispatch_raw_method(pDispatch,
                                    dwDispID,
                                    DISPATCH_PROPERTYGET,
                                    vtProp,
                                    pvProp,
                                    NULL);
}

HRESULT __cdecl
_com_dispatch_raw_propput(IDispatch* pDispatch,
                          DISPID dwDispID,
                          VARTYPE vtProp,
                          ...) 
{
    va_list argList;
    va_start(argList, vtProp);
#ifdef _MAC
    argList -= 2;
#endif

    wchar_t rgwParams[2];
    rgwParams[0] = vtProp;
    rgwParams[1] = 0;

    WORD wFlags = (WORD) ((vtProp == VT_DISPATCH || vtProp == VT_UNKNOWN)
                    ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT);

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
        SetErrorInfo(0, perrinfo);
    }

    va_end(argList);
    return hr;
}

