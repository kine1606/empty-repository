#ifndef TERMINAL_MAILBOX_MANAGER_H
#define TERMINAL_MAILBOX_MANAGER_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "BaseMailboxManager.h"
#include "TerminalSupportChecker.h"
#include "TerminalValidator.h"
#include "du.pb.h"
#include "mailbox.pb.h"
#include "ru.pb.h"
#include "terminal.pb.h"

/**
 * @brief Mailbox manager implementation for the Terminal.
 *
 * Processes incoming validation and execution responses from the DU, notifies
 * waiting interactive threads, and supports callback output formatting.
 */
class TerminalMailboxManager : public BaseMailboxManager {
public:
    using outputCallback = std::function<void(const std::string &)>;

    explicit TerminalMailboxManager(Mailbox &p_mailbox);
    ~TerminalMailboxManager() = default;

    std::optional<mailbox::MailboxRequest>
    validateMessage(const mailbox::MailboxRequest &p_request) override;

    std::optional<mailbox::MailboxRequest>
    processBusinessLogic(const mailbox::MailboxRequest &p_request) override;

    void setOutputCallback(outputCallback p_callback);

    bool waitForResponse(int p_timeoutMs);
    bool hasReceivedValidation() const;
    bool hasReceivedExecution() const;

    mailbox::ValidationResponse getLastValidation() const;
    terminal::TerminalResponse getLastTerminalResponse() const;
    du::DUResponse getLastDUResponse() const;

    void reset();

protected:
    std::string getManagerName() const override {
        return "Terminal";
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
    TerminalValidator m_validator;
    TerminalSupportChecker m_supportChecker;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_receivedValidation{false};
    bool m_receivedExecution{false};

    mailbox::ValidationResponse m_lastValidation;
    terminal::TerminalResponse m_lastTerminalResponse;
    du::DUResponse m_lastDUResponse;

    outputCallback m_outputCallback;
};

#endif
