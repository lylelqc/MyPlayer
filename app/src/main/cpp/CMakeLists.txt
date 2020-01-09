cmake_minimum_required(VERSION 3.4.1)

include_directories(include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/lib/${CMAKE_ANDROID_ARCH_ABI}")

file(GLOB src_files *.cpp)

add_library(
        native-lib
        SHARED
        ${src_files})

target_link_libraries(
        native-lib
        -Wl,--start-group
        avcodec avfilter avformat avutil swresample swscale
        -Wl,--end-group
        z
        log
        android
        OpenSLES)