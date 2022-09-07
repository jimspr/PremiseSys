// Bond.h : Declaration of the CBondHub

#pragma once

#include <syshelp.h>
#include <string>
#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <functional>
#include <deque>

#define XML_Bond_Device			L"sys://Schema/Bond/Device"
#define XML_Bond_CeilingFan		L"sys://Schema/Bond/CeilingFan"
#define XML_Bond_Hub			L"sys://Schema/Bond/BondHub"
#define XML_Bond_Fireplace		L"sys://Schema/Bond/Fireplace"
#define XML_Bond_Shade			L"sys://Schema/Bond/Shade"
#define XML_Bond_GenericDevice	L"sys://Schema/Bond/GenericDevice"
#define XML_Bond_Light			L"sys://Schema/Bond/Light"
#define XML_Bond_Bidet			L"sys://Schema/Bond/Bidet"
#define XML_Bond_Action			L"sys://Schema/Bond/Action"

struct object_t;

/////////////////////////////////////////////////////////////////////////////
// CBondHub
class ATL_NO_VTABLE CBondHub :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CBondHub>,
	public CPremiseSubscriber
{
public:
	DECLARE_NO_REGISTRY()
	DECLARE_NOT_AGGREGATABLE(CBondHub)

BEGIN_COM_MAP(CBondHub)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPremiseNotify)
END_COM_MAP()

	// Smart lighting may not be needed. Not sure how bond lights work yet.
	CComPtr<IObjectWithSite> m_spSmartLighting;

	// Worker thread and queue.
	std::thread m_workerThread;
	std::mutex m_mutex;
	std::condition_variable m_cond;
	std::deque<std::function<void(void)>> m_workItems;
	std::atomic<bool> m_stopThread = false;
	void Worker();
	void QueueWork(std::function<void(void)>&&);

	CBondHub() : m_workerThread(&CBondHub::Worker, this)
	{
	}

	STDMETHOD(OnObjectCreated)(IPremiseObject* pContainer, IPremiseObject* pCreatedObject) override;
	STDMETHOD(OnObjectDeleted)(IPremiseObject* pContainer, IPremiseObject* pDeletedObject) override;

BEGIN_NOTIFY_MAP(CBondHub)
	NOTIFY_PROPERTY(L"DiscoverToken", OnDiscoverTokenChanged)
	NOTIFY_PROPERTY(L"DiscoverDevices", OnDiscoverDevicesChanged)
	NOTIFY_PROPERTY(L"IPAddress", OnAddressChanged)
	NOTIFY_PROPERTY(L"SpeedSetting", OnFanSpeedSettingChanged)
	NOTIFY_PROPERTY(L"Speed", OnFanSpeedChanged)
	NOTIFY_PROPERTY(L"PowerState", OnPowerStateChanged)
	NOTIFY_PROPERTY(L"DiscoverActions", OnDiscoverActions)
	NOTIFY_PROPERTY(L"Trigger", OnTriggerAction)
END_NOTIFY_MAP()

	HRESULT OnDiscoverTokenChanged(IPremiseObject* pObject, VARIANT newValue);
	HRESULT OnDiscoverDevicesChanged(IPremiseObject* pObject, VARIANT newValue);
	HRESULT OnAddressChanged(IPremiseObject* pObject, VARIANT newValue);
	HRESULT OnFanSpeedSettingChanged(IPremiseObject* pObject, VARIANT newValue);
	HRESULT OnFanSpeedChanged(IPremiseObject* pObject, VARIANT newValue);
	HRESULT OnPowerStateChanged(IPremiseObject* pObject, VARIANT newValue);
	HRESULT OnDiscoverActions(IPremiseObject* pObject, VARIANT newValue);
	HRESULT OnTriggerAction(IPremiseObject* pObject, VARIANT newValue);

	HRESULT GetDeviceID(IPremiseObject *pObject, std::string& str);
	HRESULT OnBrokerAttach() override;
	HRESULT OnBrokerDetach() override;
	HRESULT GetBaseUrl(std::string& url);
	HRESULT CreateDevice(const std::string& id);
	HRESULT CreateAction(IPremiseObject* p, const std::string& name);
	HRESULT SetDeviceInfo(CComPtr<IPremiseObject> spDevice, const object_t& obj);
	HRESULT QueryStateOfDevices();

	void UpdateVersion();
	void SetValueA(const wchar_t* name, const char* value);
	void InvokeCommand(const char* command, std::string& output, object_t& obj, bool use_token);
	void InvokeAction(const std::string& action, const std::string& payload);
	void SendFan(IPremiseObject* pObject, long nLevel);
	void SendPower(IPremiseObject* pObject, bool state);
	void AddTokenToHeader(void* curl);
};
