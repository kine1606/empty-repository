#include "FileHandler.h"

#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <sys/file.h>
#include <unistd.h>

namespace {
// RAII Helper for cross-platform Process-Level File Locking

enum class LockMode { Shared, Exclusive };

class ProcessLock {
public:
  ProcessLock(const std::string &targetFile, LockMode mode) {
    // The idea: create a temp file to lock instead of lock the real file.
    // when you are done, the changing real file will be the atomic step.

    std::string lockFilePath = targetFile + ".lock";
    m_fd = open(lockFilePath.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_fd == -1) {
      throw std::runtime_error("Cannot create/open process lock file.");
    }
    int operation = (mode == LockMode::Exclusive) ? LOCK_EX : LOCK_SH;
    if (flock(m_fd, operation) == -1) {
      close(m_fd);
      throw std::runtime_error("Cannot acquire process file lock.");
    }
  }
  ~ProcessLock() {
    flock(m_fd, LOCK_UN);
    close(m_fd);
  }

private:
  // file descriptor, an integer (number ticket ) that Linux gives your process
  // When call flock(LOCK_EX), the kernel marks that file as locked
  // If Process B also call flock, the kernel steps in and freezes Process B
  // until A unlocks it.
  int m_fd;
};
} // namespace

FileHandler::FileHandler(const std::string &p_path) : m_filePath(p_path) {}

std::string FileHandler::getTempFilePath() { return this->m_filePath + ".tmp"; }

std::uintmax_t FileHandler::size() const {
  ProcessLock lock(this->m_filePath,
                   LockMode::Shared); // Shared lock for reading
  std::error_code ec;
  auto fileSize = std::filesystem::file_size(this->m_filePath, ec);
  if (ec)
    return 0; // throw exception here
  return fileSize;
}
void FileHandler::write(const std::string &p_data) {
  ProcessLock lock(this->m_filePath,
                   LockMode::Exclusive); // Exclusive lock for writing
  const std::string TEMP_FILE_PATH = getTempFilePath();
  try {
    std::ofstream file(TEMP_FILE_PATH, std::ios::binary);
    if (!file)
      throw std::runtime_error("Cannot open temp file: " + TEMP_FILE_PATH);

    // Faster bulk write
    file.write(p_data.data(), p_data.size());
    file.close();

    if (!file)
      throw std::runtime_error("Cannot close temp file safely: " +
                               TEMP_FILE_PATH);

    // POSIX rename is atomic. Windows rename replaces natively in modern C++.
    std::filesystem::rename(TEMP_FILE_PATH, this->m_filePath);
  } catch (...) {
    std::filesystem::remove(TEMP_FILE_PATH);
    throw;
  }
}
void FileHandler::update(
    const std::function<std::string(const std::string &)> &p_updater) {
  ProcessLock lock(
      this->m_filePath,
      LockMode::Exclusive); // Exclusive lock (protects read + write cycle)

  std::string currentData;
  std::ifstream input(this->m_filePath, std::ios::binary | std::ios::ate);

  if (input) {
    // High-performance bulk read
    auto size = input.tellg();
    input.seekg(0, std::ios::beg);
    currentData.resize(size);
    if (size > 0)
      input.read(currentData.data(), size);
  }
  input.close();

  std::string updatedData = p_updater(currentData);
  const std::string TEMP_FILE_PATH = getTempFilePath();

  try {
    std::ofstream output(TEMP_FILE_PATH, std::ios::binary);
    if (!output)
      throw std::runtime_error("Cannot open temp file: " + TEMP_FILE_PATH);

    output.write(updatedData.data(), updatedData.size());
    output.close();
    if (!output)
      throw std::runtime_error("Failed to close/flush temp file: " +
                               TEMP_FILE_PATH);

    std::filesystem::rename(TEMP_FILE_PATH, this->m_filePath);
  } catch (...) {
    std::filesystem::remove(TEMP_FILE_PATH);
    throw;
  }
}

void FileHandler::append(const std::string &p_data) {
  ProcessLock lock(this->m_filePath, LockMode::Exclusive); // Exclusive lock
  std::ofstream file(this->m_filePath, std::ios::app | std::ios::binary);

  if (!file)
    throw std::runtime_error("Cannot open file: " + this->m_filePath);

  file.write(p_data.data(), p_data.size());
  file.close();

  if (!file)
    throw std::runtime_error("Cannot safely close appended file: " +
                             this->m_filePath);
}

void FileHandler::appendWithBreakLine(const std::string &p_data) {
  append(p_data + "\n"); // DRY principle: reuse the optimized append
}

const std::string FileHandler::read() const {
  ProcessLock lock(this->m_filePath, LockMode::Shared); // Shared lock
  std::ifstream file(this->m_filePath, std::ios::binary | std::ios::ate);

  if (!file)
    throw std::runtime_error("Cannot open file: " + this->m_filePath);

  // High-performance bulk read
  auto size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::string data;
  data.resize(size);

  if (size > 0) {
    file.read(data.data(), size);
  }

  if (file.fail() && !file.eof()) {
    throw std::runtime_error("Cannot successfully read file: " +
                             this->m_filePath);
  }

  return data;
}

void FileHandler::clear() {
  ProcessLock lock(this->m_filePath, LockMode::Exclusive); // Exclusive lock
  std::ofstream file(this->m_filePath, std::ios::trunc);
  if (!file)
    throw std::runtime_error("Cannot clear file: " + this->m_filePath);
}