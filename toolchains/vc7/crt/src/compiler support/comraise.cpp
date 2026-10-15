/***
* comraise.cpp - Throw a com_error object.  In a separate file so users can easily define
*                their own to replace this one.
*
*   Copyright (C) 1996-2001 Microsoft Corporation
*   All rights reserved.
*
****/

#include <comdef.h>

#pragma hdrstop

#pragma warning(disable:4290)

void __stdcall
_com_raise_error(HRESULT hr, IErrorInfo* perrinfo) throw(_com_error)
{
    throw _com_error(hr, perrinfo);
}
