#ifndef LIBRTSPLINK_H
#define LIBRTSPLINK_H

#include <opencv2/videoio/videoio.hpp>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Enum definitions
typedef enum {
    e_rtsplink_ip_camera_connection_status_no_connection,
    e_rtsplink_ip_camera_connection_status_linking_connection,
    e_rtsplink_ip_camera_connection_status_normal_connection
} rtsplink_ip_camera_connection_status;

typedef enum {
    e_rtsplink_rtsp_backend_lib_FFMPEG,
    e_rtsplink_rtsp_backend_lib_GSTREAMER
} rtsplink_rtsp_backend_lib;

// Struct definitions
typedef struct {
    char ip[32];
    int port;
    char username[64];
    char password[64];
    char mount_point[64];
    char url[4096];
} rtsplink_camera_connection_information;

typedef struct {
    cv::VideoCapture* opencv_videoCapture_ptr;
    rtsplink_ip_camera_connection_status camera_connection_status_flag;
    rtsplink_rtsp_backend_lib backend_lib_flag;
    pthread_t connection_control_queue_id;
    rtsplink_camera_connection_information camera_connection_information_struct;
    int fps;
} rtsplink_camera;

typedef struct {
    cv::Mat* image_ptr;
    double read_time;
} rtsplink_image;

// Default struct macros
#define RTSPLINK_CAMERA_CONNECTION_INFORMATION_DEFAULT \
    (rtsplink_camera_connection_information){{0}, -1, {0}, {0}, {0}, {0}}

#define RTSPLINK_CAMERA_DEFAULT \
    (rtsplink_camera){nullptr, e_rtsplink_ip_camera_connection_status_no_connection, \
    e_rtsplink_rtsp_backend_lib_GSTREAMER, 0, RTSPLINK_CAMERA_CONNECTION_INFORMATION_DEFAULT, -1}

/**
 * @brief Connects to an IP camera using the RTSP protocol in a background thread.
 * 
 * @param camera_ip IP address of the camera.
 * @param camera_username Username for the camera.
 * @param camera_password Password for the camera.
 * @param rtsp_port RTSP port.
 * @param rtsp_mount_point RTSP mount point.
 * @param backend_lib_flag Backend library (GStreamer or FFMPEG).
 * @return 0 on success, non-zero on failure.
 */
int rtsplink_connect_and_follow_ip_camera_connection_background(
    const char* camera_ip,
    const char* camera_username,
    const char* camera_password,
    int rtsp_port,
    const char* rtsp_mount_point,
    rtsplink_rtsp_backend_lib backend_lib_flag
);

/**
 * @brief Releases and disconnects the RTSP camera.
 */
void rtsplink_release_and_close_ip_camera(void);

/**
 * @brief Gets the FPS value of the connected camera.
 * 
 * @return FPS as integer.
 */
int rtsplink_get_ip_camera_fps(void);

/**
 * @brief Retrieves an image from the connected IP camera.
 * 
 * @param image_read_system_time Pointer to receive the image's read timestamp.
 * @param timeout_restart_camera_connection_milisec Timeout in milliseconds to reconnect.
 * @param err_code Pointer to an integer to receive the error code.
 * @return Pointer to the retrieved cv::Mat image.
 */
cv::Mat* rtsplink_get_image_from_ip_camera(
    double* image_read_system_time,
    int timeout_restart_camera_connection_milisec,
    int* err_code
);

/**
 * @brief Returns the current RTSP connection status.
 * 
 * @return Current connection status.
 */
rtsplink_ip_camera_connection_status rtsplink_get_ip_camera_connection_status(void);

#ifdef __cplusplus
}
#endif

#endif // LIBRTSPLINK_H

