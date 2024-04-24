CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
PROJECT(ImageProcessing)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# SET(CMAKE_BUILD_TYPE "Release")

find_package(OpenCV REQUIRED) # CV

include_directories(${OpenCV_INCLUDE_DIRS}) # CV
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

link_directories(${OpenCV_LIBRARY_DIRS}) # CV

ADD_EXECUTABLE(${PROJECT_NAME} src/main.cpp) ## !

TARGET_LINK_LIBRARIES(
    ${PROJECT_NAME} 
    # ${ZED_LIBS} 
    ${OpenCV_LIBRARIES} # CV
)
