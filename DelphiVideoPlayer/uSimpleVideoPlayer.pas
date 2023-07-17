unit uSimpleVideoPlayer;

interface

uses
  Winapi.Windows, Winapi.Messages, Winapi.CommCtrl,
  System.SysUtils, System.Variants, System.Classes,
  Vcl.Graphics, Vcl.Controls, Vcl.Forms,
  Vcl.Dialogs, Vcl.StdCtrls,
  uMultimediaTimer, Vcl.ComCtrls, Vcl.ExtCtrls, 
  uVideoPlayer, uUtils;

type
  TfrmSimpleVideoPlayer = class(TForm)
    edtFileName: TEdit;
    btnLoad: TButton;
    btnStart: TButton;
    btnStop: TButton;
    btnUnload: TButton;
    img: TImage;
    tbSeekPlayer: TTrackBar;
    lblCurrentPosition: TLabel;
    lblFinalPosition: TLabel;
    cbAutoReset: TCheckBox;

    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);

    procedure btnStartClick(Sender: TObject);
    procedure btnStopClick(Sender: TObject);
    procedure btnLoadClick(Sender: TObject);
    procedure btnUnloadClick(Sender: TObject);
    procedure cbAutoResetClick(Sender: TObject);
  private
    fVideoPlayer: TVideoPlayer;
    fBitmap: TBitmap;
    fBmpInfo: BITMAPINFO;
    fTimer: TTimer;
    fFPS, fPrevFPS: Integer;
    fCanChangeTheSlider: Boolean;

    procedure OnVideoPlayerImage(pFrameData: TFrameData);
    procedure OnFPSTimer(Sender: TObject);
  protected
    procedure HandleTrackBarDown(var msg: TMessage); message WM_HSCROLL;
  end;

var
  frmSimpleVideoPlayer: TfrmSimpleVideoPlayer;

implementation

{$R *.dfm}

procedure TfrmSimpleVideoPlayer.FormCreate(Sender: TObject);
begin
  DoubleBuffered := True;

  fCanChangeTheSlider := True;
  
  fTimer := TTimer.Create(nil);
  fTimer.Interval := 1000;
  fTimer.OnTimer := OnFPSTimer;
  fTimer.Enabled := True;

  fVideoPlayer := TVideoPlayer.Create;
  fVideoPlayer.SetOutputWidth(640);
  fVideoPlayer.SetOutputHeight(480);
  fVideoPlayer.SetImageCallback(OnVideoPlayerImage);

  fBitmap := TBitmap.Create;
  fBitmap.SetSize(640, 480);
  fBitmap.PixelFormat := pf24bit;
  fBitmap.Canvas.Brush.Color := clBlack;
  fBitmap.Canvas.Rectangle(0, 0, 640, 480);
  img.Picture.Assign(fBitmap);

  FillChar(fBmpInfo, Sizeof(fBmpInfo), 0);
  fBmpInfo.bmiHeader.biSize          := sizeof(BITMAPINFO) - sizeof(RGBQUAD);
  fBmpInfo.bmiHeader.biWidth         := 640;
  fBmpInfo.bmiHeader.biHeight        := -480;
  fBmpInfo.bmiHeader.biPlanes        := 1;
  fBmpInfo.bmiHeader.biBitCount      := 24;
  fBmpInfo.bmiHeader.biCompression   := BI_RGB;
end;

procedure TfrmSimpleVideoPlayer.FormDestroy(Sender: TObject);
begin
  fTimer.Free;
  fVideoPlayer.Free;
  fBitmap.Free;
end;

procedure TfrmSimpleVideoPlayer.btnLoadClick(Sender: TObject);
begin
  fVideoPlayer.LoadFile(AnsiString(edtFileName.Text));
  tbSeekPlayer.Max := fVideoPlayer.GetStreamDuration;
  lblFinalPosition.Caption := FormatVideoDate(FromSeconds(fVideoPlayer.GetStreamDuration));
end;

procedure TfrmSimpleVideoPlayer.btnStartClick(Sender: TObject);
begin
  fVideoPlayer.Play;
end;

procedure TfrmSimpleVideoPlayer.btnStopClick(Sender: TObject);
begin
  fVideoPlayer.Stop;
end;

procedure TfrmSimpleVideoPlayer.btnUnloadClick(Sender: TObject);
begin
  fVideoPlayer.Flush;
end;

procedure TfrmSimpleVideoPlayer.cbAutoResetClick(Sender: TObject);
begin
  fVideoPlayer.SetAutoReset(cbAutoReset.Checked);
end;

procedure TfrmSimpleVideoPlayer.OnVideoPlayerImage(pFrameData: TFrameData);
begin
  TThread.Synchronize(nil,
  procedure
  begin
    Inc(fFPS);
    SetDIBits(GetDC(0), fBitmap.Handle, 0, 480, pFrameData.Data, fBmpInfo, DIB_RGB_COLORS);
    img.Picture.Assign(fBitmap);
    img.Canvas.Font.Color := clGreen;
    img.Canvas.Font.Size := 12;
    img.Canvas.Font.Style := [TFontStyle.fsBold];
    img.Canvas.Brush.Style := TBrushStyle.bsClear;
    img.Canvas.TextOut(10, 10, Format('FPS: %d', [fPrevFPS]));
    if fCanChangeTheSlider then
    begin
      SendMessage(tbSeekPlayer.Handle, TBM_SETPOS, 1,
        Round(pFrameData.Position * fVideoPlayer.GetStreamTimeBase));
      lblCurrentPosition.Caption := FormatVideoDate(FromSeconds(Round(pFrameData.Position * fVideoPlayer.GetStreamTimeBase)));
    end;
  end);
end;

procedure TfrmSimpleVideoPlayer.HandleTrackBarDown(var msg: TMessage);
var
  vPosition: Integer;
begin
  // This happens when the user is seeking
  if (msg.LParam = tbSeekPlayer.Handle) and (msg.WParamLo = TB_THUMBTRACK) then
  begin
    fCanChangeTheSlider := False;
    lblCurrentPosition.Caption := FormatVideoDate(FromSeconds(msg.WParamHi));
  end;

  // This happens when the user is done seeking.
  if (msg.LParam = tbSeekPlayer.Handle) and (msg.WParamLo = TB_ENDTRACK) then
  begin
    // We have to send message because reading the "position" field
    // of the trackbar is not updated at this point.
    fCanChangeTheSlider := True;
    vPosition := SendMessageW(tbSeekPlayer.Handle, TBM_GETPOS, 0, 0);
    fVideoPlayer.Seek(vPosition);
  end;
end;

procedure TfrmSimpleVideoPlayer.OnFPSTimer(Sender: TObject);
begin
  fPrevFPS := fFPS;
  fFPS := 0;
end;

end.
