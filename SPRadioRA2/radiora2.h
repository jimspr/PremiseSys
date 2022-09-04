#pragma once
#include "repeater.h"
#include "rem.h"

class ATL_NO_VTABLE CTelnetPort : public CREM<1, 23>
{
public:
};

extern const CLSID CLSID_RadioRA2;

/////////////////////////////////////////////////////////////////////////////
// CRadioRA2
class ATL_NO_VTABLE CRadioRA2 : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRadioRA2, &CLSID_RadioRA2>,
	public CPremiseDriverImpl
{
public:
	CRadioRA2()
	{
	}

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CRadioRA2)

BEGIN_COM_MAP(CRadioRA2)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPremiseNotify)
END_COM_MAP()

BEGIN_NOTIFY_MAP(CRadioRA2) 
END_NOTIFY_MAP() 

	HRESULT CreateControllerForSite(IPremiseObject* pObject, IObjectWithSite** ppSite, bool bFirstTime)
	{
		if (IsObjectOfExplicitType(pObject, XML_RadioRA2_Repeater))
			return CMainRepeater::CreateInstance(ppSite);
		if (IsObjectOfExplicitType(pObject, XML_RadioRA2_Port))
			return CTelnetPort::CreateInstance(ppSite);

		return E_FAIL;
	}
};
