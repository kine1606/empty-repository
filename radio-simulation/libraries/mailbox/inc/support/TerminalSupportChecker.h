#ifndef TERMINAL_SUPPORT_CHECKER_H
#define TERMINAL_SUPPORT_CHECKER_H

#include "ISupportChecker.h"

/**
 * @brief Evaluates whether an incoming mailbox request is intended for the Terminal node.
 */
class TerminalSupportChecker : public ISupportChecker {
public:
    TerminalSupportChecker() = default;
    ~TerminalSupportChecker() = default;

    /**
     * @brief Checks if destination matches the Terminal.
     *
     * @param p_request Request to inspect.
     * @return true if destination is Terminal or TERM01.
     */
    bool isSupported(const mailbox::MailboxRequest &p_request) override {
        return p_request.destination() == "Terminal" || p_request.destination() == "TERM01";
    }
};

#endif
