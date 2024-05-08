CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(ImageProcessing)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# SET(CMAKE_BUILD_TYPE "Release")

message("COMPILER: ${CMAKE_CXX_COMPILER_ID}")
message("VERSION: ${CMAKE_CXX_COMPILER_VERSION}")

set (CMAKE_CXX_FLAGS "-lstdc++ -lm")

find_package(OpenCV REQUIRED) # CV

include_directories(${OpenCV_INCLUDE_DIRS}) # CV
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
enable_testing()
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

# message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

link_directories(${OpenCV_LIBRARY_DIRS}) # CV

ADD_LIBRARY("${PROJECT_NAME}_testing" src/main.cpp)
ADD_EXECUTABLE(${PROJECT_NAME} src/main.cpp) ## !

TARGET_LINK_LIBRARIES(
    ${PROJECT_NAME} 
    # ${ZED_LIBS} 
    ${OpenCV_LIBRARIES} # CV
)

add_subdirectory(test)