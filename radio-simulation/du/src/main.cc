#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "GrpcServer.h"
#include "Logger.h"
#include "Mailbox.h"
#include "MailboxWorker.h"
#include "NodeAManager.h"
#include "NodeBManager.h"
#include "NodeClient.h"
#include "NodeServiceImpl.h"
#include "mailbox.grpc.pb.h"
#include "mailbox.pb.h"

int main() {
    Logger::initialize("node-demo");

    std::cout << "============================================================\n";
    std::cout << "   Radio Simulation: gRPC + Mailbox Communication Demo      \n";
    std::cout << "   Node A (Requester) <---> Node B (Processor)              \n";
    std::cout << "============================================================\n\n";

    const std::string NODE_A_ADDRESS = "127.0.0.1:50051";
    const std::string NODE_B_ADDRESS = "127.0.0.1:50052";

    // -------------------------------------------------------------------------
    // 1. Initialize Node A Infrastructure
    // -------------------------------------------------------------------------
    std::cout << "[Step 1] Initializing Node A...\n";
    std::shared_ptr<Mailbox> mailboxA = std::make_shared<Mailbox>();
    NodeServiceImpl serviceA{mailboxA};
    std::unique_ptr<grpc::Server> serverA = startGrpcServer(NODE_A_ADDRESS, serviceA);
    if (!serverA) {
        std::cerr << "Failed to start gRPC server for Node A on " << NODE_A_ADDRESS << std::endl;
        return 1;
    }
    std::cout << "  -> Node A gRPC server listening on " << NODE_A_ADDRESS << "\n";

    NodeAManager managerA{*mailboxA};
    // Node A's worker consumes incoming responses from Node B.
    // It does not need to send further responses, so callback is nullptr.
    MailboxWorker workerA{*mailboxA, managerA, nullptr};
    workerA.start();
    std::cout << "  -> Node A MailboxWorker started.\n\n";

    // -------------------------------------------------------------------------
    // 2. Initialize Node B Infrastructure
    // -------------------------------------------------------------------------
    std::cout << "[Step 2] Initializing Node B...\n";
    std::shared_ptr<Mailbox> mailboxB = std::make_shared<Mailbox>();
    NodeServiceImpl serviceB{mailboxB};
    std::unique_ptr<grpc::Server> serverB = startGrpcServer(NODE_B_ADDRESS, serviceB);
    if (!serverB) {
        std::cerr << "Failed to start gRPC server for Node B on " << NODE_B_ADDRESS << std::endl;
        return 1;
    }
    std::cout << "  -> Node B gRPC server listening on " << NODE_B_ADDRESS << "\n";

    // Node B needs a client connected to Node A's gRPC server to send responses
    // back to Node A's mailbox instead of returning them directly in the RPC.
    std::shared_ptr<grpc::Channel> channelToA =
        grpc::CreateChannel(NODE_A_ADDRESS, grpc::InsecureChannelCredentials());
    std::shared_ptr<NodeClient> clientToA = std::make_shared<NodeClient>(channelToA);

    NodeBManager managerB{*mailboxB};

    // Node B's workerLoop will call m_sendResponse twice:
    //  1. Distinct response #1: Validation response
    //  2. Distinct response #2: Business logic response after internal processing
    MailboxWorker workerB{*mailboxB, managerB, [clientToA](const mailbox::MailboxRequest &p_response) {
        std::cout << "[Node B Worker Loop] Dispatching distinct response to "
                  << p_response.destination()
                  << " via gRPC SendMessage() [RequestId="
                  << p_response.request_id() << "]...\n";
        clientToA->sendMessage(p_response);
    }};
    workerB.start();
    std::cout << "  -> Node B MailboxWorker started.\n\n";

    // -------------------------------------------------------------------------
    // 3. Node A creates and sends request to Node B
    // -------------------------------------------------------------------------
    std::cout << "[Step 3] Node A preparing NodeBRequest...\n";
    std::shared_ptr<grpc::Channel> channelToB =
        grpc::CreateChannel(NODE_B_ADDRESS, grpc::InsecureChannelCredentials());
    NodeClient clientToB{channelToB};

    mailbox::NodeBRequest nodeBReq;
    nodeBReq.set_command("CONFIGURE_RADIO_FREQUENCY");
    nodeBReq.set_transaction_id(777);
    nodeBReq.set_data("carrier_freq=3.5GHz; bandwidth=100MHz; tx_power=43dBm");

    mailbox::MailboxRequest requestEnvelope;
    requestEnvelope.set_request_id("REQ-TX-777");
    requestEnvelope.set_source("NodeA");
    requestEnvelope.set_destination("NodeB");
    requestEnvelope.mutable_payload()->PackFrom(nodeBReq);

    std::cout << "  -> RequestId:       " << requestEnvelope.request_id() << "\n";
    std::cout << "  -> Source:          " << requestEnvelope.source() << "\n";
    std::cout << "  -> Destination:     " << requestEnvelope.destination() << "\n";
    std::cout << "  -> Payload Type:    NodeBRequest\n";
    std::cout << "  -> Payload Command: " << nodeBReq.command() << "\n";
    std::cout << "  -> Payload Data:    " << nodeBReq.data() << "\n\n";

    std::cout << "[Step 4] Node A sending request to Node B via gRPC (expects Empty response)...\n";
    clientToB.sendMessage(requestEnvelope);
    std::cout << "  -> gRPC SendMessage completed. Node B enqueued request to its Mailbox.\n\n";

    // -------------------------------------------------------------------------
    // 4. Node A awaits the two distinct asynchronous responses from Node B
    // -------------------------------------------------------------------------
    std::cout << "[Step 5] Node A waiting for Distinct Response #1 (Validation Response)...\n";
    bool gotValidation = managerA.waitForValidation(3000);
    if (!gotValidation) {
        std::cerr << "Timeout waiting for validation response from Node B!\n";
        workerA.stop();
        workerB.stop();
        serverA->Shutdown();
        serverB->Shutdown();
        return 1;
    }
    mailbox::ValidationResponse valResp = managerA.getLastValidation();
    std::cout << "  [SUCCESS] Received Validation Response!\n";
    std::cout << "     - RequestId: " << valResp.request_id() << "\n";
    std::cout << "     - Is Valid:  " << (valResp.is_valid() ? "true" : "false") << "\n";
    std::cout << "     - Message:   " << valResp.message() << "\n\n";

    std::cout << "[Step 6] Node A waiting for Distinct Response #2 (Business Logic Response)...\n";
    bool gotBusiness = managerA.waitForBusinessResponse(3000);
    if (!gotBusiness) {
        std::cerr << "Timeout waiting for business logic response from Node B!\n";
        workerA.stop();
        workerB.stop();
        serverA->Shutdown();
        serverB->Shutdown();
        return 1;
    }
    mailbox::NodeAResponse bizResp = managerA.getLastBusinessResponse();
    std::cout << "  [SUCCESS] Received Business Logic Response!\n";
    std::cout << "     - RequestId: " << bizResp.request_id() << "\n";
    std::cout << "     - Status:    " << bizResp.status() << "\n";
    std::cout << "     - Code:      " << bizResp.result_code() << "\n";
    std::cout << "     - Details:   " << bizResp.result_details() << "\n\n";

    // -------------------------------------------------------------------------
    // 5. Clean Shutdown
    // -------------------------------------------------------------------------
    std::cout << "[Step 7] Shutting down Node A and Node B workers and gRPC servers...\n";
    workerA.stop();
    workerB.stop();

    serverA->Shutdown();
    serverB->Shutdown();

    std::cout << "============================================================\n";
    std::cout << "   Demo Completed Successfully!                             \n";
    std::cout << "============================================================\n";

    return 0;
}