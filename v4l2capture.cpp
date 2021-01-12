#include "v4l2capture.h"
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <iomanip>
#include <string>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

V4L2Capture::V4L2Capture(char *devName, int width, int height)
    : m_devName(devName)
    , m_capW(width)
    , m_capH(height)
    , m_fdCam(-1)
    , m_buffers(nullptr)
    , m_nBuffers(0)
{
}

V4L2Capture::~V4L2Capture() {
}

int V4L2Capture::openDevice()
{
    printf("video dev : %s\n", m_devName);
    m_fdCam = open(m_devName, O_RDWR);
    if (m_fdCam < 0) {
        perror("Can't open video device");
    }
    return 0;
}

int V4L2Capture::closeDevice()
{
    if (m_fdCam <= 0)
        return -1;

    int ret = 0;
    if ((ret = close(m_fdCam)) < 0)
        perror("Can't close video device");

    return 0;
}

int V4L2Capture::initDevice(int& width, int& height)
{
    /* 使用IOCTL命令VIDIOC_QUERYCAP，获取摄像头的基本信息*/
    struct v4l2_capability cam_cap;		//显示设备信息
    int ret = ioctl(m_fdCam, VIDIOC_QUERYCAP, &cam_cap);
    if (ret < 0)
        perror("Can't get device information: VIDIOCGCAP");

    printf("Driver Name:%s\nCard Name:%s\nBus info:%s\nDriver Version:%u.%u.%u\n",
            cam_cap.driver, cam_cap.card, cam_cap.bus_info,
            (cam_cap.version >> 16) & 0XFF, (cam_cap.version >> 8) & 0XFF,
            cam_cap.version & 0XFF);

    /* 使用IOCTL命令VIDIOC_ENUM_FMT，获取摄像头所有支持的格式*/
    struct v4l2_fmtdesc cam_fmtdesc;	//查询所有支持的格式：VIDIOC_ENUM_FMT
    cam_fmtdesc.index = 0;
    cam_fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format:\n");
    while (ioctl(m_fdCam, VIDIOC_ENUM_FMT, &cam_fmtdesc) != -1)
    {
        printf("\t%d.%s\n", cam_fmtdesc.index + 1, cam_fmtdesc.description);
        cam_fmtdesc.index++;
    }

    /* 使用IOCTL命令VIDIOC_CROPCAP，获取摄像头的捕捉能力*/
    struct v4l2_cropcap cam_cropcap;	//设置摄像头的捕捉能力
    struct v4l2_crop cam_crop;			//图像的缩放

    cam_cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (0 == ioctl(m_fdCam, VIDIOC_CROPCAP, &cam_cropcap))
    {
        printf("Default rec:\n\tleft:%d\n\ttop:%d\n\twidth:%d\n\theight:%d\n",
                cam_cropcap.defrect.left, cam_cropcap.defrect.top,
                cam_cropcap.defrect.width, cam_cropcap.defrect.height);
        /* 使用IOCTL命令VIDIOC_S_CROP，获取摄像头的窗口取景参数*/
        cam_crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        cam_crop.c = cam_cropcap.defrect;		//默认取景窗口大小
        if (-1 == ioctl(m_fdCam, VIDIOC_S_CROP, &cam_crop)) {
            //printf("Can't set crop para\n");
        }
    } else {
        printf("Can't set cropcap para\n");
    }

    /* 使用IOCTL命令VIDIOC_S_FMT，设置摄像头帧信息*/
    struct v4l2_format cam_format;		//设置摄像头的视频制式、帧格式等
    cam_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    cam_format.fmt.pix.width = m_capW;
    cam_format.fmt.pix.height = m_capH;
    cam_format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;		//要和摄像头支持的类型对应
    cam_format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    ret = ioctl(m_fdCam, VIDIOC_S_FMT, &cam_format);
    if (ret < 0) {
        perror("Can't set frame information");
    }
    /* 使用IOCTL命令VIDIOC_G_FMT，获取摄像头帧信息*/
    cam_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(m_fdCam, VIDIOC_G_FMT, &cam_format);
    if (ret < 0) {
        perror("Can't get frame information");
    }
    printf("Current data format information:\n\twidth:%d\n\theight:%d\n",
            cam_format.fmt.pix.width, cam_format.fmt.pix.height);
    width = cam_format.fmt.pix.width;
    height = cam_format.fmt.pix.height;
    ret = initBuffers();
    if (ret < 0) {
        perror("Buffers init error");
        return -1;
        //exit(-1);
    }
    return 0;
}

int V4L2Capture::initBuffers()
{
    /* 使用IOCTL命令VIDIOC_REQBUFS，申请帧缓冲*/
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    int ret = ioctl(m_fdCam, VIDIOC_REQBUFS, &req);
    if (ret < 0) {
        perror("Request frame buffers failed");
    }
    if (req.count < 2)
    {
        perror("Request frame buffers while insufficient buffer memory");
    }
    m_buffers = (struct cam_buffer*) calloc(req.count, sizeof(*m_buffers));
    if (!m_buffers)
    {
        perror("Out of memory");
    }

    for (m_nBuffers = 0; m_nBuffers < req.count; m_nBuffers++) {
        struct v4l2_buffer buf;
        CLEAR(buf);
        // 查询序号为n_buffers 的缓冲区，得到其起始物理地址和大小
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = m_nBuffers;
        ret = ioctl(m_fdCam, VIDIOC_QUERYBUF, &buf);
        if (ret < 0) {
            printf("VIDIOC_QUERYBUF %d failed\n", m_nBuffers);
            return -1;
        }
        m_buffers[m_nBuffers].length = buf.length;
        //printf("buf.length= %d\n",buf.length);
        // 映射内存
        m_buffers[m_nBuffers].start = mmap(
                NULL, // start anywhere
                buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fdCam,
                buf.m.offset);
        if (MAP_FAILED == m_buffers[m_nBuffers].start) {
            printf("mmap buffer%d failed\n", m_nBuffers);
            return -1;
        }
    }
    return 0;
}

int V4L2Capture::startCapture()
{
    for (int i = 0; i < m_nBuffers; i++)
    {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (-1 == ioctl(m_fdCam, VIDIOC_QBUF, &buf))
        {
            printf("VIDIOC_QBUF buffer%d failed\n", i);
            return -1;
        }
    }
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(m_fdCam, VIDIOC_STREAMON, &type))
    {
        printf("VIDIOC_STREAMON error");
        return -1;
    }
    return 0;
}

int V4L2Capture::stopCapture()
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(m_fdCam, VIDIOC_STREAMOFF, &type))
    {
        printf("VIDIOC_STREAMOFF error\n");
        return -1;
    }
    return 0;
}

int V4L2Capture::freeBuffers()
{
    unsigned int i;
    for (i = 0; i < m_nBuffers; ++i)
    {
        if (-1 == munmap(m_buffers[i].start, m_buffers[i].length))
        {
            printf("munmap buffer%d failed\n", i);
            return -1;
        }
    }
    free(m_buffers);
    return 0;
}

int V4L2Capture::getFrame(void **frame_buf, int* len)
{
    struct v4l2_buffer queue_buf;
    CLEAR(queue_buf);
    queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    queue_buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(m_fdCam, VIDIOC_DQBUF, &queue_buf))
    {
        printf("VIDIOC_DQBUF error\n");
        return -1;
    }
    *frame_buf = m_buffers[queue_buf.index].start;
    *len = m_buffers[queue_buf.index].length;
    m_frameIndex = queue_buf.index;
    return 0;
}

int V4L2Capture::backFrame()
{
    if (m_frameIndex != -1)
    {
        struct v4l2_buffer queue_buf;
        CLEAR(queue_buf);
        queue_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        queue_buf.memory = V4L2_MEMORY_MMAP;
        queue_buf.index = m_frameIndex;
        if (-1 == ioctl(m_fdCam, VIDIOC_QBUF, &queue_buf))
        {
            printf("VIDIOC_QBUF error\n");
            return -1;
        }
        return 0;
    }
    return -1;
}
