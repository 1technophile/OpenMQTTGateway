project(openzwave_platform)

file(GLOB ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/*.cpp")
file(GLOB ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/*.h")

set(${PROJECT_NAME}_files ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers} CACHE INTERNAL "")

if(WIN32)
	add_subdirectory(windows)
	set(${PROJECT_NAME}_files ${${PROJECT_NAME}_windows_files} ${${PROJECT_NAME}_files} CACHE INTERNAL "")
else()
	add_subdirectory(unix)
	set(${PROJECT_NAME}_files ${${PROJECT_NAME}_unix_files} ${${PROJECT_NAME}_files} CACHE INTERNAL "")
endif()

