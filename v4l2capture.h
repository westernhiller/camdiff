#ifndef V4L2CAPTURE_H
#define V4L2CAPTURE_H

class V4L2Capture
{
public:
    V4L2Capture(char *devName, int width, int height);
    virtual ~V4L2Capture();

    int openDevice();
    int closeDevice();
    int initDevice(int &width, int &height);
    int startCapture();
    int stopCapture();
    int freeBuffers();
    int getFrame(void **, int *);
    int backFrame();
    static void test();

private:
    int initBuffers();

    struct cam_buffer
    {
        void* start;
        unsigned int length;
    };
    char *  m_devName;
    int     m_capW;
    int     m_capH;
    int     m_fdCam;
    cam_buffer *m_buffers;
    unsigned int m_nBuffers;
    int     m_frameIndex;
};

#endif // V4L2CAPTURE_H
