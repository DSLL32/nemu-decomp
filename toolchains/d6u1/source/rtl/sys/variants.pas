
{ *********************************************************************** }
{                                                                         }
{ Delphi / Kylix Cross-Platform Runtime Library                           }
{ Variants Unit                                                           }
{                                                                         }
{ Copyright (C) 1995-2001 Borland Software Corporation                    }
{                                                                         }
{ *********************************************************************** }

unit Variants;

{$RANGECHECKS OFF}

interface

uses
  Types;

{ Variant support procedures and functions }

function VarType(const V: Variant): TVarType;
function VarAsType(const V: Variant; AVarType: TVarType): Variant;
function VarIsType(const V: Variant; AVarType: TVarType): Boolean; overload;
function VarIsType(const V: Variant; const AVarTypes: array of TVarType): Boolean; overload;
function VarIsByRef(const V: Variant): Boolean;

function VarIsEmpty(const V: Variant): Boolean;
procedure VarCheckEmpty(const V: Variant);
function VarIsNull(const V: Variant): Boolean;
function VarIsClear(const V: Variant): Boolean;

function VarIsCustom(const V: Variant): Boolean;
function VarIsOrdinal(const V: Variant): Boolean;
function VarIsFloat(const V: Variant): Boolean;
function VarIsNumeric(const V: Variant): Boolean;
function VarIsStr(const V: Variant): Boolean;

function VarToStr(const V: Variant): string;
function VarToStrDef(const V: Variant; const ADefault: string): string;
function VarToWideStr(const V: Variant): WideString;
function VarToWideStrDef(const V: Variant; const ADefault: WideString): WideString;

function VarToDateTime(const V: Variant): TDateTime;
function VarFromDateTime(const DateTime: TDateTime): Variant;

function VarInRange(const AValue, AMin, AMax: Variant): Boolean;
function VarEnsureRange(const AValue, AMin, AMax: Variant): Variant;

type
  TVariantRelationship = (vrEqual, vrLessThan, vrGreaterThan, vrNotEqual);

function VarSameValue(const A, B: Variant): Boolean;
function VarCompareValue(const A, B: Variant): TVariantRelationship;

function VarIsEmptyParam(const V: Variant): Boolean;

function VarSupports(const V: Variant; const IID: TGUID; out Intf): Boolean; overload;
function VarSupports(const V: Variant; const IID: TGUID): Boolean; overload;

{ Variant copy support }

procedure VarCopyNoInd(var Dest: Variant; const Source: Variant);

{ Variant array support procedures and functions }

function VarIsArray(const A: Variant): Boolean; overload;
function VarIsArray(const A: Variant; AResolveByRef: Boolean): Boolean; overload;

function VarArrayCreate(const Bounds: array of Integer; AVarType: TVarType): Variant;
function VarArrayOf(const Values: array of Variant): Variant;

function VarArrayRef(const A: Variant): Variant;

function VarTypeIsValidArrayType(const AVarType: TVarType): Boolean;
function VarTypeIsValidElementType(const AVarType: TVarType): Boolean;

{ The following functions will handle normal variant arrays as well as
  variant arrays references by another variant using byref }

function VarArrayDimCount(const A: Variant): Integer;
function VarArrayLowBound(const A: Variant; Dim: Integer): Integer;
function VarArrayHighBound(const A: Variant; Dim: Integer): Integer;

function VarArrayLock(const A: Variant): Pointer;
procedure VarArrayUnlock(const A: Variant);

function VarArrayGet(const A: Variant; const Indices: array of Integer): Variant;
procedure VarArrayPut(var A: Variant; const Value: Variant; const Indices: array of Integer);

{ Variant <--> Dynamic Arrays }

procedure DynArrayToVariant(var V: Variant; const DynArray: Pointer; TypeInfo: Pointer);
procedure DynArrayFromVariant(var DynArray: Pointer; const V: Variant; TypeInfo: Pointer);

{ Global constants }

function Unassigned: Variant; // Unassigned standard constant
function Null: Variant;       // Null standard constant

var
  EmptyParam: OleVariant;    // "Empty parameter" standard constant which can be
  {$EXTERNALSYM EmptyParam}  // passed as an optional parameter on a dual
                             // interface.

{ Custom variant base class }

type
  TVarCompareResult = (crLessThan, crEqual, crGreaterThan);
  TCustomVariantType = class(TObject, IInterface)
  private
    FVarType: TVarType;
  protected
    { IInterface }
    function QueryInterface(const IID: TGUID; out Obj): HResult; virtual; stdcall;
    function _AddRef: Integer; stdcall;
    function _Release: Integer; stdcall;

    procedure SimplisticClear(var V: TVarData);
    procedure SimplisticCopy(var Dest: TVarData; const Source: TVarData;
      const Indirect: Boolean = False);

    procedure RaiseInvalidOp;
    procedure RaiseCastError;
    procedure RaiseDispError;

    function LeftPromotion(const V: TVarData; const Operator: TVarOp;
      out RequiredVarType: TVarType): Boolean; virtual;
    function RightPromotion(const V: TVarData; const Operator: TVarOp;
      out RequiredVarType: TVarType): Boolean; virtual;
    function OlePromotion(const V: TVarData;
      out RequiredVarType: TVarType): Boolean; virtual;
    procedure DispInvoke(var Dest: TVarData; const Source: TVarData;
      CallDesc: PCallDesc; Params: Pointer); virtual;

    procedure VarDataInit(var Dest: TVarData);
    procedure VarDataClear(var Dest: TVarData);

    procedure VarDataCopy(var Dest: TVarData; const Source: TVarData);
    procedure VarDataCopyNoInd(var Dest: TVarData; const Source: TVarData);

    procedure VarDataCast(var Dest: TVarData; const Source: TVarData);
    procedure VarDataCastTo(var Dest: TVarData; const Source: TVarData;
      const AVarType: TVarType); overload;
    procedure VarDataCastTo(var Dest: TVarData; const AVarType: TVarType); overload;
    procedure VarDataCastToOleStr(var Dest: TVarData);

    procedure VarDataFromStr(var V: TVarData; const Value: string);
    procedure VarDataFromOleStr(var V: TVarData; const Value: WideString);
    function VarDataToStr(const V: TVarData): string;

    function VarDataIsEmptyParam(const V: TVarData): Boolean;
    function VarDataIsByRef(const V: TVarData): Boolean;
    function VarDataIsArray(const V: TVarData): Boolean;

    function VarDataIsOrdinal(const V: TVarData): Boolean;
    function VarDataIsFloat(const V: TVarData): Boolean;
    function VarDataIsNumeric(const V: TVarData): Boolean;
    function VarDataIsStr(const V: TVarData): Boolean;
  public
    constructor Create; overload;
    constructor Create(RequestedVarType: TVarType); overload;
    destructor Destroy; override;
    property VarType: TVarType read FVarType;

    function IsClear(const V: TVarData): Boolean; virtual;
    procedure Cast(var Dest: TVarData; const Source: TVarData); virtual;
    procedure CastTo(var Dest: TVarData; const Source: TVarData;
      const AVarType: TVarType); virtual;
    procedure CastToOle(var Dest: TVarData; const Source: TVarData); virtual;

    // The following two procedures must be overridden by your custom
    //  variant type class.  Simplistic versions of Clear and Copy are
    //  available in the protected section of this class but depending on the
    //  type of data contained in your custom variant type those functions
    //  may not handle your situation.
    procedure Clear(var V: TVarData); virtual; abstract;
    procedure Copy(var Dest: TVarData; const Source: TVarData;
      const Indirect: Boolean); virtual; abstract;

    procedure BinaryOp(var Left: TVarData; const Right: TVarData;
      const Operator: TVarOp); virtual;
    procedure UnaryOp(var Right: TVarData; const Operator: TVarOp); virtual;
    function CompareOp(const Left, Right: TVarData;
      const Operator: TVarOp): Boolean; virtual;
    procedure Compare(const Left, Right: TVarData;
      var Relationship: TVarCompareResult); virtual;
  end;
  TCustomVariantTypeClass = class of TCustomVariantType;

  TVarDataArray = array of TVarData;
  IVarInvokeable = interface
    ['{1CB65C52-BBCB-41A6-9E58-7FB916BEEB2D}']
    function DoFunction(var Dest: TVarData; const V: TVarData;
      const Name: string; const Arguments: TVarDataArray): Boolean;
    function DoProcedure(const V: TVarData; const Name: string;
      const Arguments: TVarDataArray): Boolean;
    function GetProperty(var Dest: TVarData; const V: TVarData;
      const Name: string): Boolean;
    function SetProperty(const V: TVarData; const Name: string;
      const Value: TVarData): Boolean;
  end;

  TInvokeableVariantType = class(TCustomVariantType, IVarInvokeable)
  protected
    procedure DispInvoke(var Dest: TVarData; const Source: TVarData;
      CallDesc: PCallDesc; Params: Pointer); override;
  public
    { IVarInvokeable }
    function DoFunction(var Dest: TVarData; const V: TVarData;
      const Name: string; const Arguments: TVarDataArray): Boolean; virtual;
    function DoProcedure(const V: TVarData; const Name: string;
      const Arguments: TVarDataArray): Boolean; virtual;
    function GetProperty(var Dest: TVarData; const V: TVarData;
      const Name: string): Boolean; virtual;
    function SetProperty(const V: TVarData; const Name: string;
      const Value: TVarData): Boolean; virtual;
  end;

  IVarInstanceReference = interface
    ['{5C176802-3F89-428D-850E-9F54F50C2293}']
    function GetInstance(const V: TVarData): TObject;
  end;

  function FindCustomVariantType(const AVarType: TVarType;
    out CustomVariantType: TCustomVariantType): Boolean; overload;
  function FindCustomVariantType(const TypeName: string;
    out CustomVariantType: TCustomVariantType): Boolean; overload;

type
  TAnyProc = procedure (var V: TVarData);
  TVarDispProc = procedure (Dest: PVariant; const Source: Variant;
      CallDesc: PCallDesc; Params: Pointer); cdecl;

var
  VarDispProc: TVarDispProc;
  ClearAnyProc: TAnyProc;  { Handler clearing a varAny }
  ChangeAnyProc: TAnyProc; { Handler to change any to variant }
  RefAnyProc: TAnyProc;    { Handler to add a reference to an varAny }

implementation

uses
  SysConst, SysUtils, VarUtils;

{ ----------------------------------------------------- }
{       Variant support                                 }
{ ----------------------------------------------------- }

var
  GVariantManager,
  GOldVariantManager: TVariantManager;

type
  TBaseType = (btErr, btNul, btInt, btFlt, btCur, btStr, btBol, btDat, btI64);

const
  varLast = varInt64;

const
  VarDataInline: array [varEmpty..varLast] of Boolean =
    (True,  // varEmpty
     True,  // varNull
     True,  // varSmallInt
     True,  // varInteger
     True,  // varSingle
     True,  // varDouble
     True,  // varCurrency
     True,  // varDate
     False, // varOleStr
     False, // varDispatch
     True,  // varError
     True,  // varBoolean
     False, // varVariant
     False, // varUnknown
     False, // (unused, varDecimal)
     False, // (unused, undefined)
     True,  // varShortInt
     True,  // varByte
     True,  // varWord
     True,  // varLongWord
     True); // varInt64

const
  BaseTypeMap: array[0..varLast] of TBaseType = (
    btErr,  { varEmpty    }
    btNul,  { varNull     }
    btInt,  { varSmallint }
    btInt,  { varInteger  }
    btFlt,  { varSingle   }
    btFlt,  { varDouble   }
    btCur,  { varCurrency }
    btDat,  { varDate     }
    btStr,  { varOleStr   }
    btErr,  { varDispatch }
    btErr,  { varError    }
    btBol,  { varBoolean  }
    btErr,  { varVariant  }
    btErr,  { varUnknown  }
    btErr,  { vt_decimal  }
    btErr,  { undefined   }
    btInt,  { varShortInt }
    btInt,  { varByte     }
    btI64,  { varWord     }
    btI64,  { varLongWord }
    btI64); { varInt64    }

const
  OpTypeMap: array[TBaseType, TBaseType] of TBaseType = (
         {btErr, btNul, btInt, vtFlt, btCur, btStr, btBol, btDat, btI64}
  {btErr}(btErr, btErr, btErr, btErr, btErr, btErr, btErr, btErr, btErr),
  {btNul}(btErr, btNul, btNul, btNul, btNul, btNul, btNul, btNul, btNul),
  {btInt}(btErr, btNul, btInt, btFlt, btCur, btFlt, btInt, btDat, btI64),
  {btFlt}(btErr, btNul, btFlt, btFlt, btCur, btFlt, btFlt, btDat, btFlt),
  {btCur}(btErr, btNul, btCur, btCur, btCur, btCur, btCur, btDat, btCur),
  {btStr}(btErr, btNul, btFlt, btFlt, btCur, btStr, btBol, btDat, btFlt),
  {btBol}(btErr, btNul, btInt, btFlt, btCur, btBol, btBol, btDat, btI64),
  {btDat}(btErr, btNul, btDat, btDat, btDat, btDat, btDat, btDat, btDat),
  {btI64}(btErr, btNul, btI64, btFlt, btCur, btFlt, btI64, btDat, btI64));

{ TCustomVariantType support }
{ Currently we have reserve room for 1791 ($6FF) custom types.  But, since the
  first sixteen are reserved, we actually only have room for 1775 ($6EF) types. }
const
  CMaxNumberOfCustomVarTypes = $06FF;
  CMinVarType = $0100;
  CMaxVarType = CMinVarType + CMaxNumberOfCustomVarTypes;
  CIncVarType = $000F;
  CFirstUserType = CMinVarType + CIncVarType;
  CInvalidCustomVariantType: TCustomVariantType = TCustomVariantType($FFFFFFFF);

procedure _DispInvokeError;
asm
        MOV     AL,System.reVarDispatch
        JMP     System.Error
end;

procedure VarCastError;
asm
        MOV     AL,System.reVarTypeCast
        JMP     System.Error
end;

procedure VarInvalidOp;
asm
        MOV     AL,System.reVarInvalidOp
        JMP     System.Error
end;

procedure _VarInit(var V: TVarData);
begin
  VariantInit(V);
end;

procedure _VarClear(var V: TVarData); forward;

procedure VarArrayClear(var V: TVarData);
var
  LArrayRef: PVarArray;
  I, LUpperBound, LDimCount: Integer;
  LVarBounds: array[0..63] of TVarArrayBound;
  LVarPoint: array[0..63] of Integer;
  LVarData: Pointer;

  // this support function handles bounds checking
  function InBounds(At: Integer): Boolean;
  begin
    Result := LVarPoint[At] < LVarBounds[At].LowBound + LVarBounds[At].ElementCount;
  end;

  // this support function handles the coordinate navigation dealing with
  //   bounds wrapping and the like
  function Increment(At: Integer): Boolean;
  begin
    Result := True;
    Inc(LVarPoint[At]);
    if LVarPoint[At] >= LVarBounds[At].LowBound + LVarBounds[At].ElementCount then
      if At = 0 then
        Result := False
      else
      begin
        LVarPoint[At] := LVarBounds[At].LowBound;
        Result := Increment(At - 1);
      end;
  end;
begin
  // sanity test
  if not VarTypeIsValidElementType(V.VType) then
    VarInvalidOp;

  // if the array contains variants then we need to preprocess them
  if (V.VType and varTypeMask) = varVariant then
  begin

    // where is the array reference?
    if (V.VType and varByRef) <> 0 then
      LArrayRef := PVarArray(V.VPointer^)
    else
      LArrayRef := V.VArray;

    // figure our bounds
    LDimCount := LArrayRef^.DimCount;
    for I := 0 to LDimCount - 1 do
      with LVarBounds[I] do
      begin
        if SafeArrayGetLBound(LArrayRef, I + 1, LowBound) <> VAR_OK then
          System.Error(System.reVarArrayBounds);
        if SafeArrayGetUBound(LArrayRef, I + 1, LUpperBound) <> VAR_OK then
          System.Error(System.reVarArrayBounds);
        ElementCount := LUpperBound - LowBound + 1;
      end;

    // initialize the coordinates
    for I := 0 to LDimCount - 1 do
      LVarPoint[I] := LVarBounds[I].LowBound;

    // find the variant at the current coordinate
    repeat
      if InBounds(LDimCount) then
      begin
        if SafeArrayPtrOfIndex(LArrayRef, PVarArrayCoorArray(@LVarPoint), LVarData) <> VAR_OK then
          System.Error(System.reVarArrayBounds);

        // clear the variant
        _VarClear(PVarData(LVarData)^);
      end;
    until not Increment(LDimCount - 1);
  end;

  // finish the clear
  VariantClear(V);
end;

procedure VarClearRare(var V: TVarData);
var
  LHandler: TCustomVariantType;
begin

  // let CORBA deal with its own type
  if V.VType = varAny then
    ClearAnyProc(V)

  // ok, finally is it a custom variant type?
  else if FindCustomVariantType(V.VType, LHandler) then
    LHandler.Clear(V)
  else
    VarInvalidOp;
end;

procedure _VarClear(var V: TVarData);
begin
  // only if there is something to do
  if V.VType <> varEmpty then

    // byrefs and those inline data types are easy
    if ((V.VType <= High(VarDataInline)) and VarDataInline[V.VType]) or ((V.VType and varByRef) <> 0) then
      V.VType := varEmpty

    // custom handle the arrays
    else if (V.VType and varArray) <> 0 then
      VarArrayClear(V)

    // clear the pascal string correctly for reference counting
    else if V.VType = varString then
    begin
      V.VType := varEmpty;
      String(V.VString) := '';
    end

    // quick test for the simple ones
    else if (V.VType <= varLast) then
      VariantClear(V)

    // more to do?
    else
      VarClearRare(V);
end;

procedure _DispInvoke(Dest: PVarData; const Source: TVarData;
  CallDesc: PCallDesc; Params: Pointer); cdecl;
var
  LHandler: TCustomVariantType;
begin
  // clear the destination but only if it filled and is not pointing to the source
  if (Dest <> nil) and (Dest <> @Source) then
    _VarClear(Dest^);

  // we only do this if it is one of those special types
  case Source.VType and varTypeMask of
    varDispatch, varUnknown, varAny:
      if Assigned(VarDispProc) then
        VarDispProc(PVariant(Dest), Variant(Source), CallDesc, @Params);
  else
    // finally check to see if is one of those custom types
    if FindCustomVariantType(Source.VType, LHandler) then
      LHandler.DispInvoke(Dest^, Source, CallDesc, @Params)
    else
      VarInvalidOp;
  end;
end;

type
  TVarArrayForEach = procedure(var Dest: TVarData; const Src: TVarData);

procedure VarArrayCopyForEach(var Dest: TVarData; const Src: TVarData; AProc: TVarArrayForEach);
var
  I, LUBound, LDimCount: Integer;
  LVarArrayRef, LSrcArrayRef: PVarArray;
  LVarBounds: array[0..63] of TVarArrayBound;
  LVarPoint: array[0..63] of Integer;
  PFrom, PTo: Pointer;

  // this support function handles bounds checking
  function InBounds(At: Integer): Boolean;
  begin
    Result := LVarPoint[At] < LVarBounds[At].LowBound + LVarBounds[At].ElementCount;
  end;

  // this support function handles the coordinate navigation dealing with
  //   bounds wrapping and the like
  function Increment(At: Integer): Boolean;
  begin
    Result := True;
    Inc(LVarPoint[At]);
    if LVarPoint[At] >= LVarBounds[At].LowBound + LVarBounds[At].ElementCount then
      if At = 0 then
        Result := False
      else
      begin
        LVarPoint[At] := LVarBounds[At].LowBound;
        Result := Increment(At - 1);
      end;
  end;
begin
  // sanity test
  if not VarTypeIsValidElementType(Src.VType) then
    VarInvalidOp;

  // if the array contains variants then we need to handle the copy ourself
  if (Src.VType and varTypeMask) = varVariant then
  begin

    // where is the array reference
    if (TVarData(Src).VType and varByRef) <> 0 then
      LSrcArrayRef := PVarArray(TVarData(Src).VPointer^)
    else
      LSrcArrayRef := TVarData(Src).VArray;

    // figure our bounds
    LDimCount := LSrcArrayRef^.DimCount;
    for I := 0 to LDimCount - 1 do
      with LVarBounds[I] do
      begin
        if SafeArrayGetLBound(LSrcArrayRef, I + 1, LowBound) <> VAR_OK then
          System.Error(System.reVarArrayBounds);
        if SafeArrayGetUBound(LSrcArrayRef, I + 1, LUBound) <> VAR_OK then
          System.Error(System.reVarArrayBounds);
        ElementCount := LUBound - LowBound + 1;
      end;

    // create a destination array
    LVarArrayRef := SafeArrayCreate(varVariant, LDimCount, PVarArrayBoundArray(@LVarBounds)^);
    if LVarArrayRef = nil then
      System.Error(System.reVarArrayCreate);

    // set up the destination variant
    _VarClear(Dest);
    Dest.VType := varVariant or varArray;
    Dest.VArray := LVarArrayRef;

    // initialize the coordinates
    for I := 0 to LDimCount - 1 do
      LVarPoint[I] := LVarBounds[I].LowBound;

    // find the variant at the current coordinate
    repeat
      if InBounds(LDimCount - 1) then
      begin
        if SafeArrayPtrOfIndex(LSrcArrayRef, PVarArrayCoorArray(@LVarPoint), PFrom) <> VAR_OK then
          System.Error(System.reVarArrayBounds);
        if SafeArrayPtrOfIndex(LVarArrayRef, PVarArrayCoorArray(@LVarPoint), PTo) <> VAR_OK then
          System.Error(System.reVarArrayBounds);

        // call the proc
        AProc(PVarData(PTo)^, PVarData(PFrom)^);
      end;
    until not Increment(LDimCount - 1);
  end

  // array of something other than a variant, just copy it
  else
    VariantCopy(Dest, Src);
end;

procedure VarArrayCopyProc(var Dest: TVarData; const Src: TVarData);
begin
  Variant(Dest) := Variant(Src);
end;

procedure VarCopyCommon(var Dest: TVarData; const Source: TVarData; Indirect: Boolean);
var
  LSourceHandler: TCustomVariantType;
begin
  if @Dest = @Source then
    Exit;

  // clear the destination
  _VarClear(Dest);

  // copy the source
  //   inline types can simply be block copied
  if ((Source.VType <= High(VarDataInline)) and VarDataInline[Source.VType]) or ((Source.VType and varByRef) <> 0) then
    Dest := Source

  // Pascal string
  else if Source.VType = varString then
  begin
    Dest.VType := varString;
    Dest.VString := nil;
    String(Dest.VString) := String(Source.VString);
  end

  // CORBA Any
  else if Source.VType = varAny then
  begin
    Dest := Source; // block copy
    RefAnyProc(Dest);
  end

  // something custom?
  else if FindCustomVariantType(Source.VType, LSourceHandler) then
    LSourceHandler.Copy(Dest, Source, Indirect)

  // else let the OS deal with it
  else if Indirect then
  begin

    // if its an array then we need to manually copy it
    if Source.VType and varArray <> 0 then
      VarArrayCopyForEach(Dest, Source, VarArrayCopyProc)

    // else regular OS copy will do
    else if VariantCopyInd(Dest, Source) <> VAR_OK then
      VarInvalidOp;
  end
  else
    // if its an array and not by ref then we need to manually copy it
    if (Source.VType and varArray <> 0) and
       (Source.VType and varByRef = 0) then
      VarArrayCopyForEach(Dest, Source, VarArrayCopyProc)

    // else reqular OS copy will do
    else
      VariantCopy(Dest, Source);
end;

procedure _VarCopy(var Dest: TVarData; const Source: TVarData);
asm
        MOV     CL, 0
        JMP     VarCopyCommon
end;

procedure VarCopyNoInd(var Dest: Variant; const Source: Variant);
asm
        MOV     CL, 1
        JMP     VarCopyCommon
end;

procedure _VarFromWStr(var V: TVarData; const Value: WideString); forward;

procedure VarInt64FromVar(var Dest: TVarData; const Source: TVarData;
  ADestType: TVarType);
begin
  case Source.VType of
    varEmpty:;
    varSmallInt: Dest.VInt64 := Source.VSmallInt;
    varInteger:  Dest.VInt64 := Source.VInteger;
    varSingle:   Dest.VInt64 := Round(Source.VSingle);
    varDouble:   Dest.VInt64 := Round(Source.VDouble);
    varCurrency: Dest.VInt64 := Round(Source.VCurrency);
    varDate:     Dest.VInt64 := Round(Source.VDate);
    varOleStr:   Dest.VInt64 := StrToInt64(Source.VOleStr);
    varBoolean:  Dest.VInt64 := SmallInt(Source.VBoolean);
    varShortInt: Dest.VInt64 := Source.VShortInt;
    varByte:     Dest.VInt64 := Source.VByte;
    varWord:     Dest.VInt64 := Source.VWord;
    varLongWord: Dest.VInt64 := Source.VLongWord;
    varInt64:    Dest.VInt64 := Source.VInt64;
  else
    VarCastError;
  end;
  Dest.VType := ADestType;
end;

procedure VarInt64ToVar(var Dest: TVarData; const Source: TVarData;
  ADestType: TVarType);
begin
  case ADestType of
    varEmpty:;
    varSmallInt: Dest.VSmallInt := Source.VInt64;
    varInteger:  Dest.VInteger := Source.VInt64;
    varSingle:   Dest.VSingle := Source.VInt64;
    varDouble:   Dest.VDouble := Source.VInt64;
    varCurrency: Dest.VCurrency := Source.VInt64;
    varDate:     Dest.VDate := Source.VInt64;
    varOleStr:   _VarFromWStr(Dest, IntToStr(Source.VInt64));
    varBoolean:  Dest.VBoolean := Source.VInt64 <> 0;
    varShortInt: Dest.VShortInt := Source.VInt64;
    varByte:     Dest.VByte := Source.VInt64;
    varWord:     Dest.VWord := Source.VInt64;
    varLongWord: Dest.VLongWord := Source.VInt64;
    varInt64:    Dest.VInt64 := Source.VInt64;
  else
    VarCastError;
  end;
  Dest.VType := ADestType;
end;

procedure _VarOleStrToString(var Dest: TVarData; const Source: TVarData);
var
  LStringPtr: Pointer;
begin
  LStringPtr := nil;
  OleStrToStrVar(Source.VOleStr, string(LStringPtr));
  _VarClear(Dest);
  Dest.VType := varString;
  Dest.VString := LStringPtr;
end;

procedure VarOleStrToString(var Dest: Variant; const Source: Variant);
asm
        JMP     _VarOleStrToString
end;

procedure _VarStringToOleStr(var Dest: TVarData; const Source: TVarData);
var
  OleStrPtr: PWideChar;
begin
  OleStrPtr := StringToOleStr(string(Source.VString));
  _VarClear(Dest);
  Dest.VType := varOleStr;
  Dest.VOleStr := OleStrPtr;
end;

procedure VarStringToOleStr(var Dest: Variant; const Source: Variant);
asm
        JMP    _VarStringToOleStr
end;

procedure VarChangeType(var Dest: TVarData; const Source: TVarData;
  ADestType: TVarType);

  procedure AnyChangeType(var Dest: TVarData; const Source: TVarData;
    ADestType: TVarType);
  var
    LTemp: TVarData;
  begin
    _VarInit(LTemp);
    try
      _VarCopy(LTemp, Source);
      ChangeAnyProc(LTemp);
      VarChangeType(Dest, LTemp, ADestType);
    finally
      _VarClear(LTemp);
    end;
  end;

  procedure SimpleVarChangeType(var Dest: TVarData; const Source: TVarData;
    ADestType: TVarType);
  begin
    _VarClear(Dest);
    if Source.VType = varInt64 then
      VarInt64ToVar(Dest, Source, ADestType)
    else if ADestType = varInt64 then
      VarInt64FromVar(Dest, Source, ADestType)
    else if VariantChangeTypeEx(Dest, Source, $400, 0, ADestType) <> 0 then
      VarCastError;
  end;

var
  LTemp: TVarData;
  LSourcePtr: PVarData;
  LHandler: TCustomVariantType;
begin
  // do we use a temp or not?
  if @Dest = @Source then
  begin
    _VarInit(LTemp);
    _VarCopy(LTemp, Source);
    LSourcePtr := @LTemp;
  end
  else
    LSourcePtr := @Source;

  // do the actual conversion
  try

    // same type?
    if LSourcePtr^.VType = ADestType then

      // but are they the same?
      if @Dest = @Source then
        Exit

      // else just copy them
      else
        _VarCopy(Dest, LSourcePtr^)

    // source is simple and dest is..
    else if LSourcePtr^.VType <= varLast then

      // ..simple
      if ADestType <= varLast then
        SimpleVarChangeType(Dest, LSourcePtr^, ADestType)

      // ..pascal string
      else if ADestType = varString then
      begin
        SimpleVarChangeType(Dest, LSourcePtr^, varOleStr);
        _VarOleStrToString(Dest, Dest);
      end

      // ..CORBA any
      else if ADestType = varAny then
        VarCastError

      // ..array of
      else if (ADestType and varArray) <> 0 then

        // ..simple
        if (ADestType and varTypeMask) <= varLast then
          SimpleVarChangeType(Dest, LSourcePtr^, ADestType)

        // failure
        else
          VarCastError

      // ..custom type
      else if FindCustomVariantType(ADestType, LHandler) then
        LHandler.Cast(Dest, LSourcePtr^)

      // failure
      else
        VarCastError

    // source is string and dest is..
    else if LSourcePtr^.VType = varString then
    begin

      // convert over to a olestring
      _VarStringToOleStr(Dest, LSourcePtr^);

      // redo change type if needed
      if ADestType <> varOleStr then
        VarChangeType(Dest, Dest, ADestType);
    end

    // source is an any and dest is..
    else if LSourcePtr.VType = varAny then
      AnyChangeType(Dest, LSourcePtr^, ADestType)

    // source is an array and dest is..
    else if (LSourcePtr^.VType and varArray) <> 0 then

      // ..simple
      if ADestType <= varLast then
        SimpleVarChangeType(Dest, LSourcePtr^, ADestType)

      // ..pascal string
      else if ADestType = varString then
      begin
        SimpleVarChangeType(Dest, LSourcePtr^, varOleStr);
        _VarOleStrToString(Dest, Dest);
      end

      // failure
      else
        VarCastError

    // source is custom..
    else if FindCustomVariantType(LSourcePtr^.VType, LHandler) then
      LHandler.CastTo(Dest, LSourcePtr^, ADestType)

    // source is something else
    else
      SimpleVarChangeType(Dest, LSourcePtr^, ADestType);

  // if we copied the source then clear the tempory
  finally
    if LSourcePtr = @LTemp then
      _VarClear(LTemp);
  end;
end;

procedure _VarCast(var Dest: TVarData; const Source: TVarData; AVarType: Integer);
begin
  VarChangeType(Dest, Source, AVarType);
end;

(* VarCast when the destination is OleVariant *)
procedure _VarCastOle(var Dest: TVarData; const Source: TVarData; AVarType: Integer);
var
  LHandler: TCustomVariantType;
begin
  if (AVarType = varString) or (AVarType = varAny) then
    VarCastError
  else if FindCustomVariantType(Source.VType, LHandler) then
    LHandler.CastTo(Dest, Source, AVarType)
  else
    VarChangeType(Dest, Source, AVarType);
end;

function _VarToInt(const V: TVarData): Integer;
var
  LTemp: TVarData;
begin
  case V.VType of
    varInteger  : Result := V.VInteger;
    varSmallInt : Result := V.VSmallInt;
    varByte     : Result := V.VByte;
    varShortInt : Result := V.VShortInt;
    varWord     : Result := V.VWord;
    varLongWord : Result := V.VLongWord;
    varInt64    : Result := V.VInt64;

    varDouble   : Result := Round(V.VDouble);
    varSingle   : Result := Round(V.VSingle);
    varCurrency : Result := Round(V.VCurrency);
    varDate     : Result := Round(V.VDate);

    varString   : Result := StrToInt(String(V.VString));
    varOleStr   : Result := StrToInt(String(V.VOleStr));
  else
    _VarInit(LTemp);
    VarChangeType(LTemp, V, varInteger);
    Result := LTemp.VInteger;
  end;
end;

function _VarToInt64(const V: TVarData): Int64;
var
  LTemp: TVarData;
begin
  case V.VType of
    varInteger  : Result := V.VInteger;
    varSmallInt : Result := V.VSmallInt;
    varByte     : Result := V.VByte;
    varShortInt : Result := V.VShortInt;
    varWord     : Result := V.VWord;
    varLongWord : Result := V.VLongWord;
    varInt64    : Result := V.VInt64;

    varDouble   : Result := Round(V.VDouble);
    varSingle   : Result := Round(V.VSingle);
    varCurrency : Result := Round(V.VCurrency);
    varDate     : Result := Round(V.VDate);

    varString   : Result := StrToInt64(String(V.VString));
    varOleStr   : Result := StrToInt64(String(V.VOleStr));
  else
    _VarInit(LTemp);
    VarChangeType(LTemp, V, varInt64);
    Result := LTemp.VInt64;
  end;
end;

function _VarToBool(const V: TVarData): Boolean;
var
  LTemp: TVarData;
begin
  case V.VType of
    varBoolean  : Result := V.VBoolean;
    varInteger  : Result := V.VInteger <> 0;
    varSmallInt : Result := V.VSmallInt <> 0;
    varByte     : Result := V.VByte <> 0;
    varShortInt : Result := V.VShortInt <> 0;
    varWord     : Result := V.VWord <> 0;
    varLongWord : Result := V.VLongWord <> 0;
    varInt64    : Result := V.VInt64 <> 0;

    varDouble   : Result := V.VDouble <> 0;
    varSingle   : Result := V.VSingle <> 0;
    varCurrency : Result := V.VCurrency <> 0;
    varDate     : Result := V.VDate <> 0;

    varString   : Result := StrToBool(String(V.VString));
    varOleStr   : Result := StrToBool(String(V.VOleStr));
  else
    _VarInit(LTemp);
    VarChangeType(LTemp, V, varBoolean);
    Result := LTemp.VBoolean;
  end;
end;

function _VarToReal(const V: TVarData): Extended;
var
  LTemp: TVarData;
begin
  case V.VType of
    varInteger  : Result := V.VInteger;
    varSmallInt : Result := V.VSmallInt;
    varByte     : Result := V.VByte;
    varShortInt : Result := V.VShortInt;
    varWord     : Result := V.VWord;
    varLongWord : Result := V.VLongWord;
    varInt64    : Result := V.VInt64;

    varDouble   : Result := V.VDouble;
    varSingle   : Result := V.VSingle;
    varCurrency : Result := V.VCurrency;
    varDate     : Result := V.VDate;

    varString   : Result := StrToFloat(String(V.VString));
    varOleStr   : Result := StrToFloat(String(V.VOleStr));
  else
    _VarInit(LTemp);
    VarChangeType(LTemp, V, varDouble);
    Result := LTemp.VDouble;
  end;
end;

function _VarToDate(const V: TVarData): TDateTime;
var
  LTemp: TVarData;
begin
  case V.VType of
    varInteger  : Result := V.VInteger;
    varSmallInt : Result := V.VSmallInt;
    varByte     : Result := V.VByte;
    varShortInt : Result := V.VShortInt;
    varWord     : Result := V.VWord;
    varLongWord : Result := V.VLongWord;
    varInt64    : Result := V.VInt64;

    varDouble   : Result := V.VDouble;
    varSingle   : Result := V.VSingle;
    varCurrency : Result := V.VCurrency;
    varDate     : Result := V.VDate;

    varString   : Result := StrToDateTime(String(V.VString));
    varOleStr   : Result := StrToDateTime(String(V.VOleStr));
  else
    _VarInit(LTemp);
    VarChangeType(LTemp, V, varDate);
    Result := LTemp.VDouble;
  end;
end;

function _VarToCurr(const V: TVarData): Currency;
var
  LTemp: TVarData;
begin
  case V.VType of
    varInteger  : Result := V.VInteger;
    varSmallInt : Result := V.VSmallInt;
    varByte     : Result := V.VByte;
    varShortInt : Result := V.VShortInt;
    varWord     : Result := V.VWord;
    varLongWord : Result := V.VLongWord;
    varInt64    : Result := V.VInt64;

    varDouble   : Result := V.VDouble;
    varSingle   : Result := V.VSingle;
    varCurrency : Result := V.VCurrency;
    varDate     : Result := V.VDate;

    varString   : Result := StrToCurr(String(V.VString));
    varOleStr   : Result := StrToCurr(String(V.VOleStr));
  else
    _VarInit(LTemp);
    VarChangeType(LTemp, V, varCurrency);
    Result := LTemp.VCurrency;
  end;
end;

procedure _VarToLStr(var S: string; const V: TVarData);
var
  LTemp: TVarData;
begin
  case V.VType of
    varString   : S := String(V.VString);
    varOleStr   : S := String(V.VOleStr);

    varBoolean  : S := BoolToStr(V.VBoolean, True);
    varInteger  : S := IntToStr(V.VInteger);
    varSmallInt : S := IntToStr(V.VSmallInt);
    varByte     : S := IntToStr(V.VByte);
    varShortInt : S := IntToStr(V.VShortInt);
    varWord     : S := IntToStr(V.VWord);
    varLongWord : S := IntToStr(V.VLongWord);
    varInt64    : S := IntToStr(V.VInt64);

    varDouble   : S := FloatToStr(V.VDouble);
    varSingle   : S := FloatToStr(V.VSingle);
    varCurrency : S := CurrToStr(V.VCurrency);
    varDate     : S := DateTimeToStr(V.VDate);
  else
    _VarInit(LTemp);
    try
      VarChangeType(LTemp, V, varString);
      S := String(LTemp.VString);
    finally
      String(LTemp.VString) := '';
    end;
  end;
end;

procedure _VarToPStr(var S; const V: TVarData);
var
  LTemp: string;
begin
  _VarToLStr(LTemp, V);
  ShortString(S) := LTemp;
end;

procedure _VarToWStr(var S: WideString; const V: TVarData);
var
  LTemp: TVarData;
begin
  case V.VType of
    varOleStr   : S := WideString(V.VOleStr);
    varString   : S := WideString(String(V.VString));

    varBoolean  : S := WideString(BoolToStr(V.VBoolean, True));
    varInteger  : S := WideString(IntToStr(V.VInteger));
    varSmallInt : S := WideString(IntToStr(V.VSmallInt));
    varByte     : S := WideString(IntToStr(V.VByte));
    varShortInt : S := WideString(IntToStr(V.VShortInt));
    varWord     : S := WideString(IntToStr(V.VWord));
    varLongWord : S := WideString(IntToStr(V.VLongWord));
    varInt64    : S := WideString(IntToStr(V.VInt64));

    varDouble   : S := WideString(FloatToStr(V.VDouble));
    varSingle   : S := WideString(FloatToStr(V.VSingle));
    varCurrency : S := WideString(CurrToStr(V.VCurrency));
    varDate     : S := WideString(DateTimeToStr(V.VDate));
  else
    _VarInit(LTemp);
    try
      VarChangeType(LTemp, V, varOleStr);
      S := WideString(LTemp.VOleStr);
    finally
      VariantClear(LTemp);
    end;
  end;
end;

procedure AnyToIntf(var Intf: IInterface; const V: TVarData);
var
  LTemp: TVarData;
begin
  _VarInit(LTemp);
  try
    _VarCopy(LTemp, V);
    ChangeAnyProc(LTemp);
    if LTemp.VType <> varUnknown then
      VarCastError;
    Intf := IInterface(LTemp.VUnknown);
  finally
    VariantClear(LTemp);
  end;
end;

procedure _VarToIntf(var Intf: IInterface; const V: TVarData);
var
  LHandler: TCustomVariantType;
begin
  case V.VType of
    varEmpty                : Intf := nil;
    varUnknown,
    varDispatch             : Intf := IInterface(V.VUnknown);
    varUnknown + varByRef,
    varDispatch + varByRef  : Intf := IInterface(V.VPointer^);
    varAny                  : AnyToIntf(Intf, V);
  else
    if not (FindCustomVariantType(V.VType, LHandler) and
            LHandler.GetInterface(IInterface, Intf)) then
      VarCastError;
  end;
end;

procedure _VarToDisp(var Dispatch: IDispatch; const V: TVarData);
var
  LHandler: TCustomVariantType;
begin
  case V.VType of
    varEmpty             : Dispatch := nil;
    varDispatch          : Dispatch := IDispatch(V.VDispatch);
    varDispatch+varByRef : Dispatch := IDispatch(V.VPointer^);
  else
    if not (FindCustomVariantType(V.VType, LHandler) and
            LHandler.GetInterface(IDispatch, Dispatch)) then
      VarCastError;
  end;
end;

procedure _VarToDynArray(var DynArray: Pointer; const V: TVarData; TypeInfo: Pointer);
asm
        CALL    DynArrayFromVariant
        OR      EAX, EAX
        JNZ     @@1
        JMP     VarCastError
@@1:
end;

{ pardon the strange case format but it lays out better this way }
procedure _VarFromInt(var V: TVarData; const Value, Range: Integer);
begin
  _VarClear(V);
  case ShortInt(Range) of
   {-4: varInteger is handled by the else clause }
    -2: begin V.VType := varSmallInt;  V.VSmallInt := Value; end;
    -1: begin V.VType := varShortInt;  V.VShortInt := Value; end;
     1: begin V.VType := varByte;      V.VByte := Value;     end;
     2: begin V.VType := varWord;      V.VWord := Value;     end;
     4: begin V.VType := varLongWord;  V.VLongWord := Value; end;
    -3, 0, 3: V.VType := varError;
  else
    V.VType := varInteger; V.VInteger := Value;
  end;
end;

procedure _OleVarFromInt(var V: TVarData; const Value, Range: Integer);
begin
  _VarClear(V);
  V.VType := varInteger;
  V.VInteger := Value;
end;

procedure _VarFromInt64(var V: TVarData; const Value: Int64);
begin
  _VarClear(V);
  V.VType := varInt64;
  V.VInt64 := Value;
end;

procedure _VarFromBool(var V: TVarData; const Value: Boolean);
begin
  _VarClear(V);
  V.VType := varBoolean;
  V.VBoolean := Value;
end;

procedure _VarFromReal; // var V: Variant; const Value: Real
asm
        PUSH    EAX
        CALL    _VarClear
        POP     EAX
        MOV     [EAX].TVarData.VType,varDouble
        FSTP    [EAX].TVarData.VDouble
        FWAIT
end;

procedure _VarFromTDateTime; // var V: Variant; const Value: TDateTime
asm
        PUSH    EAX
        CALL    _VarClear
        POP     EAX
        MOV     [EAX].TVarData.VType,varDate
        FSTP    [EAX].TVarData.VDouble
        FWAIT
end;

procedure _VarFromCurr; // var V: Variant; const Value: Currency
asm
        PUSH    EAX
        CALL    _VarClear
        POP     EAX
        MOV     [EAX].TVarData.VType,varCurrency
        FISTP   [EAX].TVarData.VCurrency
        FWAIT
end;

procedure _VarFromLStr(var V: TVarData; const Value: string);
begin
  _VarClear(V);
  V.VString := nil;
  V.VType := varString;
  String(V.VString) := Value;
end;

procedure _VarFromPStr(var V: TVarData; const Value: ShortString);
begin
  _VarFromLStr(V, Value);
end;

procedure _VarFromWStr(var V: TVarData; const Value: WideString);
begin
  _VarClear(V);
  V.VOleStr := nil;
  V.VType := varOleStr;
  WideString(Pointer(V.VOleStr)) := Copy(Value, 1, MaxInt);
end;

procedure _VarFromIntf(var V: TVarData; const Value: IInterface);
begin
  _VarClear(V);
  V.VUnknown := nil;
  V.VType := varUnknown;
  IInterface(V.VUnknown) := Value;
end;

procedure _VarFromDisp(var V: TVarData; const Value: IDispatch);
begin
  _VarClear(V);
  V.VDispatch := nil;
  V.VType := varDispatch;
  IInterface(V.VDispatch) := Value;
end;

procedure _VarFromDynArray(var V: TVarData; const DynArray: Pointer; TypeInfo: Pointer);
asm
        PUSH    EAX
        CALL    DynArrayToVariant
        POP     EAX
        CMP     [EAX].TVarData.VType,varEmpty
        JNE     @@1
        JMP     VarCastError
@@1:
end;

procedure _OleVarFromLStr(var V: TVarData {OleVariant}; const Value: string);
begin
  _VarFromWStr(V, WideString(Value));
end;

procedure _OleVarFromPStr(var V: TVarData {OleVariant}; const Value: ShortString);
begin
  _OleVarFromLStr(V, Value);
end;

procedure _OleVarFromVar(var Dest: TVarData {OleVariant}; const Source: TVarData); forward;

procedure OleVarFromAny(var V: TVarData {OleVariant}; const Value: TVarData);
var
  LTemp: TVarData;
begin
  _VarInit(LTemp);
  try
    _VarCopy(LTemp, Value);
    ChangeAnyProc(LTemp);
    _VarCopy(V, LTemp);
  finally
    _VarClear(LTemp);
  end;
end;

procedure OleVarFromVarArrayProc(var Dest: TVarData; const Src: TVarData);
begin
  _OleVarFromVar(Dest, Src);
end;

procedure _OleVarFromVar(var Dest: TVarData {OleVariant}; const Source: TVarData);
var
  LHandler: TCustomVariantType;
begin
  // convert the special Delphish types over to their OLE equiv
  case Source.VType of
    varShortInt, varByte, varWord:
      VarChangeType(Dest, Source, varInteger);
    varLongWord:
      if Source.VLongWord <= Cardinal(High(Integer)) then
        VarChangeType(Dest, Source, varInteger)
      else
        VarChangeType(Dest, Source, varDouble);
    varInt64:
      if (Source.VInt64 <= High(Integer)) and
         (Source.VInt64 >= Low(Integer)) then
        VarChangeType(Dest, Source, varInteger)
      else
        VarChangeType(Dest, Source, varDouble);
    varString:
      _OleVarFromLStr(Dest, String(Source.VString));
    varAny:
      OleVarFromAny(Dest, Source);
  else

    // do our own special handling for arrays
    if (Source.VType and varArray) <> 0 then
      VarArrayCopyForEach(Dest, Source, OleVarFromVarArrayProc)

    // if the base type of the variant is not custom then let the OS deal with it
    else if (Source.VType and varTypeMask) < CFirstUserType then
      _VarCopy(Dest, Source)

    // now check for custom
    else if FindCustomVariantType(Source.VType, LHandler) then
      LHandler.CastToOle(Dest, Source)

    // else fail
    else
      VarInvalidOp;
  end;
end;

procedure VarStrCat(var Dest: Variant; const Source: Variant);
begin
  if TVarData(Dest).VType = varString then
    Dest := string(Dest) + string(Source)
  else
    Dest := WideString(Dest) + WideString(Source);
end;

procedure _SimpleVarOp(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp); forward;

procedure VarOpRare(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
var
  LNewLeftType, LNewRightType: TVarType;
  LLeftHandler, LRightHandler: TCustomVariantType;
  LTemp: TVarData;
begin
  // simple and ???
  if Left.VType < CFirstUserType then
  begin

    // simple and custom but the custom doesn't really exist (nasty but possible )
    if not FindCustomVariantType(Right.VType, LRightHandler) then
      VarInvalidOp

    // does the custom want to take over?
    else if LRightHandler.LeftPromotion(Left, OpCode, LNewLeftType) then
    begin

      // convert the left side
      if Left.VType <> LNewLeftType then
      begin
        _VarInit(LTemp);
        try
          VarChangeType(LTemp, Left, LNewLeftType);
          _VarCopy(Left, LTemp);
          if Left.VType <> LNewLeftType then
            VarCastError;
        finally
          _VarClear(LTemp);
        end;
      end;
      LRightHandler.BinaryOp(Left, Right, OpCode);
    end

    // simple then converts custom then
    else
    begin
      _VarInit(LTemp);
      try
        // convert the right side to the left side's type
        VarChangeType(LTemp, Right, Left.VType);
        if LTemp.VType <> Left.VType then
          VarCastError;
        _SimpleVarOp(Left, LTemp, OpCode);
      finally
        _VarClear(LTemp);
      end;
    end;
  end

  // custom and something else
  else
  begin
    if not FindCustomVariantType(Left.VType, LLeftHandler) then
      VarInvalidOp;

    // does the left side like what is in the right side?
    if LLeftHandler.RightPromotion(Right, OpCode, LNewRightType) then
    begin

      // make the right side right
      if Right.VType <> LNewRightType then
      begin
        _VarInit(LTemp);
        try
          VarChangeType(LTemp, Right, LNewRightType);
          if LTemp.VType <> LNewRightType then
            VarCastError;
          LLeftHandler.BinaryOp(Left, LTemp, OpCode);
        finally
          _VarClear(LTemp);
        end;
      end

      // type is correct so lets go!
      else
        LLeftHandler.BinaryOp(Left, Right, OpCode);
    end

    // custom and simple and the right one can't convert the simple
    else if Right.VType < CFirstUserType then
    begin

      // convert the left side to the right side's type
      if Left.VType <> Right.VType then
      begin
        _VarInit(LTemp);
        try
          VarChangeType(LTemp, Left, Right.VType);
          _VarCopy(Left, LTemp);
          if Left.VType <> Right.VType then
            VarCastError;
        finally
          _VarClear(LTemp);
        end;
      end;
      _SimpleVarOp(Left, Right, OpCode);
    end

    // custom and custom but the right one doesn't really exist (nasty but possible )
    else if not FindCustomVariantType(Right.VType, LRightHandler) then
      VarInvalidOp

    // custom and custom and the right one can handle the left's type
    else if LRightHandler.LeftPromotion(Left, OpCode, LNewLeftType) then
    begin

      // convert the left side
      if Left.VType <> LNewLeftType then
      begin
        _VarInit(LTemp);
        try
          VarChangeType(LTemp, Left, LNewLeftType);
          _VarCopy(Left, LTemp);
          if Left.VType <> LNewLeftType then
            VarCastError;
        finally
          _VarClear(LTemp);
        end;
      end;
      LRightHandler.BinaryOp(Left, Right, OpCode);
    end

    // custom and custom but neither type can deal with each other
    else
      VarInvalidOp;
  end;
end;

procedure _VarOp(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
begin
  // simple and simple (optimized test to make sure both types are not custom)
  if (Left.VType < CFirstUserType) and (Right.VType < CFirstUserType) then
    _SimpleVarOp(Left, Right, OpCode)

  // something rare then
  else
    VarOpRare(Left, Right, OpCode);
end;

procedure _SimpleVarOp(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
  procedure Failure(const Prefix: string);
  begin
    raise EInvalidOp.CreateFmt('VarOp %s: Lt = %d, Rt = %d, Op = %d',
      [Prefix, Left.VType, Right.VType, OpCode]);
  end;

  procedure AnyOp(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
  var
    LTemp: TVarData;
  begin
    if Left.VType = varAny then
      ChangeAnyProc(Left);
    if Right.VType = varAny then
    begin
      _VarInit(LTemp);
      try
        _VarCopy(LTemp, Right);
        ChangeAnyProc(LTemp);
        _VarOp(Left, LTemp, OpCode);
      finally
        _VarClear(LTemp);
      end;
    end
    else
      _VarOp(Left, Right, OpCode);
  end;

  function CheckType(T: TVarType): TVarType;
  begin
    Result := T and varTypeMask;
    if Result > varLast then
      if Result = varString then
        Result := varOleStr
      else if Result = varAny then
        AnyOp(Left, Right, OpCode)
      else
        Failure('CheckType');
  end;

  procedure RealOp(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
  var
    L, R: Double;
  begin
    L := _VarToReal(Left);
    R := _VarToReal(Right);
    case OpCode of
      opAdd      :  L := L + R;
      opSubtract :  L := L - R;
      opMultiply :  L := L * R;
      opDivide   :  L := L / R;
    else
      Failure('RealOp');
    end;
    Variant(Left) := L;
  end;

  procedure DateOp(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
  var
    L, R: TDateTime;
  begin
    L := _VarToDate(Left);
    R := _VarToDate(Right);
    case OpCode of
      opAdd      :  L := L + R;
      opSubtract :  L := L - R;
    else
      Failure('DateOp');
    end;
    Variant(Left) := L;
  end;

  procedure IntOp(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
  var
    L, R: Integer;
    Overflow: Boolean;
  begin
    Overflow := False;
    L := _VarToInt(Left);
    R := _VarToInt(Right);
    case OpCode of
      opAdd        : asm
                       MOV  EAX, L
                       MOV  EDX, R
                       ADD  EAX, EDX
                       SETO Overflow
                       MOV  L, EAX
                     end;
      opSubtract   : asm
                       MOV  EAX, L
                       MOV  EDX, R
                       SUB  EAX, EDX
                       SETO Overflow
                       MOV  L, EAX
                     end;
      opMultiply   : asm
                       MOV  EAX, L
                       MOV  EDX, R
                       IMUL EDX
                       SETO Overflow
                       MOV  L, EAX
                     end;
      opDivide     : Failure('IntOp');
      opIntDivide  : L := L div R;
      opModulus    : L := L mod R;
      opShiftLeft  : L := L shl R;
      opShiftRight : L := L shr R;
      opAnd        : L := L and R;
      opOr         : L := L or R;
      opXor        : L := L xor R;
    else
      Failure('IntOp');
    end;

    if Overflow then
      RealOp(Left, Right, OpCode)
    else
      Variant(Left) := L;
  end;

  procedure Int64Op(var Left: TVarData; const Right: TVarData; const OpCode: TVarOp);
  var
    L, R: Int64;
    Overflow: Boolean;
  begin
    Overflow := False;
    L := _VarToInt64(Left);
    R := _VarToInt64(Right);
    case OpCode of
      opAdd, opSubtract, opMultiply:
        {$RANGECHECKS ON}
        try
          case OpCode of
            opAdd      : L := L + R;
            opSubtract : L := L - R;
            opMultiply : L := L * R;
          end;
        except
          on EOverflow do
            Overflow := True;
        end;
        {$RANGECHECKS OFF}
      opDivide     : Failure('Int64Op');
      opIntDivide  : L := L div R;
      opModulus    : L := L mod R;
      opShiftLeft  : L := L shl R;
      opShiftRight : L := L shr R;
      opAnd        : L := L and R;
      opOr         : L := L or R;
      opXor        : L := L xor R;
    else
      Failure('Int64Op');
    end;

    if Overflow then
      RealOp(Left, Right, OpCode)
    else
      Variant(Left) := L;
  end;

var
  LeftBase, RightBase: TBaseType;
  C: Currency;
  D: Double;
begin
  LeftBase := BaseTypeMap[CheckType(Left.VType)];
  RightBase := BaseTypeMap[CheckType(Right.VType)];
  case OpTypeMap[LeftBase, RightBase] of
    btErr:
      VarInvalidOp;

    btNul:
      begin
        _VarClear(Left);
        Left.VType := varNull;
      end;

    btInt:
      if OpCode = opDivide then
        RealOp(Left, Right, OpCode)
      else
        IntOp(Left, Right, OpCode);

    btFlt:
      if OpCode >= opIntDivide then
        IntOp(Left, Right, OpCode)
      else
        RealOp(Left, Right, OpCode);

    btCur:
      case OpCode of
        opAdd      : Variant(Left) := _VarToCurr(Left) + _VarToCurr(Right);
        opSubtract : Variant(Left) := _VarToCurr(Left) - _VarToCurr(Right);
        opMultiply,
        opDivide   :
          begin
            if (LeftBase = btCur) and (RightBase = btCur) then
            begin
              if OpCode = opMultiply then
                Variant(Left) := Left.VCurrency * Right.VCurrency
              else
                Variant(Left) := Left.VCurrency / Right.VCurrency;
            end
            else if RightBase = btCur then
            begin   // LeftBase <> btCur
              if OpCode = opMultiply then
              begin
                C := _VarToReal(Left) * Right.VCurrency;
                Variant(Left) := C;
              end
              else
              begin
                D := _VarToCurr(Left) / Right.VCurrency;
                Variant(Left) := D;
              end;
            end
            else  // LeftBase = btCur, RightBase <> btCur
            begin
              if OpCode = opMultiply then
              begin
                C := Left.VCurrency * _VarToReal(Right);
                Variant(Left) := C;
              end
              else
              begin
                C := Left.VCurrency / _VarToReal(Right);
                Variant(Left) := C;
              end;
            end;
          end;
      else
        IntOp(Left, Right, OpCode);
      end;

    btStr:
      if OpCode = opAdd then
        VarStrCat(Variant(Left), Variant(Right))
      else
        if OpCode >= opIntDivide then
          IntOp(Left, Right, OpCode)
        else
          RealOp(Left, Right, OpCode);

    btBol:
      if OpCode < opAnd then
        if OpCode >= opIntDivide then
          IntOp(Left, Right, OpCode)
        else
          RealOp(Left, Right, OpCode)
      else
      begin
        case OpCode of
          opAnd:  Variant(Left) := _VarToBool(Left) and _VarToBool(Right);
          opOr :  Variant(Left) := _VarToBool(Left) or _VarToBool(Right);
          opXor:  Variant(Left) := _VarToBool(Left) xor _VarToBool(Right);
        end;
      end;

    btDat:
      case OpCode of
        opAdd:
          DateOp(Left, Right, OpCode);
        opSubtract:
          begin
            DateOp(Left, Right, OpCode);
            if (LeftBase = btDat) and (RightBase = btDat) then
              Left.VType := varDouble;
          end;
        opMultiply,
        opDivide:
          RealOp(Left, Right, OpCode);
      else
        IntOp(Left, Right, OpCode);
      end;

    btI64:
      if OpCode = opDivide then
        RealOp(Left, Right, OpCode)
      else
        Int64Op(Left, Right, OpCode);
  else
    Failure('General');
  end;
end;

function SimpleVarCmp(const Left, Right: TVarData; const OpCode: TVarOp): TVarCompareResult; forward;

function VarCompare(const Left, Right: TVarData; const OpCode: TVarOp): TVarCompareResult; forward;

function AnyCmp(const Left, Right: TVarData; const OpCode: TVarOp): TVarCompareResult;
var
  LLeft, LTemp: TVarData;
begin
  _VarInit(LLeft);
  try

    // resolve the left side into a normal simple type
    _VarCopy(LLeft, Left);
    if Left.VType = varAny then
      ChangeAnyProc(LLeft);

    // does the right side need to be reduced to a simple type
    if Right.VType = varAny then
    begin
      _VarInit(LTemp);
      try
        _VarCopy(LTemp, Right);
        ChangeAnyProc(LTemp);
        Result := VarCompare(LLeft, LTemp, OpCode);
      finally
        _VarClear(LTemp);
      end;
    end

    // right side is fine as is
    else
      Result := VarCompare(LLeft, Right, OpCode);
  finally
    _VarClear(LLeft);
  end;
end;

function VarCompareRare(const Left, Right: TVarData; const OpCode: TVarOp): TVarCompareResult;
const
  CBooleanToRelationship: array [opCmpEQ..opCmpGE, Boolean] of TVarCompareResult =
   // False          True
    ((crLessThan,    crEqual),        // opCmpEQ = 14;
     (crEqual,       crLessThan),     // opCmpNE = 15;
     (crEqual,       crLessThan),     // opCmpLT = 16;
     (crGreaterThan, crLessThan),     // opCmpLE = 17;
     (crEqual,       crGreaterThan),  // opCmpGT = 18;
     (crLessThan,    crGreaterThan)); // opCmpGE = 19;
var
  LNewLeftType, LNewRightType: TVarType;
  LLeftHandler, LRightHandler: TCustomVariantType;
  LTemp: TVarData;
begin
  // assume all is swell
  Result := crEqual;

  // simple and something else
  if Left.VType < CFirstUserType then
  begin

    // [simple and custom] but the custom doesn't really exist (nasty but possible )
    if not FindCustomVariantType(Right.VType, LRightHandler) then
      VarInvalidOp

    // does the custom want to take over?
    else if LRightHandler.LeftPromotion(Left, opCompare, LNewLeftType) then
    begin

      // convert the simple side
      if Left.VType <> LNewLeftType then
      begin
        _VarInit(LTemp);
        try
          VarChangeType(LTemp, Left, LNewLeftType);
          if (LTemp.VType and varTypeMask) <> LNewLeftType then
            VarCastError;
          Result := CBooleanToRelationship[OpCode, LRightHandler.CompareOp(LTemp, Right, OpCode)];
        finally
          _VarClear(LTemp);
        end;
      end

      // already that type!
      else
        Result := CBooleanToRelationship[OpCode, LRightHandler.CompareOp(Left, Right, OpCode)];
    end

    // convert custom side to simple's type
    else
    begin
      _VarInit(LTemp);
      try
        VarChangeType(LTemp, Right, Left.VType);
        if (LTemp.VType and varTypeMask) <> Left.VType then
          VarCastError;
        Result := SimpleVarCmp(Left, LTemp, OpCode)
      finally
        _VarClear(LTemp);
      end;
    end;
  end

  // custom and something else
  else
  begin
    if not FindCustomVariantType(Left.VType, LLeftHandler) then
      VarInvalidOp

    // does the left side like what is in the right side?
    else if LLeftHandler.RightPromotion(Right, opCompare, LNewRightType) then
    begin

      // make the right side right
      if Right.VType <> LNewRightType then
      begin
        _VarInit(LTemp);
        try
          VarChangeType(LTemp, Right, LNewRightType);
          if LTemp.VType <> LNewRightType then
            VarCastError;
          Result := CBooleanToRelationship[OpCode, LLeftHandler.CompareOp(Left, LTemp, OpCode)];
        finally
          _VarClear(LTemp);
        end;
      end

      // already that type
      else
        Result := CBooleanToRelationship[OpCode, LLeftHandler.CompareOp(Left, Right, OpCode)];
    end

    // [custom and simple]
    else if Right.VType < CFirstUserType then
    begin
      _VarInit(LTemp);
      try
        // convert the left side to the right side's type
        VarChangeType(LTemp, Left, Right.VType);
        if LTemp.VType <> Right.VType then
          VarCastError;
        Result := SimpleVarCmp(LTemp, Right, OpCode)
      finally
        _VarClear(LTemp);
      end;
    end

    // [custom and custom] but the right one doesn't really exist (nasty but possible )
    else if not FindCustomVariantType(Right.VType, LRightHandler) then
      VarInvalidOp

    // [custom and custom] and the right one can handle the left's type
    else if LRightHandler.LeftPromotion(Left, opCompare, LNewLeftType) then
    begin

      // convert the left side
      if Left.VType <> LNewLeftType then
      begin
        _VarInit(LTemp);
        try
          VarChangeType(LTemp, Left, LNewLeftType);
          if LTemp.VType <> LNewLeftType then
            VarCastError;
          Result := CBooleanToRelationship[OpCode, LRightHandler.CompareOp(LTemp, Right, OpCode)];
        finally
          _VarClear(LTemp);
        end;
      end

      // it's already correct!
      else
        Result := CBooleanToRelationship[OpCode, LRightHandler.CompareOp(Left, Right, OpCode)];
    end

    // [custom and custom] but neither type can deal with each other
    else
      VarInvalidOp;
  end;
end;

function VarCompare(const Left, Right: TVarData; const OpCode: TVarOp): TVarCompareResult;
begin
  // simple and simple (optimized test to make sure both types are not custom)
  if (Left.VType < CFirstUserType) and (Right.VType < CFirstUserType) then

    // are ANYs involved?
    if (Left.VType = varAny) or (Right.VType = varAny) then
      Result := AnyCmp(Left, Right, OpCode)

    // just simple vs simple
    else
      Result := SimpleVarCmp(Left, Right, OpCode)

  // something rare then!
  else
    Result := VarCompareRare(Left, Right, OpCode);
end;

procedure _VarCmp(const Left, Right: Variant; const OpCode: TVarOp); // compiler requires result in flags
asm
        //  IN:  EAX = Left
        //  IN:  EDX = Right
        //  OUT:  Flags register indicates less than, greater than, or equal
        CALL    VarCompare
        CMP     AL, crEqual
end;

function SimpleVarCmp(const Left, Right: TVarData; const OpCode: TVarOp): TVarCompareResult;

  procedure Failure(const Prefix: string);
  begin
    raise EInvalidOp.CreateFmt('VarCmp %s: Lt = %d, Rt = %d, Op = %d',
      [Prefix, Left.VType, Right.VType, OpCode]);
  end;

  function CheckType(T: TVarType): TVarType;
  begin
    Result := T and varTypeMask;
    if Result > varLast then
      if Result = varString then
        Result := varOleStr
      else
        Failure('CheckType');
  end;

  function IntCompare(A, B: Integer): TVarCompareResult;
  begin
    if A < B then
      Result := crLessThan
    else if A > B then
      Result := crGreaterThan
    else
      Result := crEqual;
  end;

  function Int64Compare(const A, B: Int64): TVarCompareResult;
  begin
    if A < B then
      Result := crLessThan
    else if A > B then
      Result := crGreaterThan
    else
      Result := crEqual;
  end;

  function RealCompare(const A, B: Double): TVarCompareResult;
  begin
    if A < B then
      Result := crLessThan
    else if A > B then
      Result := crGreaterThan
    else
      Result := crEqual;
  end;

  // keep string temps out of the main proc
  function StringCompare(const L, R: TVarData): TVarCompareResult;
  var
    A, B: string;
  begin
    _VarToLStr(A, L);
    _VarToLStr(B, R);
    Result := IntCompare(StrComp(PChar(A), PChar(B)), 0);
  end;

var
  L, R: TBaseType;
begin
  Result := crEqual;
  L := BaseTypeMap[CheckType(Left.VType)];
  R := BaseTypeMap[CheckType(Right.VType)];
  case OpTypeMap[L, R] of
    btErr:  Failure('Error');
    btNul:  Result := IntCompare(Ord(L), Ord(R));
    btInt:  Result := IntCompare(_VarToInt(Left), _VarToInt(Right));
    btI64:  Result := Int64Compare(_VarToInt64(Left), _VarToInt64(Right));
    btFlt:  Result := RealCompare(_VarToReal(Left), _VarToReal(Right));
    btDat:  Result := RealCompare(_VarToDate(Left), _VarToDate(Right));
    btCur:  Result := RealCompare(_VarToCurr(Left), _VarToCurr(Right));
    btStr:  Result := StringCompare(Left, Right);
    btBol:  Result := IntCompare(Integer(_VarToBool(Left)), Integer(_VarToBool(Right)));
  else
    Failure('General');
  end;
end;

procedure _SimpleVarNeg(var V: TVarData); forward;

procedure _VarNeg(var V: TVarData);
var
  LHandler: TCustomVariantType;
begin
  if V.VType < CFirstUserType then
    _SimpleVarNeg(V)
  else if FindCustomVariantType(V.VType, LHandler) then
    LHandler.UnaryOp(V, opNegate)
  else
    VarInvalidOp;
end;

procedure _SimpleVarNeg(var V: TVarData);
var
  T: TVarType;
begin
  T := V.VType and varTypeMask;
  if T > varLast then
    if T = varString then
      T := varOleStr
    else
      VarInvalidOp;

  case BaseTypeMap[T] of
    btErr : VarInvalidOp;
    btNul : ;  // do nothing
    btBol,
    btInt : _VarFromInt(V, -_VarToInt(V), -4);
    btI64 : _VarFromInt64(V, -_VarToInt64(V));
    btStr,
    btFlt : Variant(V) := -_VarToReal(V);
    btCur : V.VCurrency := -V.VCurrency;
    btDat : V.VDate := -V.VDate;
  end;
end;

procedure _SimpleVarNot(var V: TVarData); forward;

procedure _VarNot(var V: TVarData);
var
  LHandler: TCustomVariantType;
begin
  if V.VType < CFirstUserType then
    _SimpleVarNot(V)
  else if FindCustomVariantType(V.VType, LHandler) then
    LHandler.UnaryOp(V, opNot)
  else
    VarInvalidOp;
end;

procedure _SimpleVarNot(var V: TVarData);
var
  T: TVarType;
begin
  T := V.VType and varTypeMask;
  case T of
    varEmpty   : VarInvalidOp;
    varBoolean : V.VBoolean := not V.VBoolean;
    varNull    : ; // do nothing
    varAny     :
      begin
        ChangeAnyProc(V);
        _VarNot(V);
      end;
  else
    if (T = varInt64) then
      _VarFromInt64(V, not _VarToInt64(V))
    else if (T = varString) or (T <= varLast) then
      _VarFromInt(V, not _VarToInt(V), -4)
    else
      VarInvalidOp;
  end;
end;

procedure _VarCopyNoInd; // ARGS PLEASE!
asm
        JMP     VarCopyNoInd
end;

procedure _VarAddRef(var V: Variant);
asm
        CMP     [EAX].TVarData.VType,varOleStr
        JB      @@1
        PUSH    [EAX].Integer[12]
        PUSH    [EAX].Integer[8]
        PUSH    [EAX].Integer[4]
        PUSH    [EAX].Integer[0]
        MOV     [EAX].TVarData.VType,varEmpty
        MOV     EDX,ESP
        CALL    _VarCopy
        ADD     ESP,16
@@1:
end;

function VarType(const V: Variant): TVarType;
begin
  Result := TVarData(V).VType;
end;

function VarAsType(const V: Variant; AVarType: TVarType): Variant;
begin
  VarChangeType(TVarData(Result), TVarData(V), AVarType);
end;

function VarIsType(const V: Variant; AVarType: TVarType): Boolean;
begin
  Result := TVarData(V).VType = AVarType;
end;

function VarIsType(const V: Variant; const AVarTypes: array of TVarType): Boolean;
var
  I: Integer;
begin
  Result := False;
  for I := Low(AVarTypes) to High(AVarTypes) do
    if TVarData(V).VType = AVarTypes[I] then
    begin
      Result := True;
      Break;
    end;
end;

function VarIsClear(const V: Variant): Boolean;
var
  LHandler: TCustomVariantType;
begin
  with TVarData(V) do
    if VType < CFirstUserType then
      Result := (VType = varEmpty) or
                (((VType = varDispatch) or (VType = varUnknown)) and
                  (VDispatch = nil))
    else if FindCustomVariantType(VType, LHandler) then
      Result := LHandler.IsClear(TVarData(V))
    else
      Result := False;
end;

function VarTypeIsCustom(const AVarType: TVarType): Boolean;
var
  LHandler: TCustomVariantType;
begin
  Result := FindCustomVariantType(AVarType, LHandler);
end;

function VarIsCustom(const V: Variant): Boolean;
begin
  Result := VarTypeIsCustom(TVarData(V).VType);
end;

function VarTypeIsOrdinal(const AVarType: TVarType): Boolean;
begin
  Result := AVarType in [varSmallInt, varInteger, varBoolean, varShortInt,
                         varByte, varWord, varLongWord, varInt64];
end;

function VarIsOrdinal(const V: Variant): Boolean;
begin
  Result := VarTypeIsOrdinal(TVarData(V).VType);
end;

function VarTypeIsFloat(const AVarType: TVarType): Boolean;
begin
  Result := AVarType in [varSingle, varDouble, varCurrency];
end;

function VarIsFloat(const V: Variant): Boolean;
begin
  Result := VarTypeIsFloat(TVarData(V).VType);
end;

function VarTypeIsNumeric(const AVarType: TVarType): Boolean;
begin
  Result := VarTypeIsOrdinal(AVarType) or VarTypeIsFloat(AVarType);
end;

function VarIsNumeric(const V: Variant): Boolean;
begin
  Result := VarTypeIsNumeric(TVarData(V).VType);
end;

function VarTypeIsStr(const AVarType: TVarType): Boolean;
begin
  Result := (AVarType = varOleStr) or (AVarType = varString);
end;

function VarIsStr(const V: Variant): Boolean;
begin
  Result := VarTypeIsStr(TVarData(V).VType);
end;

function VarIsEmpty(const V: Variant): Boolean;
begin
  Result := TVarData(V).VType = varEmpty;
end;

procedure VarCheckEmpty(const V: Variant);
begin
  if VarIsEmpty(V) then
    raise EVariantError.Create(SVarIsEmpty);
end;

function VarIsNull(const V: Variant): Boolean;
begin
  Result := TVarData(V).VType = varNull;
end;

function VarToStr(const V: Variant): string;
begin
  Result := VarToStrDef(V, '');
end;

function VarToStrDef(const V: Variant; const ADefault: string): string;
begin
  if TVarData(V).VType <> varNull then
    Result := V
  else
    Result := ADefault;
end;

function VarToWideStr(const V: Variant): WideString;
begin
  Result := VarToWideStrDef(V, '');
end;

function VarToWideStrDef(const V: Variant; const ADefault: WideString): WideString;
begin
  if TVarData(V).VType <> varNull then
    Result := V
  else
    Result := ADefault;
end;

function VarFromDateTime(const DateTime: TDateTime): Variant;
begin
  _VarClear(TVarData(Result));
  TVarData(Result).VType := varDate;
  TVarData(Result).VDate := DateTime;
end;

function VarToDateTime(const V: Variant): TDateTime;
var
  LTemp: TVarData;
begin
  _VarInit(LTemp);
  VarChangeType(LTemp, TVarData(V), varDate);
  Result := LTemp.VDate;
end;

function VarInRange(const AValue, AMin, AMax: Variant): Boolean;
begin
  Result := (AValue >= AMin) and (AValue <= AMax);
end;

function VarEnsureRange(const AValue, AMin, AMax: Variant): Variant;
begin
  Result := AValue;
  if Result < AMin then
    Result := AMin;
  if Result > AMax then
    Result := AMax;
end;

function VarSameValue(const A, B: Variant): Boolean;
begin
  if TVarData(A).VType = varEmpty then
    Result := TVarData(B).VType = varEmpty
  else if TVarData(A).VType = varNull then
    Result := TVarData(B).VType = varNull
  else if TVarData(B).VType in [varEmpty, varNull] then
    Result := False
  else
    Result := A = B;
end;

function VarCompareValue(const A, B: Variant): TVariantRelationship;
const
  CTruth: array [Boolean] of TVariantRelationship = (vrNotEqual, vrEqual);
begin
  if TVarData(A).VType = varEmpty then
    Result := CTruth[TVarData(B).VType = varEmpty]
  else if TVarData(A).VType = varNull then
    Result := CTruth[TVarData(B).VType = varNull]
  else if TVarData(B).VType in [varEmpty, varNull] then
    Result := vrNotEqual
  else if A = B then
    Result := vrEqual
  else if A < B then
    Result := vrLessThan
  else
    Result := vrGreaterThan;
end;

function VarIsEmptyParam(const V: Variant): Boolean;
begin
  Result := (TVarData(V).VType = varError) and
            (TVarData(V).VError = $80020004); {DISP_E_PARAMNOTFOUND}
end;

procedure SetClearVarToEmptyParam(var V: TVarData);
begin
  V.VType := varError;
  V.VError := $80020004; {DISP_E_PARAMNOTFOUND}
end;

function VarIsByRef(const V: Variant): Boolean;
begin
  Result := (TVarData(V).VType and varByRef) <> 0;
end;

function VarSupports(const V: Variant; const IID: TGUID; out Intf): Boolean;
var
  LInstance: IVarInstanceReference;
begin
  Result := (Supports(V, IVarInstanceReference, LInstance) and
             Supports(LInstance.GetInstance(TVarData(V)), IID, Intf)) or
            Supports(V, IID, Intf);
end;

function VarSupports(const V: Variant; const IID: TGUID): Boolean;
var
  LInstance: IVarInstanceReference;
  LInterface: IInterface;
begin
  Result := (Supports(V, IVarInstanceReference, LInstance) and
             Supports(LInstance.GetInstance(TVarData(V)), IID, LInterface)) or
            Supports(V, IID, LInterface);
end;


function _WriteVariant(var T: Text; const V: Variant; Width: Integer): Pointer;
var
  S: string;
begin
  if (TVarData(V).VType <> varEmpty) and (TVarData(V).VType <> varNull) then
    S := V;
  Write(T, S: Width);
  Result := @T;
end;

function _Write0Variant(var T: Text; const V: Variant): Pointer;
begin
  Result := _WriteVariant(T, V, 0);
end;

{ ----------------------------------------------------- }
{       Variant array support                           }
{ ----------------------------------------------------- }

function GetVarDataArrayInfo(const AVarData: TVarData; out AVarType: TVarType;
  out AVarArray: PVarArray): Boolean;
var
  LVarDataPtr: PVarData;
begin
  // original type
  LVarDataPtr := @AVarData;
  AVarType := LVarDataPtr^.VType;

  // if original type is not an array but is pointing to one
  if (AVarType = (varByRef or varVariant)) and
     ((PVarData(LVarDataPtr^.VPointer)^.VType and varArray) <> 0) then
  begin
    LVarDataPtr := PVarData(LVarDataPtr^.VPointer);
    AVarType := LVarDataPtr^.VType;
  end;

  // make sure we are pointing to an array then
  Result := (AVarType and varArray) <> 0;

  // figure out the array data pointer
  if Result then
    if (AVarType and varByRef) <> 0 then
      AVarArray := PVarArray(LVarDataPtr^.VPointer^)
    else
      AVarArray := LVarDataPtr^.VArray
  else
    AVarArray := nil;
end;

const
  tkDynArray  = 17;

function VarArrayCreate(const Bounds: array of Integer;
  AVarType: TVarType): Variant;
var
  I, LDimCount: Integer;
  LVarArrayRef: PVarArray;
  LVarBounds: array[0..63] of TVarArrayBound;
begin
  if (not Odd(High(Bounds)) or (High(Bounds) > 127)) or
     (not VarTypeIsValidArrayType(AVarType)) then
    System.Error(System.reVarArrayCreate);

  LDimCount := (High(Bounds) + 1) div 2;
  for I := 0 to LDimCount - 1 do
    with LVarBounds[I] do
    begin
      LowBound := Bounds[I * 2];
      ElementCount := Bounds[I * 2 + 1] - LowBound + 1;
    end;

  LVarArrayRef := SafeArrayCreate(AVarType, LDimCount, PVarArrayBoundArray(@LVarBounds)^);
  if LVarArrayRef = nil then
    System.Error(System.reVarArrayCreate);

  _VarClear(TVarData(Result));

  TVarData(Result).VType := AVarType or varArray;
  TVarData(Result).VArray := LVarArrayRef;
end;

function VarArrayOf(const Values: array of Variant): Variant;
var
  I: Integer;
begin
  Result := VarArrayCreate([0, High(Values)], varVariant);
  for I := 0 to High(Values) do
    Result[I] := Values[I];
end;

procedure _VarArrayRedim(var A: Variant; HighBound: Integer);
var
  VarBound: TVarArrayBound;
  LVarType: TVarType;
  LVarArray: PVarArray;
begin
  if not GetVarDataArrayInfo(TVarData(A), LVarType, LVarArray) then
    System.Error(System.reVarNotArray);

  with LVarArray^ do
    VarBound.LowBound := Bounds[DimCount - 1].LowBound;

  VarBound.ElementCount := HighBound - VarBound.LowBound + 1;

  if SafeArrayRedim(LVarArray, VarBound) <> VAR_OK then
    System.Error(System.reVarArrayCreate);
end;

function GetVarArray(const A: Variant): PVarArray;
var
  LVarType: TVarType;
begin
  if not GetVarDataArrayInfo(TVarData(A), LVarType, Result) then
    System.Error(System.reVarNotArray);
end;

function VarArrayDimCount(const A: Variant): Integer;
var
  LVarType: TVarType;
  LVarArray: PVarArray;
begin
  if GetVarDataArrayInfo(TVarData(A), LVarType, LVarArray) then
    Result := LVarArray^.DimCount
  else
    Result := 0;
end;

function VarArrayLowBound(const A: Variant; Dim: Integer): Integer;
begin
  if SafeArrayGetLBound(GetVarArray(A), Dim, Result) <> VAR_OK then
    System.Error(System.reVarArrayBounds);
end;

function VarArrayHighBound(const A: Variant; Dim: Integer): Integer;
begin
  if SafeArrayGetUBound(GetVarArray(A), Dim, Result) <> VAR_OK then
    System.Error(System.reVarArrayBounds);
end;

function VarArrayLock(const A: Variant): Pointer;
begin
  if SafeArrayAccessData(GetVarArray(A), Result) <> VAR_OK then
    System.Error(System.reVarNotArray);
end;

procedure VarArrayUnlock(const A: Variant);
begin
  if SafeArrayUnaccessData(GetVarArray(A)) <> VAR_OK then
    System.Error(System.reVarNotArray);
end;

function VarArrayRef(const A: Variant): Variant;
begin
  if (TVarData(A).VType and varArray) = 0 then
    System.Error(System.reVarNotArray);

  _VarClear(TVarData(Result));

  TVarData(Result).VType := TVarData(A).VType or varByRef;
  if (TVarData(A).VType and varByRef) <> 0 then
    TVarData(Result).VPointer := TVarData(A).VPointer
  else
    TVarData(Result).VPointer := @TVarData(A).VArray;
end;

function VarIsArray(const A: Variant): Boolean;
begin
  Result := VarIsArray(A, False);
end;

function VarIsArray(const A: Variant; AResolveByRef: Boolean): Boolean;
var
  LVarType: TVarType;
  LVarArray: PVarArray;
begin
  if AResolveByRef then
    Result := GetVarDataArrayInfo(TVarData(A), LVarType, LVarArray)
  else
    Result := (TVarData(A).VType and varArray) = varArray;
end;

function VarTypeIsValidArrayType(const AVarType: TVarType): Boolean;
var
  LVarType: TVarType;
begin
  LVarType := AVarType and varTypeMask;
  Result := (LVarType in [CMinArrayVarType..CMaxArrayVarType]) and
            CVarTypeToElementInfo[LVarType].ValidBase;
end;

function VarTypeIsValidElementType(const AVarType: TVarType): Boolean;
var
  LVarType: TVarType;
begin
  LVarType := AVarType and varTypeMask;
  Result := ((LVarType in [CMinArrayVarType..CMaxArrayVarType]) and
             CVarTypeToElementInfo[LVarType].ValidElement) or
            VarTypeIsCustom(LVarType);
end;

function _VarArrayGet(var A: Variant; IndexCount: Integer;
  const Indices: TVarArrayCoorArray): Variant; cdecl;
var
  LVarType: TVarType;
  LVarArrayPtr: PVarArray;
  LArrayVarType: Integer;
  P: Pointer;
  LResult: TVarData;
begin
  if not GetVarDataArrayInfo(TVarData(A), LVarType, LVarArrayPtr) then
    System.Error(System.reVarNotArray);

  if LVarArrayPtr^.DimCount <> IndexCount then
    System.Error(System.reVarArrayBounds);

  // use a LTemp for result just in case the result points back to source, icky
  _VarInit(LResult);
  try
    LArrayVarType := LVarType and varTypeMask;
    if LArrayVarType = varVariant then
    begin
      if SafeArrayPtrOfIndex(LVarArrayPtr, @Indices, P) <> VAR_OK then
        System.Error(System.reVarArrayBounds);
      _VarCopy(LResult, PVarData(P)^);
    end
    else
    begin
      if SafeArrayGetElement(LVarArrayPtr, @Indices, @TVarData(LResult).VPointer) <> VAR_OK then
        System.Error(System.reVarArrayBounds);
      TVarData(LResult).VType := LArrayVarType;
    end;

    // copy the LTemp result over to result
    _VarCopy(TVarData(Result), LResult);
  finally
    _VarClear(LResult);
  end;
end;

function VarArrayGet(const A: Variant; const Indices: array of Integer): Variant;
asm
        {     ->EAX     Pointer to A            }
        {       EDX     Pointer to Indices      }
        {       ECX     High bound of Indices   }
        {       [EBP+8] Pointer to result       }

        PUSH    EBX

        MOV     EBX,ECX
        INC     EBX
        JLE     @@endLoop
@@loop:
        PUSH    [EDX+ECX*4].Integer
        DEC     ECX
        JNS     @@loop
@@endLoop:
        PUSH    EBX
        PUSH    EAX
        MOV     EAX,[EBP+8]
        PUSH    EAX
        CALL    _VarArrayGet
        LEA     ESP,[ESP+EBX*4+3*4]

        POP     EBX
end;

procedure _VarArrayPut(var A: Variant; const Value: Variant;
  IndexCount: Integer; const Indices: TVarArrayCoorArray); cdecl;
type
  TAnyPutArrayProc = procedure (var A: Variant; const Value: Variant; Index: Integer);
var
  LVarType: TVarType;
  LVarArrayPtr: PVarArray;
  LValueType: TVarType;
  LValueArrayPtr: PVarArray;
  LArrayVarType: Integer;
  P: Pointer;
  LTemp: TVarData;
begin
  if not GetVarDataArrayInfo(TVarData(A), LVarType, LVarArrayPtr) then
    System.Error(System.reVarNotArray);

  if not GetVarDataArrayInfo(TVarData(Value), LValueType, LValueArrayPtr) and
     not VarTypeIsValidElementType(LValueType) and
     (LValueType <> varString) then
    System.Error(System.reVarTypeCast);

  if LVarArrayPtr^.DimCount <> IndexCount then
    System.Error(System.reVarArrayBounds);

  LArrayVarType := LVarType and varTypeMask;

  if (LArrayVarType = varVariant) and
     ((LValueType <> varString) or
      VarTypeIsCustom(LValueType)) then
  begin
    if SafeArrayPtrOfIndex(LVarArrayPtr, @Indices, P) <> VAR_OK then
      System.Error(System.reVarArrayBounds);
    PVariant(P)^ := Value;
  end else
  begin
    _VarInit(LTemp);
    try
      if LArrayVarType = varVariant then
      begin
        VarStringToOleStr(Variant(LTemp), Value);
        P := @LTemp;
      end else
      begin
        VarChangeType(LTemp, TVarData(Value), LArrayVarType);
        case LArrayVarType of
          varOleStr, varDispatch, varUnknown:
            P := LTemp.VPointer;
        else
          P := @LTemp.VPointer;
        end;
      end;
      if SafeArrayPutElement(LVarArrayPtr, @Indices, P) <> VAR_OK then
        System.Error(System.reVarArrayBounds);
    finally
      _VarClear(LTemp);
    end;
  end;
end;

procedure VarArrayPut(var A: Variant; const Value: Variant; const Indices: array of Integer);
asm
        {     ->EAX     Pointer to A            }
        {       EDX     Pointer to Value        }
        {       ECX     Pointer to Indices      }
        {       [EBP+8] High bound of Indices   }

        PUSH    EBX

        MOV     EBX,[EBP+8]

        TEST    EBX,EBX
        JS      @@endLoop
@@loop:
        PUSH    [ECX+EBX*4].Integer
        DEC     EBX
        JNS     @@loop
@@endLoop:
        MOV     EBX,[EBP+8]
        INC     EBX
        PUSH    EBX
        PUSH    EDX
        PUSH    EAX
        CALL    _VarArrayPut
        LEA     ESP,[ESP+EBX*4+3*4]

        POP     EBX
end;


function DynArrayIndex(const P: Pointer; const Indices: array of Integer; const TypInfo: Pointer): Pointer;
asm
        {     ->EAX     P                       }
        {       EDX     Pointer to Indices      }
        {       ECX     High bound of Indices   }
        {       [EBP+8] TypInfo                 }

        PUSH    EBX
        PUSH    ESI
        PUSH    EDI
        PUSH    EBP

        MOV     ESI,EDX
        MOV     EDI,[EBP+8]
        MOV     EBP,EAX

        XOR     EBX,EBX                 {  for i := 0 to High(Indices) do       }
        TEST    ECX,ECX
        JGE     @@start
@@loop:
        MOV     EBP,[EBP]
@@start:
        XOR     EAX,EAX
        MOV     AL,[EDI].TDynArrayTypeInfo.name
        ADD     EDI,EAX
        MOV     EAX,[ESI+EBX*4]         {    P := P + Indices[i]*TypInfo.elSize }
        MUL     [EDI].TDynArrayTypeInfo.elSize
        MOV     EDI,[EDI].TDynArrayTypeInfo.elType
        TEST    EDI,EDI
        JE      @@skip
        MOV     EDI,[EDI]
@@skip:
        ADD     EBP,EAX
        INC     EBX
        CMP     EBX,ECX
        JLE     @@loop

@@loopEnd:

        MOV     EAX,EBP

        POP     EBP
        POP     EDI
        POP     ESI
        POP     EBX
end;

{ Returns the DynArrayTypeInfo of the Element Type of the specified DynArrayTypeInfo }
function DynArrayElTypeInfo(typeInfo: PDynArrayTypeInfo): PDynArrayTypeInfo;
begin
  Result := nil;
  if typeInfo <> nil then
  begin
    Inc(PChar(typeInfo), Length(typeInfo.name));
    if typeInfo.elType <> nil then
      Result := typeInfo.elType^;
  end;
end;

{ Returns # of dimemsions of the DynArray described by the specified DynArrayTypeInfo}
function DynArrayDim(typeInfo: PDynArrayTypeInfo): Integer;
begin
  Result := 0;
  while (typeInfo <> nil) and (typeInfo.kind = tkDynArray) do
  begin
    Inc(Result);
    typeInfo := DynArrayElTypeInfo(typeInfo);
  end;
end;

{ Returns size of the Dynamic Array}
function DynArraySize(a: Pointer): Integer;
asm
        TEST EAX, EAX
        JZ   @@exit
        MOV  EAX, [EAX-4]
@@exit:
end;

// Returns whether array is rectangular
function IsDynArrayRectangular(const DynArray: Pointer; typeInfo: PDynArrayTypeInfo): Boolean;
var
  Dim, I, J, Size, SubSize: Integer;
  P: Pointer;
begin
  // Assume we have a rectangular array
  Result := True;

  P := DynArray;
  Dim := DynArrayDim(typeInfo);

  {NOTE: Start at 1. Don't need to test the first dimension - it's rectangular by definition}
  for I := 1 to dim-1 do
  begin
    if P <> nil then
    begin
      { Get size of this dimension }
      Size := DynArraySize(P);

      { Get Size of first sub. dimension }
      SubSize := DynArraySize(PPointerArray(P)[0]);

      { Walk through every dimension making sure they all have the same size}
      for J := 1 to Size-1 do
        if DynArraySize(PPointerArray(P)[J]) <> SubSize then
        begin
          Result := False;
          Exit;
        end;

      { Point to next dimension}
      P := PPointerArray(P)[0];
    end;
  end;
end;

// Returns Bounds of Dynamic array as an array of integer containing the 'high' of each dimension
function DynArrayBounds(const DynArray: Pointer; typeInfo: PDynArrayTypeInfo): TBoundArray;
var
  Dim, I: Integer;
  P: Pointer;
begin
  P := DynArray;

  Dim := DynArrayDim(typeInfo);
  SetLength(Result, Dim);

  for I := 0 to dim-1 do
    if P <> nil then
    begin
      Result[I] := DynArraySize(P)-1;
      P := PPointerArray(P)[0]; // Assume rectangular arrays
    end;
end;

{ Decrements to next lower index - Returns True if successful }
{ Indices: Indices to be decremented }
{ Bounds : High bounds of each dimension }
function DecIndices(var Indices: TBoundArray; const Bounds: TBoundArray): Boolean;
var
  I, J: Integer;
begin
  { Find out if we're done: all at zeroes }
  Result := False;
  for I := Low(Indices)  to High(Indices) do
    if Indices[I] <> 0  then
    begin
      Result := True;
      break;
    end;
  if not Result then
    Exit;

  { Two arrays must be of same length }
  Assert(Length(Indices) = Length(Bounds));

  { Find index of item to tweak }
  for I := High(Indices) downto Low(Bounds) do
  begin
    // If not reach zero, dec and bail out
    if Indices[I] <> 0 then
    begin
      Dec(Indices[I]);
      Exit;
    end
    else
    begin
      J := I;
      while Indices[J] = 0 do
      begin
        // Restore high bound when we've reached zero on a particular dimension
        Indices[J] := Bounds[J];
        // Move to higher dimension
        Dec(J);
        Assert(J >= 0);
      end;
      Dec(Indices[J]);
      Exit;
    end;
  end;
end;

// Returns Bounds of a DynamicArray in a format usable for creating a Variant.
//  i.e. The format of the bounds returns contains pairs of lo and hi bounds where
//       lo is always 0, and hi is the size dimension of the array-1.
function DynArrayVariantBounds(const DynArray: Pointer; typeInfo: PDynArrayTypeInfo): TBoundArray;
var
  Dim, I: Integer;
  P: Pointer;
begin
  P := DynArray;

  Dim := DynArrayDim(typeInfo);
  SetLength(Result, Dim*2);

  I := 0;
  while I < dim*2 do
  begin
    Result[I] := 0;   // Always use 0 as low-bound in low/high pair
    Inc(I);
    if P <> nil then
    begin
      Result[I] := DynArraySize(P)-1; // Adjust for 0-base low-bound
      P := PPointerArray(p)[0];       // Assume rectangular arrays
    end;
    Inc(I);
  end;
end;

// The dynamicArrayTypeInformation contains the VariantType of the element type
// when the kind == tkDynArray. This function returns that VariantType.
function DynArrayVarType(typeInfo: PDynArrayTypeInfo): Integer;
begin
  Result := varNull;
  if (typeInfo <> nil) and (typeInfo.Kind = tkDynArray) then
  begin
    Inc(PChar(typeInfo), Length(typeInfo.name));
    Result := typeInfo.varType;
  end;

  { NOTE: DECL.H and SYSTEM.PAS have different values for varString }
  if Result = $48 then
    Result := varString;
end;

// Copy Contents of Dynamic Array to Variant
// NOTE: The Dynamic array must be rectangular
//       The Dynamic array must contain items whose type is Automation compatible
// In case of failure, the function returns with a Variant of type VT_EMPTY.
procedure DynArrayToVariant(var V: Variant; const DynArray: Pointer; TypeInfo: Pointer);
var
  LVarBounds, Bounds, Indices: TBoundArray;
  DAVarType, VVarType, DynDim: Integer;
  PDAData: Pointer;
  Value: Variant;
begin
  LVarBounds := nil;
  Bounds    := nil;
  { This resets the Variant to VT_EMPTY - flag which is used to determine whether the }
  { the cast to Variant succeeded or not }
  VarClear(V);

  { Get variantType code from DynArrayTypeInfo }
  DAVarType := DynArrayVarType(PDynArrayTypeInfo(TypeInfo));

  { Validate the Variant Type }
  if ((DAVarType > varNull) and (DAVarType <= varByte)) or (DAVarType = varString) then
  begin
    {NOTE: Map varString to varOleStr for SafeArrayCreate call }
    if DAVarType = varString then
      VVarType := varOleStr
    else
      VVarType := DAVarType;

    { Get dimension of Dynamic Array }
    DynDim := DynarrayDim(PDynArrayTypeInfo(TypeInfo));

    { If more than one dimension, make sure we're dealing with a rectangular array }
    if DynDim > 1 then
      if not IsDynArrayRectangular(DynArray, PDynArrayTypeInfo(TypeInfo)) then
        Exit;

    { Get Variant-style Bounds (lo/hi pair) of Dynamic Array }
    LVarBounds := DynArrayVariantBounds(DynArray, TypeInfo);

    { Get DynArray Bounds }
    Bounds := DynArrayBounds(DynArray, TypeInfo);
    Indices:= Copy(Bounds);

    { Create Variant of SAFEARRAY }
    V := VarArrayCreate(LVarBounds, VVarType);
    Assert(VarArrayDimCount(V) = DynDim);

    repeat
      PDAData := DynArrayIndex(DynArray, Indices, TypeInfo);
      if PDAData <> nil then
      begin
        case DAVarType of
          varSmallInt:  Value := PSmallInt(PDAData)^;
          varInteger:   Value := PInteger(PDAData)^;
          varSingle:    value := PSingle(PDAData)^;
          varDouble:    value := PDouble(PDAData)^;
          varCurrency:  Value := PCurrency(PDAData)^;
          varDate:      Value := PDouble(PDAData)^;
          varOleStr:    Value := PWideString(PDAData)^;
          varDispatch:  Value := PDispatch(PDAData)^;
          varError:     Value := Integer(PError(PDAData)^);
          varBoolean:   Value := PWordBool(PDAData)^;
          varVariant:   Value := PVariant(PDAData)^;
          varUnknown:   Value := PUnknown(PDAData)^;
          varShortInt:  Value := PShortInt(PDAData)^;
          varByte:      Value := PByte(PDAData)^;
          varWord:      Value := PWord(PDAData)^;
          varLongWord:  Value := PLongWord(PDAData)^;
          varString:    Value := PString(PDAData)^;
        else
          VarClear(Value);
        end; { case }
        VarArrayPut(V, Value, Indices);
      end;
    until not DecIndices(Indices, Bounds);
  end;
end;

// Copies data from the Variant to the DynamicArray
procedure DynArrayFromVariant(var DynArray: Pointer; const V: Variant; TypeInfo: Pointer);
var
  DADimCount, VDimCount: Integer;
  DAVarType, I: Integer;
  lengthVec: System.PLongInt;
  Bounds, Indices: TBoundArray;
  Value: Variant;
  PDAData: Pointer;
begin
  { Get Variant information }
  VDimCount:= VarArrayDimCount(V);

  { Allocate vector for lengths }
  GetMem(lengthVec, VDimCount * sizeof(Integer));

  { Initialize lengths - NOTE: VarArrayxxxxBound are 1-based.}
  for I := 0  to  VDimCount-1 do
    PIntegerArray(lengthVec)[I]:= (VarArrayHighBound(V, I+1) - VarArrayLowBound(V, I+1)) + 1;

  { Set Length of DynArray }
  DynArraySetLength(DynArray, PDynArrayTypeInfo(TypeInfo), VDimCount, lengthVec);

  { Get DynArray information }
  DADimCount:= DynArrayDim(PDynArrayTypeInfo(TypeInfo));
  DAVarType := DynArrayVarType(PDynArrayTypeInfo(TypeInfo));
  Assert(VDimCount = DADimCount);

  { Get DynArray Bounds }
  Bounds := DynArrayBounds(DynArray, TypeInfo);
  Indices:= Copy(Bounds);

  { Copy data over}
  repeat
    Value   := VarArrayGet(V, Indices);
    PDAData := DynArrayIndex(DynArray, Indices, TypeInfo);
    case DAVarType of
      varSmallInt:  PSmallInt(PDAData)^   := Value;
      varInteger:   PInteger(PDAData)^    := Value;
      varSingle:    PSingle(PDAData)^     := Value;
      varDouble:    PDouble(PDAData)^     := Value;
      varCurrency:  PCurrency(PDAData)^   := Value;
      varDate:      PDouble(PDAData)^     := Value;
      varOleStr:    PWideString(PDAData)^ := Value;
      varDispatch:  PDispatch(PDAData)^   := Value;
      varError:     PError(PDAData)^      := Value;
      varBoolean:   PWordBool(PDAData)^   := Value;
      varVariant:   PVariant(PDAData)^    := Value;
      varUnknown:   PUnknown(PDAData)^    := Value;
      varShortInt:  PShortInt(PDAData)^   := Value;
      varByte:      PByte(PDAData)^       := Value;
      varWord:      PWord(PDAData)^       := Value;
      varLongWord:  PLongWord(PDAData)^   := Value;
      varString:    PString(PDAData)^     := Value;
    end; { case }
  until not DecIndices(Indices, Bounds);

  { Free vector of lengths }
  FreeMem(lengthVec);
end;

{ TCustomVariantType support }

var
  LVarTypes: array of TCustomVariantType;
  LNextVarType: Integer = CFirstUserType;
  LVarTypeSync: TMultiReadExclusiveWriteSynchronizer;

procedure ClearVariantTypeList;
var
  I: Integer;
begin
  LVarTypeSync.BeginWrite;
  try
    for I := Length(LVarTypes) - 1 downto 0 do
      if LVarTypes[I] <> CInvalidCustomVariantType then
        LVarTypes[I].Free;
  finally
    LVarTypeSync.EndWrite;
  end;
end;

{ TCustomVariantType }

procedure TCustomVariantType.BinaryOp(var Left: TVarData;
  const Right: TVarData; const Operator: TVarOp);
begin
  RaiseInvalidOp;
end;

procedure TCustomVariantType.Cast(var Dest: TVarData; const Source: TVarData);
var
  LSourceHandler: TCustomVariantType;
begin
  if FindCustomVariantType(Source.VType, LSourceHandler) then
    LSourceHandler.CastTo(Dest, Source, VarType)
  else
    RaiseCastError;
end;

procedure TCustomVariantType.CastTo(var Dest: TVarData; const Source: TVarData;
  const AVarType: TVarType);
var
  LSourceHandler: TCustomVariantType;
begin
  if (AVarType <> VarType) and
     FindCustomVariantType(Source.VType, LSourceHandler) then
    LSourceHandler.CastTo(Dest, Source, AVarType)
  else
    RaiseCastError;
end;

procedure TCustomVariantType.Compare(const Left, Right: TVarData;
  var Relationship: TVarCompareResult);
begin
  RaiseInvalidOp;
end;

function TCustomVariantType.CompareOp(const Left, Right: TVarData;
  const Operator: TVarOp): Boolean;
const
  CRelationshipToBoolean: array [opCmpEQ..opCmpGE, TVarCompareResult] of Boolean =
  //  crLessThan, crEqual, crGreaterThan
    ((False,      True,    False), // opCmpEQ = 14;
     (True,       False,   True),  // opCmpNE = 15;
     (True,       False,   False), // opCmpLT = 16;
     (True,       True,    False), // opCmpLE = 17;
     (False,      False,   True),  // opCmpGT = 18;
     (False,      True,    True)); // opCmpGE = 19;
var
  LRelationship: TVarCompareResult;
begin
  Compare(Left, Right, LRelationship);
  Result := CRelationshipToBoolean[Operator, LRelationship];
end;

procedure TCustomVariantType.CastToOle(var Dest: TVarData;
  const Source: TVarData);
var
  LBestOleType: TVarType;
begin
  if OlePromotion(Source, LBestOleType) then
    CastTo(Dest, Source, LBestOleType)
  else
    RaiseCastError;
end;

constructor TCustomVariantType.Create;
begin
  Create(LNextVarType);
  Inc(LNextVarType);
end;

constructor TCustomVariantType.Create(RequestedVarType: TVarType);
var
  LSlot, LWas, LNewLength, I: Integer;
begin
  inherited Create;

  LVarTypeSync.BeginWrite;
  try
    LSlot := RequestedVarType - CMinVarType;
    LWas := Length(LVarTypes);
    if LSlot >= LWas then
    begin
      LNewLength := ((LSlot div CIncVarType) + 1) * CIncVarType;
      if LNewLength > CMaxVarType then
        raise EVariantError.Create(SVarTypeTooManyCustom);
      SetLength(LVarTypes, LNewLength);
      for I := LWas to Length(LVarTypes) - 1 do
        LVarTypes[I] := nil;
    end;
    if LVarTypes[LSlot] <> nil then
      if LVarTypes[LSlot] = CInvalidCustomVariantType then
        raise EVariantError.CreateFmt(SVarTypeNotUsable, [RequestedVarType])
      else
        raise EVariantError.CreateFmt(SVarTypeAlreadyUsed, [RequestedVarType, LVarTypes[LSlot].ClassName]);
    LVarTypes[LSlot] := Self;
    FVarType := RequestedVarType;
  finally
    LVarTypeSync.EndWrite;
  end;
end;

destructor TCustomVariantType.Destroy;
begin
  LVarTypeSync.BeginWrite;
  try
    LVarTypes[VarType - CMinVarType] := CInvalidCustomVariantType;
  finally
    LVarTypeSync.EndWrite;
  end;

  inherited;
end;

function TCustomVariantType.IsClear(const V: TVarData): Boolean;
begin
  Result := False;
end;

function TCustomVariantType.LeftPromotion(const V: TVarData;
  const Operator: TVarOp; out RequiredVarType: TVarType): Boolean;
begin
  RequiredVarType := VarType;
  Result := True;
end;

function TCustomVariantType.OlePromotion(const V: TVarData;
  out RequiredVarType: TVarType): Boolean;
begin
  RequiredVarType := varOleStr;
  Result := True;
end;

procedure TCustomVariantType.RaiseCastError;
begin
  VarCastError;
end;

procedure TCustomVariantType.RaiseInvalidOp;
begin
  VarInvalidOp;
end;

procedure TCustomVariantType.RaiseDispError;
begin
  _DispInvokeError;
end;

function TCustomVariantType.RightPromotion(const V: TVarData;
  const Operator: TVarOp; out RequiredVarType: TVarType): Boolean;
begin
  RequiredVarType := VarType;
  Result := True;
end;

procedure TCustomVariantType.SimplisticClear(var V: TVarData);
begin
  VarDataInit(V);
end;

procedure TCustomVariantType.SimplisticCopy(var Dest: TVarData;
  const Source: TVarData; const Indirect: Boolean);
begin
  if Indirect and VarDataIsByRef(Source) then
    VarDataCopyNoInd(Dest, Source)
  else
    Dest := Source; // simple block copy
end;

procedure TCustomVariantType.UnaryOp(var Right: TVarData; const Operator: TVarOp);
begin
  RaiseInvalidOp;
end;

procedure TCustomVariantType.VarDataInit(var Dest: TVarData);
begin
  _VarInit(Dest);
end;

procedure TCustomVariantType.VarDataClear(var Dest: TVarData);
begin
  _VarClear(Dest);
end;

procedure TCustomVariantType.VarDataCopy(var Dest: TVarData; const Source: TVarData);
begin
  VarCopyCommon(Dest, Source, False);
end;

procedure TCustomVariantType.VarDataCopyNoInd(var Dest: TVarData; const Source: TVarData);
begin
  VarCopyCommon(Dest, Source, True);
end;

procedure TCustomVariantType.VarDataCast(var Dest: TVarData;
  const Source: TVarData);
begin
  VarDataCastTo(Dest, Source, VarType);
end;

procedure TCustomVariantType.VarDataCastTo(var Dest: TVarData;
  const Source: TVarData; const AVarType: TVarType);
begin
  VarChangeType(Dest, Source, AVarType);
end;

procedure TCustomVariantType.VarDataCastTo(var Dest: TVarData;
  const AVarType: TVarType);
begin
  VarDataCastTo(Dest, Dest, AVarType);
end;

procedure TCustomVariantType.VarDataCastToOleStr(var Dest: TVarData);
begin
  if Dest.VType = varString then
    _VarStringToOleStr(Dest, Dest)
  else
    VarDataCastTo(Dest, Dest, varOleStr);
end;

function TCustomVariantType.VarDataIsArray(const V: TVarData): Boolean;
begin
  Result := (V.VType and varArray) <> 0;
end;

function TCustomVariantType.VarDataIsByRef(const V: TVarData): Boolean;
begin
  Result := (V.VType and varByRef) <> 0;
end;

procedure TCustomVariantType.DispInvoke(var Dest: TVarData;
  const Source: TVarData; CallDesc: PCallDesc; Params: Pointer);
begin
  RaiseDispError;
end;

function TCustomVariantType.VarDataIsEmptyParam(const V: TVarData): Boolean;
begin
  Result := VarIsEmptyParam(Variant(V));
end;

procedure TCustomVariantType.VarDataFromStr(var V: TVarData; const Value: string);
begin
  _VarFromPStr(V, Value);
end;

procedure TCustomVariantType.VarDataFromOleStr(var V: TVarData; const Value: WideString);
begin
  _VarFromWStr(V, Value);
end;

function TCustomVariantType._AddRef: Integer;
begin
  Result := -1;
end;

function TCustomVariantType._Release: Integer;
begin
  Result := -1;
end;

function TCustomVariantType.QueryInterface(const IID: TGUID; out Obj): HResult;
const
  E_NOINTERFACE = HResult($80004002);
begin
  if GetInterface(IID, Obj) then
    Result := S_OK
  else
    Result := E_NOINTERFACE;
end;

function TCustomVariantType.VarDataIsNumeric(const V: TVarData): Boolean;
begin
  Result := VarTypeIsNumeric(V.VType);
end;

function TCustomVariantType.VarDataIsOrdinal(const V: TVarData): Boolean;
begin
  Result := VarTypeIsOrdinal(V.VType);
end;

function TCustomVariantType.VarDataIsStr(const V: TVarData): Boolean;
begin
  Result := VarTypeIsStr(V.VType);
end;

function TCustomVariantType.VarDataIsFloat(const V: TVarData): Boolean;
begin
  Result := VarTypeIsFloat(V.VType);
end;

function TCustomVariantType.VarDataToStr(const V: TVarData): string;
begin
  Result := VarToStr(Variant(V));
end;

{ TInvokeableVariantType }

procedure TInvokeableVariantType.DispInvoke(var Dest: TVarData; const Source: TVarData;
  CallDesc: PCallDesc; Params: Pointer);
type
  PParamRec = ^TParamRec;
  TParamRec = array[0..3] of LongInt;
  TStringDesc = record
    BStr: WideString;
    PStr: PString;
  end;
const
  CDoMethod    = $01;
  CPropertyGet = $02;
  CPropertySet = $04;
var
  LArguments: TVarDataArray;
  LStrings: array of TStringDesc;
  LStrCount: Integer;
  LParamPtr: Pointer;

  procedure ParseParam(I: Integer);
  const
    CArgTypeMask    = $7F;
    CArgByRef       = $80;
  var
    LArgType: Integer;
    LArgByRef: Boolean;
  begin
    LArgType := CallDesc^.ArgTypes[I] and CArgTypeMask;
    LArgByRef := (CallDesc^.ArgTypes[I] and CArgByRef) <> 0;

    // error is an easy expansion
    if LArgType = varError then
      SetClearVarToEmptyParam(LArguments[I])

    // literal string
    else if LArgType = varStrArg then
    begin
      with LStrings[LStrCount] do
        if LArgByRef then
        begin
          //BStr := StringToOleStr(PString(ParamPtr^)^);
          BStr := System.Copy(PString(LParamPtr^)^, 1, MaxInt);
          PStr := PString(LParamPtr^);
          LArguments[I].VType := varOleStr or varByRef;
          LArguments[I].VOleStr := @BStr;
        end
        else
        begin
          //BStr := StringToOleStr(PString(ParamPtr)^);
          BStr := System.Copy(PString(LParamPtr)^, 1, MaxInt);
          PStr := nil;
          LArguments[I].VType := varOleStr;
          LArguments[I].VOleStr := PWideChar(BStr);
        end;
      Inc(LStrCount);
    end

    // value is by ref
    else if LArgByRef then
    begin
      if (LArgType = varVariant) and
         (PVarData(LParamPtr^)^.VType = varString) then
        //VarCast(PVariant(ParamPtr^)^, PVariant(ParamPtr^)^, varOleStr);
        VarDataCastTo(PVarData(LParamPtr^)^, PVarData(LParamPtr^)^, varOleStr);
      LArguments[I].VType := LArgType or varByRef;
      LArguments[I].VPointer := Pointer(LParamPtr^);
    end

    // value is a variant
    else if LArgType = varVariant then
      if PVarData(LParamPtr)^.VType = varString then
      begin
        with LStrings[LStrCount] do
        begin
          //BStr := StringToOleStr(string(PVarData(ParamPtr)^.VString));
          BStr := System.Copy(string(PVarData(LParamPtr)^.VString), 1, MaxInt);
          PStr := nil;
          LArguments[I].VType := varOleStr;
          LArguments[I].VOleStr := PWideChar(BStr);
        end;
        Inc(LStrCount);
      end
      else
      begin
        LArguments[I] := PVarData(LParamPtr)^;
        Inc(Integer(LParamPtr), SizeOf(TVarData) - SizeOf(Pointer));
      end
    else
    begin
      LArguments[I].VType := LArgType;
      case CVarTypeToElementInfo[LArgType].Size of
        1, 2, 4:
        begin
          LArguments[I].VLongs[1] := PParamRec(LParamPtr)^[0];
        end;
        8:
        begin
          LArguments[I].VLongs[1] := PParamRec(LParamPtr)^[0];
          LArguments[I].VLongs[2] := PParamRec(LParamPtr)^[1];
          Inc(Integer(LParamPtr), 8 - SizeOf(Pointer));
        end;
      else
        RaiseDispError;
      end;
    end;
    Inc(Integer(LParamPtr), SizeOf(Pointer));
  end;

var
  I, LArgCount: Integer;
  LIdent: string;
  LTemp: TVarData;
begin
  // Grab the identifier
  LArgCount := CallDesc^.ArgCount;
  LIdent := Uppercase(String(PChar(@CallDesc^.ArgTypes[LArgCount])));

  // Parse the arguments
  LParamPtr := Params;
  SetLength(LArguments, LArgCount);
  LStrCount := 0;
  SetLength(LStrings, LArgCount);
  for I := 0 to LArgCount - 1 do
    ParseParam(I);

  // What type of invoke is this?
  case CallDesc^.CallType of
    CDoMethod:
      // procedure with N arguments
      if @Dest = nil then
      begin
        if not DoProcedure(Source, LIdent, LArguments) then
        begin

          // ok maybe its a function but first we must make room for a result
          VarDataInit(LTemp);
          try

            // notate that the destination shouldn't be bothered with
            // functions can still return stuff, we just do this so they
            //  can tell that they don't need to if they don't want to
            SetClearVarToEmptyParam(LTemp);

            // ok lets try for that function
            if not DoFunction(LTemp, Source, LIdent, LArguments) then
              RaiseDispError;
          finally
            VarDataClear(LTemp);
          end;
        end
      end

      // property get or function with 0 argument
      else if LArgCount = 0 then
      begin
        if not GetProperty(Dest, Source, LIdent) and
           not DoFunction(Dest, Source, LIdent, LArguments) then
          RaiseDispError;
      end

      // function with N arguments
      else if not DoFunction(Dest, Source, LIdent, LArguments) then
        RaiseDispError;

    CPropertyGet:
      if not ((@Dest <> nil) and                        // there must be a dest
              (LArgCount = 0) and                       // only no args
              GetProperty(Dest, Source, LIdent)) then   // get op be valid
        RaiseDispError;

    CPropertySet:
      if not ((@Dest = nil) and                         // there can't be a dest
              (LArgCount = 1) and                       // can only be one arg
              SetProperty(Source, LIdent, LArguments[0])) then // set op be valid
        RaiseDispError;
  else
    RaiseDispError;
  end;

  // copy back the string info
  I := LStrCount;
  while I <> 0 do
  begin
    Dec(I);
    with LStrings[I] do
      if Assigned(PStr) then
        PStr^ := System.Copy(BStr, 1, MaxInt);
  end;
end;

function TInvokeableVariantType.GetProperty(var Dest: TVarData; const V: TVarData;
  const Name: string): Boolean;
begin
  Result := False;
end;

function TInvokeableVariantType.SetProperty(const V: TVarData; const Name: string;
  const Value: TVarData): Boolean;
begin
  Result := False;
end;

function TInvokeableVariantType.DoFunction(var Dest: TVarData; const V: TVarData;
  const Name: string; const Arguments: TVarDataArray): Boolean;
begin
  Result := False;
end;

function TInvokeableVariantType.DoProcedure(const V: TVarData; const Name: string;
  const Arguments: TVarDataArray): Boolean;
begin
  Result := False;
end;

{ TCustomVariantType support }

function FindCustomVariantType(const AVarType: TVarType; out CustomVariantType: TCustomVariantType): Boolean;
begin
  // fail if no custom types are defined or if the vartype is way out of range
  Result := (LVarTypes <> nil) and
            (AVarType >= CMinVarType) and (AVarType <= CMaxVarType);
  if Result then
  begin
    LVarTypeSync.BeginRead;
    try
      Result := AVarType - CMinVarType < Length(LVarTypes);
      if Result then
      begin
        CustomVariantType := LVarTypes[AVarType - CMinVarType];
        Result := (CustomVariantType <> nil) and
                  (CustomVariantType <> CInvalidCustomVariantType);
      end;
    finally
      LVarTypeSync.EndRead;
    end;
  end;
end;

function FindCustomVariantType(const TypeName: string; out CustomVariantType: TCustomVariantType): Boolean;
var
  I: Integer;
  LPossible: TCustomVariantType;
begin
  Result := False;
  LVarTypeSync.BeginRead;
  try
    for I := Low(LVarTypes) to High(LVarTypes) do
    begin
      LPossible := LVarTypes[I];
      if (LPossible <> nil) and (LPossible <> CInvalidCustomVariantType) and
         SameText(LPossible.ClassName, TypeName) then
      begin
        CustomVariantType := LPossible;
        Result := True;
        Break;
      end;
    end;
  finally
    LVarTypeSync.EndRead;
  end;
end;

function Unassigned: Variant;
begin
  _VarClear(TVarData(Result));
end;

function Null: Variant;
begin
  _VarClear(TVarData(Result));
  TVarData(Result).VType := varNull;
end;

initialization
  SetClearVarToEmptyParam(TVarData(EmptyParam));

  VarDispProc := @_DispInvokeError;
  ClearAnyProc := @VarInvalidOp;
  ChangeAnyProc := @VarCastError;
  RefAnyProc := @VarInvalidOp;

  GVariantManager.VarClear := @_VarClear;
  GVariantManager.VarCopy := @_VarCopy;
  GVariantManager.VarCopyNoInd := @_VarCopyNoInd;
  GVariantManager.VarCast := @_VarCast;
  GVariantManager.VarCastOle := @_VarCastOle;

  GVariantManager.VarToInt := @_VarToInt;
  GVariantManager.VarToInt64 := @_VarToInt64;
  GVariantManager.VarToBool := @_VarToBool;
  GVariantManager.VarToReal := @_VarToReal;
  GVariantManager.VarToCurr := @_VarToCurr;
  GVariantManager.VarToPStr := @_VarToPStr;
  GVariantManager.VarToLStr := @_VarToLStr;
  GVariantManager.VarToWStr := @_VarToWStr;
  GVariantManager.VarToIntf := @_VarToIntf;
  GVariantManager.VarToDisp := @_VarToDisp;
  GVariantManager.VarToDynArray := @_VarToDynArray;

  GVariantManager.VarFromInt := @_VarFromInt;
  GVariantManager.VarFromInt64 := @_VarFromInt64;
  GVariantManager.VarFromBool := @_VarFromBool;
  GVariantManager.VarFromReal := @_VarFromReal;
  GVariantManager.VarFromTDateTime := @_VarFromTDateTime;
  GVariantManager.VarFromCurr := @_VarFromCurr;
  GVariantManager.VarFromPStr := @_VarFromPStr;
  GVariantManager.VarFromLStr := @_VarFromLStr;
  GVariantManager.VarFromWStr := @_VarFromWStr;
  GVariantManager.VarFromIntf := @_VarFromIntf;
  GVariantManager.VarFromDisp := @_VarFromDisp;
  GVariantManager.VarFromDynArray := @_VarFromDynArray;
  GVariantManager.OleVarFromPStr := @_OleVarFromPStr;
  GVariantManager.OleVarFromLStr := @_OleVarFromLStr;
  GVariantManager.OleVarFromVar := @_OleVarFromVar;
  GVariantManager.OleVarFromInt := @_OleVarFromInt;

  GVariantManager.VarOp := @_VarOp;
  GVariantManager.VarCmp := @_VarCmp;
  GVariantManager.VarNeg := @_VarNeg;
  GVariantManager.VarNot := @_VarNot;

  GVariantManager.DispInvoke := @_DispInvoke;
  GVariantManager.VarAddRef := @_VarAddRef;

  GVariantManager.VarArrayRedim := @_VarArrayRedim;
  GVariantManager.VarArrayGet := @_VarArrayGet;
  GVariantManager.VarArrayPut := @_VarArrayPut;

  GVariantManager.WriteVariant := @_WriteVariant;
  GVariantManager.Write0Variant := @_Write0Variant;

  GetVariantManager(GOldVariantManager);
  SetVariantManager(GVariantManager);

  LVarTypeSync := TMultiReadExclusiveWriteSynchronizer.Create;
finalization
  ClearVariantTypeList;
  FreeAndNil(LVarTypeSync);

  SetVariantManager(GOldVariantManager);
end.
