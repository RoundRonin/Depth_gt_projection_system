#include <gtest/gtest.h>

#include "../src/utils.hpp"

TEST(UtilsTest, existance_test) {
    Logger log("log");

    EXPECT_EQ(Logger::SUCCESS, log.drop());
}
