cmake_minimum_required(VERSION 3.15...3.30)

project(
	Capture
	VERSION 1.0
	LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(utilities "")

include_directories(inc)
include_directories(src/libardmpegts/inc)

add_subdirectory(src/libardmpegts)

add_executable(
	capture
	src/capture.c
	src/display.c
	src/gles_util.c
	src/encoder.c 
	src/encoder_utils.c
	src/list_common.c
	src/network.c	
)
set_target_properties(capture PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(capture PRIVATE utilities EGL GL X11 rockchip_mpp m)
