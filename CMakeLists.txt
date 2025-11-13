cmake_minimum_required(VERSION 3.20)
project(voxel-engine VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# External libraries
add_subdirectory(external/glfw)
add_subdirectory(external/glm)
add_library(glad STATIC external/glad/src/glad.c)

# Source files
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Add executable
add_executable(${PROJECT_NAME} ${SRC_FILES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_include_directories(glad PUBLIC external/glad/include)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad glm::glm)

# Extra
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
	target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()