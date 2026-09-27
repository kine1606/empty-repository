#include "gtest/gtest.h"

#include "Logger.h"
#include "Terminal.h"
#include "TerminalMailbox.h"
#include "TerminalMailboxManager.h"
#include "TerminalSupportChecker.h"
#include "TerminalValidator.h"
#include "du.pb.h"
#include "mailbox.pb.h"
#include "terminal.pb.h"

namespace {
bool initTestLogger() {
    try {
        Logger::initialize("terminal-test");
    } catch (...) {
    }
    return true;
}
const bool LOGGER_INITIALIZED = initTestLogger();
}

TEST(TerminalTest, SupportCheckerAndValidator) {
    TerminalSupportChecker checker;
    TerminalValidator validator;

    mailbox::MailboxRequest req;
    req.set_request_id("REQ-TERM-TEST-1");
    req.set_source("DU");
    req.set_destination("Terminal");

    EXPECT_TRUE(checker.isSupported(req));

    req.set_destination("OtherNode");
    EXPECT_FALSE(checker.isSupported(req));

    req.set_destination("Terminal");
    terminal::TerminalResponse termResp;
    termResp.set_request_id("REQ-TERM-TEST-1");
    termResp.set_is_success(true);
    termResp.set_output_text("OK");
    req.mutable_payload()->PackFrom(termResp);

    EXPECT_TRUE(validator.validate(req).isSuccess());
}

TEST(TerminalTest, TerminalMailboxManagerReceivesResponses) {
    TerminalMailbox mailbox;
    TerminalMailboxManager manager{mailbox};

    EXPECT_FALSE(manager.hasReceivedValidation());
    EXPECT_FALSE(manager.hasReceivedExecution());

    // 1. Send ValidationResponse
    mailbox::MailboxRequest valEnvelope;
    valEnvelope.set_request_id("REQ-001");
    valEnvelope.set_source("DU");
    valEnvelope.set_destination("Terminal");

    mailbox::ValidationResponse valResp;
    valResp.set_request_id("REQ-001");
    valResp.set_is_valid(true);
    valResp.set_message("DU validated request");
    valEnvelope.mutable_payload()->PackFrom(valResp);

    manager.validateMessage(valEnvelope);
    EXPECT_TRUE(manager.hasReceivedValidation());
    EXPECT_EQ(manager.getLastValidation().message(), "DU validated request");

    // 2. Send TerminalResponse
    mailbox::MailboxRequest execEnvelope;
    execEnvelope.set_request_id("REQ-001");
    execEnvelope.set_source("DU");
    execEnvelope.set_destination("Terminal");

    terminal::TerminalResponse termResp;
    termResp.set_request_id("REQ-001");
    termResp.set_is_success(true);
    termResp.set_return_code(0);
    termResp.set_output_text("DU Status: RUNNING");
    execEnvelope.mutable_payload()->PackFrom(termResp);

    manager.processBusinessLogic(execEnvelope);
    EXPECT_TRUE(manager.hasReceivedExecution());
    EXPECT_EQ(manager.getLastTerminalResponse().output_text(), "DU Status: RUNNING");
}