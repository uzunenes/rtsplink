#include "../include/librtsplink.h"
#include <atomic>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <opencv2/opencv.hpp>
#include <queue>
#include <sys/time.h>
#include <unistd.h>
// global variables
const char *g_default_gst_pipeline = "protocols=tcp latency=50 ! application/x-rtp, media=(string)video ! decodebin ! videoconvert ! appsink max-buffers=2 drop=True sync=true";
static struct struct_camera *g_camera_struct_ptr = nullptr;
static std::mutex g_mutex_rtsplink_ip_camera;
std::queue<struct rtsplink_image> g_image_queue;
const size_t g_image_queue_max_len = 2;
double g_last_image_read_time = -1;
std::atomic<bool> g_thread_exit_signal(0);
// --

void
sleep_milisec(unsigned int milisec)
{
    usleep(milisec * 1000);
}

double
get_system_time(void)
{
    struct timeval time;

    if(gettimeofday(&time, NULL))
    {
        return 0;
    }

    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

static cv::Mat *capture_c(cv::VideoCapture *opencv_videoCapture_ptr, double *image_read_time)
{
    cv::Mat *image_matris_ptr = nullptr;
    *image_read_time = 0;

    try
    {
        image_matris_ptr = new cv::Mat();

        opencv_videoCapture_ptr->read(*image_matris_ptr);
        *image_read_time = get_system_time();

        if (image_matris_ptr == nullptr)
        {
            printf("Kamera goruntu okuma hatasi, matris nullptr.");
            return nullptr;
        }

        if (image_matris_ptr->cols <= 0 || image_matris_ptr->rows <= 0)
        {
            printf("Kamera goruntu okuma hatasi, goruntu genislik: [%d] - yukseklik: [%d]", image_matris_ptr->cols, image_matris_ptr->rows);
            return nullptr;
        }

        if (image_matris_ptr->empty() == true)
        {
            printf("Kamera goruntu okuma hatasi, matris bos.");
            return nullptr;
        }
    }
    catch (cv::Exception &e)
    {
        printf("Kamera goruntu okuma hatasi: [%s]", e.what());
        return nullptr;
    }

    return image_matris_ptr;
}

static int open_camera(cv::VideoCapture **opencv_videoCapture_ptr, const char *url, enum _enum_rtsplink_rtsp_backend_lib backend_lib_flag)
{
    try
    {
        *opencv_videoCapture_ptr = new cv::VideoCapture();

        (*opencv_videoCapture_ptr)->set(cv::CAP_PROP_FPS, 10);
        (*opencv_videoCapture_ptr)->set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        (*opencv_videoCapture_ptr)->set(cv::CAP_PROP_FRAME_HEIGHT, 720);

        if (backend_lib_flag == e_rtsplink_rtsp_backend_lib_FFMPEG)
        {
            printf("Kamera aciliyor, [%s], backend_lib: FFMPEG.", url);
            setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp", 1);
            (*opencv_videoCapture_ptr)->open(url, cv::CAP_FFMPEG);
        }
        else
        {
            printf("Kamera aciliyor, [%s], backend_lib: GSTREAMER.", url);
            (*opencv_videoCapture_ptr)->open(url, cv::CAP_GSTREAMER);
        }

        if ((*opencv_videoCapture_ptr)->isOpened() == false)
        {
            printf("Kamera acilamadi: [%s]", url);
            return -1;
        }

        g_camera_struct_ptr->fps = (*opencv_videoCapture_ptr)->get(cv::CAP_PROP_FPS);

        printf("Kamera acildi: [%s], fps: [%d], w: [%d], h: [%d]", url, g_camera_struct_ptr->fps, (*opencv_videoCapture_ptr)->get(cv::CAP_PROP_FRAME_WIDTH), (*opencv_videoCapture_ptr)->get(cv::CAP_PROP_FRAME_HEIGHT));
    }
    catch (...)
    {
        printf("Kamera acilamadi: [%s]", url);
        return -2;
    }

    return 0;
}

void rtsplink_release_and_close_ip_camera(void)
{
    printf("Releasing and closing camera connection...");

    g_thread_exit_signal = 1;
    sleep_milisec(100);

    pthread_cancel(g_camera_struct_ptr->connection_control_queue_id);
    printf("Thread canceled! Waiting thread release...");
    pthread_join(g_camera_struct_ptr->connection_control_queue_id, NULL);
    printf("Thread released!");

    if (g_camera_struct_ptr->camera_connection_status_flag != e_rtsplink_ip_camera_connection_status_no_connection && g_camera_struct_ptr->opencv_videoCapture_ptr != nullptr)
    {
        g_camera_struct_ptr->opencv_videoCapture_ptr->release();
        delete g_camera_struct_ptr->opencv_videoCapture_ptr;
        g_camera_struct_ptr->opencv_videoCapture_ptr = nullptr;
    }
    printf("Video capture released!");

    free(g_camera_struct_ptr);
    g_camera_struct_ptr = nullptr;

    while (1)
    {
        if (g_image_queue.size() > 0)
        {
            struct rtsplink_image tmp = g_image_queue.front();
            delete tmp.image_ptr;
            tmp.image_ptr = nullptr;
            g_image_queue.pop();
            continue;
        }
        break;
    }

    printf("Released and closed camera connection.");
}

static void write_image_global_area_and_free(cv::Mat *last_image_ptr, double last_image_read_time)
{
    struct rtsplink_image m;
    m.image_ptr = last_image_ptr;
    m.read_time = last_image_read_time;

    g_mutex_rtsplink_ip_camera.lock();

    g_last_image_read_time = last_image_read_time;

    g_image_queue.push(m);

    // delete old images
    while (1)
    {
        if (g_image_queue.size() > g_image_queue_max_len)
        {
            struct rtsplink_image tmp = g_image_queue.front();
            delete tmp.image_ptr;
            tmp.image_ptr = nullptr;
            g_image_queue.pop();
            continue;
        }
        break;
    }

    g_mutex_rtsplink_ip_camera.unlock();
}

static void *camera_connection_status_function(void *giris_bilgileri_isr)
{
    size_t camera_connection_attempt_count = 0;
    cv::Mat *last_image_ptr = nullptr;
    double last_image_read_time = 0;
    size_t image_read_attempt_count = 0;
    enum _enum_rtsplink_ip_camera_connection_status camera_connection = e_rtsplink_ip_camera_connection_status_no_connection;

    while (1)
    {
        if (g_thread_exit_signal == 1)
        {
            printf("Akis fonk. cikis sinyali geldi, bye:)");
            break;
        }

        if (camera_connection == e_rtsplink_ip_camera_connection_status_no_connection)
        {
            if (open_camera(&g_camera_struct_ptr->opencv_videoCapture_ptr, g_camera_struct_ptr->camera_connection_information_struct.url, g_camera_struct_ptr->backend_lib_flag) != 0)
            {
                ++camera_connection_attempt_count;
                printf("Kamera acilamadi, deneme sayaci: [%ld].", camera_connection_attempt_count);
                sleep_milisec(1000);
                continue;
            }

            camera_connection_attempt_count = 0;
            camera_connection = e_rtsplink_ip_camera_connection_status_linking_connection;
            g_mutex_rtsplink_ip_camera.lock();
            g_camera_struct_ptr->camera_connection_status_flag = camera_connection;
            g_mutex_rtsplink_ip_camera.unlock();
        }
        else if (camera_connection == e_rtsplink_ip_camera_connection_status_linking_connection)
        {
            last_image_ptr = capture_c(g_camera_struct_ptr->opencv_videoCapture_ptr, &last_image_read_time);
            if (last_image_ptr == nullptr)
            {
                ++image_read_attempt_count;
                printf("Goruntu okunamadi, deneme sayaci: [%ld].", image_read_attempt_count);
                sleep_milisec(100);
                continue;
            }

            image_read_attempt_count = 0;
            camera_connection = e_rtsplink_ip_camera_connection_status_normal_connection;
            g_mutex_rtsplink_ip_camera.lock();
            g_camera_struct_ptr->camera_connection_status_flag = camera_connection;
            g_mutex_rtsplink_ip_camera.unlock();
            write_image_global_area_and_free(last_image_ptr, last_image_read_time);
        }
        else if (camera_connection == e_rtsplink_ip_camera_connection_status_normal_connection)
        {
            last_image_ptr = capture_c(g_camera_struct_ptr->opencv_videoCapture_ptr, &last_image_read_time);
            if (last_image_ptr == nullptr)
            {
                camera_connection = e_rtsplink_ip_camera_connection_status_linking_connection;
                g_mutex_rtsplink_ip_camera.lock();
                g_camera_struct_ptr->camera_connection_status_flag = camera_connection;
                g_mutex_rtsplink_ip_camera.unlock();
                sleep_milisec(100);
                continue;
            }

            write_image_global_area_and_free(last_image_ptr, last_image_read_time);
        }

        sleep_milisec(80);
    }

    pthread_exit(NULL);
}

// mount_pont -> "/test"
int rtsplink_connect_and_follow_ip_camera_connection_background(const char *camera_ip, const char *camera_username, const char *camera_password, int port, const char *mount_point, enum _enum_rtsplink_rtsp_backend_lib backend_lib_flag)
{
    g_camera_struct_ptr = (struct_camera *)calloc(1, sizeof(struct_camera));
    *g_camera_struct_ptr = struct_camera_default;
    g_thread_exit_signal = 0;
    g_last_image_read_time = -1;
    int result = 0;
    g_camera_struct_ptr->backend_lib_flag = backend_lib_flag;

    strcpy(g_camera_struct_ptr->camera_connection_information_struct.ip, camera_ip);
    g_camera_struct_ptr->camera_connection_information_struct.port = port;

    if (g_camera_struct_ptr->backend_lib_flag == e_rtsplink_rtsp_backend_lib_GSTREAMER)
    {
        if (camera_username == nullptr && camera_password == nullptr && mount_point == nullptr)
        {
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtspsrc location=rtsp://%s:%d %s", g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port, g_default_gst_pipeline);
        }
        else if (camera_username == nullptr && camera_password == nullptr && mount_point != nullptr)
        {
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.mount_point, mount_point);
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtspsrc location=rtsp://%s:%d%s %s", g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port, g_camera_struct_ptr->camera_connection_information_struct.mount_point, g_default_gst_pipeline);
        }
        else if (camera_username != nullptr && camera_password != nullptr && mount_point == nullptr)
        {
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.username, camera_username);
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.password, camera_password);
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtspsrc location=rtsp://%s:%s@%s:%d %s", g_camera_struct_ptr->camera_connection_information_struct.username, g_camera_struct_ptr->camera_connection_information_struct.password, g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port, g_default_gst_pipeline);
        }
        else if (camera_username != nullptr && camera_password != nullptr && mount_point != nullptr)
        {
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.username, camera_username);
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.password, camera_password);
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.mount_point, mount_point);
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtspsrc location=rtsp://%s:%s@%s:%d%s %s", g_camera_struct_ptr->camera_connection_information_struct.username, g_camera_struct_ptr->camera_connection_information_struct.password, g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port, g_camera_struct_ptr->camera_connection_information_struct.mount_point, g_default_gst_pipeline);
        }
        else
        {
            printf(": Username and pass must be [nullptr] or some value.");
            return -1;
        }
    }
    else if (g_camera_struct_ptr->backend_lib_flag == e_rtsplink_rtsp_backend_lib_FFMPEG)
    {
        if (camera_username == nullptr && camera_password == nullptr && mount_point == nullptr)
        {
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtsp://%s:%d", g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port);
        }
        else if (camera_username == nullptr && camera_password == nullptr && mount_point != nullptr)
        {
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.mount_point, mount_point);
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtsp://%s:%d%s", g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port, g_camera_struct_ptr->camera_connection_information_struct.mount_point);
        }
        else if (camera_username != nullptr && camera_password != nullptr && mount_point == nullptr)
        {
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.username, camera_username);
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.password, camera_password);
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtsp://%s:%s@%s:%d", g_camera_struct_ptr->camera_connection_information_struct.username, g_camera_struct_ptr->camera_connection_information_struct.password, g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port);
        }
        else if (camera_username != nullptr && camera_password != nullptr && mount_point != nullptr)
        {
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.username, camera_username);
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.password, camera_password);
            strcpy(g_camera_struct_ptr->camera_connection_information_struct.mount_point, mount_point);
            sprintf(g_camera_struct_ptr->camera_connection_information_struct.url, "rtsp://%s:%s@%s:%d%s", g_camera_struct_ptr->camera_connection_information_struct.username, g_camera_struct_ptr->camera_connection_information_struct.password, g_camera_struct_ptr->camera_connection_information_struct.ip, g_camera_struct_ptr->camera_connection_information_struct.port, g_camera_struct_ptr->camera_connection_information_struct.mount_point);
        }
        else
        {
            printf(": Username and pass must be [nullptr] or some value.");
            return -1;
        }
    }
    else
    {
        printf(": Unexpected backend library flag!");
        return -1;
    }
	
    result = pthread_create(&g_camera_struct_ptr->connection_control_queue_id, nullptr, camera_connection_status_function, nullptr);
    if (result != 0)
    {
        printf(": Thread olusturulamadi!");
        return -2;
    }

    return result;
}

cv::Mat *rtsplink_get_image_from_ip_camera(double *image_read_time, int timeout_restart_camera_connection_milisec, int *err_code)
{
    cv::Mat *m = nullptr;
    *image_read_time = 0;
    double suan = 0, elapsed_time_milisec = 0;
    *err_code = 0;

    suan = get_system_time();

    g_mutex_rtsplink_ip_camera.lock();

    if (g_last_image_read_time != -1)
    {
        elapsed_time_milisec = (suan - g_last_image_read_time) * 1000;
        if (elapsed_time_milisec > timeout_restart_camera_connection_milisec)
        {
            g_mutex_rtsplink_ip_camera.unlock();
            printf("Timeout reached! Total elapsed milisec: [%1.f], timeout_milisec: [%d].", elapsed_time_milisec, timeout_restart_camera_connection_milisec);
            *err_code = -2;
            return nullptr;
        }
    }

    size_t queue_len = g_image_queue.size();
    if (queue_len <= 0)
    {
        g_mutex_rtsplink_ip_camera.unlock();
        printf("Global image not ready, first image waiting..");
        *err_code = -1;
        return nullptr;
    }

    struct rtsplink_image tmp = g_image_queue.front();
    g_image_queue.pop();
    m = tmp.image_ptr;
    *image_read_time = tmp.read_time;

    g_mutex_rtsplink_ip_camera.unlock();

    return m;
}

int rtsplink_get_ip_camera_fps(void)
{
    int fps = -1;

    g_mutex_rtsplink_ip_camera.lock();
    fps = g_camera_struct_ptr->fps;
    g_mutex_rtsplink_ip_camera.unlock();

    return fps;
}

enum _enum_rtsplink_ip_camera_connection_status rtsplink_get_ip_camera_connection_status(void)
{
    enum _enum_rtsplink_ip_camera_connection_status camera_connection;

    g_mutex_rtsplink_ip_camera.lock();
    camera_connection = g_camera_struct_ptr->camera_connection_status_flag;
    g_mutex_rtsplink_ip_camera.unlock();

    return camera_connection;
}
