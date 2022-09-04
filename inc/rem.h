// REM.h : Declaration of the CREM

#pragma once

#include <sys_helpers.h>

#define XML_REMOTECOM L"sys://Schema/Networks/RemoteCom"

/////////////////////////////////////////////////////////////////////////////
// CREM

//this template class handles multiport or single port device servers
//the template argument is the number of ports

template <int number_of_ports, long default_ip_port>
class ATL_NO_VTABLE CREM : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CREM<number_of_ports, default_ip_port>>,
	public CPremiseSubscriber
{
public:
	CComPtr<IObjectWithSite> m_arrRemPorts[number_of_ports];
	CREM()
	{
	}

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CREM)

BEGIN_COM_MAP(CREM)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPremiseNotify)
END_COM_MAP()

BEGIN_NOTIFY_MAP(CREM) 
	NOTIFY_PROPERTY(L"IPAddress", OnAddressChanged) 
	NOTIFY_PROPERTY(L"IPPort", OnPortChanged) 
END_NOTIFY_MAP() 

	HRESULT STDMETHODCALLTYPE OnAddressChanged(IPremiseObject *pObject, VARIANT newValue)
	{
		if (m_spSite != pObject)
			return S_OK;
		//if (m_spSite->QueryIdentity(pObject) != S_OK) //for the child port
		//	return S_OK;
		//if (ocslen(newValue.bstrVal) != 0) //not empty
		//	pObject->SetValue(L"MAC", &CComVariant(L"")); //clear out MAC when IP address changes
		CComBSTR bstr = L"http://";
		bstr.Append(newValue.bstrVal);
		SetValue(pObject, L"_HTTPAddress", CComVariant(bstr));
		for (int i=0;i< number_of_ports;i++)
		{
			CComPtr<IPremiseObject> sp;
			m_spSite->GetObjectByIndex(NULL, i, &sp);
			if (sp != NULL)
				SetValue(sp, L"IPAddress", newValue);
		}
		return S_OK;
	}
	
	HRESULT STDMETHODCALLTYPE OnPortChanged(IPremiseObject *pObject, VARIANT newValue)
	{
		if (m_spSite != pObject)
			return S_OK;
		//if (m_spSite->QueryIdentity(pObject) != S_OK) //for the child port
		//	return S_OK;
		if (newValue.vt != VT_I4)
			return E_FAIL;
		long nPort = newValue.lVal;
		for (int i=0;i< number_of_ports;i++)
		{
			CComPtr<IPremiseObject> sp;
			m_spSite->GetObjectByIndex(NULL, i, &sp);
			if (sp != NULL)
				SetValue(sp, L"IPPort", CComVariant(nPort+i));
		}
		return S_OK;
	}

	HRESULT OnBrokerAttach()
	{
		CComVariant varPort;
		m_spSite->GetValue((BSTR)L"IPPort", &varPort);
		long nPort = varPort.lVal;
		if (nPort == 0)
		{
			nPort = default_ip_port;
			SetValueEx(m_spSite, SVCC_NOTIFY|SVCC_DRIVER, L"IPPort", CComVariant(nPort));
		}

		CComVariant varAddress;
		m_spSite->GetValue((BSTR)L"IPAddress", &varAddress);

		for (int i=0;i< number_of_ports;i++)
		{
			OLECHAR buf[32];
			wsprintfW(buf, L"Port%d", i+1);

			CComPtr<IPremiseObject> spNew;
			HRESULT hr = m_spSite->CreateEx(SVCC_FIXED | SVCC_EXIST | SVCC_NOTIFY, (BSTR)XML_REMOTECOM, buf, &spNew);
			if (FAILED(hr))
				return hr;

			hr = m_arrRemPorts[i].CoCreateInstance(CLSID_RemoteSerialPort);
			if (FAILED(hr))
				return hr;

			m_arrRemPorts[i]->SetSite(spNew);
//			spNew->SetFlags(SVF_USER_NOCHANGE_NAME, 0); //allow name change
			SetValue(spNew, L"IPPort", CComVariant(nPort+i));
			SetValue(spNew, L"IPAddress", varAddress);
		}
		return S_OK;
	}

	HRESULT OnBrokerDetach()
	{
		for (int i=0;i< number_of_ports;i++)
		{
			m_arrRemPorts[i]->SetSite(NULL);
			m_arrRemPorts[i].Release();
		}
		return S_OK;
	}
};
