using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using FFmpeg.AutoGen.Bindings.DynamicallyLoaded;

namespace DotNetVideoPlayer
{
    public partial class MainWindow : Window
    {
        private VideoPlayer _videoPlayer;
        private System.Timers.Timer _fpsTimer;
        private UInt32 _fps;
        private UInt32 _prevFPS;
        private bool _canChangeSlider = true;

        public unsafe MainWindow()
        {
            // This is done to point the FFmpeg.AutoGen library to where the
            // binaries are.
            DynamicallyLoadedBindings.LibrariesPath = Directory.GetCurrentDirectory();
            DynamicallyLoadedBindings.Initialize();

            InitializeComponent();
            
            _videoPlayer = new();
            _videoPlayer.SetOutputWidth(640);
            _videoPlayer.SetOutputHeight(480);
            _videoPlayer.SetImgCallback(OnVideoPlayerFrame);

            _fpsTimer = new();
            _fpsTimer.Elapsed += (a,b) => { _prevFPS = _fps;  _fps = 0; };
            _fpsTimer.Interval = 1000;
            _fpsTimer.Enabled = true;

            Start.Click += OnStartClick;
            Stop.Click += OnStopClick;
            Load.Click += OnLoadClick;
            Flush.Click += OnFlushClick;
            videoSlider.ValueChanged += OnValueChanged;
            videoSlider.PreviewMouseDown += OnPreviewMouseDown;
            videoSlider.PreviewMouseUp += OnPreviewMouseUp;
            AutoReset.Click += OnAutoResetClick;
        }

        public void OnLoadClick(object sender, RoutedEventArgs e)
        {
            if (_videoPlayer.Load(FileName.Text) != 0)
                return;
            videoSlider.Minimum = 0;
            videoSlider.Maximum = _videoPlayer.GetStreamDuration();
            lblFinalTime.Content = Utils.FormatVideoDate(Utils.FromSeconds(_videoPlayer.GetStreamDuration()));
        }

        public void OnStartClick(object sender, RoutedEventArgs e)
        {
            _videoPlayer.Play();
            videoSlider.Value = 0;
        }

        public void OnStopClick(object sender, RoutedEventArgs e)
        {
            _videoPlayer.Stop();
        }

        public void OnFlushClick(object sender, RoutedEventArgs e)
        {
            _videoPlayer.Flush();
        }

        public void OnPreviewMouseUp(object sender, MouseEventArgs e)
        {
            var currentPos = videoSlider.Value;
            _videoPlayer.Seek((int)currentPos);
            _canChangeSlider = true;
        }

        public void OnPreviewMouseDown(object sender, MouseEventArgs e)
        {
            _canChangeSlider = false;
        }

        public void OnValueChanged(object sender, RoutedPropertyChangedEventArgs<double> val)
        {
            if (!_canChangeSlider)
                lblCurrentTime.Content = Utils.FormatVideoDate(Utils.FromSeconds((long)val.NewValue));
        }

        public void OnAutoResetClick(object sender, RoutedEventArgs e)
        {
            if (AutoReset.IsChecked is not null)
                _videoPlayer.SetAutoReset(AutoReset.IsChecked.Value);
        }

        public unsafe void OnVideoPlayerFrame(FrameData frame)
        {
            _fps++;
            Application.Current.Dispatcher.Invoke(() =>
            {
            Cnv.Children.Clear();
            var textDraw = new TextBlock();
            textDraw.Text = $"FPS: {_prevFPS}";
            textDraw.FontSize = 14;
            textDraw.FontWeight = FontWeights.Bold;
            textDraw.Foreground = Brushes.Green;
            Cnv.Children.Add(textDraw);

            if (_canChangeSlider)
            {
                lblCurrentTime.Content = Utils.FormatVideoDate(Utils.FromSeconds((long)(frame.Position * _videoPlayer.GetStreamTimeBase())));
                videoSlider.Value = (long)(frame.Position * _videoPlayer.GetStreamTimeBase());
            }

            var bmpImg = BitmapSource.Create(640, 480, 96, 96, PixelFormats.Bgr24, null, frame.Data, 640 * 3);
            var bmpFrame = BitmapFrame.Create(bmpImg);
            imgMain.Source = bmpFrame;
            });
        }
    }
}
