#include "Database.h"
#include "DatabaseException.h"
#include "FileManager.h"

#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <mutex>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

// Checks whether a key is an absolute, wildcard-free database path.
bool isValidDatabaseKey(const std::string &p_key) {
  static const std::regex DATABASE_KEY_PATTERN(R"(^(/|(/[^/\s*]+)+)$)");
  return std::regex_match(p_key, DATABASE_KEY_PATTERN);
}

// Rejects malformed database keys.
void validateKey(const std::string &p_key) {
  if (!isValidDatabaseKey(p_key)) {
    throw InvalidDatabaseKey(p_key);
  }
}

// Rejects values that cannot be represented in the configuration format.
void validateValue(const dbValue &p_value) {
  if (std::holds_alternative<std::string>(p_value)) {
    const std::string &value = std::get<std::string>(p_value);

    if (value.empty()) {
      throw InvalidDatabaseValue(DatabaseValueError::EMPTY_CHARACTER_VALUE);
    }

    for (const char character : value) {
      if (std::isspace(static_cast<unsigned char>(character))) {
        throw InvalidDatabaseValue(
            DatabaseValueError::CHARACTER_VALUE_CONTAINS_WHITESPACE);
      }
    }
  }

  if (std::holds_alternative<float>(p_value)) {
    const float value = std::get<float>(p_value);

    if (!std::isfinite(value)) {
      throw InvalidDatabaseValue(
          DatabaseValueError::NON_FINITE_FLOATING_POINT_VALUE);
    }
  }
}

// Converts a configuration type name into its database type.
DbType parseType(const std::string &p_type) {
  if (p_type == "char") {
    return DbType::CHAR;
  }

  if (p_type == "f32") {
    return DbType::F32;
  }

  if (p_type == "s32") {
    return DbType::S32;
  }

  if (p_type == "u32") {
    return DbType::U32;
  }

  throw DatabaseParseError(DatabaseParseErrorReason::UNKNOWN_TYPE, p_type);
}

// Parses and validates one textual value according to its declared type.
dbValue parseValue(DbType p_type, const std::string &p_text) {
  try {
    std::size_t parsedLength = 0;

    if (p_type == DbType::CHAR) {
      if (p_text.empty()) {
        throw DatabaseParseError(
            DatabaseParseErrorReason::EMPTY_CHARACTER_VALUE);
      }

      return p_text;
    }

    if (p_type == DbType::F32) {
      const float value = std::stof(p_text, &parsedLength);

      if (parsedLength != p_text.size() || !std::isfinite(value)) {
        throw DatabaseParseError(DatabaseParseErrorReason::INVALID_F32_VALUE,
                                 p_text);
      }

      return value;
    }

    if (p_type == DbType::S32) {
      const long long value = std::stoll(p_text, &parsedLength);

      if (parsedLength != p_text.size() ||
          value < std::numeric_limits<std::int32_t>::min() ||
          value > std::numeric_limits<std::int32_t>::max()) {
        throw DatabaseParseError(DatabaseParseErrorReason::INVALID_S32_VALUE,
                                 p_text);
      }

      return static_cast<std::int32_t>(value);
    }

    if (p_text.empty() || p_text[0] == '-') {
      throw DatabaseParseError(DatabaseParseErrorReason::INVALID_U32_VALUE,
                               p_text);
    }

    const unsigned long long value = std::stoull(p_text, &parsedLength);

    if (parsedLength != p_text.size() ||
        value > std::numeric_limits<std::uint32_t>::max()) {
      throw DatabaseParseError(DatabaseParseErrorReason::INVALID_U32_VALUE,
                               p_text);
    }

    return static_cast<std::uint32_t>(value);
  } catch (const std::invalid_argument &) {
    throw DatabaseParseError(DatabaseParseErrorReason::INVALID_VALUE, p_text);
  } catch (const std::out_of_range &) {
    throw DatabaseParseError(DatabaseParseErrorReason::VALUE_OUT_OF_RANGE,
                             p_text);
  }
}

// Validates a query and removes its trailing wildcard when present.
std::string normalizeQuery(const std::string &p_query) {
  if (p_query == "/*") {
    return "/";
  }

  const std::size_t wildcardPosition = p_query.find('*');

  if (wildcardPosition != std::string::npos) {
    if (wildcardPosition != p_query.size() - 1 || wildcardPosition == 0 ||
        p_query[wildcardPosition - 1] != '/') {
      throw InvalidDatabaseQuery(p_query);
    }

    const std::string path = p_query.substr(0, p_query.size() - 2);

    if (!isValidDatabaseKey(path)) {
      throw InvalidDatabaseQuery(p_query);
    }

    return path;
  }

  if (!isValidDatabaseKey(p_query)) {
    throw InvalidDatabaseQuery(p_query);
  }

  return p_query;
}

// Checks whether a key is equal to or below a queried path.
bool isSameOrDescendant(const std::string &p_key, const std::string &p_path) {
  if (p_path == "/") {
    return true;
  }

  if (p_key == p_path) {
    return true;
  }

  return p_key.size() > p_path.size() &&
         p_key.compare(0, p_path.size(), p_path) == 0 &&
         p_key[p_path.size()] == '/';
}

// Returns the database type represented by a variant value.
DbType getType(const dbValue &p_value) {
  if (std::holds_alternative<std::string>(p_value)) {
    return DbType::CHAR;
  }

  if (std::holds_alternative<float>(p_value)) {
    return DbType::F32;
  }

  if (std::holds_alternative<std::int32_t>(p_value)) {
    return DbType::S32;
  }

  return DbType::U32;
}
// Returns the configuration-file name of a database value type.
std::string getTypeName(const dbValue &p_value) {
  const DbType type = getType(p_value);

  if (type == DbType::CHAR) {
    return "char";
  }

  if (type == DbType::F32) {
    return "f32";
  }

  if (type == DbType::S32) {
    return "s32";
  }

  return "u32";
}

// Parses configuration lines into validated typed database entries.
dbEntries parse(const std::string &p_content) {
  dbEntries entries;
  std::istringstream input(p_content);
  std::string line;

  while (std::getline(input, line)) {
    if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
      continue;
    }

    std::istringstream lineInput(line);
    std::string key;
    std::string typeText;
    std::string valueText;
    std::string extraText;

    if (!(lineInput >> key >> typeText >> valueText) ||
        lineInput >> extraText) {
      throw DatabaseParseError(DatabaseParseErrorReason::INVALID_FORMAT);
    }

    if (!isValidDatabaseKey(key)) {
      throw DatabaseParseError(DatabaseParseErrorReason::INVALID_KEY, key);
    }

    const DbType type = parseType(typeText);
    const dbValue value = parseValue(type, valueText);

    const auto it = entries.find(key);

    if (it != entries.end()) {
      throw DatabaseParseError(DatabaseParseErrorReason::DUPLICATE_KEY, key);
    }

    entries.emplace(key, value);
  }

  return entries;
}
// Serializes typed entries using the database configuration format.
std::string serialize(const dbEntries &p_entries) {
  std::ostringstream output;

  output << std::setprecision(std::numeric_limits<float>::max_digits10);

  for (const auto &[key, value] : p_entries) {
    output << key << " " << getTypeName(value) << " ";

    std::visit([&output](const auto &p_value) { output << p_value; }, value);

    output << "\n";
  }

  return output.str();
}

} // namespace

// Loads the configuration file when a database instance is created.
Database::Database(const std::string &p_configPath)
    : m_configPath(p_configPath),
      m_fileHandler(FileManager::getInstance().get(p_configPath)),
      m_isChanged(false) {
  this->load();
}
// Returns the singleton database initialized with the first configuration path.
Database &Database::getInstance(const std::string &p_configPath) {
  static Database instance(p_configPath);

  if (instance.m_configPath != p_configPath) {
    throw DatabaseConfigurationMismatch(instance.m_configPath, p_configPath);
  }

  return instance;
}
// Returns values matching an exact key or trailing-wildcard query.
dbValues Database::get(const std::string &p_query) const {
  const bool isWildcardQuery =
      p_query == "/*" || (p_query.size() >= 2 &&
                          p_query.compare(p_query.size() - 2, 2, "/*") == 0);

  const std::string path = normalizeQuery(p_query);

  std::shared_lock<std::shared_mutex> lock(this->m_mutex);
  dbValues matchedValues;

  if (!isWildcardQuery) {
    const auto it = this->m_entries.find(path);

    if (it != this->m_entries.end()) {
      matchedValues.push_back(it->second);
    }
  } else {
    for (const auto &[key, value] : this->m_entries) {
      if (isSameOrDescendant(key, path)) {
        matchedValues.push_back(value);
      }
    }
  }

  if (matchedValues.empty()) {
    throw DatabaseQueryNotFound(p_query);
  }

  return matchedValues;
}

// Adds a new entry to memory and marks the database as modified.
void Database::add(const std::string &p_key, const dbValue &p_value) {
  validateKey(p_key);
  validateValue(p_value);

  std::unique_lock<std::shared_mutex> lock(this->m_mutex);
  const auto it = this->m_entries.find(p_key);

  if (it != this->m_entries.end()) {
    throw DatabaseKeyAlreadyExists(p_key);
  }

  this->m_entries.emplace(p_key, p_value);
  this->m_isChanged = true;
}

// Updates an existing entry without allowing its value type to change.
void Database::update(const std::string &p_key, const dbValue &p_value) {
  validateKey(p_key);
  validateValue(p_value);

  std::unique_lock<std::shared_mutex> lock(this->m_mutex);
  const auto it = this->m_entries.find(p_key);

  if (it == this->m_entries.end()) {
    throw DatabaseKeyNotFound(p_key);
  }

  if (it->second.index() != p_value.index()) {
    throw DatabaseValueTypeMismatch(p_key);
  }

  if (it->second != p_value) {
    it->second = p_value;
    this->m_isChanged = true;
  }
}

// Removes an exact key from memory and marks the database as modified.
void Database::remove(const std::string &p_key) {
  validateKey(p_key);

  std::unique_lock<std::shared_mutex> lock(this->m_mutex);

  if (this->m_entries.erase(p_key) == 0) {
    throw DatabaseKeyNotFound(p_key);
  }

  this->m_isChanged = true;
}

// Persists modified entries through FileHandler and clears the dirty flag.
void Database::save() {
  std::unique_lock<std::shared_mutex> lock(this->m_mutex);

  if (!this->m_isChanged) {
    return;
  }

  const std::string content = serialize(this->m_entries);

  this->m_fileHandler->write(content);
  this->m_isChanged = false;
}

// Reads and parses the configuration file into the in-memory database.
void Database::load() {
  const std::string content = this->m_fileHandler->read();
  const dbEntries entries = parse(content);

  std::unique_lock<std::shared_mutex> lock(this->m_mutex);
  this->m_entries = entries;
  this->m_isChanged = false;
}
