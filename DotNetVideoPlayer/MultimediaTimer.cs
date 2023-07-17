using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using System.ComponentModel.DataAnnotations;
using System.DirectoryServices;

namespace DotNetVideoPlayer
{
    [StructLayout(LayoutKind.Sequential)]
    public struct TIMECAPS
    {
        public UInt32 wPeriodMin;
        public UInt32 wPeriodMax;
    }

    public class MultimediaTimer
    {
        public unsafe delegate void MMTimerCallback(UInt32 timerID, UInt32 msg, UInt32* dwUser, UInt32* dw1, UInt32* dw2);

        [DllImport("Winmm.dll", EntryPoint = "timeGetDevCaps")]
        public unsafe static extern UInt32 TimeGetDevCaps(TIMECAPS* caps, UInt32 CapSize);
        [DllImport("Winmm.dll", EntryPoint = "timeBeginPeriod")]
        public unsafe static extern UInt32 TimeBeginPeriod(UInt32 resolution);
        [DllImport("Winmm.dll", EntryPoint = "timeSetEvent")]
        public unsafe static extern UInt32 TimeSetEvent(UInt32 delay, UInt32 resolution, IntPtr timerCallback, UInt32* dwUser, UInt32 fuEvent);
        [DllImport("Winmm.dll", EntryPoint = "timeKillEvent")]
        public unsafe static extern UInt32 TimeKillEvent(UInt32 timerID);

        private static Dictionary<UInt32, MultimediaTimer> timerDict = new();
        public delegate void UserCallback(object sender);

        private UInt32 _timerID;
        private UInt32 _interval;
        private UInt32 _resolution;
        private MMTimerCallback _timerCallback;
        private IntPtr _timerCallbackPtr;
        private UserCallback _userCallback;

        public unsafe MultimediaTimer()
        {
            _timerCallback = new MMTimerCallback(MultimediaTimerCallback);
        }

        public unsafe void SetResolution(UInt32 resolution)
        {
            var caps = new TIMECAPS();            
            if (TimeGetDevCaps(&caps, 8) == 0)
            { 
                var timerResolution = Math.Min(Math.Max(caps.wPeriodMin, resolution), caps.wPeriodMax);
                if (TimeBeginPeriod(timerResolution) != 0)
                    throw new Exception("Unable to set required resolution");
                _resolution = timerResolution;
            }
        }

        public unsafe void Start()
        {
            _timerCallbackPtr = Marshal.GetFunctionPointerForDelegate(_timerCallback);
            _timerID = TimeSetEvent(_interval, _resolution, _timerCallbackPtr, null, 1 /*TIME_PERIODIC*/);
            timerDict.Add(_timerID, this);
        }

        public unsafe void Stop()
        {
            if (_timerID == 0)
                return;
            TimeKillEvent(_timerID);
            MultimediaTimer.timerDict.Remove(_timerID);
            _timerID = 0;
        }

        public void SetInterval(UInt32 interval)
        {
            _interval = interval;
        }

        public UInt32 GetInterval()
        {
            return _interval;
        }

        public void SetCallback(UserCallback callback)
        {
            _userCallback += callback;
        }

        public UserCallback GetCallback()
        {
            return _userCallback;
        }

        private unsafe void MultimediaTimerCallback(UInt32 timerID, UInt32 msg, UInt32* dwUser, UInt32* dw1, UInt32* dw2)
        {
            if (!MultimediaTimer.timerDict.TryGetValue(timerID, out var timer))
                return;
            if (timer._userCallback != null)
                try
                {
                    timer._userCallback(this);
                }
                catch
                {
                    //
                }
        }
    }
}
