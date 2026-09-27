#include "ValidationResult.h"

ValidationResult::ValidationResult()
    : m_success(true), m_errorCode(mailbox::OK), m_errorMessage("ok bro") {}

ValidationResult ValidationResult::success() { return ValidationResult(); }

bool ValidationResult::isSuccess() const { return this->m_success; }

mailbox::ErrorCode ValidationResult::getErrorCode() const {
  return this->m_errorCode;
}

const std::string &ValidationResult::getErrorMessage() const {
  return this->m_errorMessage;
}

ValidationResult::ValidationResult(bool p_success,
                                   mailbox::ErrorCode p_errorCode,
                                   const std::string &p_errorMessage)
    : m_success(p_success), m_errorCode(p_errorCode),
      m_errorMessage(p_errorMessage) {}