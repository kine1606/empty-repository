#include "gtest/gtest.h"

#include <chrono>
#include <thread>

#include "Logger.h"
#include "RUSupportChecker.h"
#include "RUValidator.h"
#include "RadioUnit.h"
#include "RadioUnitMailbox.h"
#include "mailbox.pb.h"
#include "ru.pb.h"
#include "terminal.pb.h"

namespace {
bool initTestLogger() {
    try {
        Logger::initialize("ru-test");
    } catch (...) {
    }
    return true;
}
const bool LOGGER_INITIALIZED = initTestLogger();
}

TEST(RadioUnitTest, SupportCheckerAndValidator) {
    RUSupportChecker checker;
    RUValidator validator;

    mailbox::MailboxRequest req;
    req.set_request_id("REQ-RU-TEST-001");
    req.set_source("DU");
    req.set_destination("RU");

    EXPECT_TRUE(checker.isSupported(req));

    req.set_destination("Terminal");
    EXPECT_FALSE(checker.isSupported(req));

    req.set_destination("RU");
    ru::RURequest ruReq;
    ruReq.set_command("STATUS");
    req.mutable_payload()->PackFrom(ruReq);

    EXPECT_TRUE(validator.validate(req).isSuccess());
}

TEST(RadioUnitTest, RUMailboxManagerProcessesRURequest) {
    RadioUnitMailbox mailbox;
    RUMailboxManager manager{mailbox};

    EXPECT_EQ(manager.getRfState(), "STANDBY");

    // 1. Send CONFIG request
    ru::RURequest configReq;
    configReq.set_command("CONFIG");
    configReq.set_tx_power_dbm(46.0);
    configReq.set_center_freq_mhz(3600.0);
    configReq.set_antenna_ports(8);

    mailbox::MailboxRequest envelope;
    envelope.set_request_id("REQ-RU-001");
    envelope.set_source("DU");
    envelope.set_destination("RU");
    envelope.mutable_payload()->PackFrom(configReq);

    auto valOpt = manager.validateMessage(envelope);
    ASSERT_TRUE(valOpt.has_value());

    auto execOpt = manager.processBusinessLogic(envelope);
    ASSERT_TRUE(execOpt.has_value());
    EXPECT_EQ(manager.getTxPowerDbm(), 46.0);
    EXPECT_EQ(manager.getCenterFreqMhz(), 3600.0);
    EXPECT_EQ(manager.getAntennaPorts(), 8);

    // 2. Send START request
    ru::RURequest startReq;
    startReq.set_command("START");
    envelope.mutable_payload()->PackFrom(startReq);

    auto startOpt = manager.processBusinessLogic(envelope);
    ASSERT_TRUE(startOpt.has_value());
    EXPECT_EQ(manager.getRfState(), "TX_ACTIVE");

    ru::RUResponse ruResp;
    ASSERT_TRUE(startOpt.value().payload().UnpackTo(&ruResp));
    EXPECT_EQ(ruResp.rf_state(), "TX_ACTIVE");
    EXPECT_EQ(ruResp.status_code(), 200);
}

TEST(RadioUnitTest, RUMailboxManagerProcessesForwardedTerminalRequest) {
    RadioUnitMailbox mailbox;
    RUMailboxManager manager{mailbox};

    terminal::TerminalRequest termReq;
    termReq.set_terminal_id("TERM01");
    termReq.set_action("STATUS");
    termReq.set_target_node("RU");

    mailbox::MailboxRequest envelope;
    envelope.set_request_id("REQ-TERM-RU-001");
    envelope.set_source("DU");
    envelope.set_destination("RU");
    envelope.mutable_payload()->PackFrom(termReq);

    auto execOpt = manager.processBusinessLogic(envelope);
    ASSERT_TRUE(execOpt.has_value());

    ru::RUResponse ruResp;
    ASSERT_TRUE(execOpt.value().payload().UnpackTo(&ruResp));
    EXPECT_EQ(ruResp.status_code(), 200);
    EXPECT_NE(ruResp.details().find("RU Hardware Status"), std::string::npos);
}