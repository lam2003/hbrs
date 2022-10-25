#pragma once

#include "common/buffer.h"
#include "common/av_define.h"

namespace rs
{

class NormalRtmpStreamer
{
public:
  explicit NormalRtmpStreamer();

  virtual ~NormalRtmpStreamer();

  int Initialize(const std::string &url, bool has_audio);

  void Close();

  int WriteVideoFrame(const VENCFrame &frame);

  int WriteAudioFrame(const AENCFrame &frame);

private:
  bool SendAACMeta();

  bool SendSpsPps(const std::string &sps, const std::string &pps);

  bool SendVideoData(const VENCFrame &frame);

  bool SendAudioData(const AENCFrame &frame);

private:
  MMZBuffer buffer_;
  RTMP *rtmp_;
  std::string url_;
  std::string sps_;
  std::string pps_;
  bool send_sps_pps_;
  bool has_audio_;
  bool init_;
};
} // namespace rs
