// RadioRA.cpp : Implementation of CRadioRA
#include "pch.h"
#include "repeater.h"

static const int LedKeypadButtonOffset = 80;

#define XML_RadioRA2_Port L"sys://Schema/RadioRA2/TelnetPort"

struct CRadioRA2Button
{
	LPCWSTR pszName;
	int nID;
	bool simple;
};

CRadioRA2Button keypadButtons[11] =
{
	{L"Button1", 1, false},
	{L"Button2", 2, false},
	{L"TopLower", 16, true}, // some keypads support two sets of lower/raise buttons
	{L"TopRaise", 17, true},
	{L"Button3", 3, false},
	{L"Button4", 4, false},
	{L"Button5", 5, false},
	{L"Button6", 6, false},
	{L"Button7", 7, false},
	{L"Lower", 18, true},
	{L"Raise", 19, true},
};

// keypadInfo describes the different types of the seeTouch in-wall keypads
// The 1/0 in the array below determines what buttons from keypadButtons above
// is applied when generating a keypad
bool keypadInfo[][11] =
{
	//			1	2	ru	rl	3	4	5	6	7	r	l
	/*1RLD*/	{	1,	1,	0,	0,	1,	0,	1,	1,	0,	1,	1},
	/*2RLD*/	{	1,	1,	1,	1,	0,	0,	1,	1,	0,	1,	1},
	/*3BD*/		{	1,	1,	0,	0,	1,	0,	1,	1,	1,	0,	0},
	/*3BRL*/	{	0,	1,	0,	0,	1,	1,	0,	0,	0,	1,	1},
	/*3BSRL*/	{	1,	0,	0,	0,	1,	0,	1,	0,	0,	1,	1},
	/*4S*/		{	1,	1,	0,	0,	1,	1,	0,	1,	0,	1,	1},
	/*5BRL*/	{	1,	1,	0,	0,	1,	1,	1,	0,	0,	1,	1},
	/*6BRL*/	{	1,	1,	0,	0,	1,	1,	1,	1,	0,	1,	1},
	/*7B*/		{	1,	1,	0,	0,	1,	1,	1,	1,	1,	0,	0},
};

CRadioRA2Button allonoff_buttons_[2] =
{
	{L"AllOff",		16, true},
	{L"AllOn",		17, true},
};

CRadioRA2Button lower_raise_buttons_[2] =
{
	{L"Lower",		24, true},
	{L"Raise",		25, true},
};

typedef void (CMainRepeater::* MFP)(LPCSTR psz);

// This table defines what to do with data coming from the repeater
JumpTableEntry<MFP> arrJT[] =
{
	{"OS Firmware Revision = ", &CMainRepeater::UpdateFirmware},
	{"~OUTPUT,", &CMainRepeater::UpdateOutput},
	{"~DEVICE,", &CMainRepeater::UpdateDevice},
	{"~SYSTEM,", &CMainRepeater::UpdateSystem},
	{"~GROUP,", &CMainRepeater::UpdateGroup},
	{NULL, NULL}
};

static bool IsObjectOfType(IPremiseObject* pObject, BSTR bstrClassName)
{
	VARIANT_BOOL bType = VARIANT_FALSE;
	HRESULT hr = pObject->IsOfType(bstrClassName, &bType);
	if (SUCCEEDED(hr) && bType)
		return true;
	return false;
}

static int FanSettingFromPercentage(double d)
{
	int nSetting = 0;
	if (d == 0.0)
		nSetting = 0;//OFF
	else if (d <= .25)
		nSetting = 25;//LOW
	else if (d <= .50)
		nSetting = 50;//medium
	else if (d <= .75)
		nSetting = 75;//mediumhigh
	else
		nSetting = 100;//high
	return nSetting;
}

static int ClampFanSetting(int d)
{
	int nSetting = 0;
	if (d == 0)
		nSetting = 0;//OFF
	else if (d <= 25)
		nSetting = 25;//LOW
	else if (d <= 50)
		nSetting = 50;//medium
	else if (d <= 75)
		nSetting = 75;//mediumhigh
	else
		nSetting = 100;//high
	return nSetting;
}

////////////////////////////////////////////////////////////////////////////
// CMainRepeater

bool CMainRepeater::OnPing()
{
	if (m_fReady)
		QuerySystem(RA2S_OSRev);
	return true;
}
HRESULT CMainRepeater::OnBrokerDetach()
{
	if (m_spSmartLighting != NULL)
		m_spSmartLighting->SetSite(NULL);
	m_spSmartLighting.Release();
	return S_OK;
}

HRESULT CMainRepeater::OnBrokerAttach()
{
	m_spSmartLighting.CoCreateInstance(CLSID_SmartLighting);
	if (m_spSmartLighting != NULL)
		m_spSmartLighting->SetSite(m_spSite);
	CComPtr<IPremiseObject> spNew;
	HRESULT hr = S_OK;

	CComPtr<IPremiseObject> spDriver;
	m_spSite->GetObject((BSTR)L"sys://Schema/RadioRA2", &spDriver);
	if (spDriver != nullptr)
	{
		CComVariant varVersion;
		if (SUCCEEDED(spDriver->GetValue((BSTR)L"Version", &varVersion)))
		{
			SetValueEx(m_spSite, SVCC_NOTIFY | SVCC_DRIVER, L"DriverVersion", varVersion);
		}
	}
	m_spSite->TransactionOpen(SVCC_NOTIFY, SVT_CREATE);
	do
	{
		hr = m_spSite->CreateEx(SVCC_FIXED | SVCC_EXIST | SVCC_NOTIFY,
			(BSTR)XML_RadioRA2_PhantomButtons, (BSTR)L"PhantomButtons", &spNew);
		if (FAILED(hr))
			break;
		// add phantom buttons
		CreatePhantomButtons(spNew);

	} while (0);
	m_spSite->TransactionCommit();

	return hr;
}

HRESULT CMainRepeater::OnConfigurePort(IPremiseObject* pPort)
{
	SetValue(pPort, L"Baud", CComVariant(SERIAL_9600)); //9600 baud
	SetValue(pPort, L"DataBits", CComVariant(SERIAL_DATABITS_8));
	SetValue(pPort, L"Parity", CComVariant(SERIAL_PARITY_NONE));
	SetValue(pPort, L"StopBits", CComVariant(SERIAL_STOPBITS_1));
	SetValue(pPort, L"FlowControl", CComVariant(SERIAL_FLOWCONTROL_NONE));
	SetValue(pPort, L"RTS", CComVariant(SERIAL_RTS_ENABLE));
	SetValue(pPort, L"DTR", CComVariant(SERIAL_DTR_ENABLE));
	SetValue(pPort, L"CTS", CComVariant(false));
	SetValue(pPort, L"DSR", CComVariant(false));
	return S_OK;
}

HRESULT CMainRepeater::OnDeviceState(DEVICE_STATE ps)
{
	switch (ps)
	{
	case PORT_OPENING:
		PrepareToOpen();
		break;
	case DEVICE_INIT:
		SetValue(m_spSite, L"Firmware", CComVariant(""));
		if (m_networkMode)
		{
			m_fReady = false; // wait for "login:" prompt
		}
		else
		{
			m_fReady = true;
			QuerySystem(RA2S_OSRev);
			QueryStateOfDevices();
		}
		//			QueryMonitoring(AllMonitoring);
		break;
	case PORT_OPENED:
		break;
	}
	return S_OK;
}

void CMainRepeater::ProcessLine(LPCSTR psz)
{
	if (m_spSite != NULL)
	{
		MFP fp = LookupJumpPartialMatch(psz, arrJT);
		if (fp != NULL)
			(this->*fp)(psz);
	}
}

HRESULT CMainRepeater::GetButtonAddress(IPremiseObject* pObject, long* pl)
{
	pl[0] = 0;
	pl[1] = 0;
	CComVariant var;
	HRESULT hr = GetValue(pObject, L"ButtonID", pl[1]);
	if (FAILED(hr))
		return hr;
	CComPtr<IPremiseObject> spParent;
	hr = pObject->get_Parent(&spParent);
	if (FAILED(hr))
		return hr;

	hr = GetValue(spParent, L"ControlNumber", pl[0]);
	if (FAILED(hr))
		return hr;
	return S_OK;
}

HRESULT CMainRepeater::GetDeviceID(IPremiseObject* pObject, long& l)
{
	l = 0;
	HRESULT hr = GetValue(pObject, L"DeviceID", l);
	if (FAILED(hr))
		return hr;
	return S_OK;
}

HRESULT CMainRepeater::GetFadeTime(IPremiseObject* pObject, double& seconds)
{
	return GetValue(pObject, L"FadeTime", seconds);
}

HRESULT CMainRepeater::GetFadeTime(IPremiseObject* pObject, char buf[64])
{
	double sec = 0;
	GetFadeTime(pObject, sec);

	sprintf(buf, "%.2f", sec);
	return S_OK;
}

HRESULT CMainRepeater::OnForceLEDChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (pObject == NULL)
		return E_POINTER;
	long addr[2];
	HRESULT hr = GetButtonAddress(pObject, addr);
	if (FAILED(hr))
		return hr;
	char buf[64];
	if (newValue.lVal == 1)//On
	{
		wsprintfA(buf, "LC,%d,%d,ON\r\n", addr[0], addr[1]);
		SendBufferedCommand(buf);
		SetValue(pObject, L"Status", CComVariant(true));
	}
	if (newValue.lVal == 0)//Off
	{
		wsprintfA(buf, "LC,%d,%d,OFF\r\n", addr[0], addr[1]);
		SendBufferedCommand(buf);
		SetValue(pObject, L"Status", CComVariant(false));
	}
	return S_OK;
}

HRESULT CMainRepeater::OnCommandChanged(IPremiseObject* pObject, VARIANT newValue)
{
	char buf[64];
	USES_CONVERSION;
	CComBSTR command = newValue.bstrVal;
	if (command.Length() != 0)
	{
		SetValueEx(m_spSite, SVCC_NOTIFY | SVCC_DRIVER, L"Command", CComVariant(L""));
		wsprintfA(buf, "%s\r\n", W2A(command));
		SendBufferedCommandAndWait(buf, 1000);
	}
	return S_OK;
}

HRESULT CMainRepeater::OnPowerStateChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (pObject == NULL)
		return E_POINTER;

	if (IsObjectOfExplicitType(pObject, XML_RadioRA2_SWITCH))
		SendSwitch(pObject, newValue.boolVal ? true : false);
	return S_OK;
}

HRESULT CMainRepeater::On_BrightnessChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (pObject == NULL)
		return E_POINTER;
	if (newValue.vt != VT_R8)
		return E_INVALIDARG;
	long nBright = (int)(newValue.dblVal * 100. + .5);

	SendDim(pObject, nBright);
	return S_OK;
}

void CMainRepeater::SendSwitch(IPremiseObject* pObject, bool bOn)
{
	HRESULT hr;
	long nZone;
	hr = GetDeviceID(pObject, nZone);
	if (FAILED(hr))
		return;
	char buf[128];
	//#OUTPUT,id,1,0<CR><LF>
	//#OUTPUT,id,1,1<CR><LF>
	wsprintfA(buf, "#OUTPUT,%d,1,%d\r\n", nZone, bOn ? 100 : 0);
	SendBufferedCommand(buf);
}

void CMainRepeater::SendDim(IPremiseObject* pObject, long nBright)
{
	long nZone;
	HRESULT hr = GetDeviceID(pObject, nZone);
	if (FAILED(hr))
		return;

	char time[64];
	hr = GetFadeTime(pObject, time);
	if (FAILED(hr))
		return;

	char buf[128];
	//#OUTPUT,1,1,75,01:30<CR><LF>
	if (time[0] == 0)
		wsprintfA(buf, "#OUTPUT,%d,1,%d\r\n", nZone, nBright);
	else
		wsprintfA(buf, "#OUTPUT,%d,1,%d,%s\r\n", nZone, nBright, time);
	SendBufferedCommand(buf);
}

void CMainRepeater::SendFan(IPremiseObject* pObject, long nLevel)
{
	long nID;
	HRESULT hr = GetDeviceID(pObject, nID);
	if (FAILED(hr))
		return;

	char buf[128];
	//#OUTPUT,1,1,75<CR><LF>
	wsprintfA(buf, "#OUTPUT,%d,1,%d\r\n", nID, nLevel);
	SendBufferedCommandNoAck(buf);
}

HRESULT CMainRepeater::OnSceneChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (!IsObjectOfExplicitType(pObject, XML_RadioRA2_GRXI))
		return S_FALSE;

	long nZone;
	HRESULT hr = GetDeviceID(pObject, nZone);
	if (FAILED(hr))
		return S_FALSE;

	if ((newValue.lVal < 0) || (newValue.lVal > 16))
		return S_FALSE;

	char buf[64];
	wsprintfA(buf, "SGS,%d,%d\r\n", nZone, newValue.lVal);
	SendBufferedCommand(buf);
	return S_OK;
}

HRESULT CMainRepeater::OnQuerySystemChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (newValue.lVal == 0)
		return S_FALSE;

	QuerySystem(RA2S_Time);
	QuerySystem(RA2S_Date);
	QuerySystem(RA2S_LatLong);
	QuerySystem(RA2S_TimeZone);
	QuerySystem(RA2S_Sunset);
	QuerySystem(RA2S_Sunrise);
	QuerySystem(RA2S_LoadShed);

	return S_OK;
}

void CMainRepeater::UpdateFirmware(LPCSTR psz)
{
	// "OS Firmware Revision = "
	// 
	psz += 23;
	SetValue(m_spSite, L"Firmware", CComVariant(psz));
}

void CMainRepeater::OnGNET()
{
	if (!m_fReady)
	{
		m_fReady = true;
		SetAckReceived();
		QuerySystem(RA2S_OSRev);
		QueryStateOfDevices();
	}
	else
		SetAckReceived();
}

void CMainRepeater::OnLogin()
{
	USES_CONVERSION;
	char buf[1024];
	wsprintfA(buf, "%s\r\n", W2A(m_bstrLogin));
	// non buffered output as we aren't "ready" yet.
	SendImmediateCommand(buf);
}

void CMainRepeater::OnPassword()
{
	USES_CONVERSION;
	char buf[1024];
	wsprintfA(buf, "%s\r\n", W2A(m_bstrPassword));
	// non buffered output as we aren't "ready" yet.
	SendImmediateCommand(buf);
	// When "GNET>" is seen then we will be ready
}

unsigned long CMainRepeater::ProcessPrompt(const char* psz)
{
	static const std::string strGnet{ "GNET> " };
	static const std::string strLogin{ "login: " };
	static const std::string strPassword{ "password: " };

	if (strGnet == psz)
	{
		OnGNET();
		return strGnet.size();
	}
	else if (strLogin == psz)
	{
		OnLogin();
		return strLogin.size();
	}
	else if (strPassword == psz)
	{
		OnPassword();
		return strPassword.size();
	}

	return 0;
}

unsigned long CMainRepeater::ProcessReadBuffer(BYTE* pBuf, DWORD dw, IPremisePort* pPort)
{
	pBuf[dw] = 0; // null terminate
	BYTE* cur = pBuf;
	bool fContinue = true;
	int left = dw;

	while (fContinue && (left > 0))
	{
		fContinue = false;

		unsigned long ret = CPremiseBufferedPortDevice::ProcessReadBuffer(cur, left, pPort);
		if (ret != 0)
		{
			fContinue = true;
			cur += ret;
			left -= ret;
		}

		// Prompts don't have a newline at the end, so we have to handle them directly.
		ret = ProcessPrompt((const char*)cur);
		if (ret != 0)
		{
			fContinue = true;
			cur += ret;
			left -= ret;
			_ASSERTE(left == 0);
		}
		_ASSERTE(left >= 0);
	}

	return cur - pBuf; // Consumed
}

static LPCSTR NextField(LPCSTR psz)
{
	while (*psz != 0)
	{
		if (*psz == ',')
			return psz + 1;
		++psz;
	}
	return psz;
}

void CMainRepeater::UpdateOutput(LPCSTR psz)
{
	// ~OUTPUT,31,1,100.00
	psz += 8;

	long id = 0;
	if (FAILED(DigitsToLong(psz, -1, id, true)))
		return;

	psz = NextField(psz);

	long action = 0;
	if (FAILED(DigitsToLong(psz, -1, action, true)))
		return;

	psz = NextField(psz);
	long output = 0;
	if (FAILED(DigitsToLong(psz, -1, output, true)))
		return;

	//char buf[128];
	//wsprintfA(buf, "output,%d,%d,%d\n", id, action, output);
	//OutputDebugStringA(buf);

	if (action == RA2_Set) // level
	{
		CComPtr<IPremiseObject> spObj;
		HRESULT hr = GetObjectByID(id, &spObj);
		if (FAILED(hr))
			return;

		if (IsObjectOfExplicitType(spObj, XML_RadioRA2_DIMMER))
		{
			float fDim = (float)output / 100.f;
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"Brightness", CComVariant(fDim));
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"PowerState", CComVariant(output ? true : false));
		}
		else if (IsObjectOfExplicitType(spObj, XML_RadioRA2_SWITCH))
		{
			bool powerstate = output ? true : false;
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"PowerState", CComVariant(powerstate));
		}
		else if (IsObjectOfExplicitType(spObj, XML_RadioRA2_FAN))
		{
			double fDim = (double)output / 100.;
			int setting = ClampFanSetting(output);
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"Speed", CComVariant(fDim));
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"Setting", CComVariant(setting));
		}
	}
}

void CMainRepeater::UpdateDevice(LPCSTR psz)
{
	// ~DEVICE,1,101,9,0
	psz += 8;

	long id = 0;
	if (FAILED(DigitsToLong(psz, -1, id, true)))
		return;

	psz = NextField(psz);

	long component = 0;
	if (FAILED(DigitsToLong(psz, -1, component, true)))
		return;

	psz = NextField(psz);


	long action = 0;
	if (FAILED(DigitsToLong(psz, -1, action, true)))
		return;

	psz = NextField(psz);
	long output = 0;
	if (FAILED(DigitsToLong(psz, -1, output, true)))
		return;

	CComPtr<IPremiseObject> spObj;
	HRESULT hr = GetObjectByComponent(id, component, &spObj);
	if (FAILED(hr))
		return;

	// Normal buttons will only get a press (and no release), so automatically generate a release
	if (IsObjectOfExplicitType(spObj, XML_Button))
	{
		switch (action)
		{
		case RA2_Press:
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"ButtonState", CComVariant(1));
			break;
		case RA2_Release:
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"ButtonState", CComVariant(0));
			break;
		case RA2_LedState:
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"Status", CComVariant(output ? true : false));
			break;
		}
	}

	// simple buttons will get an explicit press/release
	if (IsObjectOfExplicitType(spObj, XML_SimpleButton))
	{
		switch (action)
		{
		case RA2_Press:
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"ButtonState", CComVariant(1));
			break;
		case RA2_Release:
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"ButtonState", CComVariant(0));
			break;
		case RA2_LedState:
			SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"Status", CComVariant(output ? true : false));
			break;
		}
	}

	return;
}

void CMainRepeater::UpdateSystem(LPCSTR psz)
{
	char buf[64];
	// ~SYSTEM,1,101,9,0
	psz += 8;

	long action = 0;
	if (FAILED(DigitsToLong(psz, -1, action, true)))
		return;

	psz = NextField(psz);

	switch (action)
	{
	case RA2S_Time: // SS.ss, SS, MM:SS, HH:MM:SS
		SetValue(m_spSite, L"Time", CComVariant(psz));
		break;
	case RA2S_Date: // MM/DD/YY
		SetValue(m_spSite, L"Date", CComVariant(psz));
		break;
	case RA2S_LatLong: // Latitude -90.00 to +90.00 degrees, Longitude -180.00 to +180.00
	{
		char* end;
		float lat = (float)strtod(psz, &end);
		psz = NextField(psz);
		float longitude = (float)strtod(psz, &end);
		sprintf(buf, "%.2f", lat);
		SetValue(m_spSite, L"Latitude", CComVariant(buf));
		sprintf(buf, "%.2f", longitude);
		SetValue(m_spSite, L"Longitude", CComVariant(buf));
	}
	break;
	case RA2S_TimeZone: // Hours=-12 to 12, Minutes = 0 to 59
		SetValue(m_spSite, L"TimeZone", CComVariant(psz));
		break;
	case RA2S_Sunset:
		SetValue(m_spSite, L"Sunset", CComVariant(psz));
		break;
	case RA2S_Sunrise:
		SetValue(m_spSite, L"Sunrise", CComVariant(psz));
		break;
	case RA2S_OSRev:
		break;
	case RA2S_LoadShed: // (0=Disabled, 1=Enabled)
	{
		long value = 0;
		if (FAILED(DigitsToLong(psz, -1, value, true)))
			return;
		bool f = value ? true : false;
		SetValue(m_spSite, L"LoadShed", CComVariant(f));
	}
	break;
	default:
		return;
	}

	return;
}

void CMainRepeater::UpdateGroup(LPCSTR psz)
{
	// ~GROUP,16,3,3
	psz += 7; // ~GROUP,

	long group = 0;
	if (FAILED(DigitsToLong(psz, -1, group, true)))
		return;
	psz = NextField(psz);

	long action = 0;
	if (FAILED(DigitsToLong(psz, -1, action, true)))
		return;
	psz = NextField(psz);

	long state = 0;
	if (FAILED(DigitsToLong(psz, -1, state, true)))
		return;

	CComPtr<IPremiseObject> spObj;
	HRESULT hr = GetSensorByRoomID(group, &spObj);
	if (FAILED(hr))
		return;

	// only support occupancy sensors
	if (!IsObjectOfExplicitType(spObj, XML_RadioRA2_RadioPowrSavr))
		return;

	if (action != 3) //Occupancy Group State
		return;

	switch (state)
	{
	case RA2_Occupied:
		SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"State", CComVariant(1));
		break;
	case RA2_Unoccupied:
		SetValueEx(spObj, SVCC_NOTIFY | SVCC_DRIVER, L"State", CComVariant(0));
		break;
	}

	return;
}

HRESULT CMainRepeater::GetSensorByRoomID(unsigned int nID, IPremiseObject** ppObject)
{
	CComPtr<IPremiseObject> spObj(m_spSite);
	return spObj->GetChildByTypeAndProperty((BSTR)XML_RadioRA2_RadioPowrSavr, (BSTR)L"RoomID", CComVariant((int)nID), ppObject);
}

HRESULT CMainRepeater::GetObjectByID(unsigned int nID, IPremiseObject** ppObject)
{
	if (nID == 1) // main repeater
	{
		return m_spSite.CopyTo(ppObject);
	}
	else if ((nID >= 1) && (nID <= 200))
	{
		CComPtr<IPremiseObject> spObj;
		//HRESULT hr = m_spSite->GetChildByType(XML_RadioRA2_Master, &spObj);
		//if (FAILED(hr))
		//	return hr;
		spObj = m_spSite;

		return spObj->GetChildByTypeAndProperty((BSTR)XML_RadioRA2_DEVICE, (BSTR)L"DeviceID", CComVariant((int)nID), ppObject);
	}
	else
	{
		*ppObject = NULL;
		return E_INVALIDARG;
	}
}

HRESULT CMainRepeater::GetObjectByComponent(unsigned int id, unsigned int component, IPremiseObject** ppObject)
{
	*ppObject = NULL;

	if (id == 1)
	{
		// Phantom button
		if ((component >= 1) && (component <= 100))
		{
			CComVariant value((int)component);
			CComPtr<IPremiseObject> spObj;
			HRESULT hr = m_spSite->GetChildByType((BSTR)XML_RadioRA2_PhantomButtons, &spObj);
			if (FAILED(hr))
				return hr;

			return spObj->GetChildByTypeAndProperty((BSTR)XML_Button, (BSTR)L"ComponentNumber", value, ppObject);
		}
		// Phantom LED
		else if ((component >= 101) && (component <= 200))
		{
			CComVariant value((int)(component - 100));
			CComPtr<IPremiseObject> spObj;
			HRESULT hr = m_spSite->GetChildByType((BSTR)XML_RadioRA2_PhantomButtons, &spObj);
			if (FAILED(hr))
				return hr;

			return spObj->GetChildByTypeAndProperty((BSTR)XML_Button, (BSTR)L"ComponentNumber", value, ppObject);
		}
	}
	else
	{
		CComPtr<IPremiseObject> spObj;
		HRESULT hr = GetObjectByID(id, &spObj);
		if (FAILED(hr))
			return hr;
		if (IsObjectOfExplicitType(spObj, XML_Keypad))
		{
			if (component < 80)
				return spObj->GetChildByProperty((BSTR)L"ComponentNumber", CComVariant((int)component), ppObject);
			else
				return spObj->GetChildByProperty((BSTR)L"LEDComponentNumber", CComVariant((int)component), ppObject);
		}
	}
	return E_INVALIDARG;
}


void CMainRepeater::QuerySystem(RadioRA2_System_Actions action)
{
	char buf[64];
	wsprintfA(buf, "?SYSTEM,%d\r\n", action);
	SendBufferedCommand(buf);
}

void CMainRepeater::QueryMonitoring(Monitor_Type mt)
{
	char buf[64];
	wsprintfA(buf, "?MONITORING,%d\r\n", mt);
	SendBufferedCommand(buf);
}

HRESULT CMainRepeater::QueryStateOfDevices()
{
	HRESULT hr = S_OK;
	CComPtr<IPremiseObjectCollection> spCollection;
	FAILED_ASSERT_RETURN_HR(m_spSite->GetChildren(collectionTypeNoRecurse, &spCollection));
	CComPtr<IPremiseObject> spItem;
	LONG lCount = 0;

	spCollection->get_Count(&lCount);
	for (int i = 0; i < lCount; i++)
	{
		FAILED_ASSERT_RETURN_HR(spCollection->Item(CComVariant(i), &spItem));
		CComPtr<IPremiseObjectCollection> spProperties;
		if (IsObjectOfExplicitType(spItem, XML_RadioRA2_RadioPowrSavr))
		{
			long nID;
			HRESULT hr = GetValue(spItem, L"RoomID", nID);
			if (FAILED(hr))
				return hr;
			char buf[64];
			wsprintfA(buf, "?GROUP,%d,3\r\n", nID);
			SendBufferedCommand(buf);
		}
		else if (
			IsObjectOfExplicitType(spItem, XML_RadioRA2_FAN) ||
			IsObjectOfExplicitType(spItem, XML_RadioRA2_SWITCH) ||
			IsObjectOfExplicitType(spItem, XML_RadioRA2_DIMMER)
			)
		{
			long nID;
			HRESULT hr = GetValue(spItem, L"DeviceID", nID);
			if (FAILED(hr))
				return hr;
			char buf[64];
			wsprintfA(buf, "?OUTPUT,%d,1\r\n", nID);
			SendBufferedCommand(buf);
		}
		spItem.Release();
	}
	return S_OK;
}

void CMainRepeater::PrepareToOpen()
{
	m_fReady = false;
	m_networkMode = false;
	m_bstrPassword = L"";
	m_bstrLogin = L"";
	SetValueEx(m_spSite, SVCC_NOTIFY | SVCC_DRIVER, L"Login", CComVariant(L""));
	SetValueEx(m_spSite, SVCC_NOTIFY | SVCC_DRIVER, L"Password", CComVariant(L""));

	if (m_spSite == NULL)
		return;
	CComPtr<IPremiseObject> spNetwork;
	CComVariant varNet;
	if (FAILED(m_spSite->GetValue((BSTR)L"Network", &varNet)))
		return;
	if (varNet.pdispVal == NULL)
		return;

	varNet.pdispVal->QueryInterface(&spNetwork);
	if (spNetwork == NULL)
		return;

	CComPtr<IPremiseObject> spParent;
	spNetwork->get_Parent(&spParent);
	if (spParent == NULL)
		return;

	if (IsObjectOfExplicitType(spParent, XML_RadioRA2_Port))
	{
		// Get login/password information from port object.
		HRESULT hr;
		CComVariant var;
		hr = spParent->GetValue((BSTR)L"Login", &var);
		if (FAILED(hr))
			return;
		m_bstrLogin = var.bstrVal;
		SetValueEx(m_spSite, SVCC_NOTIFY | SVCC_DRIVER, L"Login", var);
		var.Clear();

		hr = spParent->GetValue((BSTR)L"Password", &var);
		if (FAILED(hr))
			return;
		m_bstrPassword = var.bstrVal;
		SetValueEx(m_spSite, SVCC_NOTIFY | SVCC_DRIVER, L"Password", var);

		m_networkMode = ((m_bstrLogin.Length() != 0) && (m_bstrPassword.Length() != 0));
	}
}

// "Press" a keypad button virtually
HRESULT CMainRepeater::OnTriggerChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (IsObjectOfExplicitType(pObject, XML_Button))
	{
		if (newValue.vt != VT_BOOL)
			return E_INVALIDARG;

		long nComponentID;
		HRESULT hr = GetValue(pObject, L"ComponentNumber", nComponentID);
		if (FAILED(hr))
			return hr;

		long nID = 1; // Repeater if phantom button
		CComPtr<IPremiseObject> spParent;
		pObject->get_Parent(&spParent);
		if (IsObjectOfExplicitType(spParent, XML_Keypad))
		{
			hr = GetValue(spParent, L"DeviceID", nID);
			if (FAILED(hr))
				return hr;
		}

		char buf[64];
		// #DEVICE,integrationid,component,action,param
		// #DEVICE,1,buttonid,3 - press
		wsprintfA(buf, "#DEVICE,%d,%d,%d\r\n", nID, nComponentID, RA2_Press);
		SendBufferedCommand(buf);
		wsprintfA(buf, "#DEVICE,%d,%d,%d\r\n", nID, nComponentID, RA2_Release);
		SendBufferedCommand(buf);
		// reset trigger back to false
		SetValueEx(pObject, SVCC_NOTIFY | SVCC_DRIVER, L"Trigger", CComVariant(false));
	}
	return S_OK;
}

HRESULT CMainRepeater::OnFlashChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (newValue.vt != VT_BOOL)
		return E_INVALIDARG;

	long deviceID;
	HRESULT hr = GetValue(pObject, L"DeviceID", deviceID);
	if (FAILED(hr))
		return hr;

	double time = 0;
	hr = GetFadeTime(pObject, time);

	bool flash = newValue.boolVal ? true : false;

	char buf[128];
	//#OUTPUT,1,1,75,01:30<CR><LF>

	if (flash)
	{
		if (time < .25f)
			sprintf(buf, "#OUTPUT,%d,5\r\n", deviceID);
		else
			sprintf(buf, "#OUTPUT,%d,5,%.2f\r\n", deviceID, time);
		SendBufferedCommand(buf);
		/* Note: I'm seeing weird behavior where all my lamp dimmers start flashing whenever any zone is 
		   flashed. The lamp dimmers will continue flashing until they are sent a command or manually 
		   controlled. This appears to be a firmware issue and not related to what is being sent. */
	}
	else
	{
		if (IsObjectOfExplicitType(pObject, XML_RadioRA2_SWITCH))
		{
			CComVariant var;
			pObject->GetValue(L"PowerState", &var);
			SendSwitch(pObject, var.boolVal);
		}
		else if (IsObjectOfExplicitType(pObject, XML_RadioRA2_DIMMER))
		{
			CComVariant var;
			pObject->GetValue(L"Brightness", &var);
			long nBright = (int)(var.dblVal * 100. + .5);
			SendDim(pObject, nBright);
		}
	}
	return S_OK;
}

HRESULT CMainRepeater::OnButtonsChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (IsObjectOfExplicitType(pObject, XML_RadioRA2_TableTop))
	{
		CComVariant var = newValue;
		var.ChangeType(VT_I4);
		int nButtons = var.lVal;
		CreateTableTopButtons(pObject, nButtons);
	}
	return S_OK;
}

HRESULT CMainRepeater::OnObjectCreated(IPremiseObject* pContainer, IPremiseObject* pCreatedObject)
{
	if (IsObjectOfExplicitType(pCreatedObject, XML_T5RL))
		CreateTableTopButtons(pCreatedObject, 5);
	else if (IsObjectOfExplicitType(pCreatedObject, XML_T10RL))
		CreateTableTopButtons(pCreatedObject, 10);
	else if (IsObjectOfExplicitType(pCreatedObject, XML_T15RL))
		CreateTableTopButtons(pCreatedObject, 15);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_1RLD"))
		CreateKeypadButtons(pCreatedObject, RRD_1RLD);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_2RLD"))
		CreateKeypadButtons(pCreatedObject, RRD_2RLD);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_3BD"))
		CreateKeypadButtons(pCreatedObject, RRD_3BD);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_3BRL"))
		CreateKeypadButtons(pCreatedObject, RRD_3BRL);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_3BSRL"))
		CreateKeypadButtons(pCreatedObject, RRD_3BSRL);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_4S"))
		CreateKeypadButtons(pCreatedObject, RRD_4S);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_5BRL"))
		CreateKeypadButtons(pCreatedObject, RRD_5BRL);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_6BRL"))
		CreateKeypadButtons(pCreatedObject, RRD_6BRL);
	else if (IsObjectOfExplicitType(pCreatedObject, L"sys://Schema/RadioRA2/RRD_7B"))
		CreateKeypadButtons(pCreatedObject, RRD_7B);
	else if (IsObjectOfExplicitType(pCreatedObject, XML_RadioRA2_VCRX))
		CreateVCRX(pCreatedObject);
	return S_OK;
}

HRESULT CMainRepeater::OnObjectDeleted(IPremiseObject* pContainer, IPremiseObject* pDeletedObject)
{
	return S_OK;
}


static void CreateSimpleButton(IPremiseObject* pObject, LPCWSTR name, int component)
{
	CComPtr<IPremiseObject> spNew;
	pObject->CreateEx(SVCC_FIXED | SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_SimpleButton, (BSTR)name, &spNew);
	if (spNew)
		SetValue(spNew, L"ComponentNumber", CComVariant(component));
}

static void CreateButton(IPremiseObject* pObject, LPCWSTR name, int component, int led)
{
	CComPtr<IPremiseObject> spNew;
	pObject->CreateEx(SVCC_FIXED | SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_Button, (BSTR)name, &spNew);
	if (spNew)
	{
		SetValue(spNew, L"ComponentNumber", CComVariant(component));
		SetValue(spNew, L"LEDComponentNumber", CComVariant(led));
	}
}

static void CreateButtonWithTrigger(IPremiseObject* pObject, LPWSTR name, int component, int led)
{
	CComPtr<IPremiseObject> spNew;
	pObject->CreateEx(SVCC_FIXED | SVCC_NOTIFY | SVCC_EXIST, (BSTR)XML_ButtonWithTrigger, name, &spNew);
	if (spNew)
	{
		SetValue(spNew, L"ComponentNumber", CComVariant(component));
		SetValue(spNew, L"LEDComponentNumber", CComVariant(led));
	}
}

static void CreateSimpleButtons(IPremiseObject* pObject, CRadioRA2Button* buttons, int n)
{
	for (int i = 0; i < n; ++i)
	{
		CreateSimpleButton(pObject, buttons[i].pszName, buttons[i].nID);
	}
}

static void CreateButtons(IPremiseObject* pObject, int n, int offset)
{
	wchar_t buf[64];
	int i;
	// starting index of 1
	for (i = 1; i <= n; ++i)
	{
		CComPtr<IPremiseObject> spNew;
		wsprintfW(buf, L"Button%d", i);
		CreateButton(pObject, buf, i, i + offset);
	}
}

static void CreateButtonsWithTrigger(IPremiseObject* pObject, int n, int offset)
{
	wchar_t buf[64];
	int i;
	// starting index of 1
	for (i = 1; i <= n; ++i)
	{
		CComPtr<IPremiseObject> spNew;
		wsprintfW(buf, L"Button%d", i);
		CreateButtonWithTrigger(pObject, buf, i, i + offset);
	}
}

HRESULT CMainRepeater::CreateKeypadButtons(IPremiseObject* pObject, Keypads kp)
{
	for (int i = 0; i < 11; ++i)
	{
		CRadioRA2Button& button = keypadButtons[i];
		bool b = keypadInfo[kp][i];
		if (b)
		{
			if (button.simple)
				CreateSimpleButton(pObject, button.pszName, button.nID);
			else
				CreateButton(pObject, button.pszName, button.nID, button.nID + LedKeypadButtonOffset);
		}
	}
	return S_OK;
}

HRESULT CMainRepeater::CreateVCRX(IPremiseObject* pObject)
{
	for (int i = 0; i < 6; ++i)
	{
		CreateButtonsWithTrigger(pObject, 6, 80);
	}
	return S_OK;
}

HRESULT CMainRepeater::CreateTableTopButtons(IPremiseObject* pObject, int nButtons)
{
	// include AllOn, AllOff, Raise, Lower
	::CreateSimpleButtons(pObject, allonoff_buttons_, 2);
	::CreateSimpleButtons(pObject, lower_raise_buttons_, 2);
	// Create main buttons
	::CreateButtons(pObject, nButtons, LedKeypadButtonOffset);

	return S_OK;
}

HRESULT CMainRepeater::CreatePhantomButtons(IPremiseObject* pObject)
{
	// buttonid is 1-100, LED id 101-200
	CreateButtonsWithTrigger(pObject, 100, 100);
	return S_OK;
}

HRESULT CMainRepeater::OnFanSettingChanged(IPremiseObject* pObject, VARIANT newValue)
{
	int nVal = newValue.lVal;//0,25,50,75,100
	SetValueEx(pObject, SVCC_NOTIFY | SVCC_DRIVER, L"Speed", CComVariant((double)nVal / 100.0));

	SendFan(pObject, nVal);
	return S_OK;
}

//percent 0.0 -- 100.0
HRESULT CMainRepeater::OnFanSpeedChanged(IPremiseObject* pObject, VARIANT newValue)
{
	if (newValue.vt != VT_R8)
		return E_INVALIDARG;

	int nSetting = FanSettingFromPercentage(newValue.dblVal);
	SetValueEx(pObject, SVCC_NOTIFY | SVCC_DRIVER, L"Setting", CComVariant(nSetting));

	SendFan(pObject, nSetting);
	return S_OK;
}
