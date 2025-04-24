#ifndef LIBRTSPLINK_H
#define LIBRTSPLINK_H

#include <opencv2/videoio/videoio.hpp>

#define struct_camera_connection_information_default (struct struct_camera_connection_information) {{0}, -1, {0}, {0}, {0}, {0} }
#define struct_camera_default (struct struct_camera) { nullptr, e_rtsplink_ip_camera_connection_status_no_connection, e_rtsplink_rtsp_backend_lib_GSTREAMER, 0, struct_camera_connection_information_default, -1}

enum _enum_rtsplink_ip_camera_connection_status
{
	e_rtsplink_ip_camera_connection_status_no_connection,
	e_rtsplink_ip_camera_connection_status_linking_connection,
	e_rtsplink_ip_camera_connection_status_normal_connection
};

enum _enum_rtsplink_rtsp_backend_lib
{
	e_rtsplink_rtsp_backend_lib_FFMPEG,
	e_rtsplink_rtsp_backend_lib_GSTREAMER
};

struct struct_camera_connection_information
{
    char ip[32];
    int port;
    char username[64];
    char password[64];
    char mount_point[64];
    char url[4096];
};

struct struct_camera
{
    cv::VideoCapture *opencv_videoCapture_ptr;
    enum _enum_rtsplink_ip_camera_connection_status camera_connection_status_flag;
    enum _enum_rtsplink_rtsp_backend_lib backend_lib_flag;
    pthread_t connection_control_queue_id;
    struct struct_camera_connection_information camera_connection_information_struct;
    int fps;
};

struct rtsplink_image
{
    cv::Mat *image_ptr;
    double read_time;
};

/**
*   Provides connection to IP camera with RTSP protocol.
*	@param camera_ip: IP address of the IP camera.
*	@param camera_username: Username of the IP camera.
*	@param camera_password: Password of the IP camera.
*	@param rtsp_port: RTSP port number of the IP camera.
*	@param rtsp_mount_point: RTSP mount point.
*	@param enum: Gstreamer/FFMPEG select flag.
*   @return None
**/
int
rtsplink_connect_and_follow_ip_camera_connection_background(const char* camera_ip, const char* camera_username, const char* camera_password, int rtsp_port, const char* rtsp_mount_point, enum _enum_rtsplink_rtsp_backend_lib backend_lib_flag);

/**
* 	Disconnects the IP camera connected with the RTSP protocol.
*	@param None
*   @return None
**/
void
rtsplink_release_and_close_ip_camera(void);

/**
*	Captures the fps value of the camera connected with the RTSP protocol.
*	@param  None
*   @return Int type fps value.
**/
int
rtsplink_get_ip_camera_fps(void);

/**
*	Get images from the connected camera with the RTSP protocol.
*	@param
*	@param image_read_system_time = Double type image read system time.
*	@param timeout_restart_camera_connection_milisec = Timeout in milliseconds for camera reconnection.
*	@param err_code = Keeps the error code that may be while get the image.
*   @return  *Cv::Mat type image.
**/
cv::Mat*
rtsplink_get_image_from_ip_camera(double* image_read_system_time, int timeout_restart_camera_connection_milisec, int* err_code);

/**
*	Get IP camera RTSP connection status.
*	@param None
*   @return _enum_rtsplink_ip_camera_connection_status type value.
**/
enum _enum_rtsplink_ip_camera_connection_status
rtsplink_get_ip_camera_connection_status(void);

#endif // LIBRTSPLINK_H

