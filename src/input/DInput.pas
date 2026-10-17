unit DInput;

{ DirectInput 5 declarations reconstructed from the API records, interface
  identities and data-format tables used by Input.dll. API values and layouts
  follow Microsoft's dinput.h shipped with Visual C++ 6. The original Pascal
  header's author and release have not yet been established. }

interface

uses Windows, MMSystem;

type
  PDIDevCaps = ^TDIDevCaps;
  TDIDevCaps = record
    dwSize, dwFlags, dwDevType, dwAxes, dwButtons, dwPOVs: DWORD;
    dwFFSamplePeriod, dwFFMinTimeResolution, dwFirmwareRevision: DWORD;
    dwHardwareRevision, dwFFDriverVersion: DWORD;
  end;
  PDIDeviceInstanceA = ^TDIDeviceInstanceA;
  TDIDeviceInstanceA = record
    dwSize: DWORD;
    guidInstance, guidProduct: TGUID;
    dwDevType: DWORD;
    tszInstanceName, tszProductName: array[0..MAX_PATH - 1] of Char;
    guidFFDriver: TGUID;
    wUsagePage, wUsage: Word;
  end;
  TDIDeviceInstance = TDIDeviceInstanceA;
  PDIDeviceObjectInstanceA = ^TDIDeviceObjectInstanceA;
  TDIDeviceObjectInstanceA = record
    dwSize: DWORD;
    guidType: TGUID;
    dwOfs, dwType, dwFlags: DWORD;
    tszName: array[0..MAX_PATH - 1] of Char;
    dwFFMaxForce, dwFFForceResolution: DWORD;
    wCollectionNumber, wDesignatorIndex, wUsagePage, wUsage: Word;
    dwDimension: DWORD;
    wExponent, wReportId: Word;
  end;
  TDIDeviceObjectInstance = TDIDeviceObjectInstanceA;
  TDIEnumDevicesCallbackA = function(const Instance: TDIDeviceInstanceA;
    Context: Pointer): Integer; stdcall;
  TDIEnumDeviceObjectsCallbackA = function(const Instance: TDIDeviceObjectInstanceA;
    Context: Pointer): Integer; stdcall;
  PDIPROPHeader = ^TDIPROPHeader;
  TDIPROPHeader = record
    dwSize, dwHeaderSize, dwObj, dwHow: DWORD;
  end;
  TDIPROPDWord = record
    diph: TDIPROPHeader;
    dwData: DWORD;
  end;
  TDIPROPRange = record
    diph: TDIPROPHeader;
    lMin, lMax: Longint;
  end;
  PDIObjectDataFormat = ^TDIObjectDataFormat;
  TDIObjectDataFormat = record
    pguid: PGUID;
    dwOfs, dwType, dwFlags: DWORD;
  end;
  PDIDataFormat = ^TDIDataFormat;
  TDIDataFormat = record
    dwSize, dwObjSize, dwFlags, dwDataSize, dwNumObjs: DWORD;
    rgodf: PDIObjectDataFormat;
  end;
  PDIDeviceObjectData = ^TDIDeviceObjectData;
  TDIDeviceObjectData = record
    dwOfs, dwData, dwTimeStamp, dwSequence: DWORD;
  end;
  TDJoyState = record
    lX, lY, lZ, lRx, lRy, lRz: Longint;
    rglSlider: array[0..1] of Longint;
    rgdwPOV: array[0..3] of DWORD;
    rgbButtons: array[0..31] of Byte;
  end;
  TDIJoyState = TDJoyState;
  TDIMouseState = record
    lX, lY, lZ: Longint;
    rgbButtons: array[0..3] of Byte;
  end;
  TDIConstantForce = record
    lMagnitude: Longint;
  end;
  PDIEnvelope = ^TDIEnvelope;
  TDIEnvelope = record
    dwSize, dwAttackLevel, dwAttackTime, dwFadeLevel, dwFadeTime: DWORD;
  end;
  PDIEffect = ^TDIEffect;
  TDIEffect = record
    dwSize, dwFlags, dwDuration, dwSamplePeriod, dwGain: DWORD;
    dwTriggerButton, dwTriggerRepeatInterval, cAxes: DWORD;
    rgdwAxes: PDWORD;
    rglDirection: PLongint;
    lpEnvelope: PDIEnvelope;
    cbTypeSpecificParams: DWORD;
    lpvTypeSpecificParams: Pointer;
  end;
  TDIEffectInfoA = record
    dwSize: DWORD;
    guid: TGUID;
    dwEffType, dwStaticParams, dwDynamicParams: DWORD;
    tszName: array[0..MAX_PATH - 1] of Char;
  end;
  TDIEffEscape = record
    dwSize, dwCommand: DWORD;
    lpvInBuffer: Pointer;
    cbInBuffer: DWORD;
    lpvOutBuffer: Pointer;
    cbOutBuffer: DWORD;
  end;
  IDirectInputEffect = interface(IUnknown)
    ['{E7E1F7C0-88D2-11D0-9AD0-00A0C9A06E35}']
    function Initialize(hinst: THandle; dwVersion: DWORD; const rguid: TGUID): HRESULT; stdcall;
    function GetEffectGuid(out rguid: TGUID): HRESULT; stdcall;
    function GetParameters(var Effect: TDIEffect; Flags: DWORD): HRESULT; stdcall;
    function SetParameters(const Effect: TDIEffect; Flags: DWORD): HRESULT; stdcall;
    function Start(Iterations, Flags: DWORD): HRESULT; stdcall;
    function Stop: HRESULT; stdcall;
    function GetEffectStatus(out Flags: DWORD): HRESULT; stdcall;
    function Download: HRESULT; stdcall;
    function Unload: HRESULT; stdcall;
    function Escape(var Escape: TDIEffEscape): HRESULT; stdcall;
  end;
  TDIEnumEffectsCallbackA = function(const Info: TDIEffectInfoA; Context: Pointer): Integer; stdcall;
  TDIEnumCreatedEffectObjectsCallback = function(Effect: IDirectInputEffect;
    Context: Pointer): Integer; stdcall;
  IDirectInputDeviceA = interface(IUnknown)
    ['{5944E680-C92E-11CF-BFC7-444553540000}']
    function GetCapabilities(var Caps: TDIDevCaps): HRESULT; stdcall;
    function EnumObjects(Callback: TDIEnumDeviceObjectsCallbackA; Context: Pointer; Flags: DWORD): HRESULT; stdcall;
    function GetProperty(Prop: PGUID; var Header: TDIPROPHeader): HRESULT; stdcall;
    function SetProperty(Prop: PGUID; const Header: TDIPROPHeader): HRESULT; stdcall;
    function Acquire: HRESULT; stdcall;
    function Unacquire: HRESULT; stdcall;
    function GetDeviceState(Size: DWORD; var Data): HRESULT; stdcall;
    function GetDeviceData(Size: DWORD; Data: PDIDeviceObjectData; var Count: DWORD; Flags: DWORD): HRESULT; stdcall;
    function SetDataFormat(const Format: TDIDataFormat): HRESULT; stdcall;
    function SetEventNotification(Event: THandle): HRESULT; stdcall;
    function SetCooperativeLevel(Window: HWND; Flags: DWORD): HRESULT; stdcall;
    function GetObjectInfo(var Info: TDIDeviceObjectInstanceA; ObjectID, How: DWORD): HRESULT; stdcall;
    function GetDeviceInfo(var Info: TDIDeviceInstanceA): HRESULT; stdcall;
    function RunControlPanel(Window: HWND; Flags: DWORD): HRESULT; stdcall;
    function Initialize(hinst: THandle; dwVersion: DWORD; const rguid: TGUID): HRESULT; stdcall;
  end;
  IDirectInputDevice2A = interface(IDirectInputDeviceA)
    ['{5944E682-C92E-11CF-BFC7-444553540000}']
    function CreateEffect(const rguid: TGUID; const Effect: TDIEffect; var NewEffect: IDirectInputEffect; Outer: IUnknown): HRESULT; stdcall;
    function EnumEffects(Callback: TDIEnumEffectsCallbackA; Context: Pointer; TypeFlags: DWORD): HRESULT; stdcall;
    function GetEffectInfo(var Info: TDIEffectInfoA; const rguid: TGUID): HRESULT; stdcall;
    function GetForceFeedbackState(out State: DWORD): HRESULT; stdcall;
    function SendForceFeedbackCommand(Flags: DWORD): HRESULT; stdcall;
    function EnumCreatedEffectObjects(Callback: TDIEnumCreatedEffectObjectsCallback; Context: Pointer; Flags: DWORD): HRESULT; stdcall;
    function Escape(var Escape: TDIEffEscape): HRESULT; stdcall;
    function Poll: HRESULT; stdcall;
    function SendDeviceData(Size: DWORD; Data: PDIDeviceObjectData; var Count: DWORD; Flags: DWORD): HRESULT; stdcall;
  end;
  IDirectInputA = interface(IUnknown)
    ['{89521360-AA8A-11CF-BFC7-444553540000}']
    function CreateDevice(const rguid: TGUID; var Device: IDirectInputDeviceA; Outer: IUnknown): HRESULT; stdcall;
    function EnumDevices(DeviceType: DWORD; Callback: TDIEnumDevicesCallbackA; Context: Pointer; Flags: DWORD): HRESULT; stdcall;
    function GetDeviceStatus(const rguid: TGUID): HRESULT; stdcall;
    function RunControlPanel(Window: HWND; Flags: DWORD): HRESULT; stdcall;
    function Initialize(hinst: THandle; dwVersion: DWORD): HRESULT; stdcall;
  end;
  IDirectInput = IDirectInputA;
  IDirectInputDevice = IDirectInputDeviceA;
  IDirectInputDevice2 = IDirectInputDevice2A;
  TDirectInputCreateA = function(hinst: THandle; dwVersion: DWORD;
    out DI: IDirectInputA; Outer: IUnknown): HRESULT; stdcall;

const
  DIRECTINPUT_VERSION = $0500;
  DI_OK = 0;
  DI_FFNOMINALMAX = 10000;
  DIENUM_STOP = 0;
  DIENUM_CONTINUE = 1;
  DIDEVTYPE_MOUSE = 2;
  DIDEVTYPE_KEYBOARD = 3;
  DIDEVTYPE_JOYSTICK = 4;
  DIEDFL_ALLDEVICES = 0;
  DIEDFL_ATTACHEDONLY = 1;
  DIEDFL_FORCEFEEDBACK = $100;
  DIDC_FORCEFEEDBACK = $100;
  DIDFT_ALL = 0;
  DIDFT_RELAXIS = 1;
  DIDFT_ABSAXIS = 2;
  DIDFT_AXIS = 3;
  DIDFT_BUTTON = $C;
  DIDFT_POV = $10;
  DIDFT_ANYINSTANCE = $00FFFF00;
  DIDFT_OPTIONAL = $80000000;
  DIDF_ABSAXIS = 1;
  DIDF_RELAXIS = 2;
  DIPH_DEVICE = 0;
  DIPH_BYOFFSET = 1;
  DIPH_BYID = 2;
  DIPROP_BUFFERSIZE = PGUID(1);
  DIPROP_AXISMODE = PGUID(2);
  DIPROP_GRANULARITY = PGUID(3);
  DIPROP_RANGE = PGUID(4);
  DIPROP_DEADZONE = PGUID(5);
  DIPROP_SATURATION = PGUID(6);
  DIPROP_FFGAIN = PGUID(7);
  DIPROP_FFLOAD = PGUID(8);
  DIPROP_AUTOCENTER = PGUID(9);
  DIPROPAXISMODE_ABS = 0;
  DIPROPAXISMODE_REL = 1;
  DIPROPAUTOCENTER_OFF = 0;
  DIPROPAUTOCENTER_ON = 1;
  DISCL_EXCLUSIVE = 1;
  DISCL_NONEXCLUSIVE = 2;
  DISCL_FOREGROUND = 4;
  DISCL_BACKGROUND = 8;
  DIEFF_OBJECTIDS = 1;
  DIEFF_OBJECTOFFSETS = 2;
  DIEFF_CARTESIAN = $10;
  DIEFF_POLAR = $20;
  DIEFF_SPHERICAL = $40;
  DIEB_NOTRIGGER = $FFFFFFFF;
  DIEP_TYPESPECIFICPARAMS = $100;
  DIEP_START = $20000000;
  DIES_SOLO = 1;
  DIES_NODOWNLOAD = $80000000;
  DIJOFS_X = 0;
  DIJOFS_Y = 4;
  DIJOFS_Z = 8;
  DIJOFS_RX = 12;
  DIJOFS_RY = 16;
  DIJOFS_RZ = 20;
  DIJOFS_SLIDER0 = 24;
  DIJOFS_SLIDER1 = 28;
  DIJOFS_POV0 = 32;
  DIJOFS_POV1 = 36;
  DIJOFS_POV2 = 40;
  DIJOFS_POV3 = 44;
  DIJOFS_BUTTON0 = 48;
  DIERR_INPUTLOST = HRESULT($8007001E);
  DIERR_NOTACQUIRED = HRESULT($8007000C);
  DIERR_OTHERAPPHASPRIO = HRESULT($80070005);
  IID_IDirectInputDevice2: TGUID = '{5944E682-C92E-11CF-BFC7-444553540000}';
  GUID_XAxis: TGUID = '{A36D02E0-C9F3-11CF-BFC7-444553540000}';
  GUID_YAxis: TGUID = '{A36D02E1-C9F3-11CF-BFC7-444553540000}';
  GUID_ZAxis: TGUID = '{A36D02E2-C9F3-11CF-BFC7-444553540000}';
  GUID_RxAxis: TGUID = '{A36D02F4-C9F3-11CF-BFC7-444553540000}';
  GUID_RyAxis: TGUID = '{A36D02F5-C9F3-11CF-BFC7-444553540000}';
  GUID_RzAxis: TGUID = '{A36D02E3-C9F3-11CF-BFC7-444553540000}';
  GUID_Slider: TGUID = '{A36D02E4-C9F3-11CF-BFC7-444553540000}';
  GUID_Key: TGUID = '{55728220-D33C-11CF-BFC7-444553540000}';
  GUID_POV: TGUID = '{A36D02F2-C9F3-11CF-BFC7-444553540000}';
  GUID_ConstantForce: TGUID = '{13541C20-8E33-11D0-9AD0-00A0C9A06E35}';
  MouseObjects: array[0..6] of TDIObjectDataFormat = (
    (pguid: @GUID_XAxis; dwOfs: 0; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE; dwFlags: 0),
    (pguid: @GUID_YAxis; dwOfs: 4; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE; dwFlags: 0),
    (pguid: @GUID_ZAxis; dwOfs: 8; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 12; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE; dwFlags: 0),
    (pguid: nil; dwOfs: 13; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE; dwFlags: 0),
    (pguid: nil; dwOfs: 14; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 15; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0));
  c_dfDIMouse: TDIDataFormat = (dwSize: SizeOf(TDIDataFormat);
    dwObjSize: SizeOf(TDIObjectDataFormat); dwFlags: DIDF_RELAXIS;
    dwDataSize: SizeOf(TDIMouseState); dwNumObjs: 7; rgodf: @MouseObjects);

var
  DInputModule: HMODULE;
  KeyboardObjects: array[0..255] of TDIObjectDataFormat;

const
  c_dfDIKeyboard: TDIDataFormat = (dwSize: SizeOf(TDIDataFormat);
    dwObjSize: SizeOf(TDIObjectDataFormat); dwFlags: DIDF_RELAXIS;
    dwDataSize: 256; dwNumObjs: 256; rgodf: @KeyboardObjects);
  JoystickObjects: array[0..43] of TDIObjectDataFormat = (
    (pguid: @GUID_XAxis; dwOfs: 0; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_YAxis; dwOfs: 4; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_ZAxis; dwOfs: 8; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_RxAxis; dwOfs: 12; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_RyAxis; dwOfs: 16; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_RzAxis; dwOfs: 20; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_Slider; dwOfs: 24; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_Slider; dwOfs: 28; dwType: DIDFT_AXIS or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: $100),
    (pguid: @GUID_POV; dwOfs: 32; dwType: DIDFT_POV or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: @GUID_POV; dwOfs: 36; dwType: DIDFT_POV or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: @GUID_POV; dwOfs: 40; dwType: DIDFT_POV or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: @GUID_POV; dwOfs: 44; dwType: DIDFT_POV or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 48; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 49; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 50; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 51; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 52; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 53; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 54; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 55; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 56; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 57; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 58; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 59; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 60; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 61; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 62; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 63; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 64; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 65; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 66; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 67; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 68; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 69; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 70; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 71; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 72; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 73; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 74; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 75; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 76; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 77; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 78; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0),
    (pguid: nil; dwOfs: 79; dwType: DIDFT_BUTTON or DIDFT_ANYINSTANCE or DIDFT_OPTIONAL; dwFlags: 0));
  c_dfDIJoystick: TDIDataFormat = (dwSize: SizeOf(TDIDataFormat);
    dwObjSize: SizeOf(TDIObjectDataFormat); dwFlags: DIDF_ABSAXIS;
    dwDataSize: SizeOf(TDJoyState); dwNumObjs: 44; rgodf: @JoystickObjects);

var
  Joystick2Objects: array[0..163] of TDIObjectDataFormat;
  DirectInputCreateA: TDirectInputCreateA;
  DirectInputCreateW: Pointer;
  DirectInputCreate: TDirectInputCreateA;

function DIErrorString(HR: HRESULT): string;

implementation

uses SysUtils, Forms;

function DIErrorString(HR: HRESULT): string;
begin
  case HR of
    0: Result := 'The operation completed successfully.';
    1: Result := '"The operation had no effect." or "The device buffer overflowed and some input was lost." or "The device exists but is not currently attached." or "The change in device properties had no effect."';
    3: Result := 'The parameters of the effect were successfully updated, but the effect could not be downloaded because the associated device was not acquired in exclusive mode.';
    4: Result := 'The effect was stopped, the parameters were updated, and the effect was restarted.';
    2: Result := 'The device is a polled device. As a result, device buffering will not collect any data and event notifications will not be signaled until the IDirectInputDevice2::Poll method is called.';
    8: Result := 'The parameters of the effect were successfully updated, but some of them were beyond the capabilities of the device and were truncated to the nearest supported value.';
    12: Result := 'Equal to DI_EFFECTRESTARTED | DI_TRUNCATED.';
    HRESULT($800700AA): Result := 'The operation cannot be performed while the device is acquired.';
    HRESULT($800704DF): Result := 'This object is already initialized';
    HRESULT($80070077): Result := 'The object could not be created due to an incompatible driver version or mismatched or incomplete driver components.';
    HRESULT($80070481): Result := 'The application was written for an unsupported prerelease version of DirectInput.';
    HRESULT($80040201): Result := 'The device is full.';
    HRESULT($80040154): Result := 'The device or device instance is not registered with DirectInput. This value is equal to the REGDB_E_CLASSNOTREG standard COM return value.';
    HRESULT($80040208): Result := 'The parameters were updated in memory but were not downloaded to the device because the device does not support updating an effect while it is still playing.';
    HRESULT($80040204): Result := 'The device cannot be reinitialized because there are still effects attached to it.';
    HRESULT($80004005): Result := 'An undetermined error occurred inside the DirectInput subsystem. This value is equal to the E_FAIL standard COM return value.';
    HRESULT($80040206): Result := 'The effect could not be downloaded because essential information is missing. For example, no axes have been associated with the effect, or no type-specific information has been supplied.';
    HRESULT($8007001E): Result := 'Access to the input device has been lost. It must be reacquired.';
    HRESULT($80070057): Result := 'An invalid parameter was passed to the returning function, or the object was not in a state that permitted the function to be called. This value is equal to the E_INVALIDARG standard COM return value.';
    HRESULT($80040202): Result := 'Not all the requested information fitted into the buffer.';
    HRESULT($80040110): Result := 'This object does not support aggregation.';
    HRESULT($80004002): Result := 'The specified interface is not supported by the object. This value is equal to the E_NOINTERFACE standard COM return value.';
    HRESULT($8007000C): Result := 'The operation cannot be performed unless the device is acquired.';
    HRESULT($80040207): Result := 'The device is not buffered. Set the DIPROP_BUFFERSIZE property to enable buffering.';
    HRESULT($80040203): Result := 'The effect is not downloaded.';
    HRESULT($80040205): Result := 'The operation cannot be performed unless the device is acquired in DISCL_EXCLUSIVE mode.';
    HRESULT($80070002): Result := 'The requested object does not exist.';
    HRESULT($80070015): Result := 'This object has not been initialized.';
    HRESULT($8007047E): Result := 'The application requires a newer version of DirectInput.';
    HRESULT($80070005): Result := '"The device already has an event notification associated with it." or "The specified property cannot be changed." or "Another application has a higher priority level, preventing this call from succeeding. "';
    HRESULT($8007000E): Result := 'The DirectInput subsystem could not allocate sufficient memory to complete the call. This value is equal to the E_OUTOFMEMORY standard COM return value.';
    HRESULT($80004001): Result := 'The function called is not supported at this time. This value is equal to the E_NOTIMPL standard COM return value.';
    HRESULT($80070007): Result := 'Data is not yet available.';
    HRESULT($800405CC): Result := 'No more memory for effects of this kind (not documented)';
  else
    Result := Format('Unrecognized Error: $%x', [HR]);
  end;
end;

function RunningUnderDelphi: Boolean;
var
  Version: TOSVersionInfo;
begin
  Version.dwOSVersionInfoSize := SizeOf(Version);
  GetVersionEx(Version);
  Result := (Version.dwPlatformId = VER_PLATFORM_WIN32_NT) and
    (UpperCase(ExtractFileName(Application.ExeName)) = 'DELPHI32.EXE');
end;

procedure InitKeyboardFormat;
var
  I: Integer;
begin
  for I := 0 to 255 do
    with KeyboardObjects[I] do
    begin
      pguid := @GUID_Key;
      dwOfs := I;
      dwFlags := 0;
      dwType := DIDFT_BUTTON or DIDFT_OPTIONAL or (DWORD(I) shl 8);
    end;
end;

procedure InitJoystick2Format;
var
  I, J, Offset: DWORD;
begin
  Move(JoystickObjects, Joystick2Objects, SizeOf(JoystickObjects));
  for I := 44 to 139 do
    Move(JoystickObjects[43], Joystick2Objects[I], SizeOf(TDIObjectDataFormat));
  for I := 0 to 2 do
  begin
    Move(JoystickObjects, Joystick2Objects[140 + I * 8],
      8 * SizeOf(TDIObjectDataFormat));
    for J := 0 to 7 do
      Joystick2Objects[140 + I * 8 + J].dwFlags := (I + 1) shl 8;
  end;
  Offset := Joystick2Objects[43].dwOfs + 1;
  for I := 44 to 163 do
  begin
    Joystick2Objects[I].dwOfs := Offset;
    if I < 140 then Inc(Offset) else Inc(Offset, 4);
  end;
end;

initialization
  InitKeyboardFormat;
  InitJoystick2Format;
  if not RunningUnderDelphi then
  begin
    DInputModule := LoadLibrary('DInput.dll');
    @DirectInputCreateA := GetProcAddress(DInputModule, 'DirectInputCreateA');
    DirectInputCreateW := GetProcAddress(DInputModule, 'DirectInputCreateW');
    DirectInputCreate := DirectInputCreateA;
  end;
finalization
  FreeLibrary(DInputModule);
end.
