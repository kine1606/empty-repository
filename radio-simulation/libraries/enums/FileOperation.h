#ifndef FILE_OPERATION_H
#define FILE_OPERATION_H

#include <string_view>

/**
 * @brief Enum class representing file operations.
 */
enum class FileOperation {
  CREATE,
  REMOVE,
  READ,
  WRITE,
  APPEND,
  UPDATE,
  CLEAR,
  OPEN,
  CLOSE
};

/**
 * @brief Map the FileOperation enum value to string text.
 */
constexpr std::string_view displayOperation(FileOperation operation) {
  switch (operation) {
  case FileOperation::CREATE:
    return "create";
  case FileOperation::REMOVE:
    return "remove";
  case FileOperation::READ:
    return "read";
  case FileOperation::WRITE:
    return "write";
  case FileOperation::APPEND:
    return "append";
  case FileOperation::UPDATE:
    return "update";
  case FileOperation::CLEAR:
    return "clear";
  case FileOperation::OPEN:
    return "open";
  case FileOperation::CLOSE:
    return "close";
  default:
    return "operate";
  }
}
#endif