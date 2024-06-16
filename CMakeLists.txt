CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(ImageProcessing)
set(ENV{MAKEFLAGS} "--no-print-directory")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(LINK_SHARED_ZED "Link with the ZED SDK shared executable" ON) 

message("COMPILER: ${CMAKE_CXX_COMPILER_ID}")
message("VERSION: ${CMAKE_CXX_COMPILER_VERSION}")


SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set (CMAKE_EXPORT_COMPILE_COMMANDS 1)

cmake_policy(SET CMP0153 OLD)
cmake_policy(SET CMP0146 OLD)


add_compile_options(-Wno-deprecated-enum-float-conversion)
add_compile_options(-Wno-sign-compare)
add_compile_options(-Wall -Wextra -Wpedantic ) 

find_package(ZED 3 REQUIRED)
find_package(OpenCV REQUIRED) # CV
find_package(CUDA ${ZED_CUDA_VERSION} REQUIRED)

include_directories(SYSTEM ${CUDA_INCLUDE_DIRS})
include_directories(SYSTEM ${ZED_INCLUDE_DIRS})
include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS}) # CV
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

include(FetchContent)
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR})
endif()

link_directories(${ZED_LIBRARY_DIR})
link_directories(${CUDA_LIBRARY_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS}) # CV

ADD_EXECUTABLE(${PROJECT_NAME} include/sl_utils.hpp src/main.cpp) ## !

if (LINK_SHARED_ZED)
    SET(ZED_LIBS ${ZED_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY})
else()
    SET(ZED_LIBS ${ZED_STATIC_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_LIBRARY})
endif()

TARGET_LINK_LIBRARIES(
    ${PROJECT_NAME} 
    PRIVATE
    nlohmann_json::nlohmann_json
    PUBLIC
    ${ZED_LIBS} 
    ${OpenCV_LIBRARIES} # CV
)