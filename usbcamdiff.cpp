#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "v4l2capture.h"
#include "jpegdecode.h"

using namespace std;
using namespace cv;

struct CamParam
{
    char* title;
    char* dev;
    int width;
	int height;
};     

static void * pthread(void *arg)       
{
	struct CamParam *param;
 
    printf("pthread start!\n");
     
    /* 令主线程继续执行 */
    usleep(2000000);
     
    /* 打印传入参数 */
    param = (struct CamParam *)arg;      
	V4L2Capture * pCapturer = new V4L2Capture(param->dev, param->width, param->height);

    pCapturer->openDevice();

    if(-1 == pCapturer->initDevice(param->width, param->height))
    {
        cout << "failed initializing camera " << param->dev << endl;
        return nullptr;
    }
    namedWindow(param->title);

    pCapturer->startCapture();

    unsigned char *yuv422frame = nullptr;
    unsigned long yuvframeSize = 0;
    double t1 = (double)getTickCount();
    bool finish = false;
    char filename[32];
    int i = 1;
    Mat lastFrame;
    bool bColor = true;
    bool bBinary = false;

	while(!finish)
	{
        if(-1 != pCapturer->getFrame((void **) &yuv422frame, (int *)&yuvframeSize))
        {
            Mat imgFrame = Jpeg2Mat(yuv422frame, yuvframeSize);
            if(imgFrame.data)
            {
                Mat scaled;
                resize(imgFrame, scaled, Size(640, 480));
//                resize(imgFrame, scaled, Size(1280, 720));

                if(lastFrame.data)
                {
                    Mat imgDiff = lastFrame - scaled;
                    if(bBinary)
                    {
                        cvtColor(imgDiff, imgDiff, COLOR_RGB2GRAY);
                        int th = threshold(imgDiff, imgDiff, 0, 255, THRESH_OTSU);
//                        if(th < 10)
//                        {
 //                           threshold(imgDiff, imgDiff, 0, 255, THRESH_BINARY)    ;
 //                       }
                    }
                    else if(!bColor)
                        cvtColor(imgDiff, imgDiff, COLOR_RGB2GRAY);
                    imshow(param->title, imgDiff);
                }
                    
                lastFrame = scaled;

                switch(waitKey(100))
                {
                    case 27:
                        finish = true;
                        break;
                    case 's':
                    case 'S':
                        sprintf(filename, "%s%d.png", param->title, i++);
                        imwrite(filename, imgFrame);
                        break;
                    case 'c':
                    case 'C':
                        bColor = !bColor;
                        break;
                    case 'b':
                    case 'B':
                        bBinary = !bBinary;
                        break;
                    default:
                        break;
                }
            } 
            pCapturer->backFrame();
        }
        usleep(10000);
	}
     
    return nullptr;
}

int  main(int argc, char*argv[])
{
	if(argc != 2)
	{
		cout << "Usage: usbcamdiff + camera" <<endl;
		return -1;
	}

    pthread_t tidpvis;
    struct CamParam *pvis  = (struct CamParam *)malloc(sizeof(struct CamParam));           
    pvis->title = "VISIBLE";
    pvis->dev =  argv[1];	
    pvis->width = 640;
    pvis->height = 480;

   /* 创建线程pthread */
    if ((pthread_create(&tidpvis, NULL, pthread, (void*)pvis)) == -1)
    {
        printf("create visible thread error!\n");
        return 1;
    }
     
    /* 令线程pthread先运行 */
    usleep(1000000);
     
    /* 线程pthread睡眠2s，此时main可以先执行 */
    printf("mian continue!\n");

    /* 等待线程pthread释放 */
    if (pthread_join(tidpvis, NULL))                  
    {
        printf("thread is not exit...\n");
        return -2;
    }

	return 0;
}
