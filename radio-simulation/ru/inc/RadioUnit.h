#ifndef RADIO_UNIT_H
#define RADIO_UNIT_H

#include <memory>
#include <string>

#include "GrpcServer.h"
#include "MailboxWorker.h"
#include "NodeClient.h"
#include "RUMailboxManager.h"
#include "RadioUnitMailbox.h"

/**
 * @brief Represents the Radio Unit (RU) node in the radio simulation.
 *
 * Runs the RU gRPC server on 127.0.0.1:50052, processes RF configuration
 * and transmission requests, and dispatches responses back to the DU's mailbox.
 */
class RadioUnit {
public:
    using workerPtr = std::unique_ptr<MailboxWorker>;
    using serverPtr = std::unique_ptr<grpc::Server>;
    using clientPtr = std::shared_ptr<NodeClient>;

    RadioUnit() = default;
    ~RadioUnit() = default;

    /**
     * @brief Starts the RU services, listening on @p p_bindAddress and
     * configuring outbound replies to @p p_duAddress.
     *
     * @param p_bindAddress Listening address (e.g., "127.0.0.1:50052").
     * @param p_duAddress Remote DU address (e.g., "127.0.0.1:50051").
     * @return true if initialization and server start succeeded.
     */
    bool start(const std::string &p_bindAddress, const std::string &p_duAddress);

    /**
     * @brief Stops worker thread, closes outbound channels, and terminates server.
     */
    void stop();

    bool isRunning() const;

    RadioUnitMailbox &getMailbox();
    RUMailboxManager &getManager();

private:
    RadioUnitMailbox m_mailbox;
    RUMailboxManager m_manager{this->m_mailbox};
    workerPtr m_worker;
    serverPtr m_server;
    clientPtr m_clientToDU;
    bool m_running{false};
};

#endif
