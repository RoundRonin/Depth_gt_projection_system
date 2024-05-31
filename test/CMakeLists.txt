CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
set(this ImageProcessing_tests)

set (CMAKE_CXX_FLAGS "-lstdc++ -lm")

find_package(ZED 3 REQUIRED)
find_package(OpenCV REQUIRED) # CV
find_package(CUDA ${ZED_CUDA_VERSION} REQUIRED)

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${ZED_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS}) # CV
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

link_directories(${ZED_LIBRARY_DIR})
link_directories(${CUDA_LIBRARY_DIRS})
link_directories(${OpenCV_LIBRARY_DIRS}) # CV 

if (LINK_SHARED_ZED)
    SET(ZED_LIBS ${ZED_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_CUDART_LIBRARY})
else()
    SET(ZED_LIBS ${ZED_STATIC_LIBRARIES} ${CUDA_CUDA_LIBRARY} ${CUDA_LIBRARY})
endif()

ADD_EXECUTABLE(${this} test.cpp)
TARGET_LINK_LIBRARIES(${this} PUBLIC
    gtest_main
    "${PROJECT_NAME}_testing"
    ${ZED_LIBS} 
    ${OpenCV_LIBRARIES} # CV
)

ADD_TEST(
    NAME ${this}
    COMMAND ${this}
)