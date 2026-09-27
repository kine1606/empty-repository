#include "gtest/gtest.h"

#include <chrono>
#include <memory>
#include <thread>

#include "Logger.h"
#include "Mailbox.h"
#include "MailboxWorker.h"
#include "NodeAManager.h"
#include "NodeBManager.h"
#include "mailbox.pb.h"

namespace {
struct MailboxDemoTestLogger {
    MailboxDemoTestLogger() {
        try {
            Logger::initialize("mailbox-demo-test");
        } catch (const std::logic_error &) {
            // Logger was already initialized by another test translation unit in this binary
        }
    }
};
MailboxDemoTestLogger g_demoLogger;
} // namespace

TEST(MailboxDemoTest, NodeAToNodeBTwoDistinctResponses) {
    // 1. Setup Mailbox and Manager for Node A
    Mailbox mailboxA;
    NodeAManager managerA(mailboxA);
    MailboxWorker workerA(mailboxA, managerA, nullptr);
    workerA.start();

    // 2. Setup Mailbox and Manager for Node B
    Mailbox mailboxB;
    NodeBManager managerB(mailboxB);

    // Node B's worker sends responses back to Node A's mailbox!
    // This demonstrates the mailbox-to-mailbox asynchronous return mechanism.
    MailboxWorker workerB(mailboxB, managerB, [&mailboxA](const mailbox::MailboxRequest &p_response) {
        mailboxA.enqueue(p_response);
    });
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
    const auto valResp = managerA.getLastValidation();
    EXPECT_EQ(valResp.request_id(), "REQ-TEST-001");
    EXPECT_TRUE(valResp.is_valid());
    EXPECT_EQ(valResp.error_code(), mailbox::ErrorCode::OK);
    EXPECT_EQ(valResp.message(), "NodeB validated NodeBRequest successfully");

    // 6. Verify Node A receives distinct response 2: Business Logic Response
    EXPECT_TRUE(managerA.waitForBusinessResponse(2000));
    EXPECT_TRUE(managerA.hasReceivedBusinessResponse());
    const auto bizResp = managerA.getLastBusinessResponse();
    EXPECT_EQ(bizResp.request_id(), "REQ-TEST-001");
    EXPECT_EQ(bizResp.status(), "SUCCESS");
    EXPECT_EQ(bizResp.result_code(), 200);
    EXPECT_NE(bizResp.result_details().find("radio_frame_payload_0x42"), std::string::npos);

    // 7. Clean teardown
    workerA.stop();
    workerB.stop();
}
