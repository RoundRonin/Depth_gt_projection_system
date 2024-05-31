CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
PROJECT(ImageProcessing)
set(ENV{MAKEFLAGS} "--no-print-directory")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# SET(CMAKE_BUILD_TYPE "Release")
option(LINK_SHARED_ZED "Link with the ZED SDK shared executable" ON) 

message("COMPILER: ${CMAKE_CXX_COMPILER_ID}")
message("VERSION: ${CMAKE_CXX_COMPILER_VERSION}")

# set (CMAKE_CXX_FLAGS "-lstdc++ -lm")
set (CMAKE_EXPORT_COMPILE_COMMANDS 1)

cmake_policy(SET CMP0153 OLD)
cmake_policy(SET CMP0146 OLD)

# TODO add jq as requirement

# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

# add_compile_options(-Wdeprecated-anon-enum-enum-conversion)
# add_compile_options(-Wno-deprecated-enum-enum-conversion)
# add_compile_options(-Wno-deprecated-anon-enum-enum-conversion)
# add_compile_options(-Wno-c11-extensions)
add_compile_options(-Wno-deprecated-enum-float-conversion)
add_compile_options(-Wno-sign-compare)
add_compile_options(-Wall -Wextra -Wpedantic ) # m_version(version)

find_package(ZED 3 REQUIRED)
find_package(OpenCV REQUIRED) # CV
find_package(CUDA ${ZED_CUDA_VERSION} REQUIRED)

include_directories(SYSTEM ${CUDA_INCLUDE_DIRS})
include_directories(SYSTEM ${ZED_INCLUDE_DIRS})
include_directories(SYSTEM ${OpenCV_INCLUDE_DIRS}) # CV
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# enable_testing()



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


# FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
# FetchContent_MakeAvailable(json)

# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )

# FetchContent_GetProperties(googletest)
# if(NOT googletest_POPULATED)
#   FetchContent_Populate(googletest)
#   add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
# endif()

# message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

link_directories(${ZED_LIBRARY_DIR})
link_directories(${CUDA_LIBRARY_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS}) # CV

# ADD_LIBRARY("${PROJECT_NAME}_testing" src/main.cpp)
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
# add_subdirectory(test)