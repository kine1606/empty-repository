#include <gtest/gtest.h>

#include "Logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// Requires Logger::waitUntilIdle() and Logger::flush(), with worker-busy
// tracking under the queue mutex. Run this suite in its own test executable
// if another suite also initializes the Logger singleton.

class LoggerAsyncTest : public ::testing::Test
{
  protected:
    const std::string m_testOutputDirectory = "test_output";
    const std::string m_testOutputFile = this->m_testOutputDirectory + "/test_log.txt";

    static void SetUpTestSuite()
    {
        Logger::initialize("LoggerAsyncTest", true, "", false, LogLevel::INFO, true);
    }
    void SetUp() override
    {
        Logger& logger = Logger::getInstance();
        logger.flush();
        logger.setFileLogOutput(false);

        std::filesystem::remove_all(this->m_testOutputDirectory);
        std::filesystem::create_directory(this->m_testOutputDirectory);

        logger.setConsoleLogOutput(true);
        logger.setMinimumLogLevel(LogLevel::INFO);
    }

    void TearDown() override
    {
        Logger& logger = Logger::getInstance();
        logger.flush();
        logger.setFileLogOutput(false);
        std::filesystem::remove_all(this->m_testOutputDirectory);
    }

    std::set<std::filesystem::path> listLogFiles() const
    {
        std::set<std::filesystem::path> files;

        for (const auto& entry : std::filesystem::directory_iterator(this->m_testOutputDirectory))
        {

            if (!entry.is_regular_file())
            {
                continue;
            }

            const std::filesystem::path path = entry.path();

            if (path.extension() != ".txt")
            {
                continue;
            }

            files.insert(path);
        }

        return files;
    }

    // Discover the file created by an action without predicting the timestamp.
    template <typename Action> std::filesystem::path findCreatedFile(const Action& p_action) const
    {
        const std::set<std::filesystem::path> before = listLogFiles();
        p_action();
        const std::set<std::filesystem::path> after = listLogFiles();
        std::vector<std::filesystem::path> created;
        for (const auto& path : after)
        {
            if (before.count(path) == 0)
            {
                created.push_back(path);
            }
        }
        if (created.size() != 1)
        {
            throw std::runtime_error("Expected exactly one newly created log file.");
        }
        return created.front();
    }

    std::filesystem::path startSession(const std::string& p_basePath) const
    {
        return findCreatedFile(
            [&p_basePath]()
            {
                Logger::getInstance().setFileLogOutput(true, p_basePath);
            });
    }

    std::string readFile(const std::filesystem::path& p_path) const
    {
        std::ifstream file(p_path, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open test log file: " + p_path.string());
        }
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
};

// [LOG-ASYNC-01] Worker processes queued logs
TEST_F(LoggerAsyncTest, WorkerProcessesQueuedLogs)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    const std::string payload(MAX_ENTRY_BYTES / 2, 'A');

    std::size_t batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", payload);
        Logger::getInstance().waitUntilIdle();
        batchSize += payload.size();
    }

    logger.waitUntilIdle();
    EXPECT_NE(readFile(activeFile).find(payload), std::string::npos);
}

// [LOG-ASYNC-02] Console output is processed asynchronously
TEST_F(LoggerAsyncTest, ConsoleOutputIsProcessed)
{
    testing::internal::CaptureStdout();

    INFO("async-console-message");

    Logger::getInstance().waitUntilIdle();

    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("async-console-message"), std::string::npos);
}

// [LOG-ASYNC-03] Queued entries preserve order
TEST_F(LoggerAsyncTest, QueuedLogsPreserveOrder)
{
    Logger& logger = Logger::getInstance();
    ASSERT_GE(MAX_PENDING_ENTRIES, 100);

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    for (int index = 0; index < 100; ++index)
    {
        INFO("async-order-%1", index);
    }

    logger.flush();

    const std::string contents = readFile(activeFile);

    ASSERT_FALSE(contents.empty()) << "File is empty: " << activeFile;

    ASSERT_NE(contents.find("async-order-0"), std::string::npos) << "First message is absent. File contents:\n"
                                                                 << contents;

    std::size_t previousPosition = 0;

    for (int index = 0; index < 100; ++index)
    {
        const std::string marker = "async-order-" + std::to_string(index) + "\n";

        const std::size_t position = contents.find(marker);

        ASSERT_NE(position, std::string::npos) << "Missing message at index: " << index;
        EXPECT_EQ(contents.find(marker, position + marker.size()), std::string::npos);

        if (index > 0)
        {
            EXPECT_GT(position, previousPosition);
        }

        previousPosition = position;
    }
}

// [LOG-ASYNC-04] Multiple threads can enqueue logs
TEST_F(LoggerAsyncTest, MultipleThreadsCanLogConcurrently)
{
    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    constexpr int THREAD_COUNT = 4;
    constexpr int LOGS_PER_THREAD = 100;
    ASSERT_GE(MAX_PENDING_ENTRIES, THREAD_COUNT * LOGS_PER_THREAD);

    std::vector<std::thread> threads;

    for (int threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex)
    {
        threads.emplace_back(
            [threadIndex]()
            {
                for (int logIndex = 0; logIndex < LOGS_PER_THREAD; ++logIndex)
                {
                    INFO("thread-%1-log-%2", threadIndex, logIndex);
                }
            });
    }

    for (std::thread& thread : threads)
    {
        thread.join();
    }

    Logger::getInstance().waitUntilIdle();

    Logger::getInstance().setFileLogOutput(false);

    const std::string contents = readFile(activeFile);

    for (int threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex)
    {
        for (int logIndex = 0; logIndex < LOGS_PER_THREAD; ++logIndex)
        {
            const std::string marker =
                "thread-" + std::to_string(threadIndex) + "-log-" + std::to_string(logIndex) + "\n";
            const std::size_t position = contents.find(marker);
            ASSERT_NE(position, std::string::npos) << marker;
            EXPECT_EQ(contents.find(marker, position + marker.size()), std::string::npos) << marker;
        }
    }
}

// [LOG-ASYNC-05] Oversized entries are dropped
TEST_F(LoggerAsyncTest, OversizedEntriesAreDropped)
{
    Logger& logger = Logger::getInstance();

    const std::uint64_t droppedBefore = logger.droppedEntries();

    const std::string oversizedMessage(MAX_ENTRY_BYTES + 1, 'A');

    INFO("%1", oversizedMessage);

    EXPECT_EQ(logger.droppedEntries(), droppedBefore + 1);
}

// [LOG-ASYNC-06] Normal async load does not drop entries
TEST_F(LoggerAsyncTest, NormalLoadDoesNotDropEntries)
{
    Logger& logger = Logger::getInstance();

    const std::uint64_t droppedBefore = logger.droppedEntries();

    constexpr int LOG_COUNT = 100;
    ASSERT_GE(MAX_PENDING_ENTRIES, LOG_COUNT);

    for (int index = 0; index < LOG_COUNT; ++index)
    {
        INFO("normal-load-%1", index);
    }

    EXPECT_EQ(logger.droppedEntries(), droppedBefore);
}

// [LOG-ASYNC-07] Partial async file batch flushes when disabled
TEST_F(LoggerAsyncTest, PartialFileBatchFlushesWhenDisabled)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    INFO("async-partial-batch");

    logger.waitUntilIdle();
    EXPECT_TRUE(readFile(activeFile).empty());
    logger.setFileLogOutput(false);

    const std::string contents = readFile(activeFile);

    EXPECT_NE(contents.find("async-partial-batch"), std::string::npos);
}

// [LOG-ASYNC-08] Partial async batch flushes before path change
TEST_F(LoggerAsyncTest, PartialFileBatchFlushesBeforePathChange)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path firstFile = startSession(this->m_testOutputFile);

    INFO("first-async-file-message");

    Logger::getInstance().waitUntilIdle();
    EXPECT_TRUE(readFile(firstFile).empty());

    const std::string secondOutputFile = this->m_testOutputDirectory + "/second_log.txt";

    const std::filesystem::path secondFile = findCreatedFile(
        [&logger, &secondOutputFile]()
        {
            logger.changeLogFilePath(secondOutputFile);
        });

    const std::string firstContents = readFile(firstFile);

    EXPECT_NE(firstContents.find("first-async-file-message"), std::string::npos);

    INFO("second-async-file-message");

    Logger::getInstance().waitUntilIdle();

    logger.setFileLogOutput(false);

    const std::string secondContents = readFile(secondFile);

    EXPECT_NE(secondContents.find("second-async-file-message"), std::string::npos);

    EXPECT_EQ(readFile(firstFile).find("second-async-file-message"), std::string::npos);
    EXPECT_EQ(secondContents.find("first-async-file-message"), std::string::npos);
}

// [LOG-ASYNC-09] Async batched output rotates log file
TEST_F(LoggerAsyncTest, AsyncFileRotationWorks)
{
    const std::filesystem::path firstFile = startSession(this->m_testOutputFile);

    const std::string filler(MAX_FILE_SIZE - 10, 'A');

    {
        std::ofstream file(firstFile, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(file.is_open());

        file.write(filler.data(), static_cast<std::streamsize>(filler.size()));
        file.close();
        ASSERT_FALSE(file.fail());
    }

    const std::string payload(MAX_ENTRY_BYTES / 2, 'R');

    std::size_t batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", payload);
        Logger::getInstance().waitUntilIdle();
        batchSize += payload.size();
    }

    std::filesystem::path secondFile = firstFile;
    secondFile.replace_filename(firstFile.stem().stem().string() + ".1.txt");

    Logger::getInstance().waitUntilIdle();
    ASSERT_TRUE(std::filesystem::exists(secondFile));

    EXPECT_EQ(readFile(firstFile), filler);
    EXPECT_NE(readFile(secondFile).find(payload), std::string::npos);
}

// [LOG-ASYNC-10] Multiple async batches append correctly
TEST_F(LoggerAsyncTest, MultipleFileBatchesAppendCorrectly)
{
    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    const std::string firstPayload(MAX_ENTRY_BYTES / 2, 'A');

    std::size_t batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", firstPayload);
        Logger::getInstance().waitUntilIdle();
        batchSize += firstPayload.size();
    }

    Logger::getInstance().waitUntilIdle();
    ASSERT_NE(readFile(activeFile).find(firstPayload), std::string::npos);

    const std::string firstContents = readFile(activeFile);

    const std::string secondPayload(MAX_ENTRY_BYTES / 2, 'B');

    batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", secondPayload);
        Logger::getInstance().waitUntilIdle();
        batchSize += secondPayload.size();
    }

    Logger::getInstance().waitUntilIdle();
    ASSERT_NE(readFile(activeFile).find(secondPayload), std::string::npos);

    const std::string secondContents = readFile(activeFile);

    EXPECT_EQ(secondContents.substr(0, firstContents.size()), firstContents);

    EXPECT_LT(secondContents.find(firstPayload), secondContents.find(secondPayload));
}

// [LOG-ASYNC-11] Async output selection
TEST_F(LoggerAsyncTest, OutputSelectionIsRespected)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    // File only.
    logger.setConsoleLogOutput(false);

    testing::internal::CaptureStdout();

    INFO("async-file-only");

    Logger::getInstance().waitUntilIdle();

    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(output.empty());

    // Flush partial file batch.
    logger.setFileLogOutput(false);

    EXPECT_NE(readFile(activeFile).find("async-file-only"), std::string::npos);

    // Console only.
    logger.setConsoleLogOutput(true);

    testing::internal::CaptureStdout();

    INFO("async-console-only");

    Logger::getInstance().waitUntilIdle();

    const std::string consoleOutput = testing::internal::GetCapturedStdout();

    EXPECT_NE(consoleOutput.find("async-console-only"), std::string::npos);
    EXPECT_EQ(readFile(activeFile).find("async-console-only"), std::string::npos);
}

// [LOG-ASYNC-12] Async log filtering
TEST_F(LoggerAsyncTest, LogFilteringIsRespected)
{
    Logger& logger = Logger::getInstance();

    logger.setMinimumLogLevel(LogLevel::INFO);

    testing::internal::CaptureStdout();

    DEBUG("async-filtered");
    INFO("async-accepted");

    Logger::getInstance().waitUntilIdle();

    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output.find("async-filtered"), std::string::npos);
    EXPECT_NE(output.find("async-accepted"), std::string::npos);
}

// [LOG-ASYNC-13] Console is not batched with file output
TEST_F(LoggerAsyncTest, ConsoleWritesWhileFileRemainsBuffered)
{
    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    testing::internal::CaptureStdout();

    INFO("async-small-message");

    Logger::getInstance().waitUntilIdle();

    const std::string output = testing::internal::GetCapturedStdout();

    // Console receives the entry immediately when processed.
    EXPECT_NE(output.find("async-small-message"), std::string::npos);

    // File batch has not reached its byte threshold yet.
    EXPECT_TRUE(readFile(activeFile).empty());

    // Flush remaining partial batch.
    Logger::getInstance().setFileLogOutput(false);

    EXPECT_NE(readFile(activeFile).find("async-small-message"), std::string::npos);
}

// [LOG-ASYNC-14] Explicit flush preserves queued logs before file disable
TEST_F(LoggerAsyncTest, FlushedLogsArePreservedWhenFileOutputIsDisabled)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    constexpr int LOG_COUNT = 100;
    ASSERT_GE(MAX_PENDING_ENTRIES, LOG_COUNT);

    for (int index = 0; index < LOG_COUNT; ++index)
    {
        INFO("queued-before-disable-%1", index);
    }

    // Disabling alone does not drain the queue. Explicitly finish the logs first.
    logger.flush();
    logger.setFileLogOutput(false);

    const std::string contents = readFile(activeFile);

    for (int index = 0; index < LOG_COUNT; ++index)
    {
        const std::string marker = "queued-before-disable-" + std::to_string(index) + "\n";

        const std::size_t position = contents.find(marker);
        ASSERT_NE(position, std::string::npos) << marker;
        EXPECT_EQ(contents.find(marker, position + marker.size()), std::string::npos) << marker;
    }
}

// [LOG-ASYNC-15] Concurrent producers preserve all entries
TEST_F(LoggerAsyncTest, ConcurrentProducersDoNotLoseEntries)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    constexpr int THREAD_COUNT = 4;
    constexpr int LOGS_PER_THREAD = 100;
    ASSERT_GE(MAX_PENDING_ENTRIES, THREAD_COUNT * LOGS_PER_THREAD);

    std::vector<std::thread> threads;

    for (int threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex)
    {
        threads.emplace_back(
            [threadIndex]()
            {
                for (int logIndex = 0; logIndex < LOGS_PER_THREAD; ++logIndex)
                {
                    INFO("producer-%1-entry-%2", threadIndex, logIndex);
                }
            });
    }

    for (std::thread& thread : threads)
    {
        thread.join();
    }

    Logger::getInstance().waitUntilIdle();

    logger.setFileLogOutput(false);

    const std::string contents = readFile(activeFile);

    for (int threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex)
    {
        for (int logIndex = 0; logIndex < LOGS_PER_THREAD; ++logIndex)
        {
            const std::string marker =
                "producer-" + std::to_string(threadIndex) + "-entry-" + std::to_string(logIndex) + "\n";

            const std::size_t position = contents.find(marker);
            ASSERT_NE(position, std::string::npos) << marker;
            EXPECT_EQ(contents.find(marker, position + marker.size()), std::string::npos) << marker;
        }
    }
}
