// hub.cpp : Implementation of CBondHub
#include "pch.h"
#include "hub.h"
#include <string>
#include <map>
#include <memory>
#include <vector>
//#include <variant>
#include <curl/curl.h>
#include "json_parser.h"

using namespace std;

static bool IsObjectOfType(IPremiseObject* pObject, BSTR bstrClassName)
{
	VARIANT_BOOL bType = VARIANT_FALSE;
	HRESULT hr = pObject->IsOfType(bstrClassName, &bType);
	if (SUCCEEDED(hr) && bType)
		return true;
	return false;
}

static const string GetFanActionArgument(int percent)
{
	if (percent == 0)
		return "0";
	if (percent <= 33)
		return "1";
	if (percent <= 67)
		return "2";
	return "3";
}

static int FanSettingFromPercentage(double d)
{
	int nSetting = 0;
	if (d == 0.0)
		nSetting = 0;//off
	else if (d <= .33)
		nSetting = 33;//low
	else if (d <= .67)
		nSetting = 67;//medium
	else
		nSetting = 100;//high
	return nSetting;
}

////////////////////////////////////////////////////////////////////////////
// CBondHub

void CBondHub::Worker()
{
	while (1)
	{
		unique_lock<mutex> lock(m_mutex);
		m_cond.wait(lock, [this] {return !m_workItems.empty(); });
		while (!m_workItems.empty())
		{
			auto& f = m_workItems.front();
			// Release lock while running function.
			lock.unlock();
			// Note that reference to front is not invalidated when adding to back.
			f();
			lock.lock();
			m_workItems.pop_front();
		}
		if (m_stopThread)
			return;
	}
}

// This will put work on a worker queue to be processed by worker thread. Prevent hanging current thread.
void CBondHub::QueueWork(std::function<void(void)>&& fn)
{
	m_mutex.lock();
	m_workItems.emplace_back(std::move(fn));
	m_mutex.unlock();
	m_cond.notify_all();
}

HRESULT CBondHub::OnBrokerAttach()
{
	m_spSmartLighting.CoCreateInstance(CLSID_SmartLighting);
	if (m_spSmartLighting != NULL)
		m_spSmartLighting->SetSite(m_spSite);
	CComPtr<IPremiseObject> spNew;
	HRESULT hr = S_OK;

	CComPtr<IPremiseObject> spDriver;
	m_spSite->GetObject((BSTR)L"sys://Schema/Bond", &spDriver);
	if (spDriver != nullptr)
	{
		CComVariant varVersion;
		if (SUCCEEDED(spDriver->GetValue((BSTR)L"Version", &varVersion)))
			SetValueEx(m_spSite, SVCC_NOTIFY | SVCC_DRIVER, L"DriverVersion", varVersion);
	}

	/* This is here for testing, as it avoids me having to reeenter this info every time I try something. */
#if 0
	SetValue(m_spSite, "IPAddress", "192.168.1.30");
	SetValue(m_spSite, "BondToken", "be4cdfea4ca9033c");
#endif

	UpdateVersion();

	return hr;
}

HRESULT CBondHub::OnBrokerDetach()
{
	if (m_spSmartLighting != NULL)
		m_spSmartLighting->SetSite(NULL);
	m_spSmartLighting.Release();
	QueueWork([this]() { m_stopThread = true; });
	m_workerThread.join();

	return S_OK;
}


HRESULT CBondHub::GetDeviceID(IPremiseObject* pObject, string& str)
{
	return GetValue(pObject, L"DeviceID", str);
}

void CBondHub::SendFan(IPremiseObject* pObject, long percent)
{
	string id;
	HRESULT hr = GetDeviceID(pObject, id);
	if (FAILED(hr))
		return;

	// Post /v2/devices/{device_id}/actions/SetSpeed
	//{"argument":3}
	string action;
	string args;
	if (percent == 0)
	{
		action = "/v2/devices/" + id + "/actions/TurnOff";
		args = "{}";
	}
	else if (percent <= 33)
	{
		action = "/v2/devices/" + id + "/actions/SetSpeed";
		args = "{\"argument\":1}";
	}
	else if (percent <= 67)
	{
		action = "/v2/devices/" + id + "/actions/SetSpeed";
		args = "{\"argument\":2}";
	}
	else
	{
		action = "/v2/devices/" + id + "/actions/SetSpeed";
		args = "{\"argument\":3}";
	}

	QueueWork([= /* capture by value */]() { InvokeAction(action, args); });
}

void CBondHub::SendPower(IPremiseObject* pObject, bool state)
{
	string id;
	HRESULT hr = GetDeviceID(pObject, id);
	if (FAILED(hr))
		return;

	string action;
	string args = "{}";
	if (state)
		action = "/v2/devices/" + id + "/actions/TurnLightOn";
	else
		action = "/v2/devices/" + id + "/actions/TurnLightOff";

	QueueWork([= /* capture by value */]() { InvokeAction(action, args); });
}

HRESULT CBondHub::QueryStateOfDevices()
{
	HRESULT hr = S_OK;
	CComPtr<IPremiseObjectCollection> spCollection;
	FAILED_ASSERT_RETURN_HR(m_spSite->GetChildren(collectionTypeNoRecurse, &spCollection));
	LONG lCount = 0;

	spCollection->get_Count(&lCount);
	for (int i = 0; i < lCount; i++)
	{
		CComPtr<IPremiseObject> spItem;
		FAILED_ASSERT_RETURN_HR(spCollection->Item(CComVariant(i), &spItem));
		CComPtr<IPremiseObjectCollection> spProperties;
		if (IsObjectOfExplicitType(spItem, XML_Bond_CeilingFan))
		{
			string strID;
			HRESULT hr = GetDeviceID(spItem, strID);
			if (FAILED(hr))
				return hr;
			// TODO - Query state variables?
		}
	}
	return S_OK;
}

HRESULT CBondHub::OnObjectCreated(IPremiseObject* pContainer, IPremiseObject* pCreatedObject)
{
	if (IsObjectOfExplicitType(pCreatedObject, XML_Bond_CeilingFan))
	{
	}
	return S_OK;
}

HRESULT CBondHub::OnObjectDeleted(IPremiseObject* pContainer, IPremiseObject* pDeletedObject)
{
	return S_OK;
}

HRESULT CBondHub::OnFanSpeedSettingChanged(IPremiseObject* pObject, VARIANT newValue)
{
	int nVal = newValue.lVal;//0,25,50,75,100
	pObject->SetValueEx(SVCC_NOTIFY | SVCC_DRIVER, L"Speed", &CComVariant((double)nVal / 100.0));

	SendFan(pObject, nVal);
	return S_OK;
}

//percent 0.0 -- 100.0
HRESULT CBondHub::OnFanSpeedChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (newValue.vt != VT_R8)
		return E_INVALIDARG;

	int nSetting = FanSettingFromPercentage(newValue.dblVal);
	pObject->SetValueEx(SVCC_NOTIFY | SVCC_DRIVER, L"SpeedSetting", &CComVariant(nSetting));

	SendFan(pObject, nSetting);
	return S_OK;
}

HRESULT CBondHub::OnPowerStateChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (pObject == NULL)
		return E_POINTER;

	if (IsObjectOfExplicitType(pObject, XML_Bond_CeilingFan))
		SendPower(pObject, newValue.boolVal ? true : false);
	return S_OK;
}

HRESULT CBondHub::GetBaseUrl(string& url)
{
	url.clear();
	string strAddress;
	HRESULT hr = GetValue(m_spSite, L"IPAddress", strAddress);
	if (FAILED(hr))
		return hr;
	url = "http://" + strAddress;

	return S_OK;
}

HRESULT CBondHub::OnDiscoverTokenChanged(IPremiseObject* pObject, VARIANT newValue)
{
	ObjectLock lock(this);
	if (newValue.vt == VT_BOOL && newValue.boolVal == VARIANT_TRUE)
	{
		QueueWork([=]()
		{
			string output;
			object_t obj;
			InvokeCommand("/v2/token", output, obj, false /*use_token*/);
			auto iter = obj.values.find("token");
			if (iter != obj.values.end())
			{
				const auto& value = iter->second;
				SetValue(m_spSite, "BondToken", value.str);
			}
		});
	}
	return S_OK;
}

HRESULT CBondHub::OnAddressChanged(IPremiseObject* pObject, VARIANT newValue)
{
	UpdateVersion();
	return S_OK;
}

HRESULT CBondHub::SetDeviceInfo(CComPtr<IPremiseObject> spDevice, const object_t& obj)
{
	SetValue(spDevice, "BondName", obj.get_string("name"));
	SetValue(spDevice, "BondType", obj.get_string("type"));
	SetValue(spDevice, "Location", obj.get_string("location"));
	SetValue(spDevice, "Template", obj.get_string("template"));
	return S_OK;
}

HRESULT CBondHub::CreateDevice(const string& id)
{
	CComPtr<IPremiseObject> spNew;

	// Get information about device.
	string output;
	object_t obj;
	string command = "/v2/devices/" + id;
	InvokeCommand(command.c_str(), output, obj, true /*use_token*/);
	auto& type = obj.get_string("type");
	auto& name = obj.get_string("name");
	CComBSTR bstrName(name.c_str());

	if (type == "CF") // ceiling fan
		m_spSite->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_CeilingFan, bstrName, &spNew);
	else if (type == "FP") // fireplace
		m_spSite->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_Fireplace, bstrName, &spNew);
	else if (type == "MS") // motorized window coverings
		m_spSite->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_Shade, bstrName, &spNew);
	else if (type == "GX") // Generic Device
		m_spSite->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_GenericDevice, bstrName, &spNew);
	else if (type == "LT") // Light
		m_spSite->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_Light, bstrName, &spNew);
	else if (type == "BD") // Bidet
		m_spSite->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_Bidet, bstrName, &spNew);
	else
		m_spSite->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_Device, bstrName, &spNew);
	if (spNew)
	{
		SetValue(spNew, L"DisplayName", CComVariant(bstrName));
		SetValue(spNew, "DeviceID", id);
		SetDeviceInfo(spNew, obj);
		DiscoverActions(spNew);
	}

	return S_OK;
}

HRESULT CBondHub::CreateAction(IPremiseObject* p, const string& name)
{
	CComPtr<IPremiseObject> spNew;

	CComBSTR bstrName(name.c_str());
	p->CreateEx(SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Bond_Action, bstrName, &spNew);

	if (spNew)
	{
		// TODO - Consider automatically providing default arguments for actions that need them.
	}

	return S_OK;
}

HRESULT CBondHub::OnDiscoverDevicesChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (newValue.vt == VT_BOOL && newValue.boolVal == VARIANT_TRUE)
	{
		QueueWork([=]()
		{
			string output;
			object_t obj;
			InvokeCommand("/v2/devices", output, obj, true /*use_token*/);
			for (auto& item : obj.values)
			{
				ObjectLock lock(this);
				if (item.first != "_")
					CreateDevice(item.first);
			}
		});
	}
	return S_OK;
}

void CBondHub::DiscoverActions(IPremiseObject* pObject)
{
	string strID;
	if (SUCCEEDED(GetDeviceID(pObject, strID)))
	{
		string strCommand = "/v2/devices/" + strID + "/actions";
		CComPtr<IPremiseObject> spObject{ pObject };
		QueueWork([=]()
		{
			string output;
			object_t obj;
			InvokeCommand(strCommand.c_str(), output, obj, true /*use_token*/);
			for (auto& item : obj.values)
			{
				ObjectLock lock(this);
				if (item.first != "_")
					CreateAction(spObject, item.first);
			}
		});
	}
}

HRESULT CBondHub::OnDiscoverActions(IPremiseObject* pObject, VARIANT newValue)
{
	if (newValue.vt == VT_BOOL && newValue.boolVal == VARIANT_TRUE)
		DiscoverActions(pObject);
	return S_OK;
}

HRESULT CBondHub::OnTriggerAction(IPremiseObject* pObject, VARIANT newValue)
{
	HRESULT hr = E_FAIL;
	if (newValue.vt != VT_BOOL)
		return hr;
	if (newValue.boolVal != VARIANT_TRUE)
		return S_OK;

	// pObject is the action. Device is the parent.
	CComPtr<IPremiseObject> spParent;
	pObject->get_Parent(&spParent);
	string strID;
	hr = GetDeviceID(spParent, strID);
	if (FAILED(hr))
		return hr;

	string strAction;
	hr = GetValue(pObject, L"Name", strAction);
	if (FAILED(hr))
		return hr;

	string strArgument;
	hr = GetValue(pObject, L"Argument", strArgument);
	if (FAILED(hr))
		return hr;

	string strCommand = "/v2/devices/" + strID + "/actions/" + strAction;

	string strArgs;
	if (!strArgument.empty()) // If not empty, create arguments.
		strArgs = "{\"argument\":" + strArgument + "}";
	else
		strArgs = "{}";

	QueueWork([=]()
	{
		string output;
		object_t obj;
		InvokeAction(strCommand, strArgs);
	});

	return S_OK;
}

void CBondHub::UpdateVersion()
{
	QueueWork([=]()
	{
		string output;
		object_t obj;
		InvokeCommand("/v2/sys/version", output, obj, false /*use_token*/);
		auto iter = obj.values.find("fw_ver");
		if (iter != obj.values.end())
			SetValue(m_spSite, "Firmware", iter->second.str);
	});
}

// Adds the Bond token to the CURL header.
void CBondHub::AddTokenToHeader(CURL* curl)
{
	string strToken;
	if (SUCCEEDED(GetValue(m_spSite, L"BondToken", strToken)))
	{
		curl_slist* list = nullptr;
		string header = "BOND-Token: " + strToken;
		list = curl_slist_append(list, header.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	}
}

void CBondHub::InvokeCommand(const char* command, string& output, object_t& obj, bool use_token)
{
	// Make sure we are on worker thread.
	_ASSERTE(std::this_thread::get_id() == m_workerThread.get_id());

	output.clear();
	string url;
	HRESULT hr = GetBaseUrl(url);
	if (FAILED(hr))
		return;

	CURL* curl = curl_easy_init();
	if (curl)
	{
		url += command;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		if (use_token)
			AddTokenToHeader(curl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
		/* Note the plus in front of the lambda causes decay to regular function pointer. */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
			+[](void* ptr, size_t size, size_t nmemb, string* s) -> size_t
		{
			s->append((char*)ptr, size * nmemb);
			return size * nmemb;
		});

		auto res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if (res == CURLE_OK)
			simple_json_parser p(output, obj);
	}
	SetValue(m_spSite, "LastResponse", output);
}

void CBondHub::InvokeAction(const string& action, const string& payload)
{
	// Make sure we are on worker thread.
	_ASSERTE(std::this_thread::get_id() == m_workerThread.get_id());

	string output;
	object_t obj;

	string url;
	HRESULT hr = GetBaseUrl(url);
	if (FAILED(hr))
		return;

	CURL* curl = curl_easy_init();
	if (curl)
	{
		url += action;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		AddTokenToHeader(curl);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
		/* Note the plus in front of the lambda causes decay to regular function pointer. */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
			+[](void* ptr, size_t size, size_t nmemb, string* s) -> size_t
		{
			s->append((char*)ptr, size * nmemb);
			return size * nmemb;
		});

		/* now specify data to pass to our callback */
		if (!payload.empty())
		{
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
			/* Note the plus in front of the lambda causes decay to regular function pointer. */
			curl_easy_setopt(curl, CURLOPT_READFUNCTION,
				+[](char* buffer, size_t size, size_t nitems, void* userdata) -> size_t
			{
				const string& str = *(reinterpret_cast<const string*>(userdata));
				memcpy(buffer, str.data(), str.size());
				return str.size();
			});
			curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
			/* Set the size of the data to upload */
			curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)payload.size());
		}

		auto res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		if ((res == CURLE_OK) && !output.empty())
			simple_json_parser p(output, obj);
	}
	SetValue(m_spSite, "LastResponse", output);
}
