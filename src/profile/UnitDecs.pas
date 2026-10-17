unit UnitDecs;

interface

uses Windows;

type
  TNemu = record
    MainWindow: PHandle;
    Instance: THandle;
    { The host API copies $D0 bytes; these fields are not read by this plugin. }
    Unused08: array[$08..$CF] of Byte;
  end;
  PNemu = ^TNemu;

implementation

end.
