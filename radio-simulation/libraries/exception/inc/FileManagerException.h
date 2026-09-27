#ifndef FILE_MANAGER_EXCEPTION_H
#define FILE_MANAGER_EXCEPTION_H

#include "Exception.h"
#include "FileOperation.h"

/**
 * @brief Exception class for handling FileManager's errors.
 */
class FileManagerException : public Exception {
public:
  /**
   * @brief Constructs a FileManagerException with a given error message.
   * @param p_message The error message to be associated with the exception.
   */
  explicit FileManagerException(const std::string &p_message);
};

/**
 * @brief Exception class for handling file not found errors.
 */
class FileNotFound : public FileManagerException {
public:
  /**
   * @brief Constructs a FileNotFound exception with a given file path.
   * @param p_path The path of the file that was not found.
   */
  explicit FileNotFound(const std::string &p_path);
};

/**
 * @brief Exception class for handling file already exists errors.
 */
class FileAlreadyExists : public FileManagerException {
public:
  /**
   * @brief Constructs a FileAlreadyExists exception with a given file path.
   * @param p_path The path of the file that already exists.
   */
  explicit FileAlreadyExists(const std::string &p_path);
};

/**
 * @brief Exception class for handling file operation failed errors.
 */
class FileOperationFailed : public FileManagerException {
public:
  /**
   * @brief Constructs a FileOperationFailed exception with a given file path
   * and operation.
   * @param p_path The path of the file on which the operation failed.
   * @param p_operation The file operation that failed.
   */
  explicit FileOperationFailed(const std::string &p_path,
                               const FileOperation p_operation);
};

#endif