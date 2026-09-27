#include <gtest/gtest.h>

#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

class LoggerTestSync : public ::testing::Test
{
  protected:
    const std::string m_testOutputDirectory = "test_output";
    const std::string m_testOutputFile = this->m_testOutputDirectory + "/test_log.txt";

    static void SetUpTestSuite()
    {
        Logger::initialize("LoggerTest");
    }

    void SetUp() override
    {
        std::filesystem::remove_all(this->m_testOutputDirectory);
        std::filesystem::create_directory(this->m_testOutputDirectory);

        Logger& logger = Logger::getInstance();
        logger.setConsoleLogOutput(true);
        logger.setFileLogOutput(false);
        logger.setMinimumLogLevel(LogLevel::INFO);
    }

    void TearDown() override
    {
        Logger::getInstance().setFileLogOutput(false);
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

// [LOG-SYNC-01] File output configuration
TEST_F(LoggerTestSync, FileOutputConfigurationIsHandled)
{
    Logger& logger = Logger::getInstance();

    EXPECT_NO_THROW(logger.setFileLogOutput(false));

    EXPECT_NO_THROW(logger.setFileLogOutput(true, this->m_testOutputFile));

    EXPECT_THROW(logger.setFileLogOutput(true, ""), std::invalid_argument);

    logger.setFileLogOutput(false);

    const auto newDirectory = std::filesystem::path(this->m_testOutputDirectory) / "test-path";

    const auto newBasePath = newDirectory / "test_log.txt";

    std::filesystem::create_directory(newDirectory);

    ASSERT_TRUE(std::filesystem::exists(newDirectory));

    ASSERT_NO_THROW(logger.setFileLogOutput(true, newBasePath.string()));

    EXPECT_TRUE(std::filesystem::is_directory(newDirectory));
    EXPECT_FALSE(std::filesystem::is_empty(newDirectory));
}

// [LOG-SYNC-02] File creation failure
TEST_F(LoggerTestSync, FileCreationFailurePreservesCurrentSession)
{
    Logger& logger = Logger::getInstance();

    const auto activeFile = startSession(this->m_testOutputFile);

    const auto blocker = std::filesystem::path(this->m_testOutputDirectory) / "regular-file";

    {
        std::ofstream file(blocker);
        ASSERT_TRUE(file.is_open());
    }

    const auto invalidPath = blocker / "test_log.txt";

    EXPECT_ANY_THROW(logger.setFileLogOutput(true, invalidPath.string()));

    INFO("Hello I recovered");

    logger.setFileLogOutput(false);

    const std::string contents = readFile(activeFile);
    EXPECT_NE(contents.find("Hello I recovered"), std::string::npos);
}

// [LOG-SYNC-03] Log filtering
TEST_F(LoggerTestSync, LogFilterHandlesLogLevels)
{
    Logger& logger = Logger::getInstance();

    logger.enableLogLevel(LogLevel::TRACE);
    testing::internal::CaptureStdout();

    TRACE("Bodoi");

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoi"), std::string::npos);
    EXPECT_NE(output.find("[TRACE]"), std::string::npos);

    logger.resetLogLevels();
    testing::internal::CaptureStdout();

    TRACE("Cuho");

    output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output, "");

    testing::internal::CaptureStdout();

    logger.log(LogLevel::INVALID, __FILE__, __LINE__, "INVALID");

    output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "");

    EXPECT_THROW(logger.setMinimumLogLevel(LogLevel::INVALID), std::invalid_argument);

    testing::internal::CaptureStdout();

    DEBUG("filtered-marker");
    INFO("accepted-marker");

    output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output.find("filtered-marker"), std::string::npos);
    EXPECT_NE(output.find("accepted-marker"), std::string::npos);

    EXPECT_THROW(logger.enableLogLevel(LogLevel::INVALID), std::invalid_argument);

    testing::internal::CaptureStdout();

    DEBUG("filtered-marker");
    INFO("accepted-marker");

    output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(output.find("filtered-marker"), std::string::npos);
    EXPECT_NE(output.find("accepted-marker"), std::string::npos);
}

// [LOG-SYNC-04] All log levels
TEST_F(LoggerTestSync, AllLogLevelsAreHandled)
{
    Logger& logger = Logger::getInstance();
    logger.setMinimumLogLevel(LogLevel::TRACE);

    testing::internal::CaptureStdout();

    TRACE("Bodoi1");

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoi1"), std::string::npos);
    EXPECT_NE(output.find("[TRACE]"), std::string::npos);

    testing::internal::CaptureStdout();

    DEBUG("Bodoi2");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoi2"), std::string::npos);
    EXPECT_NE(output.find("[DEBUG]"), std::string::npos);

    testing::internal::CaptureStdout();

    INFO("Bodoi3");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoi3"), std::string::npos);
    EXPECT_NE(output.find("[INFO]"), std::string::npos);

    testing::internal::CaptureStdout();

    WARN("Bodoi4");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoi4"), std::string::npos);
    EXPECT_NE(output.find("[WARN]"), std::string::npos);

    testing::internal::CaptureStdout();

    ERROR("Bodoi5");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoi5"), std::string::npos);
    EXPECT_NE(output.find("[ERROR]"), std::string::npos);

    testing::internal::CaptureStdout();

    FATAL("Bodoi6");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoi6"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);
}

// [LOG-SYNC-05] Message formatting
TEST_F(LoggerTestSync, MessageFormattingIsHandled)
{
    testing::internal::CaptureStdout();

    FATAL("Bodoicuho");

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bodoicuho"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    FATAL("1+1=%1", 2);

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("1+1=2"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    FATAL("100%%");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("100%"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    FATAL("Thanh %0 que toi", "Hoa");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Thanh %0 que toi"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    FATAL("%36 mai dinh");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("%36 mai dinh"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    const std::string overflowPlaceholder = "%" + std::to_string(std::numeric_limits<std::size_t>::max()) + "00";

    FATAL(overflowPlaceholder + " mai dinh");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find(overflowPlaceholder + " mai dinh"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    FATAL("end%");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("end%"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    FATAL("%x");

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("%x"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);
}

// [LOG-SYNC-06] Serialization
TEST_F(LoggerTestSync, ParameterSerializationIsHandled)
{
    testing::internal::CaptureStdout();

    FATAL("%1 la %2", 36, "Thanh Hoa");

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("36 la Thanh Hoa"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    std::vector<std::string> content1 = {"Mot", "Hai", "Ba", "Zo"};

    FATAL("Vector chua :%1", content1);

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Vector chua :Mot Hai Ba Zo"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    testing::internal::CaptureStdout();

    std::vector<std::string> content2 = {"Mot", "Hai", "Ba", "Zo"};

    FATAL("Bien chua :%1, Vector chua :%2", 36, content2);

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Bien chua :36, Vector chua :Mot Hai Ba Zo"), std::string::npos);
    EXPECT_NE(output.find("[FATAL]"), std::string::npos);

    const std::vector<std::string> emptyValues;

    testing::internal::CaptureStdout();

    FATAL("begin[%1]end", emptyValues);

    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("begin[]end\n"), std::string::npos);
}

// [LOG-SYNC-07] Output matrix
TEST_F(LoggerTestSync, OutputMatrixIsCorrect)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path firstFile = startSession(this->m_testOutputFile);

    testing::internal::CaptureStdout();
    INFO("Both outputs enabled");
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Both outputs enabled"), std::string::npos);

    logger.setFileLogOutput(false);

    EXPECT_NE(readFile(firstFile).find("Both outputs enabled"), std::string::npos);

    const std::filesystem::path secondFile = findCreatedFile(
        [&logger, this]()
        {
            logger.setFileLogOutput(true, this->m_testOutputFile);
        });

    logger.setConsoleLogOutput(false);

    testing::internal::CaptureStdout();
    INFO("File output only");
    output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(output.empty());

    logger.setFileLogOutput(false);

    EXPECT_NE(readFile(secondFile).find("File output only"), std::string::npos);

    logger.setConsoleLogOutput(true);

    const std::string previousContents = readFile(secondFile);

    testing::internal::CaptureStdout();
    INFO("Console output only");
    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Console output only"), std::string::npos);
    EXPECT_EQ(readFile(secondFile), previousContents);

    logger.setConsoleLogOutput(false);

    testing::internal::CaptureStdout();
    INFO("Both outputs disabled");
    output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(output.empty());
    EXPECT_EQ(readFile(secondFile), previousContents);
}

// [LOG-SYNC-08] Normal file append
TEST_F(LoggerTestSync, FileOutputCreatesAndAppendsCorrectly)
{
    Logger& logger = Logger::getInstance();

    ASSERT_FALSE(std::filesystem::exists(this->m_testOutputFile));

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    ASSERT_TRUE(std::filesystem::exists(activeFile));
    EXPECT_FALSE(std::filesystem::exists(this->m_testOutputFile));

    const std::string filename = activeFile.filename().string();
    const std::string suffix = "_test_log.0.txt";

    ASSERT_GE(filename.size(), suffix.size());
    EXPECT_EQ(filename.substr(filename.size() - suffix.size()), suffix);

    const std::string firstPayload(MAX_ENTRY_BYTES / 2, 'A');

    std::size_t batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", firstPayload);
        batchSize += firstPayload.size();
    }

    const std::string firstContents = readFile(activeFile);

    EXPECT_NE(firstContents.find(firstPayload), std::string::npos);

    const std::string secondPayload(MAX_ENTRY_BYTES / 2, 'B');

    batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", secondPayload);
        batchSize += secondPayload.size();
    }

    const std::string secondContents = readFile(activeFile);

    EXPECT_EQ(secondContents.substr(0, firstContents.size()), firstContents);

    const auto firstPosition = secondContents.find(firstPayload);
    const auto secondPosition = secondContents.find(secondPayload);

    ASSERT_NE(firstPosition, std::string::npos);
    ASSERT_NE(secondPosition, std::string::npos);
    EXPECT_LT(firstPosition, secondPosition);
}

// [LOG-SYNC-09] File rotation
TEST_F(LoggerTestSync, LogRotationPerformsCorrectly)
{
    const std::filesystem::path firstFile = startSession(this->m_testOutputFile);

    const std::string filler(MAX_FILE_SIZE - 10, 'A');

    {
        std::ofstream file(firstFile, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(file.is_open());
        file.write(filler.data(), static_cast<std::streamsize>(filler.size()));
    }

    const std::string firstPayload(MAX_ENTRY_BYTES / 2, 'R');

    std::size_t batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", firstPayload);
        batchSize += firstPayload.size();
    }

    std::filesystem::path secondFile = firstFile;
    secondFile.replace_filename(firstFile.stem().stem().string() + ".1.txt");

    ASSERT_TRUE(std::filesystem::exists(secondFile));
    EXPECT_NE(readFile(secondFile).find(firstPayload), std::string::npos);
    EXPECT_EQ(listLogFiles().size(), 2U);

    {
        std::ofstream file(secondFile, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(file.is_open());
        file.write(filler.data(), static_cast<std::streamsize>(filler.size()));
    }

    const std::string secondPayload(MAX_ENTRY_BYTES / 2, 'S');

    batchSize = 0;

    while (batchSize < MAX_FILE_BATCH_BYTES)
    {
        INFO("%1", secondPayload);
        batchSize += secondPayload.size();
    }

    std::filesystem::path thirdFile = firstFile;
    thirdFile.replace_filename(firstFile.stem().stem().string() + ".2.txt");

    ASSERT_TRUE(std::filesystem::exists(thirdFile));
    EXPECT_NE(readFile(thirdFile).find(secondPayload), std::string::npos);
    EXPECT_EQ(listLogFiles().size(), 3U);
}

// [LOG-SYNC-10] Configuration changes
TEST_F(LoggerTestSync, ConfigurationChangesAreRespected)
{
    Logger& logger = Logger::getInstance();
    const std::filesystem::path firstFile = startSession(this->m_testOutputFile);

    EXPECT_THROW(logger.changeLogFilePath(""), std::invalid_argument);

    INFO("Original session still works");

    const std::string SECOND_OUTPUT_FILE = this->m_testOutputDirectory + "/second_log.txt";

    const std::filesystem::path secondFile = findCreatedFile(
        [&logger, &SECOND_OUTPUT_FILE]()
        {
            logger.changeLogFilePath(SECOND_OUTPUT_FILE);
        });

    // Path change flushed the previous session.
    const std::string firstContents = readFile(firstFile);

    EXPECT_NE(firstContents.find("Original session still works"), std::string::npos);

    INFO("Change log file path test");

    logger.setFileLogOutput(false);

    const std::string secondContents = readFile(secondFile);

    EXPECT_NE(secondContents.find("Change log file path test"), std::string::npos);
    EXPECT_EQ(readFile(firstFile), firstContents);

    INFO("Disable log file path test");
    EXPECT_EQ(readFile(secondFile), secondContents);

    EXPECT_THROW(logger.setFileLogOutput(true, ""), std::invalid_argument);

    const std::filesystem::path thirdFile = startSession(this->m_testOutputFile);

    INFO("Re-enable log file path test");

    logger.setFileLogOutput(false);

    EXPECT_NE(readFile(thirdFile).find("Re-enable log file path test"), std::string::npos);
    EXPECT_EQ(readFile(firstFile), firstContents);
    EXPECT_EQ(readFile(secondFile), secondContents);

    logger.setConsoleLogOutput(false);

    testing::internal::CaptureStdout();
    INFO("Disable console log test");
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(output.empty());

    logger.setConsoleLogOutput(true);

    testing::internal::CaptureStdout();
    INFO("Enable console log test");
    output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Enable console log test"), std::string::npos);
}

// [LOG-SYNC-11] Oversize messages
TEST_F(LoggerTestSync, OversizeMessagesAreDropped)
{
    Logger& logger = Logger::getInstance();

    const std::uint64_t droppedBefore = logger.droppedEntries();

    const std::string oversizedMessage(MAX_ENTRY_BYTES + 1, 'A');

    testing::internal::CaptureStdout();

    INFO("%1", oversizedMessage);

    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(output.empty());
    EXPECT_EQ(logger.droppedEntries(), droppedBefore + 1);
}

// [LOG-SYNC-13] Partial batch flush on disable
TEST_F(LoggerTestSync, PartialFileBatchFlushesWhenDisabled)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path activeFile = startSession(this->m_testOutputFile);

    INFO("partial-batch-message");

    // Batch should still be buffered.
    EXPECT_TRUE(readFile(activeFile).empty());

    // Disabling file output must flush the remaining buffer.
    logger.setFileLogOutput(false);

    const std::string contents = readFile(activeFile);

    EXPECT_NE(contents.find("partial-batch-message"), std::string::npos);
}

// [LOG-SYNC-14] Partial batch flush on path change
TEST_F(LoggerTestSync, PartialFileBatchFlushesBeforePathChange)
{
    Logger& logger = Logger::getInstance();

    const std::filesystem::path firstFile = startSession(this->m_testOutputFile);

    INFO("first-file-message");

    const std::string secondOutputFile = this->m_testOutputDirectory + "/second_log.txt";

    const std::filesystem::path secondFile = findCreatedFile(
        [&logger, &secondOutputFile]()
        {
            logger.changeLogFilePath(secondOutputFile);
        });

    // Changing path must flush pending entries to the old file.
    const std::string firstContents = readFile(firstFile);

    EXPECT_NE(firstContents.find("first-file-message"), std::string::npos);

    INFO("second-file-message");

    // Flush the second file before reading it.
    logger.setFileLogOutput(false);

    const std::string secondContents = readFile(secondFile);

    EXPECT_NE(secondContents.find("second-file-message"), std::string::npos);
    EXPECT_EQ(firstContents.find("second-file-message"), std::string::npos);
}