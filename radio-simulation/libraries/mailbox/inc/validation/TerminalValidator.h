#ifndef TERMINAL_VALIDATOR_H
#define TERMINAL_VALIDATOR_H

#include "BaseValidator.h"

/**
 * @brief Validates incoming mailbox responses destined for the Terminal.
 */
class TerminalValidator : public BaseValidator {
public:
    TerminalValidator() = default;
    ~TerminalValidator() = default;

    /**
     * @brief Validates common fields and verifies supported Terminal response schemas.
     *
     * @param p_request Mailbox request/response to validate.
     * @return ValidationResult indicating outcome.
     */
    ValidationResult validate(const mailbox::MailboxRequest &p_request) override;
};

#endif
