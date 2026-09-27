#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

#include "Logger.h"
#include "Terminal.h"

int main(int argc, char *argv[]) {
    try {
        Logger::initialize("terminal-app");
    } catch (...) {
        // Already initialized
    }

    std::string duAddress = "127.0.0.1:50051";
    std::string bindAddress = "127.0.0.1:50054";
    std::string targetNode = "DU";
    std::string singleCommand;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (0 == arg.compare("--du-address") && i + 1 < argc) {
            duAddress = argv[++i];
        } else if (0 == arg.compare("--bind-address") && i + 1 < argc) {
            bindAddress = argv[++i];
        } else if (0 == arg.compare("--target") && i + 1 < argc) {
            targetNode = argv[++i];
            std::transform(targetNode.begin(), targetNode.end(), targetNode.begin(), ::toupper);
        } else if (0 == arg.compare("--cmd") && i + 1 < argc) {
            singleCommand = argv[++i];
        } else if (0 == arg.compare("--help") || 0 == arg.compare("-h")) {
            std::cout << "Usage: terminal_simulation [OPTIONS]\n"
                      << "  --du-address <ADDR>    DU address (default: 127.0.0.1:50051)\n"
                      << "  --bind-address <ADDR>  Local listening address (default: 127.0.0.1:50054)\n"
                      << "  --target <DU|RU>       Target node for commands (default: DU)\n"
                      << "  --cmd <COMMAND>        Execute single command and exit\n";
            return 0;
        }
    }

    Terminal terminal;
    if (!terminal.start(bindAddress, duAddress)) {
        std::cerr << "Failed to start Terminal on " << bindAddress << std::endl;
        return 1;
    }

    if (!singleCommand.empty()) {
        std::string response = terminal.sendCommand(singleCommand, targetNode);
        std::cout << response << std::endl;
    } else {
        terminal.runInteractive(targetNode);
    }

    terminal.stop();
    return 0;
}