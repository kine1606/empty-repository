#ifndef BASE_VALIDATOR_H
#define BASE_VALIDATOR_H

#include "IMailboxValidator.h"
#include "ValidationResult.h"

/**
 * @brief Provides validation of fields common to all mailbox requests.
 */
class BaseValidator : public IMailboxValidator {
public:
  /**
   * @brief Validates a mailbox request against all common checks.
   *
   * Runs the full common chain: request identifier, envelope
   * (source, destination, message type), payload, and metadata.
   *
   * @param p_request Request to validate.
   * @return First validation failure, or a successful result.
   */
  ValidationResult validate(const mailbox::MailboxRequest &p_request) override;

protected:
  /**
   * @brief Validates a request identifier's presence, length, and format.
   *
   * @param p_request Request whose identifier is validated.
   * @return Validation outcome for the request identifier.
   */
  ValidationResult validateRequestId(const mailbox::MailboxRequest &p_request);

  /**
   * @brief Validates a request payload's presence and maximum size.
   *
   * @param p_request Request whose payload is validated.
   * @return Validation outcome for the payload.
   */
  ValidationResult validatePayload(const mailbox::MailboxRequest &p_request);

  /**
   * @brief Validates required metadata fields and their constraints.
   *
   * Rejects a missing metadata block, malformed or impossible timestamps,
   * and correlation identifiers that are empty, too long, or off-charset.
   * Rejects missing or unsupported versions.
   *
   * @param p_request Request whose metadata is validated.
   * @return Validation outcome for the metadata.
   */

  ValidationResult validateSource(const mailbox::MailboxRequest &p_request);

  /**
   * @brief Validates the destination node envelope of a request.
   *
   * Rejects empty destination node identifiers and unknown destination
   * node types.
   *
   * @param p_request Request whose destination is validated.
   * @return Validation outcome for the destination envelope.
   */
  ValidationResult
  validateDestination(const mailbox::MailboxRequest &p_request);

  /**
   * @brief Runs every common validation check in sequence.
   *
   * @param p_request Request to validate.
   * @return First validation failure, or a successful result.
   */
  ValidationResult validateCommon(const mailbox::MailboxRequest &p_request);
};

#endif // BASE_VALIDATOR_H
