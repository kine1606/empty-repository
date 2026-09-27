#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "DistributedUnit.h"
#include "Logger.h"
#include "NodeClient.h"
#include "du.pb.h"
#include "mailbox.pb.h"
#include "terminal.pb.h"

namespace {
std::atomic<bool> g_shutdownRequested{false};

void signalHandler(int p_signal) {
    if (p_signal == SIGINT || p_signal == SIGTERM) {
        g_shutdownRequested = true;
    }
}
}

int main(int argc, char *argv[]) {
    try {
        Logger::initialize("du-app");
    } catch (...) {
        // Already initialized
    }

    std::string bindAddress = "127.0.0.1:50051";
    std::string terminalAddress = "127.0.0.1:50054";
    std::string ruAddress = "127.0.0.1:50052";
    bool runDemo = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (0 == arg.compare("--demo") || 0 == arg.compare("--test")) {
            runDemo = true;
        } else if (0 == arg.compare("--bind-address") && i + 1 < argc) {
            bindAddress = argv[++i];
        } else if (0 == arg.compare("--terminal-address") && i + 1 < argc) {
            terminalAddress = argv[++i];
        } else if (0 == arg.compare("--ru-address") && i + 1 < argc) {
            ruAddress = argv[++i];
        }
    }

    DistributedUnit du;
    if (!du.start(bindAddress, terminalAddress, ruAddress)) {
        std::cerr << "Failed to start Distributed Unit on " << bindAddress << std::endl;
        return 1;
    }

    std::cout << "============================================================\n"
              << "       5G Distributed Unit (DU) Service                     \n"
              << "============================================================\n"
              << "  -> Listening on:        " << bindAddress << "\n"
              << "  -> Replying to:         " << terminalAddress << "\n"
              << "  -> Connected to RU:     " << ruAddress << "\n"
              << "============================================================\n";

    if (!runDemo) {
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        std::cout << "[DU] Service running. Press Ctrl+C to terminate.\n";
        while (!g_shutdownRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        du.stop();
        std::cout << "[DU] Service stopped.\n";
        return 0;
    }

    // Demo/test mode: Simulate a test request to DU
    std::cout << "[DU Demo] Running self-test request against DU...\n";
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(bindAddress, grpc::InsecureChannelCredentials());
    NodeClient client{channel};

    terminal::TerminalRequest termReq;
    termReq.set_terminal_id("TEST-TERM");
    termReq.set_action("CONFIG");
    termReq.set_parameters("freq=3.5GHz bw=100MHz power=43dBm");
    termReq.set_raw_command("config freq=3.5GHz bw=100MHz power=43dBm");

    mailbox::MailboxRequest envelope;
    envelope.set_request_id("REQ-SELFTEST-001");
    envelope.set_source("Terminal");
    envelope.set_destination("DU");
    envelope.mutable_payload()->PackFrom(termReq);
    client.sendMessage(envelope);
    std::cout << "  -> Dispatched TerminalRequest to DU: SUCCESS\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "  -> DU current state: " << du.getManager().getState() << "\n";
    std::cout << "  -> DU carrier freq:  " << du.getManager().getCarrierFreq() << "\n";

    du.stop();
    std::cout << "[DU Demo] Completed successfully.\n";
    return 0;
}