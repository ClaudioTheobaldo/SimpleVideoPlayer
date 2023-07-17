program SimpleVideoPlayer;

uses
  Vcl.Forms,
  uSimpleVideoPlayer in 'uSimpleVideoPlayer.pas' {frmSimpleVideoPlayer};

{$R *.res}

begin
  ReportMemoryLeaksOnShutdown := True;
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TfrmSimpleVideoPlayer, frmSimpleVideoPlayer);
  Application.Run;
end.
