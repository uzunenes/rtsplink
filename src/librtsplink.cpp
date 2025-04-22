#include "../include/librtsplink.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

static rtsplink_camera global_camera = RTSPLINK_CAMERA_DEFAULT;

static std::string
rtsplink_generate_rtsp_url(const rtsplink_camera_connection_information& info)
{
    char buffer[4096];
    snprintf(
        buffer,
        sizeof(buffer),
        "rtsp://%s:%s@%s:%d/%s",
        info.username,
        info.password,
        info.ip,
        info.port,
        info.mount_point
    );
    return std::string(buffer);
}

static void*
rtsplink_connection_control_thread(void* arg)
{
    (void)arg;

    while (1)
    {
        if (!global_camera.opencv_videoCapture_ptr->isOpened())
        {
            std::string url = rtsplink_generate_rtsp_url(global_camera.camera_connection_information_struct);
            global_camera.opencv_videoCapture_ptr->open(url, global_camera.backend_lib_flag == e_rtsplink_rtsp_backend_lib_GSTREAMER ? cv::CAP_GSTREAMER : cv::CAP_FFMPEG);
        }

        if (global_camera.opencv_videoCapture_ptr->isOpened())
        {
            global_camera.camera_connection_status_flag = e_rtsplink_ip_camera_connection_status_normal_connection;
        }
        else
        {
            global_camera.camera_connection_status_flag = e_rtsplink_ip_camera_connection_status_no_connection;
        }

        usleep(1000 * 1000); // 1 saniye bekle
    }

    return nullptr;
}

int
rtsplink_connect_and_follow_ip_camera_connection_background(
    const char* camera_ip,
    const char* camera_username,
    const char* camera_password,
    int rtsp_port,
    const char* rtsp_mount_point,
    rtsplink_rtsp_backend_lib backend_lib_flag
)
{
    if (global_camera.opencv_videoCapture_ptr != nullptr)
    {
        rtsplink_release_and_close_ip_camera();
    }

    global_camera.opencv_videoCapture_ptr = new cv::VideoCapture();
    global_camera.backend_lib_flag = backend_lib_flag;

    strncpy(global_camera.camera_connection_information_struct.ip, camera_ip, sizeof(global_camera.camera_connection_information_struct.ip) - 1);
    strncpy(global_camera.camera_connection_information_struct.username, camera_username, sizeof(global_camera.camera_connection_information_struct.username) - 1);
    strncpy(global_camera.camera_connection_information_struct.password, camera_password, sizeof(global_camera.camera_connection_information_struct.password) - 1);
    strncpy(global_camera.camera_connection_information_struct.mount_point, rtsp_mount_point, sizeof(global_camera.camera_connection_information_struct.mount_point) - 1);
    global_camera.camera_connection_information_struct.port = rtsp_port;

    std::string url = rtsplink_generate_rtsp_url(global_camera.camera_connection_information_struct);
    strncpy(global_camera.camera_connection_information_struct.url, url.c_str(), sizeof(global_camera.camera_connection_information_struct.url) - 1);

    pthread_create(&global_camera.connection_control_queue_id, nullptr, rtsplink_connection_control_thread, nullptr);

    return 0;
}

void
rtsplink_release_and_close_ip_camera(void)
{
    if (global_camera.opencv_videoCapture_ptr)
    {
        global_camera.opencv_videoCapture_ptr->release();
        delete global_camera.opencv_videoCapture_ptr;
        global_camera.opencv_videoCapture_ptr = nullptr;
    }

    global_camera.camera_connection_status_flag = e_rtsplink_ip_camera_connection_status_no_connection;
}

int
rtsplink_get_ip_camera_fps(void)
{
    if (global_camera.opencv_videoCapture_ptr && global_camera.opencv_videoCapture_ptr->isOpened())
    {
        return static_cast<int>(global_camera.opencv_videoCapture_ptr->get(cv::CAP_PROP_FPS));
    }

    return -1;
}

cv::Mat*
rtsplink_get_image_from_ip_camera(double* image_read_system_time, int timeout_restart_camera_connection_milisec, int* err_code)
{
    static cv::Mat frame;

    if (!global_camera.opencv_videoCapture_ptr || !global_camera.opencv_videoCapture_ptr->isOpened())
    {
        *err_code = -1;
        return nullptr;
    }

    bool success = global_camera.opencv_videoCapture_ptr->read(frame);

    if (!success || frame.empty())
    {
        *err_code = -2;
        return nullptr;
    }

    // Zamanı hesapla
    struct timeval time;
    gettimeofday(&time, nullptr);
    *image_read_system_time = static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_usec) * 1e-6;

    *err_code = 0;
    return &frame;
}

rtsplink_ip_camera_connection_status
rtsplink_get_ip_camera_connection_status(void)
{
    return global_camera.camera_connection_status_flag;
}

