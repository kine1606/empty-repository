#ifndef DISTRIBUTED_UNIT_H
#define DISTRIBUTED_UNIT_H

#include <memory>
#include <string>

#include "DistributedUnitMailbox.h"
#include "DUMailboxManager.h"
#include "GrpcServer.h"
#include "MailboxWorker.h"
#include "NodeClient.h"

/**
 * @brief Represents the Distributed Unit (DU) node in the radio simulation.
 *
 * Manages the DU gRPC server, mailbox, mailbox worker thread, and outbound
 * gRPC client for dispatching asynchronous responses back to caller mailboxes.
 */
class DistributedUnit {
public:
    using workerPtr = std::unique_ptr<MailboxWorker>;
    using serverPtr = std::unique_ptr<grpc::Server>;
    using clientPtr = std::shared_ptr<NodeClient>;

    DistributedUnit() = default;
    ~DistributedUnit() = default;

    /**
     * @brief Starts the DU services, listening on @p p_bindAddress and
     * configuring outbound replies to @p p_terminalAddress.
     *
     * @param p_bindAddress Listening address (e.g., "127.0.0.1:50051").
     * @param p_terminalAddress Target address for replies (e.g., "127.0.0.1:50054").
     * @return true if initialization and server start succeeded.
     */
    bool start(const std::string &p_bindAddress, const std::string &p_terminalAddress);

    /**
     * @brief Stops worker thread, closes outbound channels, and terminates server.
     */
    void stop();

    bool isRunning() const;

    DistributedUnitMailbox &getMailbox();
    DUMailboxManager &getManager();

private:
    DistributedUnitMailbox m_mailbox;
    DUMailboxManager m_manager{this->m_mailbox};
    workerPtr m_worker;
    serverPtr m_server;
    clientPtr m_clientToTerminal;
    bool m_running{false};
};

#endif
