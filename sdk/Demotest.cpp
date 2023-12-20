extern "C"
{
#include <stdio.h>
#include <sys/types.h>
#include <guideusb2livestream.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include "sys/time.h"
#include "time.h"
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
}

bool isFirstInitMeasure = false;
guide_measure_debugparam_t *mDebugParam;

unsigned char hotiron[12] = {0x55, 0xAA, 0x07, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x02, 0xF0};
unsigned char medical[12] = {0x55, 0xAA, 0x07, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x05, 0xF0};

double tick(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec + 1E-6 * t.tv_usec;
}

int FPS = 0;
double startTime, endTime;
int serailCallBack(guide_usb_serial_data_t *pSerialData)
{
    int i = 0;
    for (i = 0; i < pSerialData->serial_recv_data_length; i++)
    {
        if (i == pSerialData->serial_recv_data_length - 1)
        {
            printf("%x \n", pSerialData->serial_recv_data[i]);
        }
        else
        {
            printf("%x ", pSerialData->serial_recv_data[i]);
        }
    }
    return 1;
}

int connectStatusCallBack(guide_usb_device_status_e deviceStatus)
{
    if (deviceStatus == DEVICE_CONNECT_OK)
    {
        printf("stream start OK \n");
    }
    else
    {
        printf("stream end \n");
    }
    return 1;
}

int num = 0;

int frameCallBack(guide_usb_frame_data_t *pVideoData)
{
    mDebugParam->exkf = 100;
    mDebugParam->exb = 0;
    mDebugParam->emiss = 98;
    mDebugParam->transs = 0;
    mDebugParam->reflectTemp = 23.0f;
    mDebugParam->distance = 30.0f;
    mDebugParam->fEnvironmentIncrement = 2500;
    int hight = pVideoData->frame_height;
    int width = pVideoData->frame_width;
    if (pVideoData->paramLine != NULL)
    {
        int x_tempture = 0, y_tempture = 0, maxi_tempture = 0;
        int x_yuv = 0, y_yuv = 0, maxi_yuv = 0;
        float max = 0, max_yuv = 0;
        float *temp = (float *)malloc(sizeof(float) * width * hight);
        // float temp = guide_measure_convertsinglegray2temper(pVideoData->frame_src_data[256 * 192 / 2 + 256 / 2], pVideoData->paramLine, mDebugParam, 0);
        // printf("   sdk measure temp***********************************************  %.1f\n", temp);
        int result = guide_measure_convertgray2temper(width, hight, temp, pVideoData->frame_src_data, pVideoData->paramLine, mDebugParam, 0);
        for (int i = 0; i < width * hight; i++)
        {
            // float tempture = guide_measure_convertsinglegray2temper(pVideoData->frame_src_data[i], pVideoData->paramLine, mDebugParam, 1);
            float tempture = temp[i];
            if (tempture > max)
            {
                max = tempture;
                x_tempture = i % width;
                y_tempture = i / width;
                maxi_tempture = i;
            }
            if(pVideoData->frame_src_data[i] > maxi_yuv)
            {
                max_yuv = pVideoData->frame_src_data[i];
                x_yuv = i % width;
                y_yuv = i / width;
                maxi_yuv = i;
            }
        }
        if (result < 0)
        {
            printf("convertgray2temper fail\n");
        }
        // free(temp);
        float temp1 = guide_measure_convertsinglegray2temper(pVideoData->frame_src_data[maxi_tempture], pVideoData->paramLine, mDebugParam, 1);
        printf("temp maxi: %3d temp x:%3d temp y:%3d temp max:%0.3f\n",maxi_tempture, x_tempture, y_tempture, max);
        printf("yuv  maxi: %3d yuv  x:%3d yuv  y:%3d yuv  max:%0.3f\n",maxi_yuv, x_yuv, y_yuv, max_yuv);
        printf("yvu maxi - temp maxi: %5d\n", maxi_yuv - maxi_tempture);
        FPS++;
        if ((tick() - startTime) > 1)
        {
            startTime = tick();
            printf("FPS-------------------------%d\n", FPS);
            FPS = 0;
        }
    }
    return 1;
}

int main(void)
{

    guide_usb_setloglevel(LOG_INFO);
    int ret = guide_usb_initial();
    if (ret < 0)
    {
        printf("Initial fail:%d  Please try to give the device permission before running, sudo chmod -R 777 /dev/bus/usb \n", ret);
    }
    else
    {
        ret = guide_usb_opencommandcontrol((OnSerialDataReceivedCB)serailCallBack);
        printf("guide_usb_getserialdata return:%d \n", ret);

        guide_measure_loadcurve();
    }

    mDebugParam = (guide_measure_debugparam_t *)malloc(sizeof(guide_measure_debugparam_t));

    guide_usb_device_info_t *deviceInfo = (guide_usb_device_info_t *)malloc(sizeof(guide_usb_device_info_t));
    deviceInfo->width = 256;
    deviceInfo->height = 192;
    deviceInfo->video_mode = Y16_PARAM_YUV;

    ret = guide_usb_openstream(deviceInfo, (OnFrameDataReceivedCB)frameCallBack, (OnDeviceConnectStatusCB)connectStatusCallBack);
    startTime = tick();
    printf("Open return:%d \n", ret);
    if (ret < 0)
    {
        printf("Open fail! %d \n", ret);
        return ret;
    }

    int count = 50000;
    while (1)
    {
        usleep(10);
    }

    ret = guide_usb_closestream();
    printf("close usb return %d\n", ret);

    guide_measure_deloadcurve();

    if (mDebugParam != NULL)
    {
        free(mDebugParam);
        mDebugParam = NULL;
    }

    if (deviceInfo != NULL)
    {
        free(deviceInfo);
        deviceInfo = NULL;
    }

    guide_usb_closecommandcontrol();

    ret = guide_usb_exit();
    printf("exit return %d\n", ret);

    return ret;
}
