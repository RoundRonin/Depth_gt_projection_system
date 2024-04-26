CMAKE_MINIMUM_REQUIRED(VERSION 3.5)
set(this ImageProcessing_tests)

ADD_EXECUTABLE(${this} test.cpp)
TARGET_LINK_LIBRARIES(${this} PUBLIC
    gtest_main
    "${PROJECT_NAME}_testing"
)

ADD_TEST(
    NAME ${this}
    COMMAND ${this}
)