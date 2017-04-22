set(PUBLIC_HEADERS
	../inc/yardstick/yardstick.h
)

set(PRIVATE_HEADERS
	Algorithm.h
	Atomics.h
	Clock.h
	ConcurrentCircularBuffer.h
	ConcurrentQueue.h
	GlobalState.h
	PointerHash.h
	Protocol.h
	Signal.h
	Spinlock.h
	ThreadState.h
	WebsocketSink.h
)

set(SOURCES
	GlobalState.cpp
	Protocol.cpp
	ThreadState.cpp
	WebsocketSink.cpp
	yardstick.cpp
)

set(WEBBY_HEADERS
	webby/webby.h
	webby/webby_win32.h
	webby/webby_unix.h
)

set(WEBBY_SOURCES
	webby/webby.c
)

add_library(yardstick SHARED
	${PUBLIC_HEADERS}
	${PRIVATE_HEADERS}
	${WEBBY_HEADERS}
	${SOURCES}
	${WEBBY_SOURCES}
)

set_property(TARGET yardstick PROPERTY DEBUG_POSTFIX "d")
set_property(TARGET yardstick PROPERTY CXX_STANDARD 11)
target_compile_definitions(yardstick PRIVATE YARDSTICK_EXPORT _WINSOCK_DEPRECATED_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)
target_include_directories(yardstick PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(yardstick PUBLIC ../inc)

install(FILES ${PUBLIC_HEADERS} DESTINATION ${YS_INCDIR}/yardstick)
install(TARGETS yardstick
    RUNTIME DESTINATION ${YS_BINDIR}
	LIBRARY DESTINATION ${YS_BINDIR}
    ARCHIVE DESTINATION ${YS_LIBDIR}
)
if(MSVC)
	target_link_libraries(yardstick PUBLIC ws2_32)
	install(FILES $<TARGET_PDB_FILE:yardstick> DESTINATION ${YS_BINDIR})
endif(MSVC)
