unit uUtils;

interface

uses
  System.SysUtils;

type
  TVideoDate = record
    Hour: UInt32;
    Minute: UInt32;
    Second: UInt32;
  end;

function FromSeconds(pSeconds: Int64): TVideoDate;
function FormatVideoDate(pVideoDate: TVideoDate): string;

implementation

function FromSeconds(pSeconds: Int64): TVideoDate;
var
  vCount: Integer;
begin
  Result.Hour := 0;
  Result.Minute := 0;
  Result.Second := 0;
  vCount := 1;
  while True do
  begin
    if pSeconds div (vCount * 3600) > 0 then
      Inc(vCount)
    else
      Break;
  end;
  Result.Hour := vCount - 1;
  pSeconds := pSeconds - (3600 * (vCount - 1));
  vCount := 1;
  while True do
  begin
    if pSeconds div (vCount * 60) > 0 then
      Inc(vCount)
    else
      Break;
  end;
  Result.Minute := vCount - 1;
  pSeconds := pSeconds - (60 * (vCount - 1));
  Result.Second := pSeconds;
end;

function FormatVideoDate(pVideoDate: TVideoDate): string;
begin
  if pVideoDate.Hour < 10 then
    Result := '0';
  Result := Result + pVideoDate.Hour.ToString;

  Result := Result + ':';

  if pVideoDate.Minute < 10 then
    Result := Result + '0';
  Result := Result + pVideoDate.Minute.ToString;

  Result := Result + ':';

  if pVideoDate.Second < 10 then
    Result := Result + '0';
  Result := Result + pVideoDate.Second.ToString;
end;

end.