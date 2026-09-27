#ifndef DATABASE_H
#define DATABASE_H
#include <cstdint>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

class FileHandler;

/** Value types supported by the database configuration format. */
enum class DbType {
  CHAR,
  F32,
  S32,
  U32,
};

/** A value supported by the database configuration format. */
using dbValue = std::variant<std::string, float, std::int32_t, std::uint32_t>;
/** A collection of database values returned by a query. */
using dbValues = std::vector<dbValue>;
/** A collection of database entries indexed by their hierarchical key. */
using dbEntries = std::map<std::string, dbValue>;

/**
 * @brief Thread-safe, file-backed key-value database.
 *
 * Changes made through add(), update(), and remove() are kept in memory until
 * save() is called. The configuration file uses one entry per line in the form
 * `<key> <type> <value>`.
 */
class Database {
private:
  /**
   * @brief Creates a database and loads its configuration file.
   *
   * @param p_configPath Path to the configuration file.
   * @throws DatabaseParseError If the configuration contents are invalid.
   */
  explicit Database(const std::string &p_configPath);
  /**
   * @brief Loads and parses the configuration file into memory.
   *
   * @throws DatabaseParseError If the configuration contents are invalid.
   */
  void load();

  /** Path to the configuration file backing this database. */
  std::string m_configPath;

  /** File handler used to read and write the configuration file. */
  std::shared_ptr<FileHandler> m_fileHandler;

  /** Entries currently held in memory. */
  dbEntries m_entries;

  /** Whether the in-memory entries contain changes not yet saved to disk. */
  bool m_isChanged;

  /** Synchronizes access to entries and the changed state. */
  mutable std::shared_mutex m_mutex;

  /**
   * @brief Returns values matching an exact or wildcard query.
   *
   * An exact query returns the value associated with one key.
   * A wildcard query returns the value associated with the requested path,
   * if present, together with all descendant values. A root wildcard returns
   * every value in the database.
   *
   * @param p_query Exact key or trailing-wildcard query.
   * @return Matching values in database key order.
   * @throws InvalidDatabaseQuery If the query format is invalid.
   * @throws DatabaseQueryNotFound If no values match the query.
   */
  dbValues get(const std::string &p_query) const;

public:
  /**
   * @brief Returns the database singleton associated with a configuration file.
   *
   * The first call creates the singleton using @p p_configPath. Later calls
   * must provide the same path.
   *
   * @param p_configPath Path used to initialize or access the singleton.
   * @return Reference to the database singleton.
   * @throws DatabaseConfigurationMismatch If the singleton was initialized
   * with a different path.
   * @throws DatabaseParseError If the configuration contents are invalid.
   */
  static Database &getInstance(const std::string &p_configPath);
  /** Copy construction is disabled because the database owns synchronized
   * state. */
  Database(const Database &) = delete;
  /** Copy assignment is disabled because the database owns synchronized state.
   */
  Database &operator=(const Database &) = delete;
  /** Move construction is disabled because Database owns synchronized state. */
  Database(Database &&) = delete;
  /** Move assignment is disabled because Database owns synchronized state. */
  Database &operator=(Database &&) = delete;

  /**
   * @brief Returns matching values as the requested type @p T.
   *
   * An exact query examines one key. A trailing wildcard includes the value
   * at the requested path, if present, and all descendant values. A root
   * wildcard examines every database value. Requesting dbValue returns all
   * matching values, while requesting a concrete type returns only matching
   * values stored with that type.
   *
   * @tparam T dbValue, std::string, float, std::int32_t, or std::uint32_t.
   * @param p_query Exact key or trailing-wildcard query.
   * @return Matching values in database key order. For concrete types, the
   * vector is empty when no matching entry has the requested type.
   * @throws InvalidDatabaseQuery If the query format is invalid.
   * @throws DatabaseQueryNotFound If no database entries match the query.
   */
  template <typename T>
  std::vector<T> getValues(const std::string &p_query) const {
    static_assert(
        std::is_same_v<T, dbValue> || std::is_same_v<T, std::string> ||
            std::is_same_v<T, float> || std::is_same_v<T, std::int32_t> ||
            std::is_same_v<T, std::uint32_t>,
        "T must be dbValue or one of its supported value types");

    const dbValues values = this->get(p_query);

    if constexpr (std::is_same_v<T, dbValue>) {
      return values;
    } else {
      std::vector<T> typedValues;
      typedValues.reserve(values.size());

      for (const dbValue &value : values) {
        const T *typedValue = std::get_if<T>(&value);

        if (typedValue != nullptr) {
          typedValues.push_back(*typedValue);
        }
      }

      return typedValues;
    }
  }

  /**
   * @brief Adds a new entry in memory.
   *
   * @param p_key Absolute key that does not already exist.
   * @param p_value Value to store for the key.
   * @throws InvalidDatabaseKey If the key format is invalid.
   * @throws InvalidDatabaseValue If the value cannot be stored.
   * @throws DatabaseKeyAlreadyExists If the key already exists.
   */
  void add(const std::string &p_key, const dbValue &p_value);

  /**
   * @brief Updates an existing entry in memory without changing its type.
   *
   * @param p_key Absolute key of the entry to update.
   * @param p_value New value with the same type as the existing value.
   * @throws InvalidDatabaseKey If the key format is invalid.
   * @throws InvalidDatabaseValue If the value cannot be stored.
   * @throws DatabaseKeyNotFound If the key does not exist.
   * @throws DatabaseValueTypeMismatch If the new value has a different type.
   */
  void update(const std::string &p_key, const dbValue &p_value);

  /**
   * @brief Removes an entry from memory.
   *
   * @param p_key Absolute key of the entry to remove.
   * @throws InvalidDatabaseKey If the key format is invalid.
   * @throws DatabaseKeyNotFound If the key does not exist.
   */
  void remove(const std::string &p_key);

  /**
   * @brief Persists in-memory changes to the configuration file.
   *
   * Does nothing when there are no unsaved changes.
   * @throws std::runtime_error If the configuration file cannot be written.
   */
  void save();
};

#endif
