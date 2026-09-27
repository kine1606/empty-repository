#ifndef NODE_A_MANAGER_H
#define NODE_A_MANAGER_H

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

#include "BaseMailboxManager.h"
#include "NodeSupportChecker.h"
#include "NodeValidator.h"
#include "mailbox.pb.h"

/**
 * @brief Mailbox manager for Node A.
 *
 * Processes incoming validation and business responses sent back from Node B.
 */
class NodeAManager : public BaseMailboxManager {
public:
    explicit NodeAManager(Mailbox &p_mailbox);
    ~NodeAManager() override = default;

    /**
     * @brief Inspects incoming messages in Node A's mailbox.
     *
     * Detects ValidationResponse from Node B.
     * Returns nullopt so Node A does not send a reply back.
     */
    std::optional<mailbox::MailboxRequest>
    validateMessage(const mailbox::MailboxRequest &p_request) override;

    /**
     * @brief Processes business responses from Node B.
     *
     * Detects NodeAResponse from Node B.
     * Returns nullopt so Node A does not send a reply back.
     */
    std::optional<mailbox::MailboxRequest>
    processBusinessLogic(const mailbox::MailboxRequest &p_request) override;

    /**
     * @brief Waits until Node A has received the validation response.
     *
     * @param p_timeoutMs Timeout in milliseconds.
     * @return true if received before timeout.
     */
    bool waitForValidation(int p_timeoutMs);

    /**
     * @brief Waits until Node A has received the business response.
     *
     * @param p_timeoutMs Timeout in milliseconds.
     * @return true if received before timeout.
     */
    bool waitForBusinessResponse(int p_timeoutMs);

    bool hasReceivedValidation() const;
    bool hasReceivedBusinessResponse() const;

    mailbox::ValidationResponse getLastValidation() const;
    mailbox::NodeAResponse getLastBusinessResponse() const;

protected:
    std::string getManagerName() const override {
        return "NodeA";
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
    NodeValidator m_validator;
    NodeSupportChecker m_supportChecker;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_receivedValidation{false};
    bool m_receivedBusiness{false};

    mailbox::ValidationResponse m_lastValidation;
    mailbox::NodeAResponse m_lastBusiness;
};

#endif
