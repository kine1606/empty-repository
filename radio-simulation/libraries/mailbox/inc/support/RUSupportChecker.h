#ifndef RU_SUPPORT_CHECKER_H
#define RU_SUPPORT_CHECKER_H

#include "ISupportChecker.h"

/**
 * @brief Evaluates whether an incoming mailbox request is intended for the RU node.
 */
class RUSupportChecker : public ISupportChecker {
public:
    RUSupportChecker() = default;
    ~RUSupportChecker() = default;

    /**
     * @brief Checks if destination matches the Radio Unit.
     *
     * @param p_request Request to inspect.
     * @return true if destination is RU or RU01.
     */
    bool isSupported(const mailbox::MailboxRequest &p_request) override {
        return p_request.destination() == "RU" || p_request.destination() == "RU01";
    }
};

#endif
