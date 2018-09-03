project(vulkanHelloWorld)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++14  " )

add_executable(${PROJECT_NAME} main.cpp)

find_package(Boost REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_library(SHADERC_LIB shaderc_combined $ENV{VULKAN_SDK}/Lib)

MESSAGE(STATUS "Found shaderc in: ${SHADERC_LIB}")

if (VULKAN_FOUND)
    message(STATUS "Found Vulkan, Including and Linking now")
    include_directories(${Vulkan_INCLUDE_DIRS})
    target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} glfw ${SHADERC_LIB})
endif (VULKAN_FOUND)
