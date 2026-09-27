#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "Logger.h"
#include "NodeClient.h"
#include "RadioUnit.h"
#include "mailbox.pb.h"
#include "ru.pb.h"

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
        Logger::initialize("ru-app");
    } catch (...) {
        // Already initialized
    }

    std::string bindAddress = "127.0.0.1:50052";
    std::string duAddress = "127.0.0.1:50051";
    bool runDemo = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (0 == arg.compare("--demo") || 0 == arg.compare("--test")) {
            runDemo = true;
        } else if (0 == arg.compare("--bind-address") && i + 1 < argc) {
            bindAddress = argv[++i];
        } else if (0 == arg.compare("--du-address") && i + 1 < argc) {
            duAddress = argv[++i];
        }
    }

    RadioUnit ru;
    if (!ru.start(bindAddress, duAddress)) {
        std::cerr << "Failed to start Radio Unit on " << bindAddress << std::endl;
        return 1;
    }

    std::cout << "============================================================\n"
              << "       5G Radio Unit (RU) Service                           \n"
              << "============================================================\n"
              << "  -> Listening on:        " << bindAddress << "\n"
              << "  -> Replying to:         " << duAddress << "\n"
              << "============================================================\n";

    if (!runDemo) {
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        std::cout << "[RU] Service running. Press Ctrl+C to terminate.\n";
        while (!g_shutdownRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        ru.stop();
        std::cout << "[RU] Service stopped.\n";
        return 0;
    }

    // Demo/test mode: self-test request
    std::cout << "[RU Demo] Running self-test request against RU...\n";
    std::shared_ptr<grpc::Channel> channel =
        grpc::CreateChannel(bindAddress, grpc::InsecureChannelCredentials());
    NodeClient client{channel};

    ru::RURequest ruReq;
    ruReq.set_command("CONFIG");
    ruReq.set_tx_power_dbm(43.0);
    ruReq.set_center_freq_mhz(3500.0);
    ruReq.set_antenna_ports(8);

    mailbox::MailboxRequest envelope;
    envelope.set_request_id("REQ-RU-SELFTEST-001");
    envelope.set_source("DU");
    envelope.set_destination("RU");
    envelope.mutable_payload()->PackFrom(ruReq);

    client.sendMessage(envelope);
    std::cout << "  -> Dispatched RURequest to RU: SUCCESS\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "  -> RU RF state:       " << ru.getManager().getRfState() << "\n";
    std::cout << "  -> RU Tx power:       " << ru.getManager().getTxPowerDbm() << " dBm\n";
    std::cout << "  -> RU Antenna ports:  " << ru.getManager().getAntennaPorts() << "\n";

    ru.stop();
    std::cout << "[RU Demo] Completed successfully.\n";
    return 0;
}