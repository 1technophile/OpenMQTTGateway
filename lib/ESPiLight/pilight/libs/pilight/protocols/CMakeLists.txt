project(pilight_protocols)

add_subdirectory(433.92)
add_subdirectory(API)
add_subdirectory(GPIO)
add_subdirectory(generic)
add_subdirectory(network)
if(${ZWAVE} MATCHES "ON")
	add_subdirectory(zwave)
endif()
if(${IR} MATCHES "ON")
	add_subdirectory(IR)
endif()
add_subdirectory(core)

file(REMOVE_RECURSE "protocol_header.h")
file(REMOVE "protocol_header.h")
file(REMOVE_RECURSE "protocol_init.h")
file(REMOVE "protocol_init.h")

file(GLOB_RECURSE ${PROJECT_NAME}_headers_includes "protocol_header.h")
file(GLOB_RECURSE ${PROJECT_NAME}_init_includes "protocol_init.h")

include(CMakeExclude.txt)
include(CMakeInclude.txt)

if(${MODULESPACK} MATCHES "OFF" OR WIN32)
	file(GLOB ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/*.c")
	file(GLOB ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/*.h")

	set(${PROJECT_NAME}_files 
		${pilight_protocols_433.92_files}
		${pilight_protocols_ir_files}
		${pilight_protocols_api_files}
		${pilight_protocols_zwave_files}
		${pilight_protocols_generic_files}
		${pilight_protocols_network_files}
		${pilight_protocols_gpio_files}
		${pilight_protocols_core_files}
		${${PROJECT_NAME}_sources}
		${${PROJECT_NAME}_headers}
	CACHE INTERNAL "")
else()
	set(${PROJECT_NAME}_files 
		${pilight_protocols_433.92_files}
		${pilight_protocols_ir_files}
		${pilight_protocols_api_files}
		${pilight_protocols_zwave_files}
		${pilight_protocols_generic_files}
		${pilight_protocols_network_files}
		${pilight_protocols_gpio_files}
		${pilight_protocols_core_files}
	)	

	foreach(file ${${PROJECT_NAME}_files})
		get_filename_component(name ${file} NAME_WE)
		get_filename_component(ext ${file} EXT)
		if(${ext} MATCHES ".c")
			add_library(${name}_module MODULE ${file})
			target_link_libraries(${name}_module ${CMAKE_THREAD_LIBS_INIT})
			target_link_libraries(${name}_module m)
			if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
				target_link_libraries(${name}_module execinfo)
			endif()
			target_link_libraries(${name}_module ${CMAKE_DL_LIBS})
			set_target_properties(${name}_module PROPERTIES OUTPUT_NAME ${name})
			string(REPLACE "_" "-" debname ${name})
			install(FILES ${CMAKE_BINARY_DIR}/libs/pilight/protocols/lib${name}.so DESTINATION lib/pilight/protocols RENAME ${name}.so COMPONENT pilight-protocol-${debname})
		endif()
	endforeach()
endif()