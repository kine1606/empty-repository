#ifndef I_MAILBOX_VALIDATOR_H
#define I_MAILBOX_VALIDATOR_H

#include "mailbox.pb.h"

#include "ValidationResult.h"

/**
 * @brief Defines the interface for mailbox request validation.
 */
class IMailboxValidator {

public:
  /** @brief Destroys a validator through an interface pointer. */
  virtual ~IMailboxValidator() = default;

  /**
   * @brief Validates a mailbox request.
   *
   * @param request Request to validate.
   * @return Validation outcome, including any error details.
   */
  virtual ValidationResult validate(const mailbox::MailboxRequest &request) = 0;
};

#endif // I_MAILBOX_VALIDATOR_H
