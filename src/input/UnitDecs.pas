unit UnitDecs;

interface

uses Windows;

type
  TControllerData = packed record
    PluggedIn: Boolean;
    MemPack: Boolean;
    RumblePack: Boolean;
    Rumble: Boolean;
    Buttons: Cardinal;
  end;
  TControllerDataArray = array[0..3] of TControllerData;
  PControllerDataArray = ^TControllerDataArray;

implementation

end.
