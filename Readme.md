## Intoduction
librtsplink is a high-performance C++ library for low-latency RTSP streaming from IP cameras. Utilizing OpenCV and asynchronous, threaded operations, it minimizes delays and maximizes responsiveness.  librtsplink supports GStreamer and FFmpeg backends, making it perfect for real-time video applications.

## Requirements

* A C++11 compliant compiler (e.g., GCC 5.0+, Clang 3.5+, Visual Studio 2015+)
* OpenCV (3.x or 4.x)
* GStreamer (optional) or FFmpeg (optional)

## Installation

1.  **Install Dependencies:**

     ```bash
     sudo apt-get install libopencv-dev libgstreamer1.0-dev libavformat-dev libswscale-dev
     ```

2.  **Clone the Repository:**

    ```bash
    git clone [https://github.com/uzunenes/librtsplink.git](https://github.com/uzunenes/librtsplink.git)
    cd librtsplink
    ```

3.  **Build:**

    ```bash
    make
    sudo make install  # or specify a local installation directory
    ```

## Acknowledgements 
I would like to thank my teammates for their valuable support during this work.

- **Ahmet Selim Demirel**
- **Doğan Mehmet Başoğlu**
- **Elif Cansu Ada**
- **Mevlüt Ardıç**
- **Serhat Karaca**

## References
- [OpenCV Documentation](https://docs.opencv.org/)
