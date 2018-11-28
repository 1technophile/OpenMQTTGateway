cmake_minimum_required(VERSION 2.8.8)

include(CMakeConfig.txt)

project(pilight)
set(VERSION 7.0)

set(MODULESPACK OFF)
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_SKIP_RULE_DEPENDENCY TRUE)
set(CPACK_DEB_COMPONENT_INSTALL TRUE)

if(${ZWAVE} MATCHES "ON")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
endif()

if(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
  # Use the highest warning level for visual studio.
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
  if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else(CMAKE_C_FLAGS MATCHES "/W[0-4]")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif(CMAKE_C_FLAGS MATCHES "/W[0-4]")
  # Disable deprecation warnings for standard C functions in VS2005 and later
  if(CMAKE_COMPILER_2005)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
  endif(CMAKE_COMPILER_2005)
endif(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
if(CMAKE_BUILD_TOOL MATCHES "make")
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
  if(NOT CMAKE_C_FLAGS MATCHES "-Wall")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  endif(NOT CMAKE_C_FLAGS MATCHES "-Wall")
endif(CMAKE_BUILD_TOOL MATCHES "make")

if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColorReset "${Esc}[m")
  set(ColorBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

set(CMAKE_SKIP_RPATH TRUE)
if(NOT WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "  -Wl,-rpath=.,-rpath=/usr/local/lib/,-rpath=/usr/lib/,-rpath=/lib/")
	set(CMAKE_SHARED_LINKER_FLAGS " -Wl,-rpath=.,-rpath=/usr/local/lib/,-rpath=/usr/lib/,-rpath=/lib/")
	set(CMAKE_MODULE_LINKER_FLAGS " -Wl,-rpath=.,-rpath=/usr/local/lib/,-rpath=/usr/lib/,-rpath=/lib/")
else()
	set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")
endif()

if(NOT WIN32)
# Start uninstaller generator
function(WRITE_UNINSTALL_TARGET_SCRIPT)
    # Create uninstall target template file, if it doesn't exist...
    if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in)
        set(__uninstall_filename ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake.in)
        # BEGIN actual write to file...
        file(WRITE ${__uninstall_filename} "\# - uninstall target template\n\#")
        file(APPEND ${__uninstall_filename} "
if (NOT EXISTS \"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\")
    message(FATAL_ERROR \"Cannot find install manifest: \\\"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\\\"\")
endif(NOT EXISTS \"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\")

file(READ \"\@CMAKE_CURRENT_BINARY_DIR\@/install_manifest.txt\" files)
string(REGEX REPLACE \"\\n\" \";\" files \"\${files}\")

foreach (file \${files})
    message(STATUS \"Uninstalling \\\"\$ENV{DESTDIR}\${file}\\\"\")
    if (EXISTS \"\$ENV{DESTDIR}\${file}\")
        execute_process(
            COMMAND \@CMAKE_COMMAND\@ -E remove \"\$ENV{DESTDIR}\${file}\"
            OUTPUT_VARIABLE rm_out
            RESULT_VARIABLE rm_retval
        )
        if(NOT \${rm_retval} EQUAL 0)
            message(FATAL_ERROR \"Problem when removing \\\"\$ENV{DESTDIR}\${file}\\\"\")
        endif (NOT \${rm_retval} EQUAL 0)
    else (EXISTS \"\$ENV{DESTDIR}\${file}\")
        message(STATUS \"File \\\"\$ENV{DESTDIR}\${file}\\\" does not exist.\")
    endif (EXISTS \"\$ENV{DESTDIR}\${file}\")
endforeach(file)

execute_process(COMMAND update-rc.d ${PROJECT_NAME} remove)
execute_process(COMMAND ldconfig)

") # END of appending to file...
    endif()
endfunction()
# End of the uninstaller generator

	if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cmake_postinstall.cmake)
		set(__uninstall_filename ${CMAKE_CURRENT_BINARY_DIR}/cmake_postinstall.cmake)
		# BEGIN actual write to file...
		file(WRITE ${__uninstall_filename} "execute_process(COMMAND update-rc.d ${PROJECT_NAME} defaults)")
	endif()
endif()

execute_process(COMMAND git log -n 1 --pretty=format:"%h"
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	RESULT_VARIABLE git_result
	OUTPUT_VARIABLE git_hash)

execute_process(COMMAND git describe --always --abbrev=0
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	RESULT_VARIABLE git_result
	OUTPUT_VARIABLE git_ver)

string(REGEX REPLACE "\n" "" git_ver "${git_ver}")
string(REGEX REPLACE "\"" "" git_hash "${git_hash}")
add_definitions(-DHASH="${git_ver}-nightly-${git_hash}")

file(MD5 ${PROJECT_SOURCE_DIR}/res/pilight.pem md5sum)
add_definitions(-DPILIGHT_PEM_MD5="${md5sum}")

if(${MODULESPACK} MATCHES "ON" AND NOT WIN32)
	add_definitions(-DMODULE="1")
endif()

if(NOT WIN32)
	if(${PROTOCOL_ARPING} MATCHES "ON")
		set(CMAKE_PCAP_LIBS_INIT)

		find_library(CMAKE_PCAP_LIBS_INIT
			NAME pcap
			PATHS
				${CROSS_COMPILE_LIBS}
				/usr/lib
				/usr/lib32
				/usr/lib64
				/usr/lib/i386-linux-gnu		
				/usr/lib/x86_64-linux-gnu
				/usr/local/lib
				/usr/local/lib32
				/usr/local/lib64
				/usr/lib/arm-linux-gnueabi
				/usr/lib/arm-linux-gnueabihf
				/usr/lib/aarch64-linux-gnu
			NO_DEFAULT_PATH)	

		if(${CMAKE_PCAP_LIBS_INIT} MATCHES "CMAKE_PCAP_LIBS_INIT-NOTFOUND")
			message(FATAL_ERROR "Looking for libpcap - not found")
		else()
			message(STATUS "Looking for libpcap - found (${CMAKE_PCAP_LIBS_INIT})")
		endif()
	endif()
	
	set(CMAKE_UNWIND_LIBS_INIT)

	if(NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "^mips")
		find_library(CMAKE_UNWIND_LIBS_INIT
		NAME unwind
		PATHS
			${CROSS_COMPILE_LIBS}
			/usr/lib
			/usr/lib32
			/usr/lib64
			/usr/lib/i386-linux-gnu		
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			/usr/local/lib32
			/usr/local/lib64
			/usr/lib/arm-linux-gnueabi
			/usr/lib/arm-linux-gnueabihf
			/usr/lib/aarch64-linux-gnu
		NO_DEFAULT_PATH)

		if(${CMAKE_UNWIND_LIBS_INIT} MATCHES "CMAKE_UNWIND_LIBS_INIT-NOTFOUND")
			message(FATAL_ERROR "Looking for libunwind - not found")
		else()
			message(STATUS "Looking for libunwind - found (${CMAKE_UNWIND_LIBS_INIT})")
		endif()
	endif()

	find_library(CMAKE_WIRINGX_LIBS_INIT
		NAME wiringx
		PATHS
			${CROSS_COMPILE_LIBS}
			/usr/lib
			/usr/lib32
			/usr/lib64
			/usr/lib/i386-linux-gnu
			/usr/lib/x86_64-linux-gnu
			/usr/local/lib
			/usr/local/lib32
			/usr/local/lib64
			/usr/lib/arm-linux-gnueabi
			/usr/lib/arm-linux-gnueabihf
			/usr/lib/aarch64-linux-gnu
		NO_DEFAULT_PATH)

	if(${CMAKE_WIRINGX_LIBS_INIT} MATCHES "CMAKE_WIRINGX_LIBS_INIT-NOTFOUND")
		message(FATAL_ERROR "Looking for libwiringx - not found")
	else()
		message(STATUS "Looking for libwiringx - found (${CMAKE_WIRINGX_LIBS_INIT})")
	endif()
endif()

find_library(CMAKE_MBEDTLS_LIBS_INIT
	NAME mbedtls
	PATHS
		${CROSS_COMPILE_LIBS}
		/usr/lib
		/usr/lib32
		/usr/lib64
		/usr/lib/i386-linux-gnu
		/usr/lib/x86_64-linux-gnu
		/usr/local/lib
		/usr/local/lib32
		/usr/local/lib64
		/usr/lib/arm-linux-gnueabi
		/usr/lib/arm-linux-gnueabihf
		/usr/lib/aarch64-linux-gnu
	NO_DEFAULT_PATH)

if(${CMAKE_MBEDTLS_LIBS_INIT} MATCHES "CMAKE_MBEDTLS_LIBS_INIT-NOTFOUND")
	message(FATAL_ERROR "Looking for libmbedtls - not found")
else()
	message(STATUS "Looking for libmbedtls - found (${CMAKE_MBEDTLS_LIBS_INIT})")
endif()

find_library(CMAKE_MBEDCRYPTO_LIBS_INIT
	NAME mbedcrypto
	PATHS
		${CROSS_COMPILE_LIBS}
		/usr/lib
		/usr/lib32
		/usr/lib64
		/usr/lib/i386-linux-gnu
		/usr/lib/x86_64-linux-gnu
		/usr/local/lib
		/usr/local/lib32
		/usr/local/lib64
		/usr/lib/arm-linux-gnueabi
		/usr/lib/arm-linux-gnueabihf
		/usr/lib/aarch64-linux-gnu
	NO_DEFAULT_PATH)

if(${CMAKE_MBEDCRYPTO_LIBS_INIT} MATCHES "CMAKE_MBEDCRYPTO_LIBS_INIT-NOTFOUND")
	message(FATAL_ERROR "Looking for libmbedcrypto - not found")
else()
	message(STATUS "Looking for libmbedcrypto - found (${CMAKE_MBEDCRYPTO_LIBS_INIT})")
endif()

find_library(CMAKE_MBEDX509_LIBS_INIT
	NAME mbedx509
	PATHS
		${CROSS_COMPILE_LIBS}
		/usr/lib
		/usr/lib32
		/usr/lib64
		/usr/lib/i386-linux-gnu
		/usr/lib/x86_64-linux-gnu
		/usr/local/lib
		/usr/local/lib32
		/usr/local/lib64
		/usr/lib/arm-linux-gnueabi
		/usr/lib/arm-linux-gnueabihf
		/usr/lib/aarch64-linux-gnu
	NO_DEFAULT_PATH)

if(${CMAKE_MBEDX509_LIBS_INIT} MATCHES "CMAKE_MBEDX509_LIBS_INIT-NOTFOUND")
	message(FATAL_ERROR "Looking for libmbedx509 - not found")
else()
	message(STATUS "Looking for libmbedx509 - found (${CMAKE_MBEDX509_LIBS_INIT})")
endif()

find_library(CMAKE_LUA_LIBS_INIT
	NAME luajit-5.1
	PATHS
	${CROSS_COMPILE_LIBS}
	/usr/lib
	/usr/lib32
	/usr/lib64
	/usr/lib/i386-linux-gnu
	/usr/lib/x86_64-linux-gnu
	/usr/local/lib
	/usr/local/lib32
	/usr/local/lib64
	/usr/lib/arm-linux-gnueabi
	/usr/lib/arm-linux-gnueabihf
	/usr/lib/aarch64-linux-gnu
NO_DEFAULT_PATH)

if(${CMAKE_LUA_LIBS_INIT} MATCHES "CMAKE_LUA_LIBS_INIT-NOTFOUND")
	message(FATAL_ERROR "Looking for libluajit - not found")
else()
	message(STATUS "Looking for libluajit - found (${CMAKE_LUA_LIBS_INIT})")
endif()

include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/libs/libuv)
include_directories(/usr/local/include/)

add_subdirectory(inc)
add_subdirectory(libs)

if(WIN32)
	set(THREADS_USE_PTHREADS_WIN32 true)
endif()
find_package(Threads REQUIRED)
if(NOT WIN32)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")	
		find_package(Backtrace REQUIRED)
	endif()
endif()

if(${MODULESPACK} MATCHES "OFF" OR WIN32)
	if(WIN32)
		add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/res/win32/admin.obj COMMAND ${CMAKE_RC_COMPILER} -o ${PROJECT_SOURCE_DIR}/res/win32/admin.obj ${PROJECT_SOURCE_DIR}/res/win32/admin.rc)
		add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/res/win32/icon.obj COMMAND ${CMAKE_RC_COMPILER} -o ${PROJECT_SOURCE_DIR}/res/win32/icon.obj ${PROJECT_SOURCE_DIR}/res/win32/icon.rc)
		add_library(admin STATIC ${PROJECT_SOURCE_DIR}/res/win32/admin.obj)	
		add_library(icon STATIC ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)	
		set_source_files_properties(${PROJECT_SOURCE_DIR}/res/win32/admin.obj PROPERTIES EXTERNAL_OBJECT true GENERATED true)
		set_source_files_properties(${PROJECT_SOURCE_DIR}/res/win32/icon.obj PROPERTIES EXTERNAL_OBJECT true GENERATED true)
		set_target_properties(admin PROPERTIES LINKER_LANGUAGE C)	
		set_target_properties(icon PROPERTIES LINKER_LANGUAGE C)	
	endif()

	add_library(${PROJECT_NAME} OBJECT ${libs_files})
	set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)

	if(WIN32)
		add_library(${PROJECT_NAME}_shared STATIC $<TARGET_OBJECTS:${PROJECT_NAME}>)
	else()
		add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:${PROJECT_NAME}>)
	endif()
	add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:${PROJECT_NAME}>)

	target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_THREAD_LIBS_INIT})
	if(WIN32)
		target_link_libraries(${PROJECT_NAME}_shared ws2_32)
		target_link_libraries(${PROJECT_NAME}_shared user32)
		target_link_libraries(${PROJECT_NAME}_shared userenv)
		target_link_libraries(${PROJECT_NAME}_shared wsock32)
		target_link_libraries(${PROJECT_NAME}_shared iphlpapi)
		target_link_libraries(${PROJECT_NAME}_shared psapi)
		target_link_libraries(${PROJECT_NAME}_static ws2_32)
		target_link_libraries(${PROJECT_NAME}_static user32)
		target_link_libraries(${PROJECT_NAME}_static userenv)
		target_link_libraries(${PROJECT_NAME}_static wsock32)
		target_link_libraries(${PROJECT_NAME}_static iphlpapi)
		target_link_libraries(${PROJECT_NAME}_static psapi)
		if(${PROTOCOL_ARPING} MATCHES "ON")
			target_link_libraries(${PROJECT_NAME}_shared ${PROJECT_SOURCE_DIR}/res/win32/libwpcap.dll.a)
			target_link_libraries(${PROJECT_NAME}_static ${PROJECT_SOURCE_DIR}/res/win32/libwpcap.dll.a)
		endif()
	endif()

	target_link_libraries(${PROJECT_NAME}_shared m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}_shared ${Backtrace_LIBRARIES})
	endif()
	if(NOT WIN32)
		target_link_libraries(${PROJECT_NAME}_shared rt)
	endif()
	target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_WIRINGX_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_MBEDTLS_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_MBEDCRYPTO_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_MBEDX509_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_LUA_LIBS_INIT})

	target_link_libraries(${PROJECT_NAME}_static ${CMAKE_THREAD_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_static m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}_static ${Backtrace_LIBRARIES})
	endif()
	if(NOT WIN32)
		target_link_libraries(${PROJECT_NAME}_static rt)
	endif()
	target_link_libraries(${PROJECT_NAME}_static ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}_static ${CMAKE_WIRINGX_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_static ${CMAKE_MBEDTLS_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_static ${CMAKE_MBEDCRYPTO_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_static ${CMAKE_MBEDX509_LIBS_INIT})
	target_link_libraries(${PROJECT_NAME}_static ${CMAKE_LUA_LIBS_INIT})
	
	if(${PROTOCOL_ARPING} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_PCAP_LIBS_INIT})
		target_link_libraries(${PROJECT_NAME}_static ${CMAKE_PCAP_LIBS_INIT})
	endif()
		
	if(NOT WIN32)
		target_link_libraries(${PROJECT_NAME}_shared ${CMAKE_UNWIND_LIBS_INIT})
		target_link_libraries(${PROJECT_NAME}_static ${CMAKE_UNWIND_LIBS_INIT})
	endif()

	set_target_properties(${PROJECT_NAME}_shared ${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

	if(NOT WIN32)
		install(CODE "execute_process(COMMAND rm /usr/local/lib/lib${PROJECT_NAME}.so)")
		install(FILES ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}.so DESTINATION lib RENAME lib${PROJECT_NAME}.so.${VERSION} COMPONENT ${PROJECT_NAME})
		install(FILES ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}.a DESTINATION lib RENAME lib${PROJECT_NAME}.a COMPONENT ${PROJECT_NAME})
		install(CODE "execute_process(COMMAND ln -s \"/usr/local/lib/lib${PROJECT_NAME}.so.${VERSION}\" \"/usr/local/lib/lib${PROJECT_NAME}.so\")")

		if(EXISTS "/usr/local/lib/${PROJECT_NAME}/protocols")
			install(CODE "execute_process(COMMAND find /usr/local/lib/${PROJECT_NAME}/protocols/ -type f -exec rm {} \\;)")
		endif()
		if(EXISTS "/usr/local/lib/${PROJECT_NAME}/hardware")
			install(CODE "execute_process(COMMAND find /usr/local/lib/${PROJECT_NAME}/hardware/ -type f -exec rm {} \\;)")
		endif()
		if(EXISTS "/usr/local/lib/${PROJECT_NAME}/actions")
			install(CODE "execute_process(COMMAND find /usr/local/lib/${PROJECT_NAME}/actions/ -type f -exec rm {} \\;)")
		endif()
		if(EXISTS "/usr/local/lib/${PROJECT_NAME}/operators")
			install(CODE "execute_process(COMMAND find /usr/local/lib/${PROJECT_NAME}/operators/ -type f -exec rm {} \\;)")
		endif()
		if(EXISTS "/usr/local/${PROJECT_NAME}/operators")
			install(CODE "execute_process(COMMAND find /usr/local/${PROJECT_NAME}/operators/ -type f -exec rm {} \\;)")
		endif()

		if(EXISTS "/etc/${PROJECT_NAME}/config.json")
			install(CODE "execute_process(COMMAND mv /etc/${PROJECT_NAME}/config.json /etc/${PROJECT_NAME}/config.json.bak)")
		endif()
		
		if(EXISTS "/etc/${PROJECT_NAME}/pilight.pem")
			install(CODE "execute_process(COMMAND mv /etc/${PROJECT_NAME}/pilight.pem /etc/${PROJECT_NAME}/pilight.pem.bak)")
		endif()		

		install(PROGRAMS ${PROJECT_SOURCE_DIR}/res/init/${PROJECT_NAME}.initd DESTINATION /etc/init.d/ RENAME ${PROJECT_NAME})
		install(FILES ${PROJECT_SOURCE_DIR}/res/config/config.json-default DESTINATION /etc/${PROJECT_NAME}/ RENAME config.json COMPONENT ${PROJECT_NAME})
		install(FILES ${PROJECT_SOURCE_DIR}/res/pilight.pem DESTINATION /etc/${PROJECT_NAME}/ COMPONENT ${PROJECT_NAME})
	else()
		# install(FILES ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}.dll DESTINATION . RENAME lib${PROJECT_NAME}.dll COMPONENT ${PROJECT_NAME})
		install(FILES ${PROJECT_SOURCE_DIR}/res/config/config.json-default-w32 DESTINATION . RENAME config.json-default COMPONENT ${PROJECT_NAME})
	endif()

	if(${WEBSERVER} MATCHES "ON")
		if(WIN32)
			install(DIRECTORY ${PROJECT_SOURCE_DIR}/libs/webgui/ DESTINATION web/ COMPONENT webgui)
		else()
			install(DIRECTORY ${PROJECT_SOURCE_DIR}/libs/webgui/ DESTINATION /usr/local/lib/${PROJECT_NAME}/webgui COMPONENT webgui)
		endif()
	endif()	

	if(WIN32)
		add_executable(${PROJECT_NAME}-daemon daemon.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
		target_link_libraries(${PROJECT_NAME}-daemon "-Wl,--subsystem,windows")
	else()
		add_executable(${PROJECT_NAME}-daemon daemon.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-daemon ${PROJECT_NAME}_shared)
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-daemon stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-daemon ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}-daemon m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-daemon ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-daemon ${CMAKE_THREAD_LIBS_INIT})
	
	# if(WIN32)
		# add_executable(${PROJECT_NAME}-unittest unittest.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
		# target_link_libraries(${PROJECT_NAME}-unittest "-Wl,--subsystem,windows")
	# else()
		# add_executable(${PROJECT_NAME}-unittest unittest.c)
	# endif()
	# target_link_libraries(${PROJECT_NAME}-unittest ${PROJECT_NAME}_shared)
	# if(${ZWAVE} MATCHES "ON")
		# target_link_libraries(${PROJECT_NAME}-unittest stdc++)
	# endif()
	# target_link_libraries(${PROJECT_NAME}-unittest ${CMAKE_DL_LIBS})
	# target_link_libraries(${PROJECT_NAME}-unittest m)
	# if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		# target_link_libraries(${PROJECT_NAME}-unittest ${Backtrace_LIBRARIES})
	# endif()
	# target_link_libraries(${PROJECT_NAME}-unittest ${CMAKE_THREAD_LIBS_INIT})	

	if(WIN32)
		add_executable(${PROJECT_NAME}-raw raw.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-raw raw.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-raw ${PROJECT_NAME}_shared)
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-raw stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-raw ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}-raw m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-raw ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-raw ${CMAKE_THREAD_LIBS_INIT})

	if(WIN32)
		add_executable(${PROJECT_NAME}-send send.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-send send.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-send ${PROJECT_NAME}_shared)
	target_link_libraries(${PROJECT_NAME}-send ${CMAKE_DL_LIBS})
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-send stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-send m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-send ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-send ${CMAKE_THREAD_LIBS_INIT})

	if(WIN32)
		add_executable(${PROJECT_NAME}-control control.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-control control.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-control ${PROJECT_NAME}_shared)
	target_link_libraries(${PROJECT_NAME}-control ${CMAKE_DL_LIBS})
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-control stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-control m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-control ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-control ${CMAKE_THREAD_LIBS_INIT})

	if(WIN32)
		add_executable(${PROJECT_NAME}-receive receive.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-receive receive.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-receive ${PROJECT_NAME}_shared)
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-receive stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-receive ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}-receive m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-receive ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-receive ${CMAKE_THREAD_LIBS_INIT})

	if(WIN32)
		add_executable(${PROJECT_NAME}-debug debug.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-debug debug.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-debug ${PROJECT_NAME}_shared)
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-debug stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-debug ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}-debug m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-debug ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-debug ${CMAKE_THREAD_LIBS_INIT})

	if(WIN32)
		add_executable(${PROJECT_NAME}-uuid uuid.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-uuid uuid.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-uuid ${PROJECT_NAME}_shared)
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-uuid stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-uuid ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}-uuid m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-uuid ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-uuid ${CMAKE_THREAD_LIBS_INIT})

	if(WIN32)
		add_executable(${PROJECT_NAME}-sha256 sha256.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-sha256 sha256.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-sha256 ${PROJECT_NAME}_shared)
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-sha256 stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-sha256 ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}-sha256 m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-sha256 ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-sha256 ${CMAKE_THREAD_LIBS_INIT})	

	if(WIN32)
		add_executable(${PROJECT_NAME}-flash flash.c ${PROJECT_SOURCE_DIR}/res/win32/icon.obj)
	else()
		add_executable(${PROJECT_NAME}-flash flash.c)
	endif()
	target_link_libraries(${PROJECT_NAME}-flash ${PROJECT_NAME}_shared)
	if(${ZWAVE} MATCHES "ON")
		target_link_libraries(${PROJECT_NAME}-flash stdc++)
	endif()
	target_link_libraries(${PROJECT_NAME}-flash ${CMAKE_DL_LIBS})
	target_link_libraries(${PROJECT_NAME}-flash m)
	if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
		target_link_libraries(${PROJECT_NAME}-flash ${Backtrace_LIBRARIES})
	endif()
	target_link_libraries(${PROJECT_NAME}-flash ${CMAKE_THREAD_LIBS_INIT})

	if(WIN32)
		install(FILES "${PROJECT_SOURCE_DIR}/res/firmware/${PROJECT_NAME}_usb_nano.hex" DESTINATION . COMPONENT ${PROJECT_NAME})
	endif()

	if(NOT WIN32)
		if(EXISTS "/usr/local/sbin/${PROJECT_NAME}-send")
			install(CODE "execute_process(COMMAND rm /usr/local/sbin/${PROJECT_NAME}-send)")
		endif()

		if(EXISTS "/usr/local/sbin/${PROJECT_NAME}-receive")
			install(CODE "execute_process(COMMAND rm /usr/local/sbin/${PROJECT_NAME}-receive)")
		endif()

		if(EXISTS "/usr/local/sbin/${PROJECT_NAME}-control")
			install(CODE "execute_process(COMMAND rm /usr/local/sbin/${PROJECT_NAME}-control)")
		endif()

		if(EXISTS "/usr/local/sbin/${PROJECT_NAME}-uuid")
			install(CODE "execute_process(COMMAND rm /usr/local/sbin/${PROJECT_NAME}-uuid)")
		endif()
	endif()

	if(WIN32)
		install(FILES "${PROJECT_SOURCE_DIR}/LICENSE.txt" DESTINATION . COMPONENT ${PROJECT_NAME})
		install(FILES "${PROJECT_SOURCE_DIR}/res/win32/libmbedx509.dll" DESTINATION . COMPONENT ${PROJECT_NAME})
		install(FILES "${PROJECT_SOURCE_DIR}/res/win32/libmbedcrypto.dll" DESTINATION . COMPONENT ${PROJECT_NAME})
		install(FILES "${PROJECT_SOURCE_DIR}/res/win32/libmbedtls.dll" DESTINATION . COMPONENT ${PROJECT_NAME})
		install(FILES "${PROJECT_SOURCE_DIR}/res/win32/libwinpthread-1.dll" DESTINATION . COMPONENT ${PROJECT_NAME})
		install(FILES "${PROJECT_SOURCE_DIR}/res/win32/Packet.dll" DESTINATION . COMPONENT ${PROJECT_NAME})
		install(FILES "${PROJECT_SOURCE_DIR}/res/win32/wpcap.dll" DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-daemon.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-raw.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-send.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-control.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-receive.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-debug.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-flash.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uuid.exe DESTINATION . COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-sha256.exe DESTINATION . COMPONENT ${PROJECT_NAME})
	else()
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-daemon DESTINATION sbin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-raw DESTINATION sbin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-send DESTINATION bin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-control DESTINATION bin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-receive DESTINATION bin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-debug DESTINATION sbin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-flash DESTINATION sbin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-uuid DESTINATION bin COMPONENT ${PROJECT_NAME})
		install(PROGRAMS ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-sha256 DESTINATION bin COMPONENT ${PROJECT_NAME})
		install(CODE "execute_process(COMMAND update-rc.d ${PROJECT_NAME} defaults)")
		install(CODE "execute_process(COMMAND ldconfig)")
	endif()
endif()

if(WIN32)
	set(CPACK_MONOLITHIC_INSTALL True)
	set(CPACK_NSIS_INSTALL_ROOT "c:")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
	set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
	set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME} ${git_ver}")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://www.${PROJECT_NAME}.org")
	set(CPACK_NSIS_CONTACT "info@${PROJECT_NAME}.org")
	set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL True)
	set(CPACK_NSIS_MENU_LINKS
		"${PROJECT_NAME}-daemon" "${PROJECT_NAME}-daemon"
	)

	set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.txt")
	# set(CPACK_COMPONENTS_ALL ${PROJECT_NAME} webgui)
	set(CPACK_GENERATOR "NSIS")
	set(CPACK_PACKAGING_INSTALL_DIRECTORY "${PROJECT_NAME}")
	set(CPACK_SOURCE_STRIP_FILES TRUE)
	set(CPACK_STRIP_FILES TRUE)
	set(CPACK_PACKAGE_CONTACT "CurlyMo <info@${PROJECT_NAME}.org>")
	set(CPACK_PACKAGE_VERSION ${git_ver})
	set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
	set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME}")
	set(CPACK_PACKAGE_DESCRIPTION "Control RF devices with the Raspberry Pi")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Control RF devices with the Raspberry Pi")

	include(CPack)
else()
	WRITE_UNINSTALL_TARGET_SCRIPT()
	configure_file("${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
			IMMEDIATE @ONLY)
	add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

	set(CPACK_GENERATOR "DEB")
	set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local/")
	set(CPACK_SOURCE_STRIP_FILES TRUE)
	set(CPACK_STRIP_FILES TRUE)
	set(CPACK_PACKAGE_CONTACT "CurlyMo <info@${PROJECT_NAME}.org>")
	set(CPACK_PACKAGE_VERSION ${VERSION})
	set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "zlib1g-dev")
	set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
	set(CPACK_PACKAGE_DESCRIPTION "Control RF devices with the Raspberry Pi")
	set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Control RF devices with the Raspberry Pi")

	include(CPack)

	message("")
	message("${Red}-- WARNING --${ColorReset}")
	message("${Yellow}Manually compiling and installing ${PROJECT_NAME} will also clear all module folders:
/usr/local/lib/${PROJECT_NAME}/protocols/
/usr/local/lib/${PROJECT_NAME}/hardware/
/usr/local/lib/${PROJECT_NAME}/actions/
/usr/local/lib/${PROJECT_NAME}/operators/${ColorReset}")
	message("${Red}-- WARNING --${ColorReset}")
	message("")
endif()
