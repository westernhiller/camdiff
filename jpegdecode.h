#ifndef JPEGDECODE_H
#define JPEGDECODE_H

#include "turbojpeg.h"
#include <tuple>
#include <vector>
#include <fstream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui_c.h>

std::tuple<bool,std::vector<uint8_t>,uint64_t,uint64_t,uint64_t> decodeJpeg2X(uint8_t* pJpegData,uint64_t JpegdataSize,const char* convrt_flag);

std::tuple<bool,std::vector<uint8_t>,uint64_t,uint64_t,uint64_t> decodeJpeg2X(std::string filename,const char* convrt_flag);

cv::Mat Jpeg2Mat(uint8_t *jpegData, uint64_t jpegSize);

cv::Mat Jpeg2Mat(std::string filename);

#endif // JPEGDECODE_H
