cmake_minimum_required(VERSION 3.0)

project(yardstick)

add_library(yardstick SHARED
	Include/yardstick.h
	Source/yardstick.cpp
	Source/filesink.cpp
)
target_compile_definitions(yardstick PRIVATE YARDSTICK_EXPORT)
target_include_directories(yardstick PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include")
