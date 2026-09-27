#ifndef DU_VALIDATOR_H
#define DU_VALIDATOR_H

#include "BaseValidator.h"

/**
 * @brief Validates incoming mailbox requests destined for the Distributed Unit (DU).
 */
class DUValidator : public BaseValidator {
public:
    DUValidator() = default;
    ~DUValidator() = default;

    /**
     * @brief Validates common fields and verifies supported DU payload schemas.
     *
     * @param p_request Mailbox request to validate.
     * @return ValidationResult indicating success or specific validation error.
     */
    ValidationResult validate(const mailbox::MailboxRequest &p_request) override;
};

#endif
