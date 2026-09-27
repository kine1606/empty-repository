#include "FileManagerException.h"

FileManagerException::FileManagerException(const std::string &p_message)
    : Exception("[FileManager] " + p_message) {}

FileNotFound::FileNotFound(const std::string &p_path)
    : FileManagerException("File not found: " + p_path) {}

FileAlreadyExists::FileAlreadyExists(const std::string &p_path)
    : FileManagerException("File already exists: " + p_path) {}

FileOperationFailed::FileOperationFailed(const std::string &p_path,
                                         const FileOperation p_operation)
    : FileManagerException("Failed to " +
                           std::string(displayOperation(p_operation)) +
                           " file: " + p_path) {}