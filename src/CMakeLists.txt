set(YS_PUBLIC_HEADERS
	${YS_INCDIR}/yardstick/yardstick.h
)

set(YS_PRIVATE_HEADERS
	Algorithm.h
	Allocator.h
	ConcurrentCircularBuffer.h
	ConcurrentQueue.h
	GlobalState.h
	Semaphore.h
	Spinlock.h
)

set(YS_SOURCES
	Allocator.cpp
	GlobalState.cpp
	yardstick.cpp
)

add_library(yardstick SHARED
	${YS_PUBLIC_HEADERS}
	${YS_PRIVATE_HEADERS}
	${YS_SOURCES}
)

set_property(TARGET yardstick PROPERTY DEBUG_POSTFIX "d")
target_compile_definitions(yardstick PRIVATE YARDSTICK_EXPORT)
target_include_directories(yardstick PUBLIC ${YS_INCDIR})

install(TARGETS yardstick
    RUNTIME DESTINATION ${YS_BINDIR}
    ARCHIVE DESTINATION ${YS_LIBDIR}
)
if(MSVC)
	install(FILES $<TARGET_PDB_FILE:yardstick> DESTINATION ${YS_BINDIR})
endif(MSVC)
