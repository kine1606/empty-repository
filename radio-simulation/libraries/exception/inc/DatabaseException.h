#ifndef DATABASE_EXCEPTION_H
#define DATABASE_EXCEPTION_H

#include "Exception.h"

/**
 * @brief Exception class for handling Database errors.
 */
class DatabaseException : public Exception {
public:
  /**
   * @brief Constructs a DatabaseException with the given error message.
   *
   * @param p_message Error message associated with the database failure.
   */
  explicit DatabaseException(const std::string &p_message);
};

/** Reasons why an in-memory database value can be rejected. */
enum class DatabaseValueError {
  EMPTY_CHARACTER_VALUE,
  CHARACTER_VALUE_CONTAINS_WHITESPACE,
  NON_FINITE_FLOATING_POINT_VALUE,
};

/** Reasons why a database configuration line cannot be parsed. */
enum class DatabaseParseErrorReason {
  INVALID_FORMAT,
  INVALID_KEY,
  DUPLICATE_KEY,
  UNKNOWN_TYPE,
  EMPTY_CHARACTER_VALUE,
  INVALID_F32_VALUE,
  INVALID_S32_VALUE,
  INVALID_U32_VALUE,
  INVALID_VALUE,
  VALUE_OUT_OF_RANGE,
};

/** Exception thrown when a database key is malformed. */
class InvalidDatabaseKey : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for a malformed database key.
   * @param p_key The rejected database key.
   */
  explicit InvalidDatabaseKey(const std::string &p_key);
};

/** Exception thrown when a value cannot be stored in the database. */
class InvalidDatabaseValue : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for an invalid database value.
   * @param p_error The reason why the value was rejected.
   */
  explicit InvalidDatabaseValue(DatabaseValueError p_error);
};

/** Exception thrown when a database query is malformed. */
class InvalidDatabaseQuery : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for a malformed database query.
   * @param p_query The rejected query.
   */
  explicit InvalidDatabaseQuery(const std::string &p_query);
};

/** Exception thrown when no database values match a query. */
class DatabaseQueryNotFound : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for a query with no matching entries.
   * @param p_query The query that produced no matches.
   */
  explicit DatabaseQueryNotFound(const std::string &p_query);
};

/** Exception thrown when a requested database key does not exist. */
class DatabaseKeyNotFound : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for a missing database key.
   * @param p_key The key that was not found.
   */
  explicit DatabaseKeyNotFound(const std::string &p_key);
};

/** Exception thrown when adding a database key that already exists. */
class DatabaseKeyAlreadyExists : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for a duplicate database key.
   * @param p_key The key that already exists.
   */
  explicit DatabaseKeyAlreadyExists(const std::string &p_key);
};

/** Exception thrown when an update attempts to change a value's type. */
class DatabaseValueTypeMismatch : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for an incompatible value type.
   * @param p_key The key whose value type cannot be changed.
   */
  explicit DatabaseValueTypeMismatch(const std::string &p_key);
};

/** Exception thrown when the singleton is requested with a different path. */
class DatabaseConfigurationMismatch : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for conflicting singleton paths.
   * @param p_expectedPath The path used to initialize the singleton.
   * @param p_requestedPath The conflicting requested path.
   */
  DatabaseConfigurationMismatch(const std::string &p_expectedPath,
                                const std::string &p_requestedPath);
};

/** Exception thrown when a database configuration line cannot be parsed. */
class DatabaseParseError : public DatabaseException {
public:
  /**
   * @brief Constructs an exception for invalid configuration content.
   * @param p_reason The reason parsing failed.
   * @param p_value Optional token associated with the failure.
   */
  DatabaseParseError(DatabaseParseErrorReason p_reason,
                     const std::string &p_value = "");
};

#endif
