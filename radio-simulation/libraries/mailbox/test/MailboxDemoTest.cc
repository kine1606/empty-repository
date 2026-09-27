#include "gtest/gtest.h"

#include <chrono>
#include <memory>
#include <thread>

#include "DUMailboxManager.h"
#include "DistributedUnitMailbox.h"
#include "Logger.h"
#include "Mailbox.h"
#include "MailboxWorker.h"
#include "NodeAManager.h"
#include "NodeBManager.h"
#include "RUMailboxManager.h"
#include "RadioUnitMailbox.h"
#include "TerminalMailbox.h"
#include "TerminalMailboxManager.h"
#include "du.pb.h"
#include "mailbox.pb.h"
#include "ru.pb.h"
#include "terminal.pb.h"

TEST(MailboxDemoTest, NodeAToNodeBTwoDistinctResponses) {
    // 1. Setup Mailbox and Manager for Node A
    Mailbox mailboxA;
    NodeAManager managerA{mailboxA};
    MailboxWorker workerA{mailboxA, managerA, nullptr};
    workerA.start();

    // 2. Setup Mailbox and Manager for Node B
    Mailbox mailboxB;
    NodeBManager managerB{mailboxB};

    // Node B worker sends responses back to Node A mailbox
    MailboxWorker workerB{mailboxB, managerB, [&mailboxA](const mailbox::MailboxRequest &p_response) {
        mailboxA.enqueue(p_response);
    }};
    workerB.start();

    // 3. Node A creates a NodeBRequest
    mailbox::NodeBRequest nodeBReq;
    nodeBReq.set_command("PROCESS_FRAME");
    nodeBReq.set_transaction_id(101);
    nodeBReq.set_data("radio_frame_payload_0x42");

    mailbox::MailboxRequest requestEnvelope;
    requestEnvelope.set_request_id("REQ-TEST-001");
    requestEnvelope.set_source("NodeA");
    requestEnvelope.set_destination("NodeB");
    requestEnvelope.mutable_payload()->PackFrom(nodeBReq);

    // 4. Send request to Node B's mailbox
    EXPECT_TRUE(mailboxB.enqueue(requestEnvelope));

    // 5. Verify Node A receives distinct response 1: Validation Response
    EXPECT_TRUE(managerA.waitForValidation(2000));
    EXPECT_TRUE(managerA.hasReceivedValidation());
    auto valResp = managerA.getLastValidation();
    EXPECT_EQ(valResp.request_id(), "REQ-TEST-001");
    EXPECT_TRUE(valResp.is_valid());
    EXPECT_EQ(valResp.error_code(), mailbox::ErrorCode::OK);
    EXPECT_EQ(valResp.message(), "NodeB validated NodeBRequest successfully");

    // 6. Verify Node A receives distinct response 2: Business Logic Response
    EXPECT_TRUE(managerA.waitForBusinessResponse(2000));
    EXPECT_TRUE(managerA.hasReceivedBusinessResponse());
    auto bizResp = managerA.getLastBusinessResponse();
    EXPECT_EQ(bizResp.request_id(), "REQ-TEST-001");
    EXPECT_EQ(bizResp.status(), "SUCCESS");
    EXPECT_EQ(bizResp.result_code(), 200);
    EXPECT_NE(bizResp.result_details().find("radio_frame_payload_0x42"), std::string::npos);

    // 7. Clean teardown
    workerA.stop();
    workerB.stop();
}

TEST(MailboxDemoTest, TerminalToDUTwoDistinctResponses) {
    // 1. Setup Mailbox and Manager for Terminal
    TerminalMailbox terminalMailbox;
    TerminalMailboxManager terminalManager{terminalMailbox};
    MailboxWorker terminalWorker{terminalMailbox, terminalManager, nullptr};
    terminalWorker.start();

    // 2. Setup Mailbox and Manager for DU
    DistributedUnitMailbox duMailbox;
    DUMailboxManager duManager{duMailbox};

    // DU worker sends responses back to Terminal mailbox
    MailboxWorker duWorker{duMailbox, duManager, [&terminalMailbox](const mailbox::MailboxRequest &p_response) {
        terminalMailbox.enqueue(p_response);
    }};
    duWorker.start();

    // 3. Terminal creates TerminalRequest (from terminal.proto)
    terminal::TerminalRequest termReq;
    termReq.set_terminal_id("TERM01");
    termReq.set_action("CONFIG");
    termReq.set_parameters("freq=3.5GHz bw=100MHz power=43dBm");
    termReq.set_raw_command("config freq=3.5GHz bw=100MHz power=43dBm");

    mailbox::MailboxRequest reqEnvelope;
    reqEnvelope.set_request_id("REQ-TERM-001");
    reqEnvelope.set_source("Terminal");
    reqEnvelope.set_destination("DU");
    reqEnvelope.mutable_payload()->PackFrom(termReq);

    // 4. Send request to DU mailbox
    EXPECT_TRUE(duMailbox.enqueue(reqEnvelope));

    // 5. Verify Terminal receives distinct response 1: Validation Response
    EXPECT_TRUE(terminalManager.waitForResponse(2000));
    EXPECT_TRUE(terminalManager.hasReceivedValidation());
    auto valResp = terminalManager.getLastValidation();
    EXPECT_EQ(valResp.request_id(), "REQ-TERM-001");
    EXPECT_TRUE(valResp.is_valid());

    // 6. Verify Terminal receives distinct response 2: Business Logic Response
    EXPECT_TRUE(terminalManager.hasReceivedExecution());
    auto termResp = terminalManager.getLastTerminalResponse();
    EXPECT_EQ(termResp.request_id(), "REQ-TERM-001");
    EXPECT_TRUE(termResp.is_success());
    EXPECT_EQ(termResp.return_code(), 0);
    EXPECT_NE(termResp.output_text().find("DU configured successfully"), std::string::npos);

    // 7. Verify DU state updated
    EXPECT_EQ(duManager.getState(), "CONFIGURED");
    EXPECT_EQ(duManager.getCarrierFreq(), "freq=3.5GHz bw=100MHz power=43dBm");

    terminalWorker.stop();
    duWorker.stop();
}

TEST(MailboxDemoTest, TerminalToDUToRUFullMultiHop) {
    // 1. Setup Terminal Mailbox and Manager
    TerminalMailbox terminalMailbox;
    TerminalMailboxManager terminalManager{terminalMailbox};
    MailboxWorker terminalWorker{terminalMailbox, terminalManager, nullptr};
    terminalWorker.start();

    // 2. Setup RU Mailbox and Manager
    RadioUnitMailbox ruMailbox;
    RUMailboxManager ruManager{ruMailbox};

    // 3. Setup DU Mailbox and Manager
    DistributedUnitMailbox duMailbox;
    DUMailboxManager duManager{duMailbox};

    // DU worker routes RU-destined messages to RU mailbox, and Terminal-destined messages to Terminal mailbox
    MailboxWorker duWorker{duMailbox, duManager,
                           [&terminalMailbox, &ruMailbox](const mailbox::MailboxRequest &p_response) {
                               if (0 == p_response.destination().compare("RU") ||
                                   0 == p_response.destination().compare("RU01")) {
                                   ruMailbox.enqueue(p_response);
                               } else {
                                   terminalMailbox.enqueue(p_response);
                               }
                           }};
    duWorker.start();

    // RU worker routes replies back to DU mailbox
    MailboxWorker ruWorker{ruMailbox, ruManager,
                          [&duMailbox](const mailbox::MailboxRequest &p_response) {
                              duMailbox.enqueue(p_response);
                          }};
    ruWorker.start();

    // 4. Terminal creates a TerminalRequest targeting RU via DU
    terminal::TerminalRequest termReq;
    termReq.set_terminal_id("TERM01");
    termReq.set_action("START");
    termReq.set_parameters("tx_power=40 freq=3500 antennas=4");
    termReq.set_raw_command("ru start tx_power=40 freq=3500 antennas=4");
    termReq.set_target_node("RU");

    mailbox::MailboxRequest reqEnvelope;
    reqEnvelope.set_request_id("REQ-MULTI-001");
    reqEnvelope.set_source("Terminal");
    reqEnvelope.set_destination("DU");
    reqEnvelope.mutable_payload()->PackFrom(termReq);

    // 5. Enqueue request into DU mailbox (Terminal only talks to DU)
    EXPECT_TRUE(duMailbox.enqueue(reqEnvelope));

    // 6. Terminal receives execution response from RU relayed through DU
    EXPECT_TRUE(terminalManager.waitForResponse(3000));
    EXPECT_TRUE(terminalManager.hasReceivedExecution());

    auto termResp = terminalManager.getLastTerminalResponse();
    EXPECT_EQ(termResp.request_id(), "REQ-MULTI-001");
    EXPECT_TRUE(termResp.is_success());
    EXPECT_NE(termResp.output_text().find("[RU via DU]"), std::string::npos);
    EXPECT_NE(termResp.output_text().find("TX_ACTIVE"), std::string::npos);

    // 7. Verify RU frontend state was updated
    EXPECT_EQ(ruManager.getRfState(), "TX_ACTIVE");
    EXPECT_DOUBLE_EQ(ruManager.getTxPowerDbm(), 40.0);
    EXPECT_DOUBLE_EQ(ruManager.getCenterFreqMhz(), 3500.0);
    EXPECT_EQ(ruManager.getAntennaPorts(), 4);

    terminalWorker.stop();
    duWorker.stop();
    ruWorker.stop();
}
