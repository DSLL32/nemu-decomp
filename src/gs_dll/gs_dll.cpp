#include "stdafx.h"
#include "gs_dll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CGs_dllApp, CWinApp)
END_MESSAGE_MAP()

CGs_dllApp::CGs_dllApp()
{
}

CGs_dllApp theApp;

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return AfxDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
    return AfxDllCanUnloadNow();
}

STDAPI DllRegisterServer(void)
{
    COleObjectFactory::UpdateRegistryAll();
    return S_OK;
}
