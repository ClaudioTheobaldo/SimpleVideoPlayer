unit uVideoPlayer;

interface

uses
  uMultimediaTimer,
  libavutil, libavformat, libavcodec_codec, libavutil_frame,
  libavcodec, libavcodec_packet, libavcodec_avfft, libswscale,
  libavutil_error, libavcodec_codec_par, libavutil_pixfmt, libavutil_rational;

type
  TFrameData = record
    Data: PByte;
    Width: Integer;
    Height: Integer;
    Position: Int64;
  end;

  TImgCallback = procedure (pFraneData: TFrameData) of object;

  TVideoPlayer = class(TObject)
  private
    fPlayTimer: TMultimediaTimer;

    // Demuxer;
    fFormatCtx: PAVFormatContext;

    // Decoder
    fCodecCtx: PAVCodecContext;
    fCodec: PAVCodec;

    // Scaler
    fSwsCtx: PSwsContext;

    // Aux variables
    fPacket: PAVPacket;
    fDecodedFrame: PAVFrame;
    fScaledFrame: PAVFrame;

    fVideoStreamIndex: Integer;
    fOutputWidth: Integer;
    fOutputHeight: Integer;
    fLoaded: Boolean;
    fPlaying: Boolean;
    fMightHaveMoreFrames: Boolean;
    fAutoReset: Boolean;
    fFinished: Boolean;

    fIntervalBetweenFrames: Int64;
    fLastFramePTS: Int64;

    fImgCallback: TImgCallback;
    fFrameDataToShow: TFrameData;

    procedure HandleTimer(pOpaque: Pointer);

    function ReadNextFrame: PAVFrame;
    function GetVideoStreamTimeBase: Double;
    function AVFrameToFrameData(pFrame: PAVFrame): TFrameData;
    procedure ClearFrameDataToShow(var pFrame: TFrameData);
  public
    constructor Create; reintroduce;
    destructor Destroy; override;

    function LoadFile(pFilePath: AnsiString): Integer;
    function Play: Boolean;
    function Stop: Boolean;
    procedure Flush;
    procedure Seek(pPosition: UInt64);
    function GetFrameRate: Single;
    function GetStreamDuration: Int64;
    function GetStreamTimeBase: Double;
    procedure SetImageCallback(pCallback: TImgCallback);
    function GetImageCallback: TImgCallback;
    procedure SetOutputWidth(pWidth: Integer);
    function GetOutputWidth: Integer;
    procedure SetOutputHeight(pHeight: Integer);
    function GetOutputHeight: Integer;
    function IsPlaying: Boolean;
    procedure SetAutoReset(pReset: Boolean);
    function GetAutoReset: Boolean;
  end;

implementation

{ TVideoPlayer }

constructor TVideoPlayer.Create;
begin
  fOutputWidth := 320;
  fOutputHeight := 240;

  fPlayTimer := TMultimediaTimer.Create;
  fPlayTimer.SetResolution(1);
  fPlayTimer.SetInterval(1);
  fPlayTimer.SetCallback(HandleTimer);
  fPlayTimer.SetOpaque(Pointer(Self));
end;

destructor TVideoPlayer.Destroy;
begin
  Flush;
  fPlayTimer.Free;
  inherited;
end;

function TVideoPlayer.LoadFile(pFilePath: AnsiString): Integer;
begin
  {$POINTERMATH ON}
  if (fLoaded) then
    Exit(-99);

  fFormatCtx := avformat_alloc_context;
  if (fFormatCtx = nil) then
  begin
    Flush;
    Exit(-1);
  end;

  if (avformat_open_input(@fFormatCtx, PAnsiChar(pFilePath), nil, nil) <> 0) then
  begin
    Flush;
    Exit(-2);
  end;

  fVideoStreamIndex := -1;
  fVideoStreamIndex := av_find_best_stream(fFormatCtx,
    AVMEDIA_TYPE_VIDEO, -1, -1, nil, 0);

  if (fVideoStreamIndex = AVERROR_STREAM_NOT_FOUND) then
  begin
    Flush;
    Exit(-3);
  end;

  fCodec := avcodec_find_decoder(fFormatCtx.streams[fVideoStreamIndex].codecpar.codec_id);
  if (fCodec = nil) then
  begin
    Flush;
    Exit(-4);
  end;

  fCodecCtx := avcodec_alloc_context3(fCodec);
  if (fCodecCtx = nil) then
  begin
    Flush;
    Exit(-5);
  end;

  if (avcodec_parameters_to_context(fCodecCtx, fFormatCtx.streams[fVideoStreamIndex].codecpar) <> 0) then
  begin
    Flush;
    Exit(-6);
  end;

  if avcodec_open2(fCodecCtx, fCodec, nil) <> 0 then
  begin
    Flush;
    Exit(-7);
  end;

  fPacket := av_packet_alloc;
  if (fPacket = nil) then
  begin
    Flush;
    Exit(-8);
  end;

  fDecodedFrame := av_frame_alloc;
  if (fDecodedFrame = nil) then
  begin
    Flush;
    Exit(-9);
  end;

  fScaledFrame := av_frame_alloc;
  if fScaledFrame = nil then
  begin
    Flush;
    Exit(-10);
  end;

  fScaledFrame.width := fOutputWidth;
  fScaledFrame.height := fOutputHeight;
  fScaledFrame.format := Integer(AV_PIX_FMT_BGR24);

  if (av_frame_get_buffer(fScaledFrame, 0) <> 0) then
  begin
    Flush;
    Exit(-11);
  end;

  fLoaded := True;
  Exit(0);

  {$POINTERMATH OFF}
end;

function TVideoPlayer.Play: Boolean;
begin
  if (not fLoaded) then
    Exit(False);

  if (fFinished) then
  begin
    fFinished := False;
    Seek(0);
  end;

  fPlayTimer.Start;
  fPlaying := true;
  Exit(True);
end;

function TVideoPlayer.Stop: Boolean;
begin
  if (not fPlaying) then
    Exit(False);

  fPlayTimer.Stop;
  fPlaying := False;
  Exit(True);
end;

procedure TVideoPlayer.Flush;
begin
  Stop;
  if fFormatCtx <> nil then
    avformat_close_input(@fFormatCtx);
  if fCodecCtx <> nil then
  begin
    avcodec_close(fCodecCtx);
    avcodec_free_context(@fCodecCtx);
  end;
  fCodec := nil;
  if fPacket <> nil then
    av_packet_free(@fPacket);
  if fDecodedFrame <> nil then
    av_frame_free(@fDecodedFrame);
  if fScaledFrame <> nil then
    av_frame_free(@fScaledFrame);
  if fSwsCtx <> nil then
  begin
    sws_freeContext(fSwsCtx);
    fSwsCtx := nil;
  end;
  fLoaded := False;
  fMightHaveMoreFrames := False;
  if fFrameDataToShow.Data <> nil then
    ClearFrameDataToShow(fFrameDataToShow);
  fLastFramePTS := 0;
  fIntervalBetweenFrames := 0;
  fFinished := False;
end;

function TVideoPlayer.GetAutoReset: Boolean;
begin
  Exit(fAutoReset);
end;

function TVideoPlayer.GetFrameRate: Single;
var
  vStream: PAVStream;
begin
  {$POINTERMATH ON}
  if not fLoaded then
    Exit(-1.00);
  vStream := fFormatCtx.streams[fVideoStreamIndex];
  Exit(Single(vStream.avg_frame_rate.num / vStream.avg_frame_rate.den));
  {$POINTERMATH OFF}
end;

function TVideoPlayer.ReadNextFrame: PAVFrame;
var
  vR, vRR: Integer;
begin
  if (not fLoaded) then
    Exit(nil);

  if fMightHaveMoreFrames then
  begin
    vR := avcodec_receive_frame(fCodecCtx, fDecodedFrame);

    if (vR < 0) and (vR <> AVERROR_EAGAIN) then
      Exit(nil);

    if vR = AVERROR_EAGAIN then
    begin
      fMightHaveMoreFrames := False;
    end
    else
    begin
      if (fSwsCtx = nil) then
      begin
        fSwsCtx := sws_getContext(fDecodedFrame.width, fDecodedFrame.height,
          TAVPixelFormat(fDecodedFrame.format),
          fScaledFrame.width, fScaledFrame.height,
          TAVPixelFormat(fScaledFrame.format), SWS_BICUBIC, nil, nil, nil);
        if fSwsCtx = nil then
        begin
          av_frame_unref(fDecodedFrame);
          Exit(nil);
        end;
      end;

      fMightHaveMoreFrames := True;

      vR := sws_scale(fSwsCtx, @fDecodedFrame.data, @fDecodedFrame.linesize, 0,
        fDecodedFrame.height, @fScaledFrame.data, @fScaledFrame.linesize);

      if vR <> fScaledFrame.Height then
      begin
        av_frame_unref(fDecodedFrame);
        Exit(nil);
      end;

      av_frame_copy_props(fScaledFrame, fDecodedFrame);
      av_frame_unref(fDecodedFrame);
      Exit(fScaledFrame);
    end;
  end;

  while True do
  begin
    vRR := av_read_frame(fFormatCtx, fPacket);
    if vRR <> 0 then
      Break;

    if (fPacket.stream_index <> fVideoStreamIndex) then
      Continue;

    vR := avcodec_send_packet(fCodecCtx, fPacket);
    av_packet_unref(fPacket);

    if (vR < 0) then
      if (vR = AVERROR_EAGAIN) then
        Continue
      else
        Break;

    vR := avcodec_receive_frame(fCodecCtx, fDecodedFrame);
    if (vR < 0) then
      if (vR = AVERROR_EAGAIN) then
        Continue
      else
        Break;

    if (fSwsCtx = nil) then
    begin
      fSwsCtx := sws_getContext(fDecodedFrame.width, fDecodedFrame.height,
        TAVPixelFormat(fDecodedFrame.format),
        fScaledFrame.width, fScaledFrame.height,
        TAVPixelFormat(fScaledFrame.format), SWS_BICUBIC, nil, nil, nil);
      if fSwsCtx = nil then
      begin
        av_frame_unref(fDecodedFrame);
        Exit(nil);
      end;
    end;

    fMightHaveMoreFrames := True;

    vR := sws_scale(fSwsCtx, @fDecodedFrame.data, @fDecodedFrame.linesize, 0,
      fDecodedFrame.height, @fScaledFrame.data, @fScaledFrame.linesize);

    if vR <> fScaledFrame.Height then
    begin
      av_frame_unref(fDecodedFrame);
      Exit(nil);
    end;

    av_frame_copy_props(fScaledFrame, fDecodedFrame);
    av_frame_unref(fDecodedFrame);
    Exit(fScaledFrame);
  end;

  if (vRR = AVERROR_EOF) then
  begin
    if fAutoReset then
      Seek(0)
    else
    begin
      Stop;
      fFinished := True;
    end;
  end;

  Exit(nil);
end;

procedure TVideoPlayer.Seek(pPosition: UInt64);
var
  vScaledPos: UInt64;
  vWasPlaying: Boolean;
begin
  {$POINTERMATH ON}
  if (not fLoaded) then
    Exit;

  vWasPlaying := fPlaying;
  if fPlaying then
    Stop;
  {$WARNINGS OFF}
  vScaledPos := fFormatCtx.streams[fVideoStreamIndex].time_base.den * pPosition;
  {$WARNINGS ON}
  if vScaledPos > fFormatCtx.duration then
    av_seek_Frame(fFormatCtx, fVideoStreamIndex, fFormatCtx.duration, 0)
  else
    av_seek_Frame(fFormatCtx, fVideoStreamIndex, vScaledPos, 0);
  avcodec_flush_buffers(fCodecCtx);
  ClearFrameDataToShow(fFrameDataToShow);
  fIntervalBetweenFrames := 0;
  fLastFramePTS := 0;
  if vWasPlaying then
    Play;

  {$POINTERMATH OFF}
end;

procedure TVideoPlayer.SetAutoReset(pReset: Boolean);
begin
  fAutoReset := pReset;
end;

procedure TVideoPlayer.SetImageCallback(pCallback: TImgCallback);
begin
  fImgCallback := pCallback;
end;

procedure TVideoPlayer.SetOutputHeight(pHeight: Integer);
begin
  fOutputHeight := pHeight;
end;

procedure TVideoPlayer.SetOutputWidth(pWidth: Integer);
begin
  fOutputWidth := pWidth;
end;

function TVideoPlayer.GetImageCallback: TImgCallback;
begin
  Result := fImgCallback;
end;

function TVideoPlayer.GetOutputHeight: Integer;
begin
  Exit(fOutputHeight);
end;

function TVideoPlayer.GetOutputWidth: Integer;
begin
  Exit(fOutputWidth);
end;

function TVideoPlayer.GetStreamDuration: Int64;
var
  vAVRational: TAVRational;
begin
  {$POINTERMATH ON}
  if not fLoaded then
    Exit(0);
  vAVRational := fFormatCtx.streams[fVideoStreamIndex].time_base;
  Exit(Round(fFormatCtx.streams[fVideoStreamIndex].duration * av_q2d(vAVRational)));
  {$POINTERMATH OFF}
end;

function TVideoPlayer.GetStreamTimeBase: Double;
begin
  {$POINTERMATH ON}
  if not fLoaded then
    Exit(0);
  Exit(av_q2d(fFormatCtx.streams[fVideoStreamIndex].time_base));
  {$POINTERMATH OFF}
end;

function TVideoPlayer.GetVideoStreamTimeBase: Double;
begin
  {$POINTERMATH ON}
  if not fLoaded then
    Exit(0.00);
  Exit(av_q2d(fFormatCtx.streams[fVideoStreamIndex].time_base));
  {$POINTERMATH OFF}
end;

procedure TVideoPlayer.ClearFrameDataToShow(var pFrame: TFrameData);
begin
  if pFrame.Data <> nil then
    FreeMem(pFrame.Data);
  FillChar(pFrame, Sizeof(pFrame), 0);
  pFrame.Data := nil;
end;

function TVideoPlayer.AVFrameToFrameData(pFrame: PAVFrame): TFrameData;
begin
  Result.Data := GetMemory(pFrame.width * pFrame.height * 3);
  Move(pFrame.data[0][0], Result.Data[0], pFrame.width * pFrame.height * 3);
  Result.Height := pFrame.height;
  Result.Width := pFrame.width;
  Result.Position := pFrame.pts;
end;

procedure TVideoPlayer.HandleTimer(pOpaque: Pointer);
var
  vCallback: TImgCallback;
  vFrameOne, vFrameTwo: PAVFrame;
  vFrameToShow: TFrameData;
  vTimeBase: Double;
begin
  vCallback := fImgCallback;
  if not Assigned(vCallback) then
    Exit;

  if ((fIntervalBetweenFrames <= 0) and (fLastFramePTS = 0)) then
  begin
    vFrameOne := ReadNextFrame;
    if vFrameOne = nil then
      Exit;
    fFrameDataToShow := AVFrameToFrameData(vFrameOne);
    vFrameTwo := ReadNextFrame;
    if vFrameTwo = nil then
      Exit;
    vTimeBase := GetVideoStreamTimeBase * 1000;
    fIntervalBetweenFrames := Round((vFrameTwo.pts - fFrameDataToShow.Position) * vTimeBase);
    vCallback(fFrameDataToShow);
    ClearFrameDataToShow(fFrameDataToShow);
    fFrameDataToShow := AVFrameToFrameData(vFrameTwo);
    fLastFramePTS := fFrameDataToShow.Position;
    Exit;
  end;

  if (fIntervalBetweenFrames <= 0) and (fFrameDataToShow.Data <> nil) then
  begin
    vFrameOne := ReadNextFrame;
    if (vFrameOne = nil) then
      Exit;
    vFrameToShow := fFrameDataToShow;

    fFrameDataToShow := AvFrameToFrameData(vFrameOne);
    fIntervalBetweenFrames := Round((vFrameOne.pts - fLastFramePTS) * (GetVideoStreamTimeBase * 1000));
    vCallback(vFrameToShow);
    ClearFrameDataToShow(vFrameToShow);
    fLastFramePTS := fFrameDataToShow.Position;
  end;

  fIntervalBetweenFrames := fIntervalBetweenFrames - fPlayTimer.GetInterval;
end;

function TVideoPlayer.IsPlaying: Boolean;
begin
  Exit(fPlaying);
end;

end.
