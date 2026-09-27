#ifndef NODE_B_MANAGER_H
#define NODE_B_MANAGER_H

#include <memory>
#include <string>

#include "BaseMailboxManager.h"
#include "NodeSupportChecker.h"
#include "NodeValidator.h"
#include "mailbox.pb.h"

/**
 * @brief Mailbox manager for Node B.
 *
 * Validates incoming NodeBRequests and executes business logic,
 * generating distinct validation and business responses back to Node A.
 */
class NodeBManager : public BaseMailboxManager {
public:
    /**
     * @brief Constructs NodeBManager.
     *
     * @param p_mailbox Mailbox for Node B.
     */
    explicit NodeBManager(Mailbox &p_mailbox);
    ~NodeBManager() override = default;

    /**
     * @brief Validates an incoming mailbox request.
     *
     * Ensures destination is NodeB and payload contains a NodeBRequest.
     *
     * @param p_request Incoming request.
     * @return Validation response destined for Node A.
     */
    std::optional<mailbox::MailboxRequest>
    validateMessage(const mailbox::MailboxRequest &p_request) override;

protected:
    std::string getManagerName() const override {
        return "NodeB";
    }

    IMailboxValidator &getValidator() override {
        return this->m_validator;
    }

    ISupportChecker &getSupportChecker() override {
        return this->m_supportChecker;
    }

    /**
     * @brief Executes business logic for NodeB and creates NodeAResponse.
     *
     * @param p_request Original request containing NodeBRequest payload.
     * @return MailboxRequest containing NodeAResponse payload destined for Node A.
     */
    mailbox::MailboxRequest
    buildResponse(const mailbox::MailboxRequest &p_request) override;

private:
    NodeValidator m_validator;
    NodeSupportChecker m_supportChecker;
};

#endif
