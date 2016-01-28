cmake_minimum_required(VERSION 3.0)

project(yardstick)

set(YS_VERSION 0.1.0)

if(WIN32)
	set(YS_BINDIR bin/${CMAKE_VS_PLATFORM_NAME})
	set(YS_LIBDIR lib/${CMAKE_VS_PLATFORM_NAME})
elseif(CMAKE_SIZEOF_VOID_P MATCHES 8)
	SET(YS_BINDIR bin64)
	SET(YS_BINDIR lib64)
else()
	SET(YS_BINDIR bin)
	SET(YS_BINDIR lib)
endif()
set(YS_INCDIR inc)

add_subdirectory(src)

configure_file(yardstick.nuspec.in yardstick.nuspec @ONLY)
