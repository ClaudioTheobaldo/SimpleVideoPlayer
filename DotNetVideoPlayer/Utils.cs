using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DotNetVideoPlayer
{
    public struct VideoDate
    { 
        public UInt32 Hour;
        public UInt32 Minute;
        public UInt32 Second;
    }

    public static class Utils
    {
        public static VideoDate FromSeconds(Int64 seconds)
        {
            var vd = new VideoDate();
            int count = 1;
            while (true)
            {
                if ((seconds / (count * 3600)) > 0)
                    count++;
                else
                    break;
            }
            vd.Hour = (UInt32)(count - 1);
            seconds = seconds - (3600 * (count - 1));
            count = 1;
            while (true)
            {
                if ((seconds / (count * 60)) > 0)
                    count++;
                else
                    break;
            }
            vd.Minute = (UInt32)(count - 1);
            seconds = seconds - (60 * (count - 1));
            vd.Second = (UInt32)seconds;
            return vd;
        }

        public static string FormatVideoDate(VideoDate vd)
        {
            var s = string.Empty;
            if (vd.Hour < 10)
                s = "0";
            s += vd.Hour.ToString();
            s += ":";
            if (vd.Minute < 10)
                s += "0";
            s += vd.Minute.ToString();
            s += ":";
            if (vd.Second < 10)
                s += "0";
            s += vd.Second.ToString();
            return s;
        }
    }
}
