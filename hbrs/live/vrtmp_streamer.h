// #pragma once

// #include "global.h"
// #include "common/av_define.h"


// namespace rs
// {

// class VRtmpStreamer
// {
// public:
//   explicit VRtmpStreamer();

//   virtual ~VRtmpStreamer();

//   int Initialize(const std::string &url);

//   void Close();

//   int WriteVideoFrame(const VENCFrame &frame);

// private:
//   bool SendSpsPps(const std::string &sps, const std::string &pps);

//   bool SendVideoData(const VENCFrame &frame);

// private:
//   uint8_t *buf_;
//   RTMP *rtmp_;
//   std::string url_;
//   std::string sps_;
//   std::string pps_;
//   bool send_sps_pps_;
//   bool init_;
// };
// } // namespace rs
