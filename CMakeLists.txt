cmake_minimum_required(VERSION 3.20)
project(practice-engine VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# External libraries
add_subdirectory(external/glfw)
add_subdirectory(external/glm)

set(IMGUI_SOURCES
	external/imgui/imgui.cpp
	external/imgui/imgui_demo.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/imgui_widgets.cpp
	external/imgui/backends/imgui_impl_glfw.cpp
	external/imgui/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})
add_library(glad STATIC external/glad/src/glad.c)

# Source files
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

# Add executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Copy resources
add_custom_target(copy_shaders ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/src/shaders
	${CMAKE_BINARY_DIR}/src/shaders
	COMMENT "Copying shaders..."
)

add_dependencies(${PROJECT_NAME} copy_shaders)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(glad PUBLIC external/glad/include)
target_include_directories(imgui PUBLIC external/imgui external/imgui/backends external/glad/include)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm imgui)
target_link_libraries(imgui PRIVATE glfw)

# Extra
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
	else()
		set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE)
	endif()
endif()