#include <gtest/gtest.h>

#include "FileManagerException.h"

TEST(FileManagerExceptionTest, ShouldThrowAndCatchFileManagerException) {
  try {
    throw FileManagerException("Test error");
  } catch (const FileManagerException &exception) {
    EXPECT_STREQ(exception.what(), "[FileManager] Test error");
  }
}

TEST(FileManagerExceptionTest, ShouldThrowAndCatchFileNotFound) {
  try {
    throw FileNotFound("Test path");
  } catch (const FileManagerException &exception) {
    EXPECT_STREQ(exception.what(), "[FileManager] File not found: Test path");
  }
}

TEST(FileManagerExceptionTest, ShouldThrowAndCatchFileAlreadyExists) {
  try {
    throw FileAlreadyExists("Test path");
  } catch (const FileManagerException &exception) {
    EXPECT_STREQ(exception.what(),
                 "[FileManager] File already exists: Test path");
  }
}

TEST(FileManagerExceptionTest, ShouldThrowAndCatchFileOperationFailed) {
  try {
    throw FileOperationFailed("Test path", FileOperation::READ);
  } catch (const FileManagerException &exception) {
    EXPECT_STREQ(exception.what(),
                 "[FileManager] Failed to read file: Test path");
  }
}