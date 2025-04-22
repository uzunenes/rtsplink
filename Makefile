# Compiler & flags
CXX = g++
CXXFLAGS = -O3 -Wall -w -fPIC
LDFLAGS = -shared -lpthread

# Get version from file
VERSION := $(shell head -1 Version)

# Source and header directories
SRC_DIR := src
INC_DIR := include

# Object files
OBJS := librtsplink.o librtsplink_utility_functions.o

# Lib name
TARGET := librtsplink.so

# Pkg-config dependencies
PKG_DEPS := opencv libavcodec libavformat libswscale libavutil
PKG_FLAGS := $(shell pkg-config --cflags --libs $(PKG_DEPS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(PKG_FLAGS)
	strip --strip-unneeded $@

librtsplink.o: $(SRC_DIR)/librtsplink.cpp $(INC_DIR)/librtsplink.h
	$(CXX) $(CXXFLAGS) $(PKG_FLAGS) -c $< -o $@

librtsplink_utility_functions.o: $(SRC_DIR)/librtsplink_utility_functions.cpp $(INC_DIR)/librtsplink_utility_functions.h
	$(CXX) $(CXXFLAGS) $(PKG_FLAGS) -c $< -o $@

install:
	@echo "Installing librtsplink.so version $(VERSION)..."
	@rm -rf /usr/local/include/librtsplink*
	@mkdir -p /usr/local/include/librtsplink
	@cp $(INC_DIR)/librtsplink*.h /usr/local/include/librtsplink

	@rm -f /usr/local/lib/librtsplink.so*
	@cp $(TARGET) /usr/local/lib/$(TARGET).$(VERSION)
	@cd /usr/local/lib && ln -sf $(TARGET).$(VERSION) $(TARGET)

	@mkdir -p /usr/local/lib/pkgconfig
	@echo "Name: librtsplink" > librtsplink.pc
	@echo "Description: Low-latency RTSP C++ library" >> librtsplink.pc
	@echo "Version: $(VERSION)" >> librtsplink.pc
	@echo "Libs: -L/usr/local/lib -lrtsplink" >> librtsplink.pc
	@echo "Cflags: -I/usr/local/include/librtsplink" >> librtsplink.pc
	@cp librtsplink.pc /usr/local/lib/pkgconfig
	@ldconfig

uninstall:
	rm -f /usr/local/lib/librtsplink.so*
	rm -rf /usr/local/include/librtsplink*
	rm -f /usr/local/lib/pkgconfig/librtsplink.pc

clean:
	rm -f *.so *.pc *.o
