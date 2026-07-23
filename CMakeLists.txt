cmake_minimum_required(VERSION 3.5...4.2)
set(CMAKE_CXX_STANDARD 23)

#! ! ! ! ! ! !
#set this to ON to ship the game!
#basically this will change the RESOURCES_PATH to be the local path
#! ! ! ! ! ! !
#IF IT DOESN'T WORK, DELETE THE OUT FOLDER AND REBUILD!
set(PRODUCTION_BUILD OFF CACHE BOOL "Make this a production build" FORCE)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:Release>")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(tinywagondemo)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(thirdparty/glfw-3.3.2)			#window oppener
add_subdirectory(thirdparty/glad)				#opengl loader
add_subdirectory(thirdparty/stb_image)			#loading immaged
add_subdirectory(thirdparty/stb_truetype)		#loading ttf files
add_subdirectory(thirdparty/glm)				#math

add_subdirectory("tinywagon")

add_executable(tinywagondemo "demo/demo.cpp" "demo/openglErrorReporting.cpp")
target_include_directories(tinywagondemo PUBLIC "demo/")

target_compile_definitions(tinywagondemo PUBLIC GLFW_INCLUDE_NONE=1)

if(PRODUCTION_BUILD)
	# setup the ASSETS_PATH macro to be in the root folder of your exe
	target_compile_definitions(tinywagondemo PUBLIC RESOURCES_PATH="./resources/") 

else()
	# This is useful to get an ASSETS_PATH in your IDE during development
	target_compile_definitions(tinywagondemo PUBLIC RESOURCES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/resources/")

endif()

if(MSVC) # If using the VS compiler...
	target_compile_definitions(tinywagondemo PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(tinywagondemo PRIVATE glm glfw 
	glad stb_image stb_truetype tinywagon)

