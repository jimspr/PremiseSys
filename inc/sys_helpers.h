#pragma once
#include <string>

// Some helpers to avoid errors with passing literals as non-const and and temporary CComVariants as non-const.
inline HRESULT SetValue(IPremiseObject* p, const char* name, const char* value)
{
	USES_CONVERSION;
	CComBSTR bstrName{ name };
	CComVariant varOutput(A2COLE(value));
	return p->SetValue(bstrName, &varOutput);
}

inline HRESULT SetValue(IPremiseObject* p, const char* name, const std::string& value)
{
	USES_CONVERSION;
	CComBSTR bstrName{ name };
	CComVariant varOutput(A2COLE(value.c_str()));
	return p->SetValue(bstrName, &varOutput);
}

inline HRESULT SetValue(IPremiseObject* p, const char* name, bool value)
{
	USES_CONVERSION;
	CComBSTR bstrName{ name };
	CComVariant varOutput(value);
	return p->SetValue(bstrName, &varOutput);
}

inline HRESULT SetValue(IPremiseObject* p, LPCOLESTR name, const CComVariant& var)
{
	return p->SetValue((BSTR)name, const_cast<CComVariant*>(&var));
}

inline HRESULT SetValueEx(IPremiseObject* p, long code, LPCOLESTR name, const CComVariant& var)
{
	return p->SetValueEx(code, (BSTR)name, const_cast<CComVariant*>(&var));
}

inline HRESULT GetValue(IPremiseObject* p, LPCOLESTR name, std::string& str)
{
	USES_CONVERSION;
	str.clear();
	CComVariant var;
	HRESULT hr = p->GetValue((BSTR)name, &var);
	if (FAILED(hr))
		return hr;
	if (var.vt != VT_BSTR)
		return E_INVALIDARG;
	str = OLE2CA(var.bstrVal);
	return S_OK;
}

inline HRESULT GetValue(IPremiseObject* p, LPCOLESTR name, long& l)
{
	USES_CONVERSION;
	l = 0;
	CComVariant var;
	HRESULT hr = p->GetValue((BSTR)name, &var);
	if (FAILED(hr))
		return hr;
	hr = var.ChangeType(VT_I4);
	if (FAILED(hr))
		return hr;
	l = var.lVal;
	return S_OK;
}

inline HRESULT GetValue(IPremiseObject* p, LPCOLESTR name, double& d)
{
	USES_CONVERSION;
	d = 0;
	CComVariant var;
	HRESULT hr = p->GetValue((BSTR)name, &var);
	if (FAILED(hr))
		return hr;
	hr = var.ChangeType(VT_R8);
	if (FAILED(hr))
		return hr;
	d = var.dblVal;
	return S_OK;
}

inline HRESULT GetValue(IPremiseObject* p, const char* name, std::string& str)
{
	USES_CONVERSION;
	str.clear();
	CComVariant var;
	CComBSTR bstr(name);
	HRESULT hr = p->GetValue(bstr, &var);
	if (FAILED(hr))
		return hr;
	if (var.vt != VT_BSTR)
		return E_INVALIDARG;
	str = OLE2CA(var.bstrVal);
	return S_OK;
}

// These functions are not implemented. If you get a link error about them being missing, change your callsite to not take the address.
// What happens without these functions is that the CComVariant* will be placed inside a new CComVariant, which will then be passed by reference
// to one of the  functions above. This will do the WRONG thing and siliently break.
HRESULT SetValue(IPremiseObject* p, LPCOLESTR name, const CComVariant* var);
HRESULT SetValueEx(IPremiseObject* p, long code, LPCOLESTR name, const CComVariant* var);
