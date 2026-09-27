#include "Terminal.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <utility>

#include <grpcpp/grpcpp.h>
#include <readline/history.h>
#include <readline/readline.h>

#include "Logger.h"
#include "NodeServiceImpl.h"
#include "terminal.pb.h"

bool Terminal::start(const std::string &p_bindAddress, const std::string &p_duAddress) {
    if (this->m_running) {
        return true;
    }

    this->m_bindAddress = p_bindAddress;
    this->m_duAddress = p_duAddress;

    INFO("[Terminal] Starting Terminal on %1, connecting to DU at %2",
         p_bindAddress, p_duAddress);

    // Setup client to DU
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(p_duAddress, grpc::InsecureChannelCredentials());
    this->m_clientToDU = std::make_shared<NodeClient>(channel);

    // Terminal worker consumes incoming responses; no further replies needed
    this->m_worker = std::make_unique<MailboxWorker>(this->m_mailbox, this->m_manager, nullptr);
    this->m_worker->start();

    // Start gRPC server to receive replies from DU
    auto sharedMailbox = std::shared_ptr<Mailbox>(&this->m_mailbox, [](Mailbox *) {
    });
    static NodeServiceImpl s_service{sharedMailbox};
    this->m_server = startGrpcServer(p_bindAddress, s_service);
    if (!this->m_server) {
        WARN("[Terminal] Failed to start gRPC server on %1", p_bindAddress);
        this->m_worker->stop();
        return false;
    }

    this->m_running = true;
    INFO("[Terminal] Terminal initialized and running");
    return true;
}

void Terminal::stop() {
    if (!this->m_running) {
        return;
    }

    INFO("[Terminal] Stopping Terminal...");
    this->m_running = false;

    if (this->m_worker) {
        this->m_worker->stop();
    }

    if (this->m_server) {
        this->m_server->Shutdown();
    }
}

std::string Terminal::sendCommand(const std::string &p_command,
                                  const std::string &p_target,
                                  int p_timeoutMs) {
    if (!this->m_running || !this->m_clientToDU) {
        return "Error: Terminal is not running or connected to DU.";
    }

    this->m_manager.reset();
    ++this->m_sequenceNumber;

    std::string reqId = "REQ-TERM-" + std::to_string(this->m_sequenceNumber);

    // Parse action and parameters
    std::istringstream iss(p_command);
    std::string action;
    std::string params;
    iss >> action;
    std::getline(iss, params);
    if (!params.empty() && params[0] == ' ') {
        params = params.substr(1);
    }

    // Determine target node: prioritize command prefix if specified, else use p_target
    std::string targetNode = p_target;
    if (p_command.rfind("ru ", 0) == 0 ||
        p_command.rfind("RU ", 0) == 0 ||
        0 == p_command.compare("ru") ||
        0 == p_command.compare("RU")) {
        targetNode = "RU";
    }

    terminal::TerminalRequest termReq;
    termReq.set_terminal_id("TERM01");
    termReq.set_raw_command(p_command);
    termReq.set_action(action);
    termReq.set_parameters(params);
    termReq.set_target_node(targetNode);
    termReq.set_timestamp_epoch_ms(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    mailbox::MailboxRequest envelope;
    envelope.set_request_id(reqId);
    envelope.set_source("Terminal");
    envelope.set_destination("DU");
    envelope.mutable_payload()->PackFrom(termReq);

    INFO("[Terminal] Sending command '%1' (target=%2, RequestId=%3) to DU",
         p_command, targetNode, reqId);
    this->m_clientToDU->sendMessage(envelope);

    // Wait for business execution response
    bool received = this->m_manager.waitForResponse(p_timeoutMs);
    if (!received) {
        return "Timeout: No response received from DU within " + std::to_string(p_timeoutMs) + " ms.";
    }

    auto termResp = this->m_manager.getLastTerminalResponse();
    if (termResp.ByteSizeLong() > 0) {
        return termResp.output_text();
    }

    auto duResp = this->m_manager.getLastDUResponse();
    if (duResp.ByteSizeLong() > 0) {
        return duResp.details();
    }

    return "Response received from DU (empty payload).";
}

void Terminal::runInteractive(const std::string &p_defaultTarget, const std::string &p_prompt) {
    // Explicitly configure GNU Readline with Emacs editing mode
    rl_variable_bind("editing-mode", "emacs");
    rl_initialize();

    std::string currentTarget = p_defaultTarget.empty() ? "DU" : p_defaultTarget;

    std::cout << "============================================================\n"
              << "       5G Radio Simulation Interactive Terminal             \n"
              << "     GNU Readline Emacs Mode Active (Ctrl+A, Ctrl+E, etc.)  \n"
              << "============================================================\n"
              << "  -> Active Target Node:  " << currentTarget << "\n"
              << "  -> Routed Interface:    DU (127.0.0.1:50051)\n"
              << "Commands:\n"
              << "  target <du|ru>       - Switch active target node\n"
              << "  ru <command>         - Route command specifically to RU via DU\n"
              << "  status / start / stop- Send command to active target\n"
              << "  help                 - Display help menu\n"
              << "  exit / quit / Ctrl+D - Exit terminal\n"
              << "============================================================\n\n";

    char *line = nullptr;
    while (true) {
        std::string promptStr = p_prompt.empty() ? (currentTarget + "-Terminal> ") : p_prompt;
        line = readline(promptStr.c_str());
        if (!line) {
            break;
        }

        std::string input = line;
        free(line);

        // Trim leading and trailing whitespace
        size_t first = input.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }
        size_t last = input.find_last_not_of(" \t\r\n");
        input = input.substr(first, last - first + 1);

        if (input.empty()) {
            continue;
        }

        add_history(input.c_str());

        if (0 == input.compare("exit") || 0 == input.compare("quit")) {
            std::cout << "Exiting terminal...\n";
            break;
        }

        if (input.rfind("target ", 0) == 0) {
            std::string newTarget = input.substr(7);
            size_t tfirst = newTarget.find_first_not_of(" \t");
            if (tfirst != std::string::npos) {
                newTarget = newTarget.substr(tfirst);
            }
            std::transform(newTarget.begin(), newTarget.end(), newTarget.begin(), ::toupper);
            if (0 == newTarget.compare("DU") || 0 == newTarget.compare("RU")) {
                currentTarget = newTarget;
                std::cout << "Target node switched to: " << currentTarget << "\n";
            } else {
                std::cout << "Invalid target: '" << newTarget << "'. Supported: DU, RU.\n";
            }
            continue;
        }

        std::string result = this->sendCommand(input, currentTarget);
        std::cout << result << "\n";
    }
}

bool Terminal::isRunning() const {
    return this->m_running;
}

TerminalMailbox &Terminal::getMailbox() {
    return this->m_mailbox;
}

TerminalMailboxManager &Terminal::getManager() {
    return this->m_manager;
}
