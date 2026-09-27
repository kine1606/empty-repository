#ifndef VALIDATION_RESULT_H
#define VALIDATION_RESULT_H

#include "mailbox.pb.h"
#include <string>

/**
 * @brief Describes the result of validating a mailbox request.
 */
class ValidationResult {
private:
  /** @brief Whether validation succeeded. */
  bool m_success{};

  /** @brief Protocol error code associated with a validation failure. */
  mailbox::ErrorCode m_errorCode;

  /** @brief Human-readable description of a validation failure. */
  std::string m_errorMessage;

public:
  /**
   * @brief Constructs a successful validation result.
   */
  ValidationResult();

  /**
   * @brief Creates a successful validation result.
   *
   * @return A successful validation result.
   */
  static ValidationResult success();
  /**
   * @brief Reports whether validation succeeded.
   *
   * @return true when validation succeeded.
   */
  bool isSuccess() const;

  /**
   * @brief Returns the protocol error code.
   *
   * @return Error code associated with the result.
   */
  mailbox::ErrorCode getErrorCode() const;

  /**
   * @brief Returns the validation error message.
   *
   * @return Error message associated with the result.
   */
  const std::string &getErrorMessage() const;

  /**
   * @brief Constructs a validation result with explicit details.
   *
   * @param p_success Whether validation succeeded.
   * @param p_errorCode Protocol error code for the result.
   * @param p_errorMessage Human-readable error message.
   */
  ValidationResult(bool p_success, mailbox::ErrorCode p_errorCode,
                   const std::string &p_errorMessage);
};

#endif // VALIDATION_RESULT_H
