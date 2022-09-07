// RadioRA2.h : Declaration of the CRadioRA2

#pragma once

#include <syshelp.h>

#define XML_RadioRA2_DEVICE			L"sys://Schema/RadioRA2/Device"
#define XML_RadioRA2_DIMMER			L"sys://Schema/RadioRA2/Dimmer"
#define XML_RadioRA2_SWITCH			L"sys://Schema/RadioRA2/Switch"
#define XML_RadioRA2_FAN			L"sys://Schema/RadioRA2/Fan"
#define XML_RadioRA2_GRXI			L"sys://Schema/RadioRA2/GRXI_Interface"
#define XML_RadioRA2_PhantomButtons	L"sys://Schema/RadioRA2/PhantomButtons"
#define XML_RadioRA2_Zones			L"sys://Schema/RadioRA2/Zones"
#define XML_RadioRA2_Keypads		L"sys://Schema/RadioRA2/Keypads"
#define XML_RadioRA2_TableTop		L"sys://Schema/RadioRA2/TableTopKeypad"
#define XML_RadioRA2_Repeater		L"sys://Schema/RadioRA2/MainRepeater"
#define XML_RadioRA2_Port			L"sys://Schema/RadioRA2/TelnetPort"
#define XML_RadioRA2_RadioPowrSavr	L"sys://Schema/RadioRA2/RadioPowrSavr"
#define XML_RadioRA2_VCRX			L"sys://Schema/RadioRA2/VCRX"

#define XML_T5RL					L"sys://Schema/RadioRA2/T5RL"
#define XML_T10RL					L"sys://Schema/RadioRA2/T10RL"
#define XML_T15RL					L"sys://Schema/RadioRA2/T15RL"
#define XML_Button					L"sys://Schema/RadioRA2/Button"
#define XML_ButtonWithTrigger		L"sys://Schema/RadioRA2/ButtonWithTrigger"
#define XML_SimpleButton			L"sys://Schema/RadioRA2/SimpleButton"
#define XML_Keypad					L"sys://Schema/RadioRA2/Keypad"

#define XML_RadioRA2_QuerySystemInfo L"sys://Schema/RadioRA2/QuerySystemInfo" 

enum Keypads
{
	RRD_1RLD = 0, 
	RRD_2RLD, 
	RRD_3BD, 
	RRD_3BRL, 
	RRD_3BSRL, 
	RRD_4S, 
	RRD_5BRL, 
	RRD_6BRL, 
	RRD_7B,
};

enum RadioRA2_Actions
{
	RA2_Set = 1, RA2_Enable = 1,
	RA2_Disable = 2,
	RA2_Press = 3, RA2_Close = 3, RA2_Occupied = 3,
	RA2_Release = 4, RA2_Open = 4, RA2_Unoccupied = 4,
	RA2_Hold = 5,
	RA2_MultiTap = 6,
	RA2_Scene = 7,
	RA2_LedState = 9, // Params 0 = Off, 1 = On
	RA2_LightLevel = 14, // 0-100, FadeTime (SS.ss, SS, MM:SS, HH:MM:SS), OtherTime???
	RA2_ZoneLock = 15, // 0=Off, 1=On
	RA2_SceneLock = 16, // 0=Off, 1=On
	RA2_SequenceState = 17, // 0=Off, 1=Scenes 1-4, 2=Scenes5-16
	RA2_StartRaising = 18,
	RA2_StartLowering = 19,
	RA2_Stop = 20,
	RA2_GetBatteryStatus = 22, // 1 = Normal, 2 = Low
	RA2_QueryCciState = 35,
};

/*
#OUTPUT,id,Output_Actions[,params]
*/
enum Output_Actions
{
	ZoneLevel = 1,  // 0-100, FadeTime (SS.ss, SS, MM:SS, HH:MM:SS)
	StartRaising = 2,
	StartLowering = 3,
	Stop = 4, // stop raising/lowering
	StartFlash = 5, // (SS.ss, SS, MM:SS, HH:MM:SS) - to stop flash, set to level
	Pulse = 6, // 0-100, FadeTime (SS.ss, SS, MM:SS, HH:MM:SS)
	VenetianTiltLevel = 9,
	// more, TBD
};

enum RadioRA2_System_Actions
{
	RA2S_Time = 1, // SS.ss, SS, MM:SS, HH:MM:SS
	RA2S_Date = 2, // MM/DD/YY
	RA2S_LatLong = 4, // Latitude -90.00 to +90.00 degrees, Longitude -180.00 to +180.00
	RA2S_TimeZone = 5, // Hours=-12 to 12, Minutes = 0 to 59
	RA2S_Sunset = 6,
	RA2S_Sunrise = 7,
	RA2S_OSRev = 8,
	RA2S_LoadShed = 11, // (0=Disabled, 1=Enabled)
};

enum Monitor_Type
{
	MT_Diagnostic = 1,
	MT_Event = 2,
	MT_Button = 3,
	MT_LED = 4,
	MT_Zone = 5,
	MT_Occupancy = 6,
	MT_Scene = 8,
	MT_SystemVariable = 10,
	MT_ReplyState = 11,
	MT_PromptState = 12,
	MT_VenetianTilt = 14,
	MT_Sequence = 16,
	MT_HVAC = 17,
	MT_Mode = 18,

	AllMonitoring = 255
};

/////////////////////////////////////////////////////////////////////////////
// CMainRepeater
// This object is not externally createable, so we don't need all of the COM
// baggage of registry, CLSID, etc
class ATL_NO_VTABLE CMainRepeater : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMainRepeater>,
	public CPremiseBufferedPortDevice
{
public:
	DECLARE_NO_REGISTRY()
	DECLARE_NOT_AGGREGATABLE(CMainRepeater)

BEGIN_COM_MAP(CMainRepeater)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPremisePortCallback)
	COM_INTERFACE_ENTRY(IPremiseNotify)
END_COM_MAP()

	bool m_networkMode = false; // true if login/password specified
	bool m_fReady = false; // m_fReady is set once the port is opened (serial port) or once the login handshake has occurred (telnet port)
	CComPtr<IObjectWithSite> m_spSmartLighting;
	CComBSTR m_bstrLogin;
	CComBSTR m_bstrPassword;

	CMainRepeater()
	{
	}

	bool OnPing() override;
	HRESULT OnBrokerDetach() override;
	HRESULT OnBrokerAttach() override;
	void ProcessLine(LPCSTR psz);
	virtual unsigned long ProcessReadBuffer(BYTE* pBuf, DWORD dw, IPremisePort* pPort);
	HRESULT OnConfigurePort(IPremiseObject* pPort) override;
	HRESULT OnDeviceState(DEVICE_STATE ps) override;

public:
	
BEGIN_NOTIFY_MAP(CMainRepeater) 
	NOTIFY_PROPERTY(L"Command", OnCommandChanged) 
	NOTIFY_PROPERTY(L"PowerState", OnPowerStateChanged) 
	NOTIFY_PROPERTY(L"Network", OnNetworkChanged) 
	NOTIFY_PROPERTY(L"EnableLogging", OnLoggingChanged) 
	NOTIFY_PROPERTY(L"_Brightness", On_BrightnessChanged) 
	NOTIFY_PROPERTY(L"Buttons", OnButtonsChanged) 
	NOTIFY_PROPERTY(L"Trigger", OnTriggerChanged)	//phantom button "pressed"
	//NOTIFY_PROPERTY(L"SSM", OnSSMChanged) 
	//NOTIFY_PROPERTY(L"SFM", OnSFMChanged) 
	NOTIFY_PROPERTY(L"Flash", OnFlashChanged) 
	NOTIFY_PROPERTY(L"QuerySystemInfo", OnQuerySystemChanged)

	NOTIFY_PROPERTY(L"Setting", OnFanSettingChanged)
	NOTIFY_PROPERTY(L"Speed", OnFanSpeedChanged)

	NOTIFY_PROPERTY(L"ForceLED", OnForceLEDChanged) 
	NOTIFY_PROPERTY(L"Scene", OnSceneChanged)
END_NOTIFY_MAP() 

	HRESULT STDMETHODCALLTYPE OnCommandChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnPowerStateChanged(IPremiseObject *pObject, VARIANT newValue);

	HRESULT STDMETHODCALLTYPE On_BrightnessChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnForceLEDChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnSceneChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnFanSettingChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnFanSpeedChanged(IPremiseObject *pObject, VARIANT newValue);

	HRESULT STDMETHODCALLTYPE OnQuerySystemChanged(IPremiseObject *pObject, VARIANT newValue);

	HRESULT STDMETHODCALLTYPE OnTriggerChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnFlashChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnButtonsChanged(IPremiseObject *pObject, VARIANT newValue);
	HRESULT STDMETHODCALLTYPE OnObjectCreated(IPremiseObject *pContainer, IPremiseObject *pCreatedObject);
	HRESULT STDMETHODCALLTYPE OnObjectDeleted(IPremiseObject* pContainer, IPremiseObject* pDeletedObject);

	void UpdateFirmware(LPCSTR psz);
	void UpdateOutput(LPCSTR psz);
	void UpdateDevice(LPCSTR psz);
	void UpdateGroup(LPCSTR psz);
	void UpdateSystem(LPCSTR psz);

	HRESULT GetObjectByID(unsigned int id, IPremiseObject** ppObject);
	HRESULT GetSensorByRoomID(unsigned int id, IPremiseObject** ppObject);
	HRESULT GetObjectByComponent(unsigned int id, unsigned int component, IPremiseObject** ppObject);

	static HRESULT CreatePhantomButtons(IPremiseObject* pObject);
	static HRESULT CreateTableTopButtons(IPremiseObject* pObject, int nButtons);
	static HRESULT CreateKeypadButtons(IPremiseObject* pObject, Keypads kp);
	static HRESULT CreateVCRX(IPremiseObject* pObject);

	HRESULT GetDeviceID(IPremiseObject *pObject, long& l);
	HRESULT GetFadeTime(IPremiseObject* pObject, double& seconds);
	HRESULT GetFadeTime(IPremiseObject* pObject, char buf[64]);
	HRESULT GetButtonAddress(IPremiseObject *pObject, long* pl);
	void SendSwitch(IPremiseObject* pObject, bool bOn);
	void SendDim(IPremiseObject* pObject, long nBright);
	void SendFan(IPremiseObject* pObject, long nLevel);

	void QuerySystem(RadioRA2_System_Actions action);
	void QueryMonitoring(Monitor_Type mt);
	void PrepareToOpen();
	HRESULT QueryStateOfDevices();

	void OnGNET();
	void UpdateGNET(LPCSTR psz);
	void OnLogin();
	void OnPassword();
	void OnConsume();
};
