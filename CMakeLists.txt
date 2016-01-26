cmake_minimum_required(VERSION 2.8)

project(yardstick)

set(YS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(YS_BINDIR ${YS_ROOT}/bin)
set(YS_LIBDIR ${YS_ROOT}/lib)
set(YS_INCDIR ${YS_ROOT}/inc)

add_subdirectory(src)
