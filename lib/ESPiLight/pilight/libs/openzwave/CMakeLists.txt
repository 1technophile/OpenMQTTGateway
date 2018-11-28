project(openzwave)

file(GLOB ${PROJECT_NAME}_sources "${PROJECT_SOURCE_DIR}/*.cpp")
file(GLOB ${PROJECT_NAME}_headers "${PROJECT_SOURCE_DIR}/*.h")

add_subdirectory(aes)
add_subdirectory(command_classes)
add_subdirectory(platform)
add_subdirectory(value_classes)

set(${PROJECT_NAME}_files 
	${${PROJECT_NAME}_aes_files}
	${${PROJECT_NAME}_command_classes_files}
	${${PROJECT_NAME}_platform_files}
	${${PROJECT_NAME}_value_classes_files}
	${${PROJECT_NAME}_sources} 
	${${PROJECT_NAME}_headers} 
CACHE INTERNAL "")
