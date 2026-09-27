#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "FileHandler.h"

#include <memory>
#include <mutex>
#include <unordered_map>

class FileManager {
private:
  /**
   * @brief A map that associates file paths with their FileHandler instances.
   */
  static std::unordered_map<std::string, std::shared_ptr<FileHandler>>
      m_fileHandlerInstances;

  /**
   * @brief A lock to synchronize the creation of FileHandler instances.
   */
  static std::shared_mutex m_instanceMutex;

  /**
   * @brief Default constructor.
   */
  FileManager() = default;

  /**
   * @brief Deleted copy constructor.
   */
  FileManager(const FileManager &) = delete;

  /**
   * @brief Deleted copy assignment operator.
   */
  FileManager &operator=(const FileManager &) = delete;

public:
  /**
   * @brief Returns the singleton instance of FileManager.
   *
   * @return Reference to the singleton FileManager instance.
   */
  static FileManager &getInstance();

  /**
   * @brief Checks if a file exists at the specified path.
   *
   * @param p_path Path of the file to check.
   *
   * @return True if the file exists, false otherwise.
   */
  bool isfileExists(const std::string &p_path);

  /**
   * @brief Retrieves the FileHandler for the specified file path.
   *
   * If a FileHandler for the given path already exists, it returns that
   * instance. Otherwise, it creates a new FileHandler, stores it in the
   * manager, and returns it.
   *
   * @param p_path Path of the file to manage.
   *
   * @return Filehandler instance associated with the specified file path.
   *
   * @throws std::runtime_error If the file does not exist.
   */
  std::shared_ptr<FileHandler> get(const std::string &p_path);

  /**
   * @brief Lists all files in the specified directory.
   *
   * @param p_directory Path of the directory to list files from. Defaults to
   * the current directory.
   *
   * @return A vector containing the names of all files in the specified
   * directory.
   */
  std::vector<std::string> list(const std::string &p_directory = ".") const;

  /**
   * @brief Creates a new file at the specified path.
   *
   * If the file already exists, it does nothing. If the file does not exist,
   * it creates a new empty file and adds the new file's FileHandler to the
   * manager.
   *
   * @param p_path Path of the file to create.
   *
   * @throws std::runtime_error If the file cannot be created.
   */
  void create(const std::string &p_path);

  /**
   * @brief Removes the file at the specified path.
   *
   * If the file does not exist, it does nothing. If the file exists, it
   * deletes the file and removes its FileHandler from the manager if it
   * exists.
   *
   * @param p_path Path of the file to remove.
   *
   * @throws std::runtime_error If the file cannot be deleted.
   */
  void remove(const std::string &p_path);
};
#endif