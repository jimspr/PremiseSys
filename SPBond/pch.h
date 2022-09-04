// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

#define STRICT
#define _ATL_APARTMENT_THREADED
#define _CRT_SECURE_NO_WARNINGS

#pragma warning (disable:6255) // _alloca warning

#include <windows.h>
#include <WinNls.h>
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <driverutil.h>
#include <sys_helpers.h>

#endif //PCH_H
