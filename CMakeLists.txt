set(EXE_NAME "ds325_viewer")
set(DEPTHSENSE_SDK "/opt/softkinetic/DepthSenseSDK")
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(${EXE_NAME})
find_package(PCL 1.2 REQUIRED)
include_directories(${PCL_INCLUDE_DIRS} "${DEPTHSENSE_SDK}/include")
link_directories(${PCL_LIBRARY_DIRS} "${DEPTHSENSE_SDK}/lib")
add_definitions(${PCL_DEFINITIONS})
add_executable(${EXE_NAME} main.cpp)
target_link_libraries(${EXE_NAME} ${PCL_LIBRARIES} DepthSense)
