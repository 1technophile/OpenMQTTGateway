project(libuv)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROJECT_SOURCE_DIR}/include/")

file(GLOB ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/*.c")
file(GLOB ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/*.h")

if(WIN32)
	file(GLOB ${PROJECT_NAME}_sources ${${PROJECT_NAME}_sources} "${PROJECT_SOURCE_DIR}/win/*.c")
	file(GLOB ${PROJECT_NAME}_headers ${${PROJECT_NAME}_headers} "${PROJECT_SOURCE_DIR}/win/*.h")
else()
	file(GLOB ${PROJECT_NAME}_sources ${${PROJECT_NAME}_sources} "${PROJECT_SOURCE_DIR}/unix/*.c")
	file(GLOB ${PROJECT_NAME}_headers ${${PROJECT_NAME}_headers} "${PROJECT_SOURCE_DIR}/unix/*.h")

	# Darwin only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/darwin-proctitle.c")
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/fsevents.c")
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/darwin.c")
	# Sun only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/sunos.c")
	# AIX only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/aix.c")
	# OpenBSD only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/openbsd.c")
	# NetBSD only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/netbsd.c")
	# Android only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/android-ifaddrs.c")
	# Cygwin only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/cygwin.c")
	# OS390 only
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/os390.c")
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/os390-syscalls.c")
	# General
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/no-fsevents.c")
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/no-proctitle.c")
	list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/procfs-exepath.c")
	# list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/posix-poll.c")

	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/pthread-barrier.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/linux-inotify.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/linux-core.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/proctitle.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/sunos.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/kqueue.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/freebsd.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/sysinfo-loadavg.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/sysinfo-memory.c")
	# elseif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
		# list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/pthread-barrier.c")
		# list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/linux-inotify.c")
		# list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/linux-core.c")
		# list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/proctitle.c")
		# list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/kqueue.c")
		# list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/freebsd.c")
	else()
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/posix-poll.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/posix-hrtime.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/kqueue.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/pthread-barrier.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/freebsd.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/bsd-ifaddrs.c")
		list(REMOVE_ITEM ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/unix/sunos.c")
	endif()
endif()

set(${PROJECT_NAME}_files ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers} CACHE INTERNAL "")
