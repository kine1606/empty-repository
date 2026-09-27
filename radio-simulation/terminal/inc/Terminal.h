#ifndef TERMINAL_H
#define TERMINAL_H

#include <memory>
#include <string>

#include "GrpcServer.h"
#include "MailboxWorker.h"
#include "NodeClient.h"
#include "TerminalMailbox.h"
#include "TerminalMailboxManager.h"

/**
 * @brief Represents the interactive Terminal console for the radio simulation.
 *
 * Provides a GNU Readline REPL with Emacs editing mode, dispatches TerminalRequest
 * messages to the DU via gRPC, and receives asynchronous replies back into its mailbox.
 */
class Terminal {
public:
    using workerPtr = std::unique_ptr<MailboxWorker>;
    using serverPtr = std::unique_ptr<grpc::Server>;
    using clientPtr = std::shared_ptr<NodeClient>;

    Terminal() = default;
    ~Terminal() = default;

    /**
     * @brief Starts the Terminal node, listening on @p p_bindAddress and connecting
     * to the DU at @p p_duAddress.
     *
     * @param p_bindAddress Local listening address (e.g., "127.0.0.1:50054").
     * @param p_duAddress Remote DU address (e.g., "127.0.0.1:50051").
     * @return true if initialization succeeded.
     */
    bool start(const std::string &p_bindAddress, const std::string &p_duAddress);

    /**
     * @brief Stops worker thread and gRPC server.
     */
    void stop();

    /**
     * @brief Sends a single command line to DU, waits for reply, and returns output.
     *
     * @param p_command Command string (e.g. "status", "start", "config freq=3.5GHz").
     * @param p_timeoutMs Wait timeout in milliseconds.
     * @return Output response string received from DU.
     */
    std::string sendCommand(const std::string &p_command, int p_timeoutMs = 3000);

    /**
     * @brief Runs the interactive GNU Readline REPL loop in Emacs editing mode.
     *
     * @param p_prompt Custom shell prompt string.
     */
    void runInteractive(const std::string &p_prompt = "DU-Terminal> ");

    bool isRunning() const;
    TerminalMailbox &getMailbox();
    TerminalMailboxManager &getManager();

private:
    TerminalMailbox m_mailbox;
    TerminalMailboxManager m_manager{this->m_mailbox};
    workerPtr m_worker;
    serverPtr m_server;
    clientPtr m_clientToDU;
    std::string m_duAddress;
    std::string m_bindAddress;
    bool m_running{false};
    uint64_t m_sequenceNumber{0};
};

#endif
