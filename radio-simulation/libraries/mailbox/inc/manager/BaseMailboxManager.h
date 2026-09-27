#ifndef BASE_MAILBOX_MANAGER_H
#define BASE_MAILBOX_MANAGER_H

#include <optional>
#include <string>

#include "IMailboxManager.h"
#include "IMailboxValidator.h"
#include "ISupportChecker.h"
#include "Mailbox.h"

/**
 * @brief Implements the common mailbox request processing workflow.
 *
 * Derived managers provide validation, support checking, and a log label for
 * their node type.
 */
class BaseMailboxManager : public IMailboxManager {
public:
    /**
     * @brief Constructs a manager that retrieves requests from @p p_mailbox.
     *
     * @param p_mailbox Mailbox consumed by this manager.
     */
    explicit BaseMailboxManager(Mailbox &p_mailbox);

    /** @brief Destroys the manager through a base-class pointer. */
    ~BaseMailboxManager() override = default;

    /**
     * @brief Validates an incoming mailbox request and generates a validation response.
     *
     * @param p_request Request to validate.
     * @return Validation response to send back to the requester.
     */
    std::optional<mailbox::MailboxRequest>
    validateMessage(const mailbox::MailboxRequest &p_request) override;

    /**
     * @brief Executes business logic for the mailbox request and generates a response.
     *
     * @param p_request Request to process.
     * @return Business logic response to send back to the requester.
     */
    std::optional<mailbox::MailboxRequest>
    processBusinessLogic(const mailbox::MailboxRequest &p_request) override;

    /**
     * @brief Retrieves, validates, and processes one mailbox request.
     */
    std::optional<mailbox::MailboxRequest> processNextMessage() override;

protected:
    /**
     * @brief Returns the manager label used in log messages.
     *
     * @return Human-readable manager name.
     */
    virtual std::string getManagerName() const = 0;

    /**
     * @brief Returns the validator used before processing a request.
     *
     * @return Request validator owned by the derived manager.
     */
    virtual IMailboxValidator &getValidator() = 0;

    /**
     * @brief Returns the support checker used after validation.
     *
     * @return Support checker owned by the derived manager.
     */
    virtual ISupportChecker &getSupportChecker() = 0;

    /**
     * @brief Builds a validation response message to send back to the requester.
     *
     * @param p_request Original request.
     * @param p_isValid Whether the request passed validation.
     * @param p_errorCode Validation error code.
     * @param p_message Informational message.
     * @return Validation response wrapped in a MailboxRequest envelope.
     */
    virtual std::optional<mailbox::MailboxRequest>
    buildValidationResponse(const mailbox::MailboxRequest &p_request, bool p_isValid,
                            mailbox::ErrorCode p_errorCode, const std::string &p_message);

    /**
     * @brief Builds the business logic response to send back to the requester.
     *
     * @param p_request Original request.
     * @return Business response wrapped in a MailboxRequest envelope.
     */
    virtual mailbox::MailboxRequest
    buildResponse(const mailbox::MailboxRequest &p_request) = 0;

protected:
    /** @brief Mailbox from which requests are retrieved. */
    Mailbox &m_mailbox;
};

#endif // BASE_MAILBOX_MANAGER_H
