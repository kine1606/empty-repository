#ifndef I_MAILBOX_MANAGER_H
#define I_MAILBOX_MANAGER_H

#include <optional>

#include "mailbox.pb.h"

/**
 * @brief Defines the processing interface for mailbox consumers.
 */
class IMailboxManager {
public:
    /** @brief Destroys the mailbox manager through an interface pointer. */
    virtual ~IMailboxManager() = default;

    /**
     * @brief Validates an incoming mailbox request and generates a validation response.
     *
     * @param p_request Request received from the mailbox.
     * @return Validation response wrapped in MailboxRequest, or nullopt if none needed.
     */
    virtual std::optional<mailbox::MailboxRequest>
    validateMessage(const mailbox::MailboxRequest &p_request) = 0;

    /**
     * @brief Processes business logic for a mailbox request and generates a response.
     *
     * @param p_request Request received from the mailbox.
     * @return Business logic response wrapped in MailboxRequest, or nullopt if none needed.
     */
    virtual std::optional<mailbox::MailboxRequest>
    processBusinessLogic(const mailbox::MailboxRequest &p_request) = 0;

    /**
     * @brief Retrieves and processes the next available mailbox request.
     */
    virtual std::optional<mailbox::MailboxRequest> processNextMessage() = 0;
};

#endif // I_MAILBOX_MANAGER_H
