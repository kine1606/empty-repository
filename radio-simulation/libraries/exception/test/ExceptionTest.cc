#include <gtest/gtest.h>

#include "Exception.h"

TEST(ExceptionTest, ShouldThrowAndCatchException) {
  try {
    throw Exception("Test exception");
  } catch (const Exception &exception) {
    EXPECT_STREQ(exception.what(), "Test exception");
  }
}