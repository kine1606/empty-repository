#ifndef RU_VALIDATOR_H
#define RU_VALIDATOR_H

#include "BaseValidator.h"

/**
 * @brief Validates incoming mailbox requests destined for the Radio Unit (RU).
 */
class RUValidator : public BaseValidator {
public:
    RUValidator() = default;
    ~RUValidator() = default;

    /**
     * @brief Validates common fields and verifies supported RU payload schemas.
     *
     * @param p_request Mailbox request to validate.
     * @return ValidationResult indicating outcome.
     */
    ValidationResult validate(const mailbox::MailboxRequest &p_request) override;
};

#endif
