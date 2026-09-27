#ifndef RU_MAILBOX_MANAGER_H
#define RU_MAILBOX_MANAGER_H

#include <mutex>
#include <string>

#include "BaseMailboxManager.h"
#include "RUSupportChecker.h"
#include "RUValidator.h"
#include "ru.pb.h"
#include "terminal.pb.h"

/**
 * @brief Mailbox manager implementation for the Radio Unit (RU).
 *
 * Coordinates RF frontend hardware state, transmission power, frequency,
 * and antenna port status, returning detailed metrics in RUResponse.
 */
class RUMailboxManager : public BaseMailboxManager {
public:
    explicit RUMailboxManager(Mailbox &p_mailbox);
    ~RUMailboxManager() = default;

    std::optional<mailbox::MailboxRequest>
    validateMessage(const mailbox::MailboxRequest &p_request) override;

    std::optional<mailbox::MailboxRequest>
    processBusinessLogic(const mailbox::MailboxRequest &p_request) override;

    std::string getRfState() const;
    double getTxPowerDbm() const;
    double getCenterFreqMhz() const;
    int32_t getAntennaPorts() const;

protected:
    std::string getManagerName() const override {
        return "RU";
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
    RUValidator m_validator;
    RUSupportChecker m_supportChecker;

    mutable std::mutex m_stateMutex;
    std::string m_rfState{"STANDBY"};
    double m_txPowerDbm{40.0};
    double m_centerFreqMhz{3500.0};
    int32_t m_antennaPorts{4};
    std::string m_unitId{"RU-01"};
};

#endif
