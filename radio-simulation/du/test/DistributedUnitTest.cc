#include "gtest/gtest.h"

#include <chrono>
#include <thread>

#include "DistributedUnit.h"
#include "Logger.h"
#include "du.pb.h"
#include "mailbox.pb.h"
#include "terminal.pb.h"

namespace {
bool initTestLogger() {
    try {
        Logger::initialize("du-test");
    } catch (...) {
    }
    return true;
}
const bool LOGGER_INITIALIZED = initTestLogger();
}

TEST(DistributedUnitTest, DUValidatorAndManagerProcessesTerminalRequest) {
    DistributedUnit du;
    EXPECT_FALSE(du.isRunning());
    EXPECT_EQ(du.getManager().getState(), "IDLE");

    // Create a TerminalRequest
    terminal::TerminalRequest termReq;
    termReq.set_terminal_id("TERM01");
    termReq.set_action("CONFIG");
    termReq.set_parameters("freq=3.5GHz bw=100MHz power=43dBm");

    mailbox::MailboxRequest req;
    req.set_request_id("REQ-TEST-100");
    req.set_source("Terminal");
    req.set_destination("DU");
    req.mutable_payload()->PackFrom(termReq);

    // Validate request
    auto valOpt = du.getManager().validateMessage(req);
    ASSERT_TRUE(valOpt.has_value());
    EXPECT_EQ(valOpt.value().destination(), "Terminal");

    mailbox::ValidationResponse valResp;
    ASSERT_TRUE(valOpt.value().payload().UnpackTo(&valResp));
    EXPECT_TRUE(valResp.is_valid());

    // Process business logic
    auto execOpt = du.getManager().processBusinessLogic(req);
    ASSERT_TRUE(execOpt.has_value());
    EXPECT_EQ(execOpt.value().destination(), "Terminal");

    terminal::TerminalResponse termResp;
    ASSERT_TRUE(execOpt.value().payload().UnpackTo(&termResp));
    EXPECT_TRUE(termResp.is_success());
    EXPECT_EQ(termResp.return_code(), 0);
    EXPECT_EQ(du.getManager().getState(), "CONFIGURED");
}

TEST(DistributedUnitTest, DUManagerHandlesStartAndStatus) {
    DistributedUnit du;

    // Start cell
    terminal::TerminalRequest startReq;
    startReq.set_terminal_id("TERM01");
    startReq.set_action("START");

    mailbox::MailboxRequest reqStart;
    reqStart.set_request_id("REQ-TEST-101");
    reqStart.set_source("Terminal");
    reqStart.set_destination("DU");
    reqStart.mutable_payload()->PackFrom(startReq);

    auto startRespOpt = du.getManager().processBusinessLogic(reqStart);
    ASSERT_TRUE(startRespOpt.has_value());
    EXPECT_EQ(du.getManager().getState(), "RUNNING");

    // Query status
    terminal::TerminalRequest statusReq;
    statusReq.set_terminal_id("TERM01");
    statusReq.set_action("STATUS");

    mailbox::MailboxRequest reqStatus;
    reqStatus.set_request_id("REQ-TEST-102");
    reqStatus.set_source("Terminal");
    reqStatus.set_destination("DU");
    reqStatus.mutable_payload()->PackFrom(statusReq);

    auto statusRespOpt = du.getManager().processBusinessLogic(reqStatus);
    ASSERT_TRUE(statusRespOpt.has_value());
    terminal::TerminalResponse statusResp;
    ASSERT_TRUE(statusRespOpt.value().payload().UnpackTo(&statusResp));
    EXPECT_TRUE(statusResp.is_success());
    EXPECT_NE(statusResp.output_text().find("RUNNING"), std::string::npos);
}