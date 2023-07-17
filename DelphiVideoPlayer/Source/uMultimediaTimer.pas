unit uMultimediaTimer;

interface

uses
  Winapi.Windows, System.Math, System.Generics.Collections, Winapi.MMSystem;

type
  TMultimediaTimerCallback = procedure(pOpaque: Pointer) of object;

  TMultimediaTimer = class(TObject)
  private
    fTimerID: UInt32;
    fCurrentResolution: UInt32;
    fInterval: UInt32;
    fOpaque: Pointer;
    fCallback: TMultimediaTimerCallback;
    class var fTimerDict: TDictionary<UInt32, TMultimediaTimer>;
  public
    constructor Create; reintroduce;
    destructor Destroy; override;

    procedure Start;
    procedure Stop;
    function SetResolution(pResolution: UInt32): Boolean;
    function GetResolution: UInt32;
    procedure SetInterval(pInterval: UInt32);
    function GetInterval: UInt32;
    procedure SetCallback(pCallback: TMultimediaTimerCallback);
    function GetCallback: TMultimediaTimerCallback;
    function GetOpaque: Pointer;
    procedure SetOpaque(pOpaque: Pointer);
    function IsEnabled: Boolean;
  end;

implementation

procedure HandleTimer(wTimerID: UInt32; msg: UInt32; dwUser: DWORD;
  dw1: DWORD; dw2: DWORD);
var
  vTimer: TMultimediaTimer;
  vCallback: TMultimediaTimerCallback;
begin
  try
    if TMultimediaTimer.fTimerDict.TryGetValue(wTimerID, vTimer) then
    begin
      if vTimer = nil then
        Exit;
      vCallback := vTimer.GetCallback();
      if Assigned(vCallback) then
        vCallback(vTimer.GetOpaque);
    end;
  except
    Exit;
  end;
end;

{ TMultimediaTimer }

constructor TMultimediaTimer.Create;
begin
  fInterval := 100;
  fTimerID := 0;
end;

destructor TMultimediaTimer.Destroy;
begin
  Stop;
  inherited;
end;

function TMultimediaTimer.GetCallback: TMultimediaTimerCallback;
begin
  Result := fCallback;
end;

procedure TMultimediaTimer.SetCallback(pCallback: TMultimediaTimerCallback);
begin
  fCallback := pCallback;
end;

function TMultimediaTimer.GetInterval: UInt32;
begin
  Exit(fInterval);
end;

procedure TMultimediaTimer.SetInterval(pInterval: UInt32);
begin
  fInterval := pInterval;
end;

function TMultimediaTimer.GetOpaque: Pointer;
begin
  Exit(fOpaque);
end;

procedure TMultimediaTimer.SetOpaque(pOpaque: Pointer);
begin
  fOpaque := pOpaque;
end;

function TMultimediaTimer.GetResolution: UInt32;
begin
  Exit(fCurrentResolution);
end;

function TMultimediaTimer.IsEnabled: Boolean;
begin
  Exit(fTimerID > 0);
end;

function TMultimediaTimer.SetResolution(pResolution: UInt32): Boolean;
var
  vTimeInfo: TimeCaps;
  vResult: MMResult;
  vResolution: UInt32;
begin
  vResult := timeGetDevCaps(@vTimeInfo, Sizeof(vTimeInfo));
  if vResult <> TIMERR_NOERROR then
    Exit(False);
  vResolution := Min(Max(vTimeInfo.wPeriodMin, pResolution), vTimeInfo.wPeriodMax);
  fCurrentResolution := vResolution;
  timeBeginPeriod(vResolution);
  Exit(True);
end;

procedure TMultimediaTimer.Start;
begin
  fTimerID := timeSetEvent(fCurrentResolution, 1, @HandleTimer, 0, TIME_PERIODIC);
  if fTimerID <> 0 then
    TMultimediaTimer.fTimerDict.Add(fTimerID, Self);
end;

procedure TMultimediaTimer.Stop;
begin
  if fTimerID <> 0 then
  begin
    timeKillEvent(fTimerID);
    TMultimediaTimer.fTimerDict.Remove(fTimerID);
    fTimerID := 0;
  end;
end;

initialization
  TMultimediaTimer.fTimerDict := TDictionary<UInt32, TMultimediaTimer>.Create;

finalization
  TMultimediaTimer.fTimerDict.Free;

end.
