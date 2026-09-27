#include "DatabaseException.h"

#include <gtest/gtest.h>

#include <string>

TEST(DatabaseExceptionTest, FormatsInvalidKeyError) {
  const InvalidDatabaseKey exception("DU/Chip0");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Invalid database key: DU/Chip0");
}

TEST(DatabaseExceptionTest, FormatsInvalidValueError) {
  const InvalidDatabaseValue exception(
      DatabaseValueError::CHARACTER_VALUE_CONTAINS_WHITESPACE);

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Character database values must not contain whitespace");
}

TEST(DatabaseExceptionTest, FormatsInvalidQueryError) {
  const InvalidDatabaseQuery exception("DU/*");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Invalid database query: DU/*");
}

TEST(DatabaseExceptionTest, FormatsMissingQueryError) {
  const DatabaseQueryNotFound exception("/Missing/Key");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] No database entries match query: /Missing/Key");
}

TEST(DatabaseExceptionTest, FormatsMissingKeyError) {
  const DatabaseKeyNotFound exception("/Missing/Key");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Database key does not exist: /Missing/Key");
}

TEST(DatabaseExceptionTest, FormatsDuplicateKeyError) {
  const DatabaseKeyAlreadyExists exception("/DU/Chip0/Mode");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Database key already exists: /DU/Chip0/Mode");
}

TEST(DatabaseExceptionTest, FormatsTypeMismatchError) {
  const DatabaseValueTypeMismatch exception("/DU/Chip0/Power");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Cannot change database value type: /DU/Chip0/Power");
}

TEST(DatabaseExceptionTest, FormatsConfigurationMismatchError) {
  const DatabaseConfigurationMismatch exception("config.txt", "other.txt");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Database singleton uses configuration path config.txt, "
            "not other.txt");
}

TEST(DatabaseExceptionTest, FormatsParseError) {
  const DatabaseParseError exception(DatabaseParseErrorReason::UNKNOWN_TYPE,
                                     "boolean");

  EXPECT_EQ(std::string(exception.what()),
            "[Database] Unknown database type: boolean");
}
