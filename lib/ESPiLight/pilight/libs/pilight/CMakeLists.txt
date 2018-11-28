project(pilight)

include(../../CMakeConfig.txt)

if(${MODULESPACK} MATCHES "OFF" OR WIN32)
	set(${PROJECT_NAME}_files "" CACHE INTERNAL "")

	add_subdirectory(core)
	set(${PROJECT_NAME}_files	${${PROJECT_NAME}_files} ${${PROJECT_NAME}_core_files} CACHE INTERNAL "")

	add_subdirectory(config)
	set(${PROJECT_NAME}_files	${${PROJECT_NAME}_files} ${${PROJECT_NAME}_config_files} CACHE INTERNAL "")

	add_subdirectory(datatypes)
	set(${PROJECT_NAME}_files	${${PROJECT_NAME}_files} ${${PROJECT_NAME}_datatypes_files} CACHE INTERNAL "")

	if(${EVENTS} MATCHES "ON")
		add_subdirectory(events)
		set(${PROJECT_NAME}_files	${${PROJECT_NAME}_files} ${${PROJECT_NAME}_events_files} CACHE INTERNAL "")
	endif()

	add_subdirectory(protocols)
	set(${PROJECT_NAME}_files	${${PROJECT_NAME}_files} ${${PROJECT_NAME}_protocols_files} CACHE INTERNAL "")

	add_subdirectory(hardware)
	set(${PROJECT_NAME}_files	${${PROJECT_NAME}_files} ${${PROJECT_NAME}_hardware_files} CACHE INTERNAL "")

	add_subdirectory(lua)
	set(${PROJECT_NAME}_files	${${PROJECT_NAME}_files} ${${PROJECT_NAME}_lua_files} CACHE INTERNAL "")
else()
	if(${EVENTS} MATCHES "ON")
		add_subdirectory(events)
	endif()
	add_subdirectory(protocols)
	add_subdirectory(hardware)
endif()
