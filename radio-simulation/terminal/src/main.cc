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
    std::string singleCommand;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (0 == arg.compare("--du-address") && i + 1 < argc) {
            duAddress = argv[++i];
        } else if (0 == arg.compare("--bind-address") && i + 1 < argc) {
            bindAddress = argv[++i];
        } else if (0 == arg.compare("--cmd") && i + 1 < argc) {
            singleCommand = argv[++i];
        }
    }

    Terminal terminal;
    if (!terminal.start(bindAddress, duAddress)) {
        std::cerr << "Failed to start Terminal on " << bindAddress << std::endl;
        return 1;
    }

    if (!singleCommand.empty()) {
        std::string response = terminal.sendCommand(singleCommand);
        std::cout << response << std::endl;
    } else {
        terminal.runInteractive("DU-Terminal> ");
    }

    terminal.stop();
    return 0;
}