#include "Logger.h"
#include "FileManager.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <system_error>
#include <utility>

Logger::Logger(const std::string& p_moduleName, bool p_consoleLogOutput, const std::string& p_outputFilePath,
               bool p_fileLogOutput, LogLevel p_minimumLogLevel, bool p_asyncEnabled)
    : m_moduleName(p_moduleName), m_asyncEnabled(p_asyncEnabled), m_consoleLogOutput(p_consoleLogOutput)
{

    setMinimumLogLevel(p_minimumLogLevel);

    if (p_fileLogOutput)
    {
        setFileLogOutput(true, p_outputFilePath);
    }
    if (this->m_asyncEnabled)
    {
        this->m_workerThread = std::thread(&Logger::workerLoop, this);
    }
}

Logger::~Logger() noexcept
{
    closeAndWaitForActiveLogs();
    if (this->m_asyncEnabled)
    {
        requestStop();

        if (this->m_workerThread.joinable())
        {
            this->m_workerThread.join();
        }
    }

    try
    {
        std::lock_guard<std::mutex> lock(this->m_fileOutputMutex);
        flushFileBufferUnlocked();
    }
    catch (...)
    {
        std::fputs("Logger could not flush file buffer.\n", stderr);
    }
}

void Logger::initialize(const std::string& p_moduleName, bool p_consoleLogOutput, const std::string& p_outputFilePath,
                        bool p_logFileOutput, LogLevel p_minimumLogLevel, bool p_asyncEnabled)
{
    std::lock_guard<std::mutex> lock(m_instanceMutex);

    if (m_instance)
    {
        throw std::logic_error("Logger has already been initialized.");
    }

    if (p_logFileOutput && p_outputFilePath.empty())
    {
        throw std::invalid_argument("File logging requires a non-empty output path.");
    }

    m_instance = std::unique_ptr<Logger>(new Logger(p_moduleName, p_consoleLogOutput, p_outputFilePath, p_logFileOutput,
                                                    p_minimumLogLevel, p_asyncEnabled));
}

Logger& Logger::getInstance()
{
    std::lock_guard<std::mutex> lock(m_instanceMutex);

    if (!m_instance)
    {
        throw std::logic_error("Logger is not initialized.");
    }

    return *m_instance;
}

std::string Logger::formatTimestamp() const
{
    const std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    std::tm utcTime{};
    gmtime_r(&time, &utcTime);

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");

    return stream.str();
}

std::string Logger::formatMessage(const std::string& p_format, const std::vector<std::string>& p_arguments)
{
    std::ostringstream result;
    for (std::size_t i = 0; i < p_format.size(); ++i)
    {
        // Normal character check
        if (p_format[i] != '%')
        {
            result << p_format[i];
            continue;
        }
        // Double placeholder symbol (treat them as a literal % symbol)
        if (i + 1 < p_format.size() && p_format[i + 1] == '%')
        {
            result << '%';
            ++i;
            continue;
        }

        // The placeholder substituition case
        std::size_t placeholder = 0;

        // Start at the i+1 character
        std::size_t j = i + 1;

        bool overflow = false;
        for (; j < p_format.size() && p_format[j] >= '0' && p_format[j] <= '9'; ++j)
        {
            const std::size_t digit = static_cast<std::size_t>(p_format[j] - '0');

            if (!overflow)
            {
                // Overflow check
                if (placeholder > (std::numeric_limits<std::size_t>::max() - digit) / 10)
                {
                    overflow = true;
                }
                else
                {
                    placeholder = placeholder * 10 + digit;
                }
            }
        }
        // If the string doesn't overflow and the p_arguments[placeholder] exists,
        // substitute it
        if (!overflow && placeholder >= 1 && placeholder <= p_arguments.size())
        {
            result << p_arguments[placeholder - 1];
        }
        else
        // Concat the string verbatim
        {
            result << p_format.substr(i, j - i);
        }
        i = j - 1;
    }
    return result.str();
}

void Logger::processLog(LogLevel p_logLevel, const char* p_file, const int p_line, const std::string& p_message)
{
    if (this->m_consoleLogOutput == false && this->m_fileLogOutput == false)
    {
        return;
    }
    // Insert timestamp with the YYYY-MM-DDTHH:MM:SSZ format
    std::string logEntry = formatTimestamp() + " ";

    // Insert log level tag [LEVEL]
    switch (p_logLevel)
    {
    case LogLevel::FATAL:
        logEntry += "[FATAL] ";
        break;
    case LogLevel::ERROR:
        logEntry += "[ERROR] ";
        break;
    case LogLevel::WARN:
        logEntry += "[WARN] ";
        break;
    case LogLevel::INFO:
        logEntry += "[INFO] ";
        break;
    case LogLevel::DEBUG:
        logEntry += "[DEBUG] ";
        break;
    case LogLevel::TRACE:
    default:
        logEntry += "[TRACE] ";
        break;
    }

    // Append location metadata [file_name:line]
    logEntry += '[';
    logEntry += p_file;
    logEntry += ':';
    logEntry += std::to_string(p_line);
    logEntry += "] ";

    // Append module name
    logEntry += "module=" + this->m_moduleName + ' ';

    // Append message content
    logEntry += p_message;

    // Append end of line
    logEntry += '\n';

    if (logEntry.size() > MAX_ENTRY_BYTES)
    {
        this->m_droppedEntries.fetch_add(1, std::memory_order_relaxed);
        return;
    }

    if (this->m_asyncEnabled)
    {
        this->enqueueLogEntry(std::move(logEntry));
    }
    else
    {
        this->writeLogEntry(logEntry);
    }
}

void Logger::writeLogEntry(const std::string& p_entry)
{
    if (this->m_consoleLogOutput.load())
    {
        try
        {
            std::lock_guard<std::mutex> lock(this->m_consoleOutputMutex);

            std::cout << p_entry;

            if (!std::cout)
            {
                this->m_outputFailures.fetch_add(1, std::memory_order_relaxed);
            }
        }
        catch (...)
        {
            this->m_outputFailures.fetch_add(1, std::memory_order_relaxed);
        }
    }

    if (this->m_fileLogOutput.load())
    {
        try
        {
            this->bufferAndFlushFileEntries(p_entry);
        }
        catch (...)
        {
            this->m_outputFailures.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void Logger::bufferAndFlushFileEntries(const std::string& p_entry)
{
    std::lock_guard<std::mutex> lock(this->m_fileOutputMutex);

    if (!this->m_fileLogOutput.load())
    {
        return;
    }

    this->m_fileBuffer += p_entry;

    if (this->m_fileBuffer.size() < MAX_FILE_BATCH_BYTES)
    {
        return;
    }

    flushFileBufferUnlocked();
}

void Logger::flushFileBufferUnlocked()
{
    if (this->m_fileBuffer.empty())
    {
        return;
    }

    if (this->m_currentLogFile == nullptr)
    {
        this->m_currentLogFile = createSessionLogFile(this->m_outputFilePath);
    }

    const std::uintmax_t incomingSize = this->m_fileBuffer.size();
    const std::uintmax_t currentSize = this->m_currentLogFile->size();

    if (currentSize > MAX_FILE_SIZE - incomingSize)
    {
        rotateLogFile();
    }

    this->m_currentLogFile->append(this->m_fileBuffer);
    this->m_fileBuffer.clear();
}

void Logger::enqueueLogEntry(std::string p_entry)
{
    {
        std::lock_guard<std::mutex> lock(this->m_queueMutex);

        if (this->m_stopping || this->m_logQueue.size() >= MAX_PENDING_ENTRIES)
        {
            this->m_droppedEntries.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        this->m_logQueue.push(std::move(p_entry));
    }

    this->m_queueCondition.notify_one();
}

void Logger::workerLoop()
{
    while (true)
    {
        std::string entry;
        {
            std::unique_lock<std::mutex> lock(this->m_queueMutex);

            this->m_queueCondition.wait(lock,
                                        [this]()
                                        {
                                            return this->m_stopping || !this->m_logQueue.empty();
                                        });
            if (this->m_stopping && this->m_logQueue.empty())
            {
                return;
            }

            entry = std::move(this->m_logQueue.front());
            this->m_logQueue.pop();
            this->m_workerBusy = true;
        }

        this->writeLogEntry(entry);
        {
            std::lock_guard<std::mutex> lock(this->m_queueMutex);
            this->m_workerBusy = false;
        }

        this->m_flushCondition.notify_all();
    }
}

void Logger::requestStop()
{
    {
        std::lock_guard<std::mutex> lock(this->m_queueMutex);
        this->m_stopping = true;
    }

    this->m_queueCondition.notify_all();
}

void Logger::closeAndWaitForActiveLogs()
{
    std::unique_lock<std::mutex> lock(this->m_lifecycleMutex);

    this->m_closing = true;

    this->m_lifecycleCondition.wait(lock,
                                    [this]()
                                    {
                                        return this->m_activeEntries == 0;
                                    });
}

void Logger::waitUntilIdle()
{
    std::unique_lock<std::mutex> lock(this->m_queueMutex);
    this->m_flushCondition.wait(lock,
                                [this]()
                                {
                                    return this->m_logQueue.empty() && !this->m_workerBusy;
                                });
}

void Logger::flush()
{
    waitUntilIdle();

    std::lock_guard<std::mutex> lock(this->m_fileOutputMutex);
    flushFileBufferUnlocked();
}

std::shared_ptr<FileHandler> Logger::createLogFile(const std::string& p_path)
{
    FileManager& manager = FileManager::getInstance();

    manager.create(p_path);

    return manager.get(p_path);
}

void Logger::rotateLogFile()
{
    const std::size_t nextIndex = this->m_logFileIndex + 1;

    std::shared_ptr<FileHandler> nextFile = createLogFile(getLogFilePath(nextIndex));

    this->m_logFileIndex = nextIndex;
    this->m_currentLogFile = std::move(nextFile);
}

std::string Logger::createSessionLogPrefix(const std::string& p_outputFilePath) const
{

    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);

    const auto microseconds =
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % std::chrono::seconds(1);

    std::tm utcTime{};
    gmtime_r(&time, &utcTime);

    std::ostringstream timestamp;
    timestamp << std::put_time(&utcTime, "%Y%m%dT%H%M%S") << '.' << std::setw(6) << std::setfill('0')
              << microseconds.count() << 'Z';

    const std::filesystem::path basePath(p_outputFilePath);

    const std::filesystem::path prefix = basePath.parent_path() / (timestamp.str() + "_" + basePath.stem().string());

    return prefix.string();
}

std::shared_ptr<FileHandler> Logger::createSessionLogFile(const std::string& p_outputFilePath)
{
    constexpr std::size_t MAX_ATTEMPTS = 5;

    std::string extension = std::filesystem::path(p_outputFilePath).extension().string();

    if (extension.empty())
    {
        extension = ".log";
    }

    FileManager& manager = FileManager::getInstance();

    for (std::size_t attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        std::string newPrefix = createSessionLogPrefix(p_outputFilePath);
        const std::string newPath = newPrefix + ".0" + extension;

        // Skip known collisions without relying on FileManager's exception type.
        if (manager.isfileExists(newPath))
        {
            continue;
        }

        std::shared_ptr<FileHandler> newFile;
        try
        {
            newFile = createLogFile(newPath);
            // [TODO] replace this with custom logic
        }
        catch (const std::runtime_error& error)
        {
            // Handle a collision reported by create after the existence check.
            // FileManager must expose file_exists for this concurrent case.
            const std::string message = error.what();

            if (message.find("File already exists or cannot be created") != std::string::npos)
            {
                continue;
            }

            throw;
        }

        // Commit session state only after successful creation.
        this->m_sessionLogPrefix = std::move(newPrefix);
        this->m_logFileIndex = 0;
        return newFile;
    }

    throw std::runtime_error("Failed to create a unique log file after 5 attempts.");
}

std::string Logger::getLogFilePath(std::size_t p_index) const
{
    std::string extension = std::filesystem::path(this->m_outputFilePath).extension().string();

    if (extension.empty())
    {
        extension = ".log";
    }

    return this->m_sessionLogPrefix + "." + std::to_string(p_index) + extension;
}

void Logger::setMinimumLogLevel(LogLevel p_minimumLevel)
{
    std::size_t minimumIndex = static_cast<std::size_t>(p_minimumLevel);
    if (minimumIndex >= static_cast<std::size_t>(LogLevel::INVALID))
    {
        // [TODO] Replace this with custom exception
        throw std::invalid_argument("Invalid log level used.");
    }
    // Lock the log levels status array
    std::lock_guard<std::mutex> lock(this->m_enabledLevelsMutex);

    this->m_minimumLogLevel = p_minimumLevel;

    for (std::size_t i = 0; i < static_cast<std::size_t>(LogLevel::INVALID); i++)
    {
        this->m_enabledLevels[i] = (i >= minimumIndex);
    }
}

void Logger::enableLogLevel(LogLevel p_logLevel)
{
    const std::size_t index = static_cast<std::size_t>(p_logLevel);

    if (index >= static_cast<std::size_t>(LogLevel::INVALID))
    {
        // [TODO] Replace with custom exception
        throw std::invalid_argument("Invalid log level used.");
    }
    std::lock_guard<std::mutex> lock(this->m_enabledLevelsMutex);
    this->m_enabledLevels[index] = true;
}

void Logger::resetLogLevels()
{
    std::lock_guard<std::mutex> lock(this->m_enabledLevelsMutex);

    const std::size_t minimumIndex = static_cast<std::size_t>(this->m_minimumLogLevel);

    for (std::size_t i = 0; i < static_cast<std::size_t>(LogLevel::INVALID); ++i)
    {
        this->m_enabledLevels[i] = (i >= minimumIndex);
    }
}

void Logger::changeLogFilePath(const std::string& p_outputFilePath)
{
    std::lock_guard<std::mutex> lock(this->m_fileOutputMutex);

    if (p_outputFilePath.empty())
    {
        throw std::invalid_argument("Invalid target log file path");
    }

    flushFileBufferUnlocked();
    updateLogFilePath(p_outputFilePath);
}

void Logger::updateLogFilePath(const std::string& p_outputFilePath)
{

    // Allocate the path copy before createSessionLogFile commits session state.
    std::shared_ptr<FileHandler> newFile = createSessionLogFile(p_outputFilePath);

    this->m_outputFilePath = p_outputFilePath;
    this->m_currentLogFile = std::move(newFile);
}

void Logger::setConsoleLogOutput(bool p_enabled)
{
    this->m_consoleLogOutput.store(p_enabled);
}

void Logger::setFileLogOutput(bool p_enabled, const std::string& p_outputFilePath)
{
    std::lock_guard<std::mutex> lock(this->m_fileOutputMutex);

    if (!p_enabled)
    {
        flushFileBufferUnlocked();
        this->m_fileLogOutput.store(false);
        return;
    }

    if (p_outputFilePath.empty())
    {
        throw std::invalid_argument("Invalid target log file path");
    }

    updateLogFilePath(p_outputFilePath);
    this->m_fileLogOutput.store(true);
}
