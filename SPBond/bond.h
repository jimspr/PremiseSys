#pragma once

#include "hub.h"

extern const CLSID CLSID_Bond;

/////////////////////////////////////////////////////////////////////////////
// CBond
class ATL_NO_VTABLE CBond :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CBond, &CLSID_Bond>,
	public CPremiseDriverImpl
{
public:
	CBond()
	{
	}

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CBond)

BEGIN_COM_MAP(CBond)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPremiseNotify)
END_COM_MAP()

BEGIN_NOTIFY_MAP(CBond)
END_NOTIFY_MAP() 

	HRESULT CreateControllerForSite(IPremiseObject* pObject, IObjectWithSite** ppSite, bool bFirstTime)
	{
		if (IsObjectOfExplicitType(pObject, XML_Bond_Hub))
			return CBondHub::CreateInstance(ppSite);

		return E_FAIL;
	}
};
