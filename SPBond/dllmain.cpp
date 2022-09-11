#include "pch.h"
#include "bond.h"


CComModule _Module;
// {AF1A0E43-1209-44FC-A0C6-4783DADFDD15}
const CLSID CLSID_Bond = { 0xAF1A0E43, 0x1209, 0x44FC, {0xA0, 0xC6, 0x47, 0x83, 0xDA, 0xDF, 0xDD, 0x15} };
BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_Bond, CBond)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance, nullptr);
		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}
