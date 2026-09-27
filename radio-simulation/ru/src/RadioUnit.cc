#include "RadioUnit.h"

#include <grpcpp/grpcpp.h>
#include <utility>

#include "Logger.h"
#include "NodeServiceImpl.h"

bool RadioUnit::start(const std::string &p_bindAddress, const std::string &p_duAddress) {
    if (this->m_running) {
        return true;
    }

    INFO("[RadioUnit] Starting RU on %1, targeting replies to %2",
         p_bindAddress, p_duAddress);

    // Setup client to DU
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(p_duAddress, grpc::InsecureChannelCredentials());
    this->m_clientToDU = std::make_shared<NodeClient>(channel);

    // Setup worker loop with callback to send responses back to DU
    this->m_worker = std::make_unique<MailboxWorker>(
        this->m_mailbox, this->m_manager,
        [this](const mailbox::MailboxRequest &p_response) {
            INFO("[RU Worker] Dispatching response for RequestId=%1 to %2",
                 p_response.request_id(), p_response.destination());
            this->m_clientToDU->sendMessage(p_response);
        });
    this->m_worker->start();

    // Start gRPC server
    auto sharedMailbox = std::shared_ptr<Mailbox>(&this->m_mailbox, [](Mailbox *) {
    });
    static NodeServiceImpl s_service{sharedMailbox};
    this->m_server = startGrpcServer(p_bindAddress, s_service);
    if (!this->m_server) {
        WARN("[RadioUnit] Failed to start gRPC server on %1", p_bindAddress);
        this->m_worker->stop();
        return false;
    }

    this->m_running = true;
    INFO("[RadioUnit] RU initialized and running successfully");
    return true;
}

void RadioUnit::stop() {
    if (!this->m_running) {
        return;
    }
    this->m_running = false;
    INFO("[RadioUnit] Stopping RU...");
    if (this->m_worker) {
        this->m_worker->stop();
    }
    if (this->m_server) {
        this->m_server->Shutdown();
    }
}

bool RadioUnit::isRunning() const {
    return this->m_running;
}

RadioUnitMailbox &RadioUnit::getMailbox() {
    return this->m_mailbox;
}

RUMailboxManager &RadioUnit::getManager() {
    return this->m_manager;
}
