cmake_minimum_required(VERSION 3.15...3.30)

project(
	Capture
	VERSION 1.0
	LANGUAGES C CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(inc)

add_executable(capture src/capture.c src/display.c src/gles_util.c)
set_target_properties(capture PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(capture PRIVATE EGL GL X11 m)
