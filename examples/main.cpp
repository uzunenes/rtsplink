#include <csignal>
#include <cstdio>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <librtsplink/librtsplink.h>
#include <sys/time.h>
#include <unistd.h>

volatile bool g_exit_signal = 0;

static void exit_signal_handler(int sig)
{
    g_exit_signal = sig;
}

void sleep_milisec(unsigned int milisec)
{
    // max 1 saniyeyi aşan durumlarda bile güvenli uyku
    usleep(static_cast<useconds_t>(milisec) * 1000);
}

int 
get_current_system_time_and_date(char* time_date)
{
    struct timeval time;
    time_t curtime;

    char dst[128];
    memset(dst, '\0', sizeof(dst));

    #define buffer_len 32
    char buffer[buffer_len] = {0};

    if (gettimeofday(&time, NULL))
    {
        return -1;
    }

    curtime = time.tv_sec;

    strftime(buffer, buffer_len, "%m-%d-%Y %T.", localtime(&curtime));

    sprintf(time_date, "%s%ld",buffer,time.tv_usec);

    strncpy(dst, time_date, 23);
    
    sprintf(time_date, "%s", dst);
    
    return 0;
}

double get_system_time()
{
    struct timeval time {};
    
    if (gettimeofday(&time, nullptr) != 0)
    {
        perror("gettimeofday failed");
        return 0.0;
    }

    return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_usec) * 1e-6;
}

void image_write_text(cv::Mat *image_ptr, const char *text_ptr, int x, int y, double size)
{
    cv::Point pt(x, y), pt1(x - 1, y - 1);
    cv::Scalar color(0, 0, 255);
    cv::Scalar color1(255, 255, 255);
    cv::putText(*image_ptr, text_ptr, pt, cv::FONT_HERSHEY_SIMPLEX, size - 0.1, color, size * 2.5);
    cv::putText(*image_ptr, text_ptr, pt1, cv::FONT_HERSHEY_SIMPLEX, size - 0.105, color1, size * 2.5);
}

int main()
{
    const char* camera_ip = "10.E.N.S";
    const char* camera_username = "***";
    const char* camera_password = "*****";  

    int port = 554;
    const char *mount_point = nullptr;
    cv::Mat *m = nullptr;
    double image_read_time = 0;
    int err_code = 0;
    int timeout_restart_camera_connection_ms = 5000;
    char system_time[1024]; 

    // Signal handling
    signal(SIGINT, exit_signal_handler);
    signal(SIGTERM, exit_signal_handler);

    // Start camera connection
    rtsplink_connect_and_follow_ip_camera_connection_background(camera_ip, camera_username, camera_password, port, mount_point, e_rtsplink_rtsp_backend_lib_GSTREAMER);

    while (1)
    {
        if (rtsplink_get_ip_camera_connection_status() != e_rtsplink_ip_camera_connection_status_no_connection)
        {
            m = rtsplink_get_image_from_ip_camera(&image_read_time, timeout_restart_camera_connection_ms, &err_code);

            if (err_code == 0)
            {
                double now = get_system_time();
                
                double left = 0.01;
                double top = 0.86;
                double size = 1;
                int x = m->cols * left;
                int y = m->rows * top;
                get_current_system_time_and_date(system_time);

                image_write_text(m, system_time, x + 30, 100, size);

                printf("Camera image successfully acquired.\n");

                delete m;
            }
            else if (err_code == -2)
            {
                printf("No image received, restarting camera...\n");
                rtsplink_release_and_close_ip_camera();
                printf("Camera released.\n");
                sleep_milisec(1000);
                rtsplink_connect_and_follow_ip_camera_connection_background(camera_ip, camera_username, camera_password, port, mount_point, e_rtsplink_rtsp_backend_lib_GSTREAMER);
                sleep_milisec(1000);
            }
        }

        if (g_exit_signal == 1)
        {
            break;
        }

        sleep_milisec(100);
    }

    rtsplink_release_and_close_ip_camera();
    printf("Exiting program.\n");

    return 0;
}

