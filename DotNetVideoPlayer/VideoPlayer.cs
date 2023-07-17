using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using FFmpeg.AutoGen;
using FFmpeg;
using System.Windows.Documents;
using System.Windows.Navigation;
using System.Runtime.InteropServices;
using System.Threading;
using System.Text.Json.Serialization;

namespace DotNetVideoPlayer
{

    public unsafe struct FrameData
    {
        public byte[] Data;
        public int Width;
        public int Height;
        public Int64 Position;
    }

    public delegate void ImgCallback(FrameData frameData);

    public  class VideoPlayer
    {
        private unsafe AVFormatContext* _formatContext;
        private unsafe AVCodecContext* _codecContext;
        private unsafe AVCodec* _codec;
        private unsafe SwsContext* _swsContext;
        private unsafe AVPacket* _packet;
        private unsafe AVFrame* _decodedFrame;
        private unsafe AVFrame* _scaledFrame;
        private int _videoStreamIndex;
        private int _outputWidth;
        private int _outputHeight;

        private MultimediaTimer _timer;
        private FrameData _frameDataToShow;
        private bool _playing;
        private bool _loaded;
        private bool _finished;
        private bool _mightHaveMoreFrames;
        private bool _autoReset;
        private Int64 _lastPTS;
        private Int64 _intervalBetweenFrames;

        private ImgCallback _imgCallback;

        public VideoPlayer()
        {
            _outputWidth = 640;
            _outputHeight = 480;
            _timer = new();
            _timer.SetResolution(1);
            _timer.SetInterval(1);
            _timer.SetCallback(HandleTimer);
            _frameDataToShow = new FrameData();
        }

        ~VideoPlayer()
        {
            Flush();
        }

        public bool Play()
        {
            if (_playing)
                return true;
            if (!_loaded)
                return false;
            if (_finished)
            {
                _finished = false;
                Seek(0);
            }
            _timer.Start();
            _playing = true;
            return false;
        }

        public bool Stop()
        {
            if (!_loaded)
                return false;
            if (!_playing)
                return false;
            _timer.Stop();
            _playing = false;
            return true;
        }

        public unsafe int Load(string filePath)
        {
            if (_loaded)
                return -99;

            _formatContext = ffmpeg.avformat_alloc_context();
            if (_formatContext is null)
            {
                Flush();
                return -1;
            }
            fixed (AVFormatContext** fmtCtxPtr = &_formatContext)
                if (ffmpeg.avformat_open_input(fmtCtxPtr, filePath, null, null) != 0)
                {
                    Flush();
                    return -2;
                }
            _videoStreamIndex = ffmpeg.av_find_best_stream(_formatContext, AVMediaType.AVMEDIA_TYPE_VIDEO, -1, -1, null, 0);
            if (_videoStreamIndex < 0)
            {
                Flush();
                return -3;
            }
            _codec = ffmpeg.avcodec_find_decoder(_formatContext->streams[_videoStreamIndex]->codecpar->codec_id);
            if (_codec is null)
            {
                Flush();
                return -4;
            }
            _codecContext = ffmpeg.avcodec_alloc_context3(_codec);
            if (_codecContext is null)
            {
                Flush();
                return -5;
            }
            if (ffmpeg.avcodec_parameters_to_context(_codecContext, _formatContext->streams[_videoStreamIndex]->codecpar) != 0)
            {
                Flush();
                return -6;
            }
            if (ffmpeg.avcodec_open2(_codecContext, _codec, null) != 0)
            {
                Flush();
                return -7;
            }
            _packet = ffmpeg.av_packet_alloc();
            if (_packet is null)
            {
                Flush();
                return -8;
            }
            _decodedFrame = ffmpeg.av_frame_alloc();
            if (_decodedFrame is null)
            {
                Flush();
                return -9;
            }
            _scaledFrame = ffmpeg.av_frame_alloc();
            if (_scaledFrame is null)
            {
                Flush();
                return -10;
            }
            _scaledFrame->format = (int)AVPixelFormat.AV_PIX_FMT_BGR24;
            _scaledFrame->width = _outputWidth;
            _scaledFrame->height = _outputHeight;
            if (ffmpeg.av_frame_get_buffer(_scaledFrame, 0) != 0)
            {
                Flush();
                return -11;
            }

            _loaded = true;
            return 0;
        }

        public unsafe bool Flush()
        {
            Stop();
            
            if (_formatContext is not null)
                fixed (AVFormatContext** ptr = &_formatContext)
                    ffmpeg.avformat_close_input(ptr);
            if (_codecContext is not null)
            {
                ffmpeg.avcodec_close(_codecContext);
                fixed (AVCodecContext** ptr = &_codecContext)
                    ffmpeg.avcodec_free_context(ptr);
            }
            _codec = null;
            if (_packet is not null)
                fixed (AVPacket** ptr = &_packet)
                    ffmpeg.av_packet_free(ptr);
            if (_decodedFrame is not null)
                fixed (AVFrame** ptr = &_decodedFrame)
                    ffmpeg.av_frame_free(ptr);
            if (_scaledFrame is not null)
                fixed (AVFrame** ptr = &_scaledFrame)
                    ffmpeg.av_frame_free(ptr);
            if (_swsContext is not null)
            {
                ffmpeg.sws_freeContext(_swsContext);
                _swsContext = null;
            }
            _loaded = false;
            _mightHaveMoreFrames = false;
            if (_frameDataToShow.Data is not null)
                ClearFrameData(ref _frameDataToShow);
            _lastPTS = 0;
            _intervalBetweenFrames = 0;
            _finished = false;
            
            return true;
        }

        public unsafe void Seek(int position)
        {
            if (!_loaded)
                return;
            var wasPlaying = _playing;
            if (_playing)
                Stop();
            var scaledPos = Math.BigMul(_formatContext->streams[_videoStreamIndex]->time_base.den, position);
            if (scaledPos > _formatContext->duration)
                scaledPos = _formatContext->duration;
            var result = ffmpeg.av_seek_frame(_formatContext, _videoStreamIndex, scaledPos, 0);
            ffmpeg.avcodec_flush_buffers(_codecContext);
            ClearFrameData(ref _frameDataToShow);
            _intervalBetweenFrames = 0;
            _lastPTS = 0;
            if (wasPlaying)
                Play();
        }

        private unsafe AVFrame* ReadNextFrame()
        {
            if (!_loaded)
                return null;

            if (_mightHaveMoreFrames)
            {
                var r = ffmpeg.avcodec_receive_frame(_codecContext, _decodedFrame);
                if ((r < 0) && (r != -ffmpeg.EAGAIN))
                    return null;

                if (r == -ffmpeg.EAGAIN)
                {
                    _mightHaveMoreFrames = false;
                }
                else
                {
                    if (_swsContext is null)
                    {
                        _swsContext = ffmpeg.sws_getCachedContext
                        (
                            _swsContext,
                            _decodedFrame->width,
                            _decodedFrame->height,
                            (AVPixelFormat)_decodedFrame->format,
                            _scaledFrame->width,
                            _scaledFrame->height,
                            (AVPixelFormat)_scaledFrame->format,
                            ffmpeg.SWS_BICUBIC,
                            null, null, null
                        );

                        if (_swsContext is null)
                        {
                            ffmpeg.av_frame_unref(_decodedFrame);
                            return null;
                        }
                    }

                    _mightHaveMoreFrames = true;

                    r = ffmpeg.sws_scale(_swsContext, _decodedFrame->data, _decodedFrame->linesize, 0,
                            _decodedFrame->height, _scaledFrame->data, _scaledFrame->linesize);
                    
                    if (r != _scaledFrame->height)
                    {
                        ffmpeg.av_frame_unref(_decodedFrame);
                        return null;
                    }

                    ffmpeg.av_frame_copy_props(_scaledFrame, _decodedFrame);
                    ffmpeg.av_frame_unref(_decodedFrame);
                    return _scaledFrame;
                }
            }

            int rr = 0;
            while (true)
            {
                rr = ffmpeg.av_read_frame(_formatContext, _packet);
                if (rr != 0)
                    break;
                if (_packet->stream_index != _videoStreamIndex)
                    continue;

                var r = ffmpeg.avcodec_send_packet(_codecContext, _packet);
                ffmpeg.av_packet_unref(_packet);

                if (r < 0)
                    if (r == -ffmpeg.EAGAIN)
                        continue;
                    else
                        break;

                r = ffmpeg.avcodec_receive_frame(_codecContext, _decodedFrame);
                if (r < 0)
                    if (r == -ffmpeg.EAGAIN)
                        continue;
                    else
                        break;

                if (_swsContext is null)
                {
                    _swsContext = ffmpeg.sws_getCachedContext
                    (
                        _swsContext,
                        _decodedFrame->width,
                        _decodedFrame->height,
                        (AVPixelFormat)_decodedFrame->format,
                        _scaledFrame->width,
                        _scaledFrame->height,
                        (AVPixelFormat)_scaledFrame->format,
                        ffmpeg.SWS_BICUBIC,
                        null, null, null
                    );

                    if (_swsContext is null)
                    {
                        ffmpeg.av_frame_unref(_decodedFrame);
                        return null;
                    }
                }

                _mightHaveMoreFrames = true;

                r = ffmpeg.sws_scale(_swsContext, _decodedFrame->data, _decodedFrame->linesize, 0,
                        _decodedFrame->height, _scaledFrame->data, _scaledFrame->linesize);
                if (r != _scaledFrame->height)
                {
                    ffmpeg.av_frame_unref(_decodedFrame);
                    return null;
                }

                ffmpeg.av_frame_copy_props(_scaledFrame, _decodedFrame);
                ffmpeg.av_frame_unref(_decodedFrame);
                return _scaledFrame;
            }

            if (rr == ffmpeg.AVERROR_EOF)
            {
                if (_autoReset)
                    Seek(0);
                else
                {
                    Stop();
                    _finished = true;
                }
            }

            return null;
        }

        public void SetOutputHeight(int height)
        {
            _outputHeight = height;
        }

        public int GetOutputHeight()
        {
            return _outputHeight;
        }

        public void SetOutputWidth(int width)
        {
            _outputWidth = width;
        }

        public int GetOutputWidth()
        {
            return _outputWidth;
        }

        public ImgCallback GetImgCallback()
        {
            return _imgCallback;
        }

        public void SetImgCallback(ImgCallback imgCallback)
        {
            _imgCallback = imgCallback;
        }

        public void SetAutoReset(bool autoReset)
        {
            _autoReset = autoReset;
        }

        public bool GetAutoReset() 
        {
            return _autoReset;
        }

        public unsafe Int64 GetStreamDuration()
        {
            if (!_loaded)
                return 0;
            var rational = _formatContext->streams[_videoStreamIndex]->time_base;
            return (Int64)Math.Round(_formatContext->streams[_videoStreamIndex]->duration * ffmpeg.av_q2d(rational));
        }

        public unsafe double GetStreamTimeBase()
        {
            if (!_loaded)
                return 0;
            return ffmpeg.av_q2d(_formatContext->streams[_videoStreamIndex]->time_base);
        }

        public unsafe float GetFrameRate()
        {
            if (!_loaded)
                return -1.00f;
            var stream = _formatContext->streams[_videoStreamIndex];
            return stream->avg_frame_rate.num / stream->avg_frame_rate.den;
        }

        public bool IsPlaying()
        {
            return _playing;
        }

        private unsafe void ClearFrameData(ref FrameData frameData)
        {
            frameData.Data = null;
            frameData.Width = 0;
            frameData.Height = 0;
            frameData.Position = 0;
        }

        private unsafe FrameData AVFrameToFrameData(AVFrame* frame)
        {
            var frameData = new FrameData();
            var frameBufferSize = frame->width * frame->height * 3;
            frameData.Data = new byte[frameBufferSize];
            Marshal.Copy((IntPtr)frame->data[0], frameData.Data, 0, frameBufferSize);
            frameData.Height = frame->height;
            frameData.Width = frame->width;
            frameData.Position = frame->pts;
            return frameData;
        }

        private unsafe void HandleTimer(object sender)
        {
            if (_imgCallback is null)
                return;

            if ((_intervalBetweenFrames <= 0) && (_lastPTS == 0))
            {
                var frameOne = ReadNextFrame();
                if (frameOne is null)
                    return;
                _frameDataToShow = AVFrameToFrameData(frameOne);
                var frameTwo = ReadNextFrame();
                if (frameTwo is null)
                    return;
                var timeBase = GetStreamTimeBase() * 1000;
                _intervalBetweenFrames = (Int64)((frameTwo->pts - _frameDataToShow.Position) * timeBase);
                _imgCallback(_frameDataToShow);
                ClearFrameData(ref _frameDataToShow);
                _frameDataToShow = AVFrameToFrameData(frameTwo);
                _lastPTS = _frameDataToShow.Position;
                return;
            }

            if ((_intervalBetweenFrames <= 0) && (_frameDataToShow.Data is not null))
            {
                var frameOne = ReadNextFrame();
                if (frameOne is null)
                    return;
                var frameToShow = _frameDataToShow;
                _frameDataToShow = AVFrameToFrameData(frameOne);
                _intervalBetweenFrames = (Int64)((frameOne->pts - _lastPTS) * (GetStreamTimeBase() * 1000));
                _imgCallback(_frameDataToShow);
                ClearFrameData(ref _frameDataToShow);
                _lastPTS = _frameDataToShow.Position;
            }

            _intervalBetweenFrames -= _timer.GetInterval();
        }
    }
}