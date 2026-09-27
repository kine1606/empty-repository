#include <gtest/gtest.h>

#include "FileHandler.h"
#include "FileManager.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class FileManagerTest : public ::testing::Test {
protected:
  const std::string TEST_DIR = "test_files";
  const std::string TEST_FILE = TEST_DIR + "/test_file.txt";

  std::string getTestFilePath(const std::string &p_fileName) {
    return TEST_DIR + "/" + p_fileName;
  }

  void SetUp() override {
    std::filesystem::remove_all(TEST_DIR);
    std::filesystem::create_directory(TEST_DIR);

    std::ofstream file(TEST_FILE);

    ASSERT_TRUE(file.is_open());
    file.close();
  }

  void TearDown() override { std::filesystem::remove_all(TEST_DIR); }
};

TEST_F(FileManagerTest, ShouldWriteContent) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  handler->write("Test content");

  EXPECT_EQ(handler->read(), "Test content");
}

TEST_F(FileManagerTest, ShouldAppendContentToTheEndOfFile) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  handler->write("Text 1 ");
  handler->append("Text 2");

  EXPECT_EQ(handler->read(), "Text 1 Text 2");
}

TEST_F(FileManagerTest, ShouldAppendContentWithBreakLine) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  handler->write("Text 1 ");
  handler->appendWithBreakLine("Text 2");

  EXPECT_EQ(handler->read(), "Text 1 Text 2\n");
}

TEST_F(FileManagerTest, ShouldUpdateByIncrementingValue) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  handler->write("0");
  handler->update([](const std::string &data) {
    int value = std::stoi(data);
    value++;
    return std::to_string(value);
  });

  EXPECT_EQ(handler->read(), "1");
}

TEST_F(FileManagerTest, ShouldClearContent) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  handler->write("Some content");
  handler->clear();

  EXPECT_EQ(handler->read(), "");
}

TEST_F(FileManagerTest, ShouldReturnFileSize) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  handler->write("Test content");
  EXPECT_EQ(handler->size(), 12);
}

TEST_F(FileManagerTest, ShouldReturnTemporaryFilePath) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  std::string tempFilePath = handler->getTempFilePath();
  EXPECT_EQ(tempFilePath,
            std::filesystem::absolute(TEST_FILE).lexically_normal().string() +
                ".tmp");
}

TEST_F(FileManagerTest, ShouldAvoidLostUpdateInConcurrentUpdates) {
  std::shared_ptr<FileHandler> handler =
      FileManager::getInstance().get(TEST_FILE);

  handler->write("0");

  std::thread thread1([&handler]() {
    handler->update([](const std::string &data) {
      int value = std::stoi(data);

      value++;

      return std::to_string(value);
    });
  });

  std::thread thread2([&handler]() {
    handler->update([](const std::string &data) {
      int value = std::stoi(data);

      value++;

      return std::to_string(value);
    });
  });

  thread1.join();
  thread2.join();

  EXPECT_EQ(handler->read(), "2");
}

TEST_F(FileManagerTest, ShouldListFilesInDirectory) {
  std::string file1 = getTestFilePath("file1.txt");
  std::string file2 = getTestFilePath("file2.txt");
  std::string file3 = getTestFilePath("file3.txt");

  FileManager &manager = FileManager::getInstance();

  manager.create(file1);
  manager.create(file2);
  manager.create(file3);

  std::vector<std::string> files = manager.list(TEST_DIR);

  EXPECT_NE(std::find(files.begin(), files.end(), "file1.txt"), files.end());
  EXPECT_NE(std::find(files.begin(), files.end(), "file2.txt"), files.end());
  EXPECT_NE(std::find(files.begin(), files.end(), "file3.txt"), files.end());
}

TEST_F(FileManagerTest, ShouldCreateFile) {
  std::string createFile = getTestFilePath("create_file.txt");

  FileManager &manager = FileManager::getInstance();

  manager.create(createFile);
  EXPECT_TRUE(std::filesystem::exists(createFile));
}

TEST_F(FileManagerTest, ShouldRemoveFile) {
  std::string removeFile = getTestFilePath("remove_file.txt");

  FileManager &manager = FileManager::getInstance();

  manager.create(removeFile);
  manager.remove(removeFile);
  EXPECT_FALSE(std::filesystem::exists(removeFile));
}

TEST_F(FileManagerTest, ShouldThrowWhenGettingNonExistingFile) {
  std::string nonExistingGetFile = getTestFilePath("non_existing_get_file.txt");

  FileManager &manager = FileManager::getInstance();

  EXPECT_THROW(manager.get(nonExistingGetFile), std::runtime_error);
}

TEST_F(FileManagerTest, ShouldThrowWhenCreatingExistingFile) {
  FileManager &manager = FileManager::getInstance();

  EXPECT_THROW(manager.create(TEST_FILE), std::runtime_error);
}

TEST_F(FileManagerTest, ShouldThrowWhenRemovingNonExistingFile) {
  std::string nonExistingFile = getTestFilePath("non_existing_remove_file.txt");
  FileManager &manager = FileManager::getInstance();

  EXPECT_THROW(manager.remove(nonExistingFile), std::runtime_error);
}

TEST_F(FileManagerTest, ShouldDetectExistingFile) {
  FileManager &manager = FileManager::getInstance();

  EXPECT_TRUE(manager.isfileExists(TEST_FILE));
}

TEST_F(FileManagerTest, ShouldDetectNonExistingFile) {
  FileManager &manager = FileManager::getInstance();

  EXPECT_FALSE(manager.isfileExists(getTestFilePath("non_existing.txt")));
}

TEST_F(FileManagerTest, ShouldReturnSameHandlerForSamePath) {
  FileManager &manager = FileManager::getInstance();

  std::shared_ptr<FileHandler> first = manager.get(TEST_FILE);
  std::shared_ptr<FileHandler> second = manager.get(TEST_FILE);

  EXPECT_EQ(first, second);
}

TEST_F(FileManagerTest, ShouldNormalizePathsBeforeCaching) {
  FileManager &manager = FileManager::getInstance();

  std::shared_ptr<FileHandler> plain = manager.get(TEST_FILE);
  std::shared_ptr<FileHandler> redundant =
      manager.get("test_files/./test_file.txt");

  EXPECT_EQ(plain, redundant);
}

TEST_F(FileManagerTest, ShouldReturnHandlerForCreatedFile) {
  std::string createdFile = getTestFilePath("get_after_create.txt");
  FileManager &manager = FileManager::getInstance();

  manager.create(createdFile);
  std::shared_ptr<FileHandler> handler = manager.get(createdFile);

  handler->write("hello");
  EXPECT_EQ(handler->read(), "hello");
}

TEST_F(FileManagerTest, ShouldListFilesInSortedOrder) {
  FileManager &manager = FileManager::getInstance();

  manager.create(getTestFilePath("b.txt"));
  manager.create(getTestFilePath("a.txt"));
  manager.create(getTestFilePath("c.txt"));

  std::vector<std::string> expected = {"a.txt", "b.txt", "c.txt",
                                       "test_file.txt"};
  EXPECT_EQ(manager.list(TEST_DIR), expected);
}

TEST_F(FileManagerTest, ShouldMarkDirectoryEntriesWithTrailingSlash) {
  FileManager &manager = FileManager::getInstance();

  std::string subDir = getTestFilePath("subdir");
  std::filesystem::create_directory(subDir);

  std::vector<std::string> files = manager.list(TEST_DIR);

  EXPECT_NE(std::find(files.begin(), files.end(), "subdir/"), files.end());
}

TEST_F(FileManagerTest, ShouldReturnEmptyListForEmptyDirectory) {
  FileManager &manager = FileManager::getInstance();

  std::string emptyDir = getTestFilePath("empty_dir");
  std::filesystem::create_directory(emptyDir);

  EXPECT_TRUE(manager.list(emptyDir).empty());
}

TEST_F(FileManagerTest, ShouldThrowWhenListingNonExistingDirectory) {
  FileManager &manager = FileManager::getInstance();

  EXPECT_THROW(manager.list(getTestFilePath("non_existing_dir")),
               std::filesystem::filesystem_error);
}

TEST_F(FileManagerTest, ShouldThrowWhenCreatingFileInNonExistingDirectory) {
  FileManager &manager = FileManager::getInstance();

  EXPECT_THROW(manager.create(getTestFilePath("no_dir/file.txt")),
               std::runtime_error);
}

TEST_F(FileManagerTest, ShouldAllowRecreatingFileAfterRemoval) {
  std::string recreatedFile = getTestFilePath("recreate.txt");
  FileManager &manager = FileManager::getInstance();

  manager.create(recreatedFile);
  manager.remove(recreatedFile);
  EXPECT_FALSE(std::filesystem::exists(recreatedFile));

  manager.create(recreatedFile);
  EXPECT_TRUE(std::filesystem::exists(recreatedFile));

  std::shared_ptr<FileHandler> handler = manager.get(recreatedFile);
  handler->write("fresh");
  EXPECT_EQ(handler->read(), "fresh");
}