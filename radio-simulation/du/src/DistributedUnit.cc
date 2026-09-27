#include "DistributedUnit.h"

#include <grpcpp/grpcpp.h>
#include <utility>

#include "Logger.h"
#include "NodeServiceImpl.h"

bool DistributedUnit::start(const std::string &p_bindAddress, const std::string &p_terminalAddress) {
    if (this->m_running) {
        return true;
    }

    INFO("[DistributedUnit] Starting DU on %1, targeting replies to %2",
         p_bindAddress, p_terminalAddress);

    // Setup client to Terminal
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(p_terminalAddress, grpc::InsecureChannelCredentials());
    this->m_clientToTerminal = std::make_shared<NodeClient>(channel);

    // Setup worker loop with callback to send responses back to terminal
    std::shared_ptr<NodeClient> client = this->m_clientToTerminal;
    this->m_worker = std::make_unique<MailboxWorker>(
        this->m_mailbox, this->m_manager, [client](const mailbox::MailboxRequest &p_response) {
            INFO("[DU Worker] Dispatching response for RequestId=%1 to %2",
                 p_response.request_id(), p_response.destination());
            if (client) {
                client->sendMessage(p_response);
            }
        });

    this->m_worker->start();

    // Start gRPC server
    auto sharedMailbox = std::shared_ptr<Mailbox>(&this->m_mailbox, [](Mailbox *) {
    });
    static NodeServiceImpl s_service{sharedMailbox};
    this->m_server = startGrpcServer(p_bindAddress, s_service);
    if (!this->m_server) {
        WARN("[DistributedUnit] Failed to start gRPC server on %1", p_bindAddress);
        this->m_worker->stop();
        return false;
    }

    this->m_running = true;
    INFO("[DistributedUnit] DU initialized and running successfully");
    return true;
}

void DistributedUnit::stop() {
    if (!this->m_running) {
        return;
    }

    INFO("[DistributedUnit] Stopping DU...");
    this->m_running = false;

    if (this->m_worker) {
        this->m_worker->stop();
    }

    if (this->m_server) {
        this->m_server->Shutdown();
    }
}

bool DistributedUnit::isRunning() const {
    return this->m_running;
}

DistributedUnitMailbox &DistributedUnit::getMailbox() {
    return this->m_mailbox;
}

DUMailboxManager &DistributedUnit::getManager() {
    return this->m_manager;
}
