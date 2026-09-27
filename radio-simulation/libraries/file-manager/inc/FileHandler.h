#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>

/**
 * @class FileHandler
 * @brief Manages access to a specific file.
 *
 * Each FileHandler instance is associated with exactly one file.
 * It provides operations for reading, writing, updating, appending, and
 * clearing the file content. Multiple threads can read the file simultaneously
 * Modifications (write, update, append, clear) are exclusive and will block
 * other operations until completed.
 */
class FileHandler {
private:
  /**
   * @brief Path of the file.
   */
  std::string m_filePath;

  /**
   * @brief A lock to synchronize access to the file.
   */
  mutable std::shared_mutex m_fileMutex;

public:
  /**
   * @brief Constructs a FileHandler for the specified file.
   *
   * @param p_path Path of the file to manage.
   */
  explicit FileHandler(const std::string &p_path);

  /**
   * @brief Deleted copy constructor.
   */
  FileHandler(const FileHandler &) = delete;

  /**
   * @brief Deleted copy assignment operator.
   */
  FileHandler &operator=(const FileHandler &) = delete;

  /**
   * @brief Returns the size of the file.
   *
   * Protected by a shared lock.
   *
   * @return Size of the file.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  std::uintmax_t size() const;

  /**
   * @brief Writes data to the file.
   *
   * Existing file content is replaced by the provided data.
   *
   * Protected by an exclusive lock.
   *
   * @param p_data Data to write to the file.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  void write(const std::string &p_data);

  /**
   * @brief Updates the content of the file.
   *
   * The entire read-modify-write operation is protected by an exclusive lock
   * to prevent race conditions
   *
   * @param p_updater Function used to generate the updated content.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  void update(const std::function<std::string(const std::string &)> &p_updater);

  /**
   * @brief Appends data to the end of the file.
   *
   * Protected by an exclusive lock.
   *
   * @param p_data Data to append to the file.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  void append(const std::string &p_data);

  /**
   * @brief Appends data to the end of the file with a newline character.
   *
   * Protected by an exclusive lock.
   *
   * @param p_data Data to append to the file.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  void appendWithBreakLine(const std::string &p_data);

  /**
   * @brief Reads the content of the file.
   *
   * Protected by a shared lock, allowing multiple threads to read
   * simultaneously.
   *
   * @return The content of the file.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  const std::string read() const;

  /**
   * @brief Clears the content of the file.
   *
   * Protected by an exclusive lock.
   *
   * @throws std::runtime_error If the file cannot be opened.
   */
  void clear();

  /**
   * @brief Generates a temporary file path.
   *
   * The temporary file is created in the same directory as the original file.
   *
   * @return The path of the temporary file.
   */
  std::string getTempFilePath();

  /**
   * @brief Grants FileManager access to private members of FileHandler.
   *
   * This allows FileManager to create and manage FileHandler instances.
   */
  friend class FileManager;
};
#endif