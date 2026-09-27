#ifndef DU_MAILBOX_MANAGER_H
#define DU_MAILBOX_MANAGER_H

#include <mutex>
#include <string>

#include "BaseMailboxManager.h"
#include "DUSupportChecker.h"
#include "DUValidator.h"
#include "du.pb.h"
#include "terminal.pb.h"

/**
 * @brief Mailbox manager implementation for the Distributed Unit (DU).
 *
 * Coordinates request validation and business logic execution for radio operations
 * including configuration, status reporting, and cell start/stop lifecycles.
 */
class DUMailboxManager : public BaseMailboxManager {
public:
    explicit DUMailboxManager(Mailbox &p_mailbox);
    ~DUMailboxManager() = default;

    std::optional<mailbox::MailboxRequest>
    validateMessage(const mailbox::MailboxRequest &p_request) override;

    std::optional<mailbox::MailboxRequest>
    processBusinessLogic(const mailbox::MailboxRequest &p_request) override;

    std::string getState() const;
    std::string getCarrierFreq() const;
    double getBandwidthMhz() const;
    double getTxPowerDbm() const;

protected:
    std::string getManagerName() const override {
        return "DU";
    }

    IMailboxValidator &getValidator() override {
        return this->m_validator;
    }

    ISupportChecker &getSupportChecker() override {
        return this->m_supportChecker;
    }

    mailbox::MailboxRequest
    buildResponse(const mailbox::MailboxRequest &p_request) override;

private:
    DUValidator m_validator;
    DUSupportChecker m_supportChecker;

    mutable std::mutex m_stateMutex;
    std::string m_state{"IDLE"};
    std::string m_cellId{"CELL-001"};
    std::string m_carrierFreq{"3.5GHz"};
    double m_bandwidthMhz{100.0};
    double m_txPowerDbm{43.0};
};

#endif
