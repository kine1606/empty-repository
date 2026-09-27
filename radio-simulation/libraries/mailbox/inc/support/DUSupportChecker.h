#ifndef DU_SUPPORT_CHECKER_H
#define DU_SUPPORT_CHECKER_H

#include "ISupportChecker.h"

/**
 * @brief Evaluates whether an incoming mailbox request is intended for the DU node.
 */
class DUSupportChecker : public ISupportChecker {
public:
    DUSupportChecker() = default;
    ~DUSupportChecker() = default;

    /**
     * @brief Checks if destination matches the Distributed Unit.
     *
     * @param p_request Request to inspect.
     * @return true if destination is DU or DU01.
     */
    bool isSupported(const mailbox::MailboxRequest &p_request) override {
        return p_request.destination() == "DU" || p_request.destination() == "DU01";
    }
};

#endif
