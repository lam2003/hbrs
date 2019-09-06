#pragma once

//self
#include "common/av_define.h"
#include "common/err_code.h"

namespace rs
{
class Utils
{
public:
    static HI_HDMI_VIDEO_FMT_E GetHDMIFmt(VO_INTF_SYNC_E intf_sync)
    {
        HI_HDMI_VIDEO_FMT_E fmt;
        switch (intf_sync)
        {
        case VO_OUTPUT_PAL:
            fmt = HI_HDMI_VIDEO_FMT_PAL;
            break;
        case VO_OUTPUT_NTSC:
            fmt = HI_HDMI_VIDEO_FMT_NTSC;
            break;
        case VO_OUTPUT_1080P24:
            fmt = HI_HDMI_VIDEO_FMT_1080P_24;
            break;
        case VO_OUTPUT_1080P25:
            fmt = HI_HDMI_VIDEO_FMT_1080P_25;
            break;
        case VO_OUTPUT_1080P30:
            fmt = HI_HDMI_VIDEO_FMT_1080P_30;
            break;
        case VO_OUTPUT_1080P50:
            fmt = HI_HDMI_VIDEO_FMT_1080P_50;
            break;
        case VO_OUTPUT_1080P60:
            fmt = HI_HDMI_VIDEO_FMT_1080P_60;
            break;
        case VO_OUTPUT_720P50:
            fmt = HI_HDMI_VIDEO_FMT_720P_50;
            break;
        case VO_OUTPUT_720P60:
            fmt = HI_HDMI_VIDEO_FMT_720P_60;
            break;
        case VO_OUTPUT_1080I50:
            fmt = HI_HDMI_VIDEO_FMT_1080i_50;
            break;
        case VO_OUTPUT_1080I60:
            fmt = HI_HDMI_VIDEO_FMT_1080i_60;
            break;
        case VO_OUTPUT_576P50:
            fmt = HI_HDMI_VIDEO_FMT_576P_50;
            break;
        case VO_OUTPUT_480P60:
            fmt = HI_HDMI_VIDEO_FMT_480P_60;
            break;
        case VO_OUTPUT_800x600_60:
            fmt = HI_HDMI_VIDEO_FMT_VESA_800X600_60;
            break;
        case VO_OUTPUT_1024x768_60:
            fmt = HI_HDMI_VIDEO_FMT_VESA_1024X768_60;
            break;
        case VO_OUTPUT_1280x1024_60:
            fmt = HI_HDMI_VIDEO_FMT_VESA_1280X1024_60;
            break;
        case VO_OUTPUT_1366x768_60:
            fmt = HI_HDMI_VIDEO_FMT_VESA_1366X768_60;
            break;
        case VO_OUTPUT_1440x900_60:
            fmt = HI_HDMI_VIDEO_FMT_VESA_1440X900_60;
            break;
        case VO_OUTPUT_1280x800_60:
            fmt = HI_HDMI_VIDEO_FMT_VESA_1280X800_60;
            break;
        default:
            RS_ASSERT(0);
        }
        return fmt;
    }

    static SIZE_S GetSize(VO_INTF_SYNC_E intf_sync)
    {
        SIZE_S size;

        switch (intf_sync)
        {
        case VO_OUTPUT_PAL:
        case VO_OUTPUT_576P50:
            size.u32Width = 720;
            size.u32Height = 576;
            break;
        case VO_OUTPUT_NTSC:
        case VO_OUTPUT_480P60:
            size.u32Width = 720;
            size.u32Height = 480;
            break;
        case VO_OUTPUT_1080P24:
        case VO_OUTPUT_1080P25:
        case VO_OUTPUT_1080P30:
        case VO_OUTPUT_1080P50:
        case VO_OUTPUT_1080P60:
        case VO_OUTPUT_1080I50:
        case VO_OUTPUT_1080I60:
            size.u32Width = 1920;
            size.u32Height = 1080;
            break;
        case VO_OUTPUT_720P50:
        case VO_OUTPUT_720P60:
            size.u32Width = 1280;
            size.u32Height = 720;
            break;
        case VO_OUTPUT_800x600_60:
            size.u32Width = 800;
            size.u32Height = 600;
            break;
        case VO_OUTPUT_1024x768_60:
            size.u32Width = 1024;
            size.u32Height = 768;
            break;
        case VO_OUTPUT_1280x1024_60:
            size.u32Width = 1280;
            size.u32Height = 1024;
            break;
        case VO_OUTPUT_1366x768_60:
            size.u32Width = 1366;
            size.u32Height = 768;
            break;
        case VO_OUTPUT_1440x900_60:
            size.u32Width = 1400;
            size.u32Height = 900;
            break;
        case VO_OUTPUT_1280x800_60:
            size.u32Width = 1280;
            size.u32Height = 800;
            break;
        default:
            RS_ASSERT(0);
        }
        return size;
    }

    static int32_t GetFrameRate(VO_INTF_SYNC_E intf_sync)
    {
        int32_t frame_rate;

        switch (intf_sync)
        {
        case VO_OUTPUT_PAL:
        case VO_OUTPUT_1080P25:
            frame_rate = 25;
            break;
        case VO_OUTPUT_1080P24:
            frame_rate = 24;
            break;
        case VO_OUTPUT_NTSC:
        case VO_OUTPUT_1080P30:
            frame_rate = 30;
            break;
        case VO_OUTPUT_1080P50:
        case VO_OUTPUT_720P50:
        case VO_OUTPUT_1080I50:
        case VO_OUTPUT_576P50:
            frame_rate = 50;
            break;
        case VO_OUTPUT_1080P60:
        case VO_OUTPUT_720P60:
        case VO_OUTPUT_800x600_60:
        case VO_OUTPUT_1024x768_60:
        case VO_OUTPUT_1280x1024_60:
        case VO_OUTPUT_1366x768_60:
        case VO_OUTPUT_1440x900_60:
        case VO_OUTPUT_1280x800_60:
        case VO_OUTPUT_1080I60:
        case VO_OUTPUT_480P60:
            frame_rate = 60;
            break;
        default:
            RS_ASSERT(0);
        }
        return frame_rate;
    }

    static uint64_t GetSteadyMilliSeconds()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }

    static uint64_t GetSteadyMicroSeconds()
    {
        using namespace std::chrono;
        return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
    }

    static VideoInputFormat GetVIFmt(hd_dis_resolution_e type)
    {
        VideoInputFormat fmt;

        switch (type)
        {
        case RES_1080P60:
        {
            fmt.has_signal = true;
            fmt.width = 1920;
            fmt.height = 1080;
            fmt.interlaced = false;
            fmt.frame_rate = 60;
            break;
        }
        case RES_1080P50:
        {
            fmt.has_signal = true;
            fmt.width = 1920;
            fmt.height = 1080;
            fmt.interlaced = false;
            fmt.frame_rate = 50;
            break;
        }
        case RES_720P50:
        {
            fmt.has_signal = true;
            fmt.width = 1280;
            fmt.height = 720;
            fmt.interlaced = false;
            fmt.frame_rate = 50;
            break;
        }
        case RES_720P60:
        {
            fmt.has_signal = true;
            fmt.width = 1280;
            fmt.height = 720;
            fmt.interlaced = false;
            fmt.frame_rate = 60;
            break;
        }
        case RES_1080I60:
        {
            fmt.has_signal = true;
            fmt.width = 1920;
            fmt.height = 1080;
            fmt.interlaced = true;
            fmt.frame_rate = 60;
            break;
        }
        case RES_1080I50:
        {
            fmt.has_signal = true;
            fmt.width = 1920;
            fmt.height = 1080;
            fmt.interlaced = true;
            fmt.frame_rate = 50;
            break;
        }
        case RES_1080P30:
        {
            fmt.has_signal = true;
            fmt.width = 1920;
            fmt.height = 1080;
            fmt.interlaced = false;
            fmt.frame_rate = 30;
            break;
        }
        case RES_1080P25:
        {
            fmt.has_signal = true;
            fmt.width = 1920;
            fmt.height = 1080;
            fmt.interlaced = false;
            fmt.frame_rate = 25;
            break;
        }
        case RES_NTSC:
        {
            fmt.has_signal = true;
            fmt.width = 720;
            fmt.height = 480;
            fmt.interlaced = false;
            fmt.frame_rate = 30;
            break;
        }
        case RES_PAL:
        {
            fmt.has_signal = true;
            fmt.width = 720;
            fmt.height = 576;
            fmt.interlaced = false;
            fmt.frame_rate = 25;
            break;
        }
        case RES_720P25:
        {
            fmt.has_signal = true;
            fmt.width = 1280;
            fmt.height = 720;
            fmt.interlaced = false;
            fmt.frame_rate = 25;
            break;
        }
        case RES_720P30:
        {
            fmt.has_signal = true;
            fmt.width = 1280;
            fmt.height = 720;
            fmt.interlaced = false;
            fmt.frame_rate = 30;
            break;
        }
        case RES_NONE:
        {
            fmt.has_signal = false;
            fmt.width = 0;
            fmt.height = 0;
            fmt.interlaced = false;
            fmt.frame_rate = 0;
            break;
        }
        default:
            RS_ASSERT(0);
        }
        return fmt;
    }

    static inline int32_t Align(int num, int align = RS_ALIGN_WIDTH)
    {
        return (num + align - 1) & ~(align - 1);
    }

    static std::string ModifyMP4FileName(const std::string &path, int no)
    {
        size_t index = path.find_last_of('.');
        if (std::string::npos == index)
            return path;

        std::string sub_str1 = path.substr(0, index);
        std::string sub_str2 = path.substr(index);

        std::ostringstream oss;
        oss << sub_str1 << "_" << no << sub_str2;
        return oss.str();
    }

    static void GetAACSpec(int samplate_rate, int channel, uint8_t spec[2])
    {
        uint16_t *p = reinterpret_cast<uint16_t *>(spec);

        (*p) = 0;
        (*p) |= (2 << 11);

        if (samplate_rate == 48000)
        {
            (*p) |= (3 << 7);
        }
        else if (samplate_rate == 44100)
        {
            (*p) |= (4 << 7);
        }
        else if (samplate_rate == 32000)
        {
            (*p) |= (5 << 7);
        }
        else if (samplate_rate == 24000)
        {
            (*p) |= (6 << 7);
        }
        else if (samplate_rate == 16000)
        {
            (*p) |= (8 << 7);
        }

        if (channel == 1)
        {
            (*p) |= (1 << 3);
        }
        else if (channel == 2)
        {
            (*p) |= (2 << 3);
        }

        uint8_t temp = (*p) >> 8;
        (*p) <<= 8;
        (*p) |= temp;
    }

    static std::vector<std::string> SplitOneOf(const std::string &str,
                                               const std::string &delims,
                                               const size_t maxSplits)
    {
        std::string remaining(str);
        std::vector<std::string> result;
        size_t splits = 0, pos;

        while (((maxSplits == 0) || (splits < maxSplits)) &&
               ((pos = remaining.find_first_of(delims)) != std::string::npos))
        {
            result.push_back(remaining.substr(0, pos));
            remaining = remaining.substr(pos + 1);
            splits++;
        }

        if (remaining.length() > 0)
            result.push_back(remaining);

        return result;
    }

    static bool HexString2Int(const std::string &str, std::vector<int> &hex_int_arr)
    {
        std::vector<std::string> hex_str_arr = SplitOneOf(str, "\t ", 0);
        for (const std::string &hex_str : hex_str_arr)
        {
            try
            {
                int hex_int = std::stoul(hex_str, NULL, 16);
                hex_int_arr.push_back(hex_int);
            }
            catch (...)
            {
                return false;
            }
        }
        return true;
    }

    static bool HexInt2String(const std::vector<int> &int_arr, std::string &str)
    {
        std::ostringstream oss;

        for (int i : int_arr)
            oss << std::setw(2) << std::setfill('0') << std::hex << i << " ";
        str = oss.str();
        if (str.empty())
            return false;

        str = str.substr(0, str.length() - 1);

        return true;
    }

    static int32_t CreateDir(const std::string &path)
    {
        size_t pos = 0;
        while (true)
        {
            pos = path.find_first_of('/', pos);
            std::string sub_str = path.substr(0, pos);
            if (sub_str != "" && access(sub_str.c_str(), F_OK) != 0)
            {
                log_w("create dir %s", sub_str.c_str());
                if (mkdir(sub_str.c_str(), 0777) != 0)
                {
                    log_e("mkdir failed,%s", strerror(errno));
                    return static_cast<int>(KSystemError);
                }
            }
            if (pos == std::string::npos)
                break;
            pos++;
        }

        return KSuccess;
    }

    static std::string GetLocalTime(const std::string &format = "%Y_%m_%d_%H_%M_%S")
    {
        time_t t = time(nullptr);
        char buf[256];
        strftime(buf, sizeof(buf), format.c_str(), localtime(&t));
        return std::string(buf, strlen(buf));
    }
};
} // namespace rs