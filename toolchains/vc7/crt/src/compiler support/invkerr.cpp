/***
* invkerr.cpp - Native C++ compiler COM support - EXCEPINFO and IErrorInfo handler
*
*   Copyright (C) 1996-2001 Microsoft Corporation
*   All rights reserved.
*
****/
#include <comdef.h>
#pragma hdrstop

#include "invkprxy.h"

/////////////////////////////////////////////////////////////////////////////

HRESULT __stdcall _com_handle_excepinfo(EXCEPINFO& excepInfo, IErrorInfo** pperrinfo) {
    // make sure excepInfo is filled in
    if (excepInfo.pfnDeferredFillIn != NULL) {
        excepInfo.pfnDeferredFillIn(&excepInfo);
    }

    // allocate new error info, and fill it
    ICreateErrorInfo *pcerrinfo = NULL;
    if (SUCCEEDED(CreateErrorInfo(&pcerrinfo))) {
        // Set up ErrInfo object
        pcerrinfo->SetGUID(__uuidof(IDispatch));
        if (excepInfo.bstrSource != NULL) {
            pcerrinfo->SetSource(excepInfo.bstrSource);
        }
        if (excepInfo.bstrDescription != NULL) {
            pcerrinfo->SetDescription(excepInfo.bstrDescription);
        }
        if (excepInfo.bstrHelpFile != NULL) {
            pcerrinfo->SetHelpFile(excepInfo.bstrHelpFile);
        }
        pcerrinfo->SetHelpContext(excepInfo.dwHelpContext);

        if (FAILED(pcerrinfo->QueryInterface(__uuidof(IErrorInfo),
                                             (void**)pperrinfo))) {
            *pperrinfo = NULL;
        }
        pcerrinfo->Release();
    }

    if (excepInfo.bstrSource != NULL) {
        SysFreeString(excepInfo.bstrSource);
    }
    if (excepInfo.bstrDescription != NULL) {
        SysFreeString(excepInfo.bstrDescription);
    }
    if (excepInfo.bstrHelpFile != NULL) {
        SysFreeString(excepInfo.bstrHelpFile);
    }

    HRESULT hr;
    if (excepInfo.wCode != 0) {
        hr = _com_error::WCodeToHRESULT(excepInfo.wCode);
    } else {
        hr = excepInfo.scode;
    }
    return hr;
}
/////////////////////////////////////////////////////////////////////////////

