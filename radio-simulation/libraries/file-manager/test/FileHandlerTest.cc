#include "FileHandler.h"
#include "FileManager.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class FileHandlerTest : public ::testing::Test {
protected:
  const std::string testFile = "test_file.txt";
  const std::string lockFile = "test_file.txt.lock";
  const std::string tmpFile = "test_file.txt.tmp";
  const std::string missingFile = "missing_file.txt";
  const std::string missingLockFile = "missing_file.txt.lock";
  // Runs before every test
  void SetUp() override { cleanUpFiles(); }

  // Runs after every test
  void TearDown() override { cleanUpFiles(); }

  void cleanUpFiles() {
    std::error_code ec;
    std::filesystem::remove(testFile, ec);
    std::filesystem::remove(missingFile, ec);
    std::filesystem::remove(missingLockFile, ec);
    std::filesystem::remove(lockFile, ec);
    std::filesystem::remove(tmpFile, ec);
  }
};

// --- NORMAL OPERATION TESTS ---

TEST_F(FileHandlerTest, WriteAndReadNormal) {
  FileHandler fh(testFile);
  std::string expectedData = "Hello, process-safe world!";

  EXPECT_NO_THROW(fh.write(expectedData));
  EXPECT_EQ(fh.read(), expectedData);

  // Verify the lock file was created during the process
  EXPECT_TRUE(std::filesystem::exists(lockFile));
}

TEST_F(FileHandlerTest, AppendNormal) {
  FileHandler fh(testFile);
  fh.write("Line 1");
  fh.append("Line 2");

  EXPECT_EQ(fh.read(), "Line 1Line 2");
}

TEST_F(FileHandlerTest, AppendWithBreakLineNormal) {
  FileHandler fh(testFile);
  fh.write("Line 1");
  fh.appendWithBreakLine("Line 2");

  EXPECT_EQ(fh.read(), "Line 1Line 2\n");
}

TEST_F(FileHandlerTest, UpdateNormal) {
  FileHandler fh(testFile);
  fh.write("100");

  fh.update([](const std::string &current) {
    int val = std::stoi(current);
    return std::to_string(val + 50);
  });

  EXPECT_EQ(fh.read(), "150");
}

TEST_F(FileHandlerTest, ClearNormal) {
  FileHandler fh(testFile);
  fh.write("Some data to be cleared");
  ASSERT_GT(fh.size(), 0);

  fh.clear();
  EXPECT_EQ(fh.size(), 0);
  EXPECT_EQ(fh.read(), "");
}

// --- EDGE CASE TESTS ---

TEST_F(FileHandlerTest, ReadNonExistentFileThrows) {
  FileHandler fh("missing_file.txt");
  EXPECT_THROW(fh.read(), std::runtime_error);
}

TEST_F(FileHandlerTest, SizeOfNonExistentFileIsZero) {
  FileHandler fh("missing_file.txt");
  EXPECT_EQ(fh.size(), 0);
}

TEST_F(FileHandlerTest, UpdateNonExistentFileThrows) {
  FileHandler fh("missing_file.txt");
  fh.update([](const std::string &str) { return str; });
  EXPECT_EQ(fh.read(), "");
}

TEST_F(FileHandlerTest, WriteToInvalidDirectoryThrows) {
  // Attempting to write to a folder that does not exist
  FileHandler fh("non_existent_folder/test.txt");

  // Depending on the OS, creating the lock file or the temp file will fail
  EXPECT_THROW(fh.write("Data"), std::runtime_error);
}

TEST_F(FileHandlerTest, UpdateLambdaThrowsExceptionRollsBack) {
  FileHandler fh(testFile);
  fh.write("Original Data");

  // The lambda throws an exception mid-update
  EXPECT_THROW(
      {
        fh.update([](const std::string &) -> std::string {
          throw std::logic_error("Simulated failure during update");
        });
      },
      std::logic_error);

  // 1. The original file should remain untouched (Atomic property)
  EXPECT_EQ(fh.read(), "Original Data");

  // 2. The temporary file should have been cleaned up by the catch(...) block
  EXPECT_FALSE(std::filesystem::exists(tmpFile));
}

TEST_F(FileHandlerTest, EmptyStringHandling) {
  FileHandler fh(testFile);

  fh.write("");
  EXPECT_EQ(fh.size(), 0);
  EXPECT_EQ(fh.read(), "");

  fh.append("");
  EXPECT_EQ(fh.size(), 0);
  EXPECT_EQ(fh.read(), "");
}

TEST_F(FileHandlerTest, AppendCreatesFileIfMissing) {
  FileHandler fh(testFile);

  // Append should create the file implicitly via std::ios::app
  EXPECT_NO_THROW(fh.append("New File Data"));
  EXPECT_EQ(fh.read(), "New File Data");
}

// --- CONCURRENCY / LOCKING TESTS ---

TEST_F(FileHandlerTest, ConcurrentReadWriteSimulation) {
  FileHandler fh(testFile);
  fh.write("Initial");

  // Spawn multiple threads to hammer the update function.
  // Because each thread opens a new fd/handle in ProcessLock,
  // flock / LockFileEx will safely serialize them even in the same process.
  constexpr int numThreads = 20;
  std::vector<std::thread> threads;

  for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back([&fh]() {
      fh.update([](const std::string &current) { return current + "+"; });
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // "Initial" + 20 '+' characters
  std::string expectedResult = "Initial" + std::string(numThreads, '+');
  EXPECT_EQ(fh.read(), expectedResult);
}