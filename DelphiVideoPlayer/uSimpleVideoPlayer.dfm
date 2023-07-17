object frmSimpleVideoPlayer: TfrmSimpleVideoPlayer
  Left = 0
  Top = 0
  Caption = 'SimpleVideoPlayer'
  ClientHeight = 599
  ClientWidth = 800
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  TextHeight = 15
  object img: TImage
    Left = 128
    Top = 43
    Width = 640
    Height = 480
  end
  object lblCurrentPosition: TLabel
    Left = 128
    Top = 567
    Width = 27
    Height = 15
    Caption = '00:00'
  end
  object lblFinalPosition: TLabel
    Left = 734
    Top = 567
    Width = 27
    Height = 15
    Caption = '00:00'
  end
  object btnStart: TButton
    Left = 24
    Top = 74
    Width = 75
    Height = 25
    Caption = 'Start'
    TabOrder = 0
    OnClick = btnStartClick
  end
  object btnStop: TButton
    Left = 24
    Top = 105
    Width = 75
    Height = 25
    Caption = 'Stop'
    TabOrder = 1
    OnClick = btnStopClick
  end
  object edtFileName: TEdit
    Left = 24
    Top = 8
    Width = 744
    Height = 23
    TabOrder = 2
    Text = '..\Data\Wildlife.mp4'
  end
  object btnLoad: TButton
    Left = 24
    Top = 43
    Width = 75
    Height = 25
    Caption = 'Load'
    TabOrder = 3
    OnClick = btnLoadClick
  end
  object tbSeekPlayer: TTrackBar
    Left = 128
    Top = 529
    Width = 640
    Height = 32
    Max = 100
    TabOrder = 4
    StyleName = 'Windows'
  end
  object btnUnload: TButton
    Left = 24
    Top = 136
    Width = 75
    Height = 25
    Caption = 'Unload'
    TabOrder = 5
    OnClick = btnUnloadClick
  end
  object cbAutoReset: TCheckBox
    Left = 25
    Top = 176
    Width = 97
    Height = 17
    Caption = 'AutoReset'
    TabOrder = 6
    OnClick = cbAutoResetClick
  end
end
