#include "BaseValidator.h"
#include "ErrorMessages.h"

#include <string>

namespace {
constexpr std::size_t MAX_REQUEST_ID_LENGTH = 64;
constexpr std::size_t MAX_CORRELATION_ID_LENGTH = 64;
constexpr std::size_t MAX_NODE_ID_LENGTH = 64;
constexpr std::size_t MAX_PAYLOAD_SIZE = 4096;

// Factory contract (ex-MessageFactory): UTC "YYYY-MM-DDTHH:MM:SSZ".
constexpr char SUPPORTED_VERSION[] = "1.0";
constexpr std::size_t TIMESTAMP_LENGTH = 20;

bool isIdCharsetValid(const std::string &p_id) {
  for (const char c : p_id) {
    const bool isAlphaNum = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                            (c >= 'a' && c <= 'z');
    if (!isAlphaNum && c != '-' && c != '_') {
      return false;
    }
  }
  return true;
}

bool parseTwoDigits(const std::string &p_text, std::size_t p_pos, int &p_out) {
  const char high = p_text[p_pos];
  const char low = p_text[p_pos + 1];
  if (high < '0' || high > '9' || low < '0' || low > '9') {
    return false;
  }
  p_out = (high - '0') * 10 + (low - '0');
  return true;
}

bool isLeapYear(int p_year) {
  return (p_year % 4 == 0 && p_year % 100 != 0) || (p_year % 400 == 0);
}

int daysInMonth(int p_year, int p_month) {
  switch (p_month) {
  case 2:
    return isLeapYear(p_year) ? 29 : 28;
  case 4:
  case 6:
  case 9:
  case 11:
    return 30;
  default:
    return 31;
  }
}

bool isTimestampWellFormed(const std::string &p_timestamp) {
  if (p_timestamp.size() != TIMESTAMP_LENGTH) {
    return false;
  }

  if (p_timestamp[4] != '-' || p_timestamp[7] != '-' ||
      p_timestamp[10] != 'T' || p_timestamp[13] != ':' ||
      p_timestamp[16] != ':' || p_timestamp[19] != 'Z') {
    return false;
  }

  // Year parsed digit-by-digit so malformed input never throws.
  int year = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    const char c = p_timestamp[i];
    if (c < '0' || c > '9') {
      return false;
    }
    year = year * 10 + (c - '0');
  }

  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  if (!parseTwoDigits(p_timestamp, 5, month) ||
      !parseTwoDigits(p_timestamp, 8, day) ||
      !parseTwoDigits(p_timestamp, 11, hour) ||
      !parseTwoDigits(p_timestamp, 14, minute) ||
      !parseTwoDigits(p_timestamp, 17, second)) {
    return false;
  }

  if (year < 1 || month < 1 || month > 12) {
    return false;
  }

  if (day < 1 || day > daysInMonth(year, month)) {
    return false;
  }

  return hour <= 23 && minute <= 59 && second <= 59;
}
} // namespace

ValidationResult
BaseValidator::validate(const mailbox::MailboxRequest &p_request) {
  return validateCommon(p_request);
}

ValidationResult
BaseValidator::validateSource(const mailbox::MailboxRequest &p_request) {
  if (p_request.source().empty()) {
    return ValidationResult(false, mailbox::MISSING_REQUIRED_FIELD,
                            ErrorMessages::SENDER_EMPTY);
  }

  return ValidationResult::success();
}

ValidationResult
BaseValidator::validateDestination(const mailbox::MailboxRequest &p_request) {
  if (p_request.destination().empty()) {
    return ValidationResult(false, mailbox::MISSING_REQUIRED_FIELD,
                            ErrorMessages::UNSUPPORTED_DESTINATION);
  }

  return ValidationResult::success();
}

ValidationResult
BaseValidator::validateRequestId(const mailbox::MailboxRequest &p_request) {
  const std::string &requestId = p_request.request_id();

  if (requestId.empty()) {
    return ValidationResult(false, mailbox::MISSING_REQUIRED_FIELD,
                            ErrorMessages::REQUEST_ID_EMPTY);
  }

  return ValidationResult::success();
}

ValidationResult
BaseValidator::validatePayload(const mailbox::MailboxRequest &p_request) {
  if (!p_request.has_payload()) {
    return ValidationResult(false, mailbox::INVALID_PAYLOAD,
                            ErrorMessages::PAYLOAD_EMPTY);
  }

  return ValidationResult::success();
}

ValidationResult
BaseValidator::validateCommon(const mailbox::MailboxRequest &p_request) {
  ValidationResult result = validateRequestId(p_request);

  if (!result.isSuccess()) {
    return result;
  }

  result = validateSource(p_request);

  if (!result.isSuccess()) {
    return result;
  }

  result = validateDestination(p_request);

  if (!result.isSuccess()) {
    return result;
  }

  result = validatePayload(p_request);

  if (!result.isSuccess()) {
    return result;
  }

  return ValidationResult::success();
}
