#include "Database.h"
#include "DatabaseException.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>

class DatabaseTest : public ::testing::Test {
protected:
  std::string m_testDirectory = "DatabaseTestData";
  std::string m_configPath = m_testDirectory + "/config.txt";

  // Replaces the test configuration file with the requested content.
  void writeConfig(const std::string &p_content) {
    std::ofstream configFile(this->m_configPath, std::ios::trunc);
    ASSERT_TRUE(configFile.is_open());
    configFile << p_content;
    ASSERT_TRUE(configFile.good());
  }

  // Reads the complete test configuration file.
  std::string readConfig() const {
    std::ifstream configFile(this->m_configPath);
    EXPECT_TRUE(configFile.is_open());
    return std::string(std::istreambuf_iterator<char>(configFile),
                       std::istreambuf_iterator<char>());
  }

  // Removes a database entry when it exists.
  void removeIfPresent(Database &p_database, const std::string &p_key) {
    try {
      p_database.getValues<dbValue>(p_key);
      p_database.remove(p_key);
    } catch (const DatabaseException &) {
    }
  }

  // Restores the singleton database to the standard test data before each test.
  void SetUp() override {
    std::filesystem::remove_all(this->m_testDirectory);
    std::filesystem::create_directories(this->m_testDirectory);
    this->writeConfig("/DU/Chip0 char Base\n"
                      "/DU/Chip0/Mode char ACTIVE\n"
                      "/DU/Chip0/Power f32 12.5\n"
                      "/DU/Chip1/Offset s32 -25\n"
                      "/RU/Radio0/Frequency u32 3500000000\n");

    Database &database = Database::getInstance(this->m_configPath);
    this->removeIfPresent(database, "/Antenna/Port0/Status");
    this->removeIfPresent(database, "/DUMMY/Chip0/Mode");
    this->removeIfPresent(database, "/DU/Chip0");
    this->removeIfPresent(database, "/DU/Chip0/Mode");
    this->removeIfPresent(database, "/DU/Chip0/Power");
    this->removeIfPresent(database, "/DU/Chip1/Offset");
    this->removeIfPresent(database, "/RU/Radio0/Frequency");

    database.add("/DU/Chip0", dbValue{std::string{"Base"}});
    database.add("/DU/Chip0/Mode", dbValue{std::string{"ACTIVE"}});
    database.add("/DU/Chip0/Power", dbValue{12.5F});
    database.add("/DU/Chip1/Offset", dbValue{std::int32_t{-25}});
    database.add("/RU/Radio0/Frequency", dbValue{std::uint32_t{3500000000U}});
    database.save();
  }

  // Deletes files created by the current test.
  void TearDown() override {
    std::filesystem::remove_all(this->m_testDirectory);
  }
};

// 2.1. Verifies that getValues returns every supported value type.
TEST_F(DatabaseTest, ReturnsAllSupportedTypes) {
  Database &database = Database::getInstance(this->m_configPath);

  EXPECT_EQ(database.getValues<std::string>("/DU/Chip0/Mode").at(0), "ACTIVE");
  EXPECT_FLOAT_EQ(database.getValues<float>("/DU/Chip0/Power").at(0), 12.5F);
  EXPECT_EQ(database.getValues<std::int32_t>("/DU/Chip1/Offset").at(0), -25);
  EXPECT_EQ(database.getValues<std::uint32_t>("/RU/Radio0/Frequency").at(0),
            3500000000U);
}

// 2.2. Verifies that repeated access returns the same singleton instance.
TEST_F(DatabaseTest, ReturnsSameSingletonInstance) {
  Database &first = Database::getInstance(this->m_configPath);
  Database &second = Database::getInstance(this->m_configPath);

  EXPECT_EQ(&first, &second);
}

// 2.3. Verifies that the singleton rejects a different configuration path.
TEST_F(DatabaseTest, RejectsDifferentConfigurationPath) {
  EXPECT_THROW(Database::getInstance(this->m_testDirectory + "/other.txt"),
               DatabaseConfigurationMismatch);
}

// 2.4. Verifies that serialization writes the correct name for every supported
// type.
TEST_F(DatabaseTest, SavesEverySupportedTypeName) {
  Database &database = Database::getInstance(this->m_configPath);

  database.update("/DU/Chip0/Mode", dbValue{std::string{"STANDBY"}});
  database.save();

  EXPECT_EQ(this->readConfig(), "/DU/Chip0 char Base\n"
                                "/DU/Chip0/Mode char STANDBY\n"
                                "/DU/Chip0/Power f32 12.5\n"
                                "/DU/Chip1/Offset s32 -25\n"
                                "/RU/Radio0/Frequency u32 3500000000\n");
}

// 2.5. Verifies that /* returns all values in database key order.
TEST_F(DatabaseTest, ReturnsAllValues) {
  Database &database = Database::getInstance(this->m_configPath);
  const dbValues values = database.getValues<dbValue>("/*");

  EXPECT_EQ(values.size(), 5U);
  EXPECT_EQ(std::get<std::string>(values.at(0)), "Base");
  EXPECT_EQ(std::get<std::string>(values.at(1)), "ACTIVE");
  EXPECT_FLOAT_EQ(std::get<float>(values.at(2)), 12.5F);
  EXPECT_EQ(std::get<std::int32_t>(values.at(3)), -25);
  EXPECT_EQ(std::get<std::uint32_t>(values.at(4)), 3500000000U);
}

// 2.6. Verifies that a query without a wildcard returns only the exact value.
TEST_F(DatabaseTest, GetsExactValue) {
  Database &database = Database::getInstance(this->m_configPath);
  const dbValues values = database.getValues<dbValue>("/DU/Chip0");

  ASSERT_EQ(values.size(), 1U);
  EXPECT_EQ(std::get<std::string>(values.at(0)), "Base");
}

// 2.7. Verifies that a trailing wildcard returns the exact value and descendant
// values.
TEST_F(DatabaseTest, GetsValueAndDescendantValuesUsingTrailingWildcard) {
  Database &database = Database::getInstance(this->m_configPath);
  const dbValues values = database.getValues<dbValue>("/DU/Chip0/*");

  ASSERT_EQ(values.size(), 3U);
  EXPECT_EQ(std::get<std::string>(values.at(0)), "Base");
  EXPECT_EQ(std::get<std::string>(values.at(1)), "ACTIVE");
  EXPECT_FLOAT_EQ(std::get<float>(values.at(2)), 12.5F);
}

// 2.8. Verifies that getValues returns a homogeneous vector containing only the
// requested type.
TEST_F(DatabaseTest, GetsValuesUsingRequestedType) {
  Database &database = Database::getInstance(this->m_configPath);

  const std::vector<std::string> strings =
      database.getValues<std::string>("/DU/Chip0/*");
  const std::vector<float> floats = database.getValues<float>("/DU/Chip0/*");
  const std::vector<std::int32_t> signedIntegers =
      database.getValues<std::int32_t>("/DU/*");
  const std::vector<std::uint32_t> unsignedIntegers =
      database.getValues<std::uint32_t>("/*");

  ASSERT_EQ(strings.size(), 2U);
  EXPECT_EQ(strings.at(0), "Base");
  EXPECT_EQ(strings.at(1), "ACTIVE");
  ASSERT_EQ(floats.size(), 1U);
  EXPECT_FLOAT_EQ(floats.at(0), 12.5F);
  ASSERT_EQ(signedIntegers.size(), 1U);
  EXPECT_EQ(signedIntegers.at(0), -25);
  ASSERT_EQ(unsignedIntegers.size(), 1U);
  EXPECT_EQ(unsignedIntegers.at(0), 3500000000U);
}

// 2.9. Verifies that getValues returns an empty vector when no value has the
// requested type.
TEST_F(DatabaseTest, ReturnsEmptyTypedVectorWhenTypeDoesNotMatch) {
  Database &database = Database::getInstance(this->m_configPath);

  const std::vector<float> values = database.getValues<float>("/DU/Chip0/Mode");

  EXPECT_TRUE(values.empty());
}

// 2.10. Verifies that getValues preserves query validation and missing-query
// errors.
TEST_F(DatabaseTest, RejectsInvalidOrMissingTypedQueries) {
  Database &database = Database::getInstance(this->m_configPath);

  EXPECT_THROW(database.getValues<std::string>("DU/*"), InvalidDatabaseQuery);
  EXPECT_THROW(database.getValues<std::string>("/Missing/Key"),
               DatabaseQueryNotFound);
}

// 2.11. Verifies that a query does not match a different partial path segment.
TEST_F(DatabaseTest, DoesNotMatchPartialPathSegment) {
  Database &database = Database::getInstance(this->m_configPath);
  database.add("/DUMMY/Chip0/Mode", dbValue{std::string{"INACTIVE"}});

  const dbValues values = database.getValues<dbValue>("/DU/*");

  ASSERT_EQ(values.size(), 4U);
  EXPECT_EQ(std::get<std::string>(values.at(0)), "Base");
  EXPECT_EQ(std::get<std::string>(values.at(1)), "ACTIVE");
  EXPECT_FLOAT_EQ(std::get<float>(values.at(2)), 12.5F);
  EXPECT_EQ(std::get<std::int32_t>(values.at(3)), -25);
}

// 2.12. Verifies that malformed and unmatched queries are rejected.
TEST_F(DatabaseTest, RejectsInvalidOrMissingQueries) {
  Database &database = Database::getInstance(this->m_configPath);

  EXPECT_THROW(database.getValues<dbValue>("DU/*"), InvalidDatabaseQuery);
  EXPECT_THROW(database.getValues<dbValue>("/DU/*/Mode"), InvalidDatabaseQuery);
  EXPECT_THROW(database.getValues<dbValue>("/Antenna"), DatabaseQueryNotFound);
  EXPECT_THROW(database.getValues<dbValue>("/Missing/Key"),
               DatabaseQueryNotFound);
}

// 2.13. Verifies that add changes memory without immediately modifying the
// file.
TEST_F(DatabaseTest, AddsEntryOnlyInMemoryUntilSave) {
  Database &database = Database::getInstance(this->m_configPath);
  const std::string originalContent = this->readConfig();

  database.add("/Antenna/Port0/Status", dbValue{std::string{"ENABLED"}});

  EXPECT_EQ(database.getValues<std::string>("/Antenna/Port0/Status").at(0),
            "ENABLED");
  EXPECT_EQ(this->readConfig(), originalContent);
}

// 2.14. Verifies that add rejects duplicate keys and invalid key-value data.
TEST_F(DatabaseTest, RejectsDuplicateAndInvalidEntries) {
  Database &database = Database::getInstance(this->m_configPath);

  EXPECT_THROW(database.add("/DU/Chip0/Mode", dbValue{std::string{"STANDBY"}}),
               DatabaseKeyAlreadyExists);
  EXPECT_THROW(database.add("DU/Chip2/Mode", dbValue{std::string{"ACTIVE"}}),
               InvalidDatabaseKey);
  EXPECT_THROW(database.add("/DU//Chip2/Mode", dbValue{std::string{"ACTIVE"}}),
               InvalidDatabaseKey);
  EXPECT_THROW(database.add("/DU/Chip2/", dbValue{std::string{"ACTIVE"}}),
               InvalidDatabaseKey);
  EXPECT_THROW(database.add("/DU/Chip 2/Mode", dbValue{std::string{"ACTIVE"}}),
               InvalidDatabaseKey);
  EXPECT_THROW(database.add("/DU/Chip*/Mode", dbValue{std::string{"ACTIVE"}}),
               InvalidDatabaseKey);
  EXPECT_THROW(database.add("/DU/Chip2/Mode", dbValue{std::string{}}),
               InvalidDatabaseValue);
  EXPECT_THROW(
      database.add("/DU/Chip2/Mode", dbValue{std::string{"NOT ACTIVE"}}),
      InvalidDatabaseValue);
  EXPECT_THROW(database.add("/DU/Chip2/Power",
                            dbValue{std::numeric_limits<float>::infinity()}),
               InvalidDatabaseValue);
}

// 2.15. Verifies that update changes memory without immediately modifying the
// file.
TEST_F(DatabaseTest, UpdatesExistingEntryOnlyInMemory) {
  Database &database = Database::getInstance(this->m_configPath);
  const std::string originalContent = this->readConfig();

  database.update("/DU/Chip0/Power", dbValue{18.25F});

  EXPECT_FLOAT_EQ(database.getValues<float>("/DU/Chip0/Power").at(0), 18.25F);
  EXPECT_EQ(this->readConfig(), originalContent);
}

// 2.16. Verifies that update rejects missing keys, invalid keys, and type
// changes.
TEST_F(DatabaseTest, RejectsInvalidUpdates) {
  Database &database = Database::getInstance(this->m_configPath);

  EXPECT_THROW(database.update("/Missing/Key", dbValue{std::string{"ACTIVE"}}),
               DatabaseKeyNotFound);
  EXPECT_THROW(database.update("/DU/Chip0/Power", dbValue{std::uint32_t{5}}),
               DatabaseValueTypeMismatch);
  EXPECT_THROW(database.update("DU/Chip0/Power", dbValue{1.0F}),
               InvalidDatabaseKey);
}

// 2.17. Verifies that remove changes memory without immediately modifying the
// file.
TEST_F(DatabaseTest, RemovesEntryOnlyInMemory) {
  Database &database = Database::getInstance(this->m_configPath);
  const std::string originalContent = this->readConfig();

  database.remove("/DU/Chip1/Offset");

  EXPECT_THROW(database.getValues<dbValue>("/DU/Chip1/Offset"),
               DatabaseQueryNotFound);
  EXPECT_EQ(this->readConfig(), originalContent);
}

// 2.18. Verifies that remove rejects missing and malformed keys.
TEST_F(DatabaseTest, RejectsInvalidRemovals) {
  Database &database = Database::getInstance(this->m_configPath);

  EXPECT_THROW(database.remove("/Missing/Key"), DatabaseKeyNotFound);
  EXPECT_THROW(database.remove("DU/Chip0/Mode"), InvalidDatabaseKey);
}

// 2.19. Verifies that save persists all in-memory changes.
TEST_F(DatabaseTest, SavesChanges) {
  Database &database = Database::getInstance(this->m_configPath);

  database.add("/Antenna/Port0/Status", dbValue{std::string{"ENABLED"}});
  database.update("/DU/Chip0/Power", dbValue{18.25F});
  database.remove("/DU/Chip1/Offset");
  database.save();

  EXPECT_EQ(this->readConfig(), "/Antenna/Port0/Status char ENABLED\n"
                                "/DU/Chip0 char Base\n"
                                "/DU/Chip0/Mode char ACTIVE\n"
                                "/DU/Chip0/Power f32 18.25\n"
                                "/RU/Radio0/Frequency u32 3500000000\n");
}
