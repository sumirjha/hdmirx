# Build source directory

target_include_directories(utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h")

file(GLOB_RECURSE SRC_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
target_sources (utilities PRIVATE ${SRC_FILES})
