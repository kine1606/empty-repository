#include "DatabaseException.h"

namespace {

std::string getValueErrorMessage(DatabaseValueError p_error) {
  switch (p_error) {
  case DatabaseValueError::EMPTY_CHARACTER_VALUE:
    return "Character database values must not be empty";
  case DatabaseValueError::CHARACTER_VALUE_CONTAINS_WHITESPACE:
    return "Character database values must not contain whitespace";
  case DatabaseValueError::NON_FINITE_FLOATING_POINT_VALUE:
    return "Floating-point database values must be finite";
  }

  return "Invalid database value";
}

std::string getParseErrorMessage(DatabaseParseErrorReason p_reason,
                                 const std::string &p_value) {
  switch (p_reason) {
  case DatabaseParseErrorReason::INVALID_FORMAT:
    return "Expected format: <key> <type> <value>";
  case DatabaseParseErrorReason::INVALID_KEY:
    return "Invalid database key: " + p_value;
  case DatabaseParseErrorReason::DUPLICATE_KEY:
    return "Duplicate database key: " + p_value;
  case DatabaseParseErrorReason::UNKNOWN_TYPE:
    return "Unknown database type: " + p_value;
  case DatabaseParseErrorReason::EMPTY_CHARACTER_VALUE:
    return "A char value must not be empty";
  case DatabaseParseErrorReason::INVALID_F32_VALUE:
    return "Invalid f32 value: " + p_value;
  case DatabaseParseErrorReason::INVALID_S32_VALUE:
    return "Invalid s32 value: " + p_value;
  case DatabaseParseErrorReason::INVALID_U32_VALUE:
    return "Invalid u32 value: " + p_value;
  case DatabaseParseErrorReason::INVALID_VALUE:
    return "Invalid database value: " + p_value;
  case DatabaseParseErrorReason::VALUE_OUT_OF_RANGE:
    return "Database value is out of range: " + p_value;
  }

  return "Invalid database configuration";
}

} // namespace

DatabaseException::DatabaseException(const std::string &p_message)
    : Exception("[Database] " + p_message) {}

InvalidDatabaseKey::InvalidDatabaseKey(const std::string &p_key)
    : DatabaseException("Invalid database key: " + p_key) {}

InvalidDatabaseValue::InvalidDatabaseValue(DatabaseValueError p_error)
    : DatabaseException(getValueErrorMessage(p_error)) {}

InvalidDatabaseQuery::InvalidDatabaseQuery(const std::string &p_query)
    : DatabaseException("Invalid database query: " + p_query) {}

DatabaseQueryNotFound::DatabaseQueryNotFound(const std::string &p_query)
    : DatabaseException("No database entries match query: " + p_query) {}

DatabaseKeyNotFound::DatabaseKeyNotFound(const std::string &p_key)
    : DatabaseException("Database key does not exist: " + p_key) {}

DatabaseKeyAlreadyExists::DatabaseKeyAlreadyExists(const std::string &p_key)
    : DatabaseException("Database key already exists: " + p_key) {}

DatabaseValueTypeMismatch::DatabaseValueTypeMismatch(const std::string &p_key)
    : DatabaseException("Cannot change database value type: " + p_key) {}

DatabaseConfigurationMismatch::DatabaseConfigurationMismatch(
    const std::string &p_expectedPath, const std::string &p_requestedPath)
    : DatabaseException("Database singleton uses configuration path " +
                        p_expectedPath + ", not " + p_requestedPath) {}

DatabaseParseError::DatabaseParseError(DatabaseParseErrorReason p_reason,
                                       const std::string &p_value)
    : DatabaseException(getParseErrorMessage(p_reason, p_value)) {}
