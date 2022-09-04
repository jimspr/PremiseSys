#pragma once

// Some helpers to avoid errors with passing literals as non-const and and temporary CComVariants as non-const.

inline HRESULT SetValue(IPremiseObject* p, LPCOLESTR name, const CComVariant& var)
{
	return p->SetValue((BSTR)name, const_cast<CComVariant*>(&var));
}

inline HRESULT SetValueEx(IPremiseObject* p, long code, LPCOLESTR name, const CComVariant& var)
{
	return p->SetValueEx(code, (BSTR)name, const_cast<CComVariant*>(&var));
}

// These functions are not implemented. If you get a link error about them being missing, change your callsite to not take the address.
// What happens without these functions is that the CComVariant* will be placed inside a new CComVariant, which will then be passed by reference
// to one of the  functions above. This will do the WRONG thing and siliently break.
HRESULT SetValue(IPremiseObject* p, LPCOLESTR name, const CComVariant* var);
HRESULT SetValueEx(IPremiseObject* p, long code, LPCOLESTR name, const CComVariant* var);
