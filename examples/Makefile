CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# OpenCV 3
OPENCV_CFLAGS := $(shell pkg-config --cflags opencv)
OPENCV_LDFLAGS := $(shell pkg-config --libs opencv) -lopencv_imgproc -lopencv_core

# Librtsplink
RTSPLINK_CFLAGS := $(shell pkg-config --cflags librtsplink)
RTSPLINK_LDFLAGS := $(shell pkg-config --libs librtsplink)

TARGET = camera_example
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(RTSPLINK_LDFLAGS) $(OPENCV_LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(RTSPLINK_CFLAGS) $(OPENCV_CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

