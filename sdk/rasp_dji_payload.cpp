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
#include <libmemcached/memcached.h>
}

//------------------function declare------------------
void getMemData(const char *key, char **return_value);
int write_to_memcached(const char *key, const char *value);
void pixel2Meter(int x_pixel, int y_pixel, float *x_meter, float *y_meter);

//------------------global variable------------------
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
        float temp_x = 0, temp_y = 0;
        float max = 0;
        float *temp = (float *)malloc(sizeof(float) * width * hight);
        // float temp = guide_measure_convertsinglegray2temper(pVideoData->frame_src_data[256 * 192 / 2 + 256 / 2], pVideoData->paramLine, mDebugParam, 0);
        // printf("   sdk measure temp***********************************************  %.1f\n", temp);
        int result = guide_measure_convertgray2temper(width, hight, temp, pVideoData->frame_src_data, pVideoData->paramLine, mDebugParam, 0);
        guide_measure_convertsinglegray2temper(pVideoData->frame_src_data[256 * 192 / 2 + 256 / 2], pVideoData->paramLine, mDebugParam, 0);
        printf("result:%d\n", result);
        for (int i = 0; i < width * hight; i++)
        {
            float tempture = temp[i];
            if (tempture > max)
            {
                max = tempture;
                temp_x = i % width;
                temp_y = i / width;
            }
        }
        if (result < 0)
        {
            printf("convertgray2temper fail\n");
        }
        // 將原點移動到中心
        temp_x = temp_x - width / 2;
        temp_y = -(temp_y - hight / 2);
        // printf("x:%f y:%f max:%f\n", temp_x, temp_y, max);
        float x_meter, y_meter;
        pixel2Meter(temp_x, temp_y, &x_meter, &y_meter);
        char *send_json = (char *)malloc(sizeof(char) * 1024);
        // sprintf(send_json, "[{\"x\":%d,\"y\":%d,\"temp\":%f}]", temp_x, temp_y, max);
        sprintf(send_json, "[{\"x\":%f,\"y\":%f,\"temp\":%f}]", x_meter, y_meter, max);
        write_to_memcached("thermal", send_json);
        printf("send_json:%s\n", send_json);
    }
    return 1;
}


void getMemData(const char *key, char **return_value)
{
    memcached_st *memc;
    memcached_return rc;
    memcached_server_st *servers;
    // 创建 memcached 连接
    memc = memcached_create(NULL);
    servers = memcached_server_list_append(NULL, "localhost", 11211, &rc);
    rc = memcached_server_push(memc, servers);
    // 执行读取操作
    size_t value_length;
    uint32_t flags;
    *return_value = memcached_get(memc, key, strlen(key), &value_length, &flags, &rc);

    memcached_free(memc);
    memcached_server_list_free(servers);
}
int write_to_memcached(const char *key, const char *value)
{
    memcached_st *memc;
    memcached_return rc;
    memcached_server_st *servers;
    // 创建 memcached 连接
    memc = memcached_create(NULL);
    servers = memcached_server_list_append(NULL, "localhost", 11211, &rc);
    rc = memcached_server_push(memc, servers);
    // 执行写入操作
    rc = memcached_set(memc, key, strlen(key), value, strlen(value), (time_t)0, (uint32_t)0);
    // 清理资源
    memcached_free(memc);
    memcached_server_list_free(servers);
    if (rc == MEMCACHED_SUCCESS)
    {
        return 1; // 写入成功
    }
    else
    {
        return 0; // 写入失败
    }
}

void pixel2Meter(int x_pixel, int y_pixel, float *x_meter, float *y_meter)
{
    float width_rate = 1.2 / 256;
    float height_rate = 0.9 / 192;
    *x_meter = (float)x_pixel * width_rate;
    *y_meter = (float)y_pixel * height_rate;
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
