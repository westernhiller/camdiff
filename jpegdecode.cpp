#include "jpegdecode.h"

/**
* decode JPEG format to XXX format
* @param pJpegData
* @param JpegdataSize
* @param convrt_flag
* "RGB" - see TJPF_RGB
* "BGR" - see TJPF_BGR
* ...
* see all support format at enum TJPF
*/

using namespace std;
using namespace cv;

tuple<bool,vector<uint8_t>,uint64_t,uint64_t,uint64_t> decodeJpeg2X(uint8_t* pJpegData,uint64_t JpegdataSize,const char* convrt_flag)
{
    assert( pJpegData != NULL );
    int width = 0,height = 0,jpegsubsamp = 0;

    tjhandle jpeg = tjInitDecompress();

    if(jpeg == nullptr)
    {
        return make_tuple(false, vector<uint8_t>(0), 0, 0, 0);
    }

    if(tjDecompressHeader2(jpeg,pJpegData,JpegdataSize,&width,&height,&jpegsubsamp) != 0)
    {
        return make_tuple(false, vector<uint8_t>(0), 0, 0, 0);
    }

    TJPF eformat;
    if(strcmp("ABGR", convrt_flag) == 0)
        eformat = TJPF::TJPF_ABGR;
    else if(strcmp("ARGB", convrt_flag) == 0)
        eformat = TJPF::TJPF_ARGB;
    else if(strcmp("BGR", convrt_flag) == 0)
        eformat = TJPF::TJPF_BGR;
    else if(strcmp("BGRA", convrt_flag) == 0)
        eformat = TJPF::TJPF_BGRA;
    else if(strcmp("BGRX", convrt_flag) == 0)
        eformat = TJPF::TJPF_BGRX;
    else if(strcmp("CMYK", convrt_flag) == 0)
        eformat = TJPF::TJPF_CMYK;
    else if(strcmp("GRAY", convrt_flag) == 0)
        eformat = TJPF::TJPF_GRAY;
    else if(strcmp("RGB", convrt_flag) == 0)
        eformat = TJPF::TJPF_RGB;
    else if(strcmp("RGBA", convrt_flag) == 0)
        eformat = TJPF::TJPF_RGBA;
    else if(strcmp("RGBX", convrt_flag) == 0)
        eformat = TJPF::TJPF_RGBX;
    else if(strcmp("XBGR", convrt_flag) == 0)
        eformat = TJPF::TJPF_XBGR;
    else if(strcmp("XRGB", convrt_flag) == 0)
        eformat = TJPF::TJPF_XRGB;

    uint64_t pitch = tjPixelSize[eformat] * width;
    uint64_t size = pitch * height;
    vector<uint8_t> output(size);

    if(tjDecompress2(jpeg,pJpegData,JpegdataSize,&output.front(),width,pitch,height,eformat,0) != 0)
    {
        return make_tuple(false, vector<uint8_t>(0), 0, 0, 0);
    }

    return make_tuple(true, move(output), size, width, height);
}

tuple<bool,vector<uint8_t>,uint64_t,uint64_t,uint64_t> decodeJpeg2X(string filename,const char* convrt_flag)
{
    ifstream ifs(filename.c_str(),ios_base::binary | ios_base::in);

    if(!ifs.good())
        return make_tuple(false, vector<uint8_t>(0), 0, 0, 0);
    ifs.seekg(0,ios::end);
    uint64_t size = ifs.tellg();
    ifs.seekg(0,ios::beg);

    vector<char> buffer(size);
    ifs.read(&buffer.front(),size);

    return decodeJpeg2X((uint8_t*)&buffer.front(),size,convrt_flag);
}

Mat Jpeg2Mat(uint8_t *jpegData, uint64_t jpegSize)
{
    auto res = decodeJpeg2X( (uint8_t*)jpegData,jpegSize,"BGR");
    bool success = false;
    vector<uint8_t> buff;
    int width,height,size;

    tie(success,buff,size,width,height) = res;

    Mat dst(height,width,CV_8UC3,(uint8_t*)&buff.front());
    return dst.clone();
}

Mat Jpeg2Mat(string filename)
{
    auto res = decodeJpeg2X( filename ,"BGR");
    bool success = false;
    vector<uint8_t> buff;
    int width,height,size;

    tie(success,buff,size,width,height) = res;

    Mat dst(height,width,CV_8UC3,(uint8_t*)&buff.front());
    return dst.clone();
}
