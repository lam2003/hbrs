#include "osd/jpeg_dec.h"

#define M_DATA 0x00
#define M_SOF0 0xc0
#define M_DHT 0xc4
#define M_SOI 0xd8
#define M_EOI 0xd9
#define M_SOS 0xda
#define M_DQT 0xdb
#define M_DNL 0xdc
#define M_DRI 0xdd
#define M_APP0 0xe0
#define M_APPF 0xef
#define M_COM 0xfe
#define MAKEUS(a, b) ((uint16_t)(((uint16_t)(a)) << 8 | ((uint16_t)(b))))
#define MAKEUI(a, b, c, d) ((uint32_t)(((uint32_t)(a)) << 24 | ((uint32_t)(b)) << 16 | ((uint32_t)(c)) << 8 | ((uint32_t)(d))))

namespace rs
{

bool ParseSize(const std::string filename, int &result_width, int &result_height)
{
    FILE *fp = fopen(filename.c_str(), "rb");
    if (fp == nullptr)
    {
        log_e("open %s failed,%s", filename.c_str(), strerror(errno));
        return false;
    }

    fseek(fp, 0, SEEK_SET);

    result_width = 0, result_height = 0;
    uint8_t id, height, low;
    bool found = false;
    while (!found)
    {
        if (!fread(&id, sizeof(uint8_t), 1, fp) || id != 0xff || !fread(&id, sizeof(uint8_t), 1, fp))
            break;

        if (id >= M_APP0 && id <= M_APPF)
        {
            fread(&height, sizeof(uint8_t), 1, fp);
            fread(&low, sizeof(uint8_t), 1, fp);
            fseek(fp, (long)(MAKEUS(height, low) - 2), SEEK_CUR);
            continue;
        }

        switch (id)
        {
        case M_SOI:
            break;

        case M_COM:
        case M_DQT:
        case M_DHT:
        case M_DNL:
        case M_DRI:
            fread(&height, sizeof(uint8_t), 1, fp);
            fread(&low, sizeof(uint8_t), 1, fp);
            fseek(fp, (long)(MAKEUS(height, low) - 2), SEEK_CUR);
            break;

        case M_SOF0:
            fseek(fp, 3L, SEEK_CUR);
            fread(&height, sizeof(uint8_t), 1, fp);
            fread(&low, sizeof(uint8_t), 1, fp);
            result_height = (uint32_t)MAKEUS(height, low);
            fread(&height, sizeof(uint8_t), 1, fp);
            fread(&low, sizeof(uint8_t), 1, fp);
            result_width = (uint32_t)MAKEUS(height, low);
            found = true;
            break;

        case M_SOS:
        case M_EOI:
        case M_DATA:
            break;

        default:
            fread(&height, sizeof(uint8_t), 1, fp);
            fread(&low, sizeof(uint8_t), 1, fp);
            fseek(fp, (long)(MAKEUS(height, low) - 2), SEEK_CUR);
            break;
        }
    }

    fclose(fp);

    if (found)
    {
        log_d("jpeg image %s:width:%d,height:%d", filename.c_str(), result_width, result_height);
        return true;
    }
    else
    {
        log_e("parse jpeg image %s failed", filename.c_str());
        return false;
    }
}

// static int Decode(const std::string filename);

// static Decode {}
// void *SAMPLE_VDEC_SendStream(void *p)
// {
//     VDEC_STREAM_S stStream;
//     SAMPLE_VDEC_SENDPARAM_S *pstSendParam;
//     char sFileName[50], sFilePostfix[20];
//     FILE *fp = NULL;
//     HI_S32 s32BlockMode = HI_IO_BLOCK;
//     HI_U8 *pu8Buf;
//     HI_U64 u64pts;
//     HI_S32 s32IntervalTime = 40000;
//     HI_S32 i, s32Ret, len, start;
//     HI_S32 s32UsedBytes, s32ReadLen;
//     HI_BOOL bFindStart, bFindEnd;

//     start = 0;
//     u64pts = 0;
//     s32UsedBytes = 0;
//     pstSendParam = (SAMPLE_VDEC_SENDPARAM_S *)p;

//     /******************* open the stream file *****************/
//     SAMPLE_COMM_SYS_Payload2FilePostfix(pstSendParam->enPayload, sFilePostfix);
//     sprintf(sFileName, "stream_chn0%s", sFilePostfix);
//     fp = fopen(sFileName, "r");
//     if (HI_NULL == fp)
//     {
//         SAMPLE_PRT("can't open file %s in send stream thread:%d\n", sFileName, pstSendParam->VdChn);
//         return (HI_VOID *)(HI_FAILURE);
//     }
//     printf("open file [%s] ok in send stream thread:%d!\n", sFileName, pstSendParam->VdChn);

//     /******************* malloc the  stream buffer in user space *****************/
//     if (pstSendParam->s32MinBufSize != 0)
//     {
//         pu8Buf = malloc(pstSendParam->s32MinBufSize);
//         if (pu8Buf == NULL)
//         {
//             SAMPLE_PRT("can't alloc %d in send stream thread:%d\n", pstSendParam->s32MinBufSize, pstSendParam->VdChn);
//             fclose(fp);
//             return (HI_VOID *)(HI_FAILURE);
//         }
//     }
//     else
//     {
//         SAMPLE_PRT("none buffer to operate in send stream thread:%d\n", pstSendParam->VdChn);
//         return (HI_VOID *)(HI_FAILURE);
//     }

//     while (pstSendParam->bRun)
//     {
//         fseek(fp, s32UsedBytes, SEEK_SET);
//         s32RReadLen = fread(pu8Buf, 1, pstSendParam->s32MinBufSize, fp);
//         if (s32ReadLen <= 0)
//         {
//             printf("file end.\n");
//             break;
//         }

//         /******************* cutting the stream for frame *****************/
//         if ((pstSendParam->enVideoMode == VIDEO_MODE_FRAME) && (pstSendParam->enPayload == PT_H264))
//         {
//             bFindStart = HI_FALSE;
//             bFindEnd = HI_FALSE;
//             for (i = 0; i < s32ReadLen - 5; i++)
//             {
//                 if (pu8Buf[i] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
//                     ((pu8Buf[i + 3] & 0x1F) == 0x5 || (pu8Buf[i + 3] & 0x1F) == 0x1) &&
//                     ((pu8Buf[i + 4] & 0x80) == 0x80))
//                 {
//                     bFindStart = HI_TRUE;
//                     i += 4;
//                     break;
//                 }
//             }

//             for (; i < s32ReadLen - 5; i++)
//             {
//                 if (pu8Buf[i] == 0 && pu8Buf[i + 1] == 0 && pu8Buf[i + 2] == 1 &&
//                     ((pu8Buf[i + 3] & 0x1F) == 0x5 || (pu8Buf[i + 3] & 0x1F) == 0x1) &&
//                     ((pu8Buf[i + 4] & 0x80) == 0x80))
//                 {
//                     bFindEnd = HI_TRUE;
//                     break;
//                 }
//             }

//             s32ReadLen = i;
//             if (bFindStart == HI_FALSE)
//             {
//                 SAMPLE_PRT("can not find start code in send stream thread:%d\n", pstSendParam->VdChn);
//             }
//             else if (bFindEnd == HI_FALSE)
//             {
//                 s32ReadLen = i + 5;
//             }
//         }
//         else if ((pstSendParam->enPayload == PT_JPEG) || (pstSendParam->enPayload == PT_MJPEG))
//         {
//             bFindStart = HI_FALSE;
//             bFindEnd = HI_FALSE;
//             for (i = 0; i < s32ReadLen - 2; i++)
//             {
//                 if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD8)
//                 {
//                     start = i;
//                     bFindStart = HI_TRUE;
//                     i = i + 2;
//                     break;
//                 }
//             }

//             for (; i < s32ReadLen - 4; i++)
//             {
//                 if ((pu8Buf[i] == 0xFF) && (pu8Buf[i + 1] & 0xF0) == 0xE0)
//                 {
//                     len = (pu8Buf[i + 2] << 8) + pu8Buf[i + 3];
//                     i += 1 + len;
//                 }
//                 else
//                 {
//                     break;
//                 }
//             }

//             for (; i < s32ReadLen - 2; i++)
//             {
//                 if (pu8Buf[i] == 0xFF && pu8Buf[i + 1] == 0xD8)
//                 {
//                     bFindEnd = HI_TRUE;
//                     break;
//                 }
//             }

//             s32ReadLen = i;
//             if (bFindStart == HI_FALSE)
//             {
//                 printf("\033[0;31mALERT!!!,can not find start code in send stream thread:%d!!!\033[0;39m\n",
//                        pstSendParam->VdChn);
//             }
//             else if (bFindEnd == HI_FALSE)
//             {
//                 s32ReadLen = i + 2;
//             }
//         }

//         stStream.u64PTS = u64pts;
//         stStream.pu8Addr = pu8Buf + start;
//         stStream.u32Len = s32ReadLen;
//         if (pstSendParam->enVideoMode == VIDEO_MODE_FRAME)
//         {
//             u64pts += 40000;
//         }

//         /******************* send stream *****************/
//         if (s32BlockMode == HI_IO_BLOCK)
//         {
//             s32Ret = HI_MPI_VDEC_SendStream(pstSendParam->VdChn, &stStream, HI_IO_BLOCK);
//         }
//         else if (s32BlockMode == HI_IO_NOBLOCK)
//         {
//             s32Ret = HI_MPI_VDEC_SendStream(pstSendParam->VdChn, &stStream, HI_IO_NOBLOCK);
//         }
//         else
//         {
//             s32Ret = HI_MPI_VDEC_SendStream_TimeOut(pstSendParam->VdChn, &stStream, 8000);
//         }

//         if (HI_SUCCESS == s32Ret)
//         {
//             s32UsedBytes = s32UsedBytes + s32ReadLen + start;
//         }
//         else
//         {
//             if (s32BlockMode != HI_IO_BLOCK)
//             {
//                 SAMPLE_PRT("failret:%x\n", s32Ret);
//             }
//             usleep(s32IntervalTime);
//         }
//         usleep(20000);
//     }

//     printf("send steam thread %d return ...\n", pstSendParam->VdChn);
//     fflush(stdout);
//     if (pu8Buf != HI_NULL)
//     {
//         free(pu8Buf);
//     }
//     fclose(fp);
//     return (HI_VOID *)HI_SUCCESS;
// }
} // namespace rs