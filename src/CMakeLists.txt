set(PUBLIC_HEADERS
	../inc/yardstick/yardstick.h
)

set(PRIVATE_HEADERS
	Algorithm.h
	Allocator.h
	Atomics.h
	Clock.h
	ConcurrentCircularBuffer.h
	ConcurrentQueue.h
	Event.h
	GlobalState.h
	Spinlock.h
	ThreadState.h
)

set(SOURCES
	GlobalState.cpp
	Protocol.cpp
	ThreadState.cpp
	yardstick.cpp
)

add_library(yardstick SHARED
	${PUBLIC_HEADERS}
	${PRIVATE_HEADERS}
	${SOURCES}
)

set_property(TARGET yardstick PROPERTY DEBUG_POSTFIX "d")
set_property(TARGET yardstick PROPERTY CXX_STANDARD 11)
target_compile_definitions(yardstick PRIVATE YARDSTICK_EXPORT)
target_include_directories(yardstick PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(yardstick PUBLIC ../inc)

install(FILES ${PUBLIC_HEADERS} DESTINATION ${YS_INCDIR}/yardstick)
install(TARGETS yardstick
    RUNTIME DESTINATION ${YS_BINDIR}
	LIBRARY DESTINATION ${YS_BINDIR}
    ARCHIVE DESTINATION ${YS_LIBDIR}
)
if(MSVC)
	install(FILES $<TARGET_PDB_FILE:yardstick> DESTINATION ${YS_BINDIR})
endif(MSVC)

