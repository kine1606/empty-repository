#ifndef LOGGER_H
#define LOGGER_H

#include "FileHandler.h"
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static constexpr unsigned long long MAX_FILE_SIZE = 10 * 1024 * 1024;
static constexpr std::size_t MAX_PENDING_ENTRIES = 4096;
static constexpr std::size_t MAX_ENTRY_BYTES = 16 * 1024;
static constexpr std::size_t MAX_FILE_BATCH_BYTES = 64 * 1024;

#define TRACE(...) Logger::getInstance().log((LogLevel::TRACE), __FILE__, __LINE__, __VA_ARGS__)
#define DEBUG(...) Logger::getInstance().log((LogLevel::DEBUG), __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...) Logger::getInstance().log((LogLevel::INFO), __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...) Logger::getInstance().log((LogLevel::WARN), __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(...) Logger::getInstance().log((LogLevel::ERROR), __FILE__, __LINE__, __VA_ARGS__)
#define FATAL(...) Logger::getInstance().log((LogLevel::FATAL), __FILE__, __LINE__, __VA_ARGS__)

enum class LogLevel : std::uint8_t
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    INVALID
};

/**
 * @class Logger
 * @brief Receive log calls and process them.
 *
 * The logger only permits one object per module instance in this project.
 * It provides operations for creating the log entry, then print it out into our
 * selected output (console log and/or log file). Multiple threads can call
 * logger simultaneously.
 */
class Logger
{
  private:
    Logger() = delete;
    Logger(const std::string& p_moduleName, bool p_enableConsoleLogging = true,
           const std::string& p_outputFilePath = "", bool p_enableFileLogging = false,
           LogLevel p_minimumLogLevel = LogLevel::INFO, bool p_asyncEnabled = false);

    friend struct std::default_delete<Logger>;
    ~Logger() noexcept;

    inline static std::unique_ptr<Logger> m_instance = nullptr;

    std::string m_moduleName;

    std::string m_fileBuffer;

    std::mutex m_fileOutputMutex;
    std::mutex m_consoleOutputMutex;
    std::mutex m_enabledLevelsMutex;
    inline static std::mutex m_instanceMutex;

    bool m_asyncEnabled = false;
    std::thread m_workerThread;
    // To stop the worker after all processing calls are enqueued
    bool m_stopping = false;
    // To reject new log calls
    bool m_closing = false;
    std::atomic<std::uint64_t> m_outputFailures{0};
    std::mutex m_lifecycleMutex;
    std::condition_variable m_lifecycleCondition;

    std::uint64_t m_activeEntries = 0;

    std::queue<std::string> m_logQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    std::atomic<std::uint64_t> m_droppedEntries{0};
    bool m_workerBusy = false;
    std::condition_variable m_flushCondition;

    // To manage and ensure accepted log calls are always processed, not dropped
    class ActiveLogGuard
    {
      public:
        explicit ActiveLogGuard(Logger& p_logger) : m_logger(p_logger)
        {
            std::lock_guard<std::mutex> lock(this->m_logger.m_lifecycleMutex);

            if (!this->m_logger.m_closing)
            {
                ++this->m_logger.m_activeEntries;
                this->m_accepted = true;
            }
        }

        ~ActiveLogGuard()
        {
            if (!this->m_accepted)
            {
                return;
            }

            std::lock_guard<std::mutex> lock(this->m_logger.m_lifecycleMutex);

            if (--this->m_logger.m_activeEntries == 0)
            {
                this->m_logger.m_lifecycleCondition.notify_all();
            }
        }

        bool accepted() const noexcept
        {
            return this->m_accepted;
        }

        ActiveLogGuard(const ActiveLogGuard&) = delete;
        ActiveLogGuard& operator=(const ActiveLogGuard&) = delete;

      private:
        Logger& m_logger;
        bool m_accepted = false;
    };

    LogLevel m_minimumLogLevel{LogLevel::INFO};

    /**
     * @brief Base path supplied for file logging.
     *
     * Example: logs/du.log
     */
    std::string m_outputFilePath;

    /**
     * @brief Timestamped prefix identifying the current logging session.
     *
     * Example: logs/20260914T113000_du
     */
    std::string m_sessionLogPrefix;

    /**
     * @brief Index of the current log file within this session.
     *
     * Produces:
     *   <prefix>.0.log
     *   <prefix>.1.log
     *   <prefix>.2.log
     */
    std::size_t m_logFileIndex = 0;

    std::shared_ptr<FileHandler> m_currentLogFile;

    /**
     * @brief Enable console log output.
     */
    std::atomic<bool> m_consoleLogOutput{true};

    /**
     * @brief Enable log file output.
     */
    std::atomic<bool> m_fileLogOutput{false};

    /**
     * @brief The enable status of each level.
     */
    std::array<bool, static_cast<std::size_t>(LogLevel::INVALID)> m_enabledLevels{false, false, true, true, true, true};

    /**
     * @brief Convert a value to a string using stream insertion (operator<<).
     *
     * @param p_value Value supporting insertion into std::ostringstream.
     * @return String representation of the value.
     */
    template <typename T> std::string serialize(const T& p_value);

    /**
     * @brief Recursively serialize vector elements, separated by single spaces.
     *
     * @param p_value Vector of values to serialize.
     * @return Space-separated string, or an empty string for an empty vector.
     */
    template <typename T> std::string serialize(const std::vector<T>& p_value);

    /**
     * @brief Replace numbered placeholders (%1, %2, ...) with serialized
     * arguments.
     *
     * %% produces a literal %. Invalid or out-of-range placeholders remain
     * unchanged.
     *
     * @param p_format Message format string.
     * @param p_arguments Serialized arguments, indexed starting at %1.
     * @return Formatted message.
     */
    std::string formatMessage(const std::string& p_format, const std::vector<std::string>& p_arguments);

    /**
     * @brief Internal function to process a log invocation.
     *
     * @param p_logLevel The log entry's level.
     * @param p_file The source file where the log was invoked.
     * @param p_line The source line where the log was invoked.
     * @param p_message The log entry's message.
     */
    void processLog(LogLevel p_logLevel, const char* p_file, int p_line, const std::string& p_message);

    /**
     * @brief Internal function to retrieve a standard timestamp string.
     */
    std::string formatTimestamp() const;

    /**
     * @brief Internal function to process log file output and rotation.
     *
     * @param p_message The constructed log entry.
     */
    void appendLogToFile(const std::string& p_message);

    /**
     * @brief Get the existing log file, or create a new file if none exists.
     *
     * @return Handler for the newly created file.
     * @throws Propagates FileManager creation or retrieval exceptions,
     *         including failure when the file already exists.
     */
    std::shared_ptr<FileHandler> createLogFile(const std::string& p_path);

    /**
     * @brief Internal function to rotate log files.
     */
    void rotateLogFile();

    /**
     * @brief Generate the timestamped prefix for a new logging session.
     *
     * @param p_outputFilePath Base output path.
     * @return Timestamped session prefix.
     */
    std::string createSessionLogPrefix(const std::string& p_outputFilePath) const;

    /**
     * @brief Generate the session's log file fresh.
     *
     * @param p_outputFilePath Base output path.
     * @return FileHandler of the created log file.
     */
    std::shared_ptr<FileHandler> createSessionLogFile(const std::string& p_outputFilePath);

    /**
     * @brief Build a log file path for the specified file index.
     *
     * @param p_index Log file index within the current session.
     * @return Log file path.
     */
    std::string getLogFilePath(std::size_t p_index) const;

    /**
     * @brief Internal function to update log file's path.
     */
    void updateLogFilePath(const std::string& p_outputFilePath);

    /**
     * @brief Internal function to write the log entry.
     */
    void writeLogEntry(const std::string& p_entry);

    /**
     * @brief Internal function to buffer and flush file log output.
     */
    void bufferAndFlushFileEntries(const std::string& p_entry);

    /**
     * @brief Internal function to flush file log output
     */
    void flushFileBufferUnlocked();

    /**
     * @brief Queue an owned entry for the logging worker.
     */
    void enqueueLogEntry(std::string p_entry);

    /**
     * @brief Process queued entries until stopping is requested
     *        and the queue is empty.
     */
    void workerLoop();

    /**
     * @brief Stop accepting queued entries and wake the worker.
     * The worker processes remaining entries before returning.
     * The caller must join its worker before destroying the logger.
     */
    void requestStop();

    /**
     * @brief Reject new log calls and wait for accepted calls to finish.
     *
     * Sets the closing flag and waits until the active-call counter reaches zero.
     * Releases the lifecycle mutex while waiting so active calls can complete.
     *
     * @note Does not drain the asynchronous queue or flush output buffers.
     * @pre The logger must remain alive throughout this operation.
     * @pre Must not be called from within an active log call.
     */
    void closeAndWaitForActiveLogs();

  public:
    static void initialize(const std::string& p_moduleName, bool p_enableConsoleLogging = true,
                           const std::string& p_outputFilePath = "", bool p_logFileOutput = false,
                           LogLevel p_minimumLogLevel = LogLevel::INFO, bool p_asyncEnabled = false);

    static Logger& getInstance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Create a log entry with the supplied parameters as contents of the
     * log message.
     *
     * Parameters are serialized into a continuous string and printed to the
     * configured output of the Logger.
     *
     * @param p_logLevel The severity of the log entry.
     * @param p_file The source file where the log was invoked.
     * @param p_line The source line where the log was invoked.
     * @param p_format The log message format.
     * @param p_args Values substituted into the format string.
     */
    template <typename... Args>
    void log(LogLevel p_logLevel, const char* p_file, int p_line, const std::string& p_format, const Args&... p_args);

    /**
     * @brief Get the number of entries rejected because logging is stopping,
     *        the queue is full, or the entry exceeds the size limit.
     * @return Dropped entry count.
     */
    std::uint64_t droppedEntries() const noexcept
    {
        return this->m_droppedEntries.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get the number of failed console or file output attempts.
     * @return Output failure count. One entry can produce two failures.
     */
    std::uint64_t outputFailures() const noexcept
    {
        return this->m_outputFailures.load(std::memory_order_relaxed);
    }

    /**
     * @brief Waits for queued entries to finish processing, then flushes the file
     * buffer.
     *
     * @pre All producer log calls must have completed, and no new log calls may
     * begin until this function returns.
     */
    void flush();

    /**
     * @brief Waits until the queue is empty and the worker has finished
     * processing its current entry.
     *
     * Does not flush the file buffer.
     *
     * @pre All producer log calls must have completed, and no new log calls may
     * begin until this function returns.
     */
    void waitUntilIdle();

    /**
     * @brief Set the minimum log level to record.
     *
     * @param p_minimumLevel The lowest log level to record.
     */
    void setMinimumLogLevel(LogLevel p_minimumLevel);

    /**
     * @brief Enable a specific log level.
     *
     * @param p_logLevel The log level to enable.
     */
    void enableLogLevel(LogLevel p_logLevel);

    /**
     * @brief Restore enabled log levels according to the configured minimum
     * level.
     */
    void resetLogLevels();

    /**
     * @brief Change the log file path.
     *
     * @param p_outputFilePath The output log file path.
     */
    void changeLogFilePath(const std::string& p_outputFilePath);

    /**
     * @brief Change the state of console log output.
     *
     * @param p_enabled The desired state of console log output.
     */
    void setConsoleLogOutput(bool p_enabled);

    /**
     * @brief Change the state of file log output.
     *
     * @param p_enabled The desired state of file log output.
     * @param p_outputFilePath Optional output file path.
     */
    void setFileLogOutput(bool p_enabled, const std::string& p_outputFilePath = "");
};

#include "Logger.tpp"

#endif