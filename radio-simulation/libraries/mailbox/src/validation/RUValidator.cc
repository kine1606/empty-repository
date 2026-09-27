#include "RUValidator.h"

#include "ErrorMessages.h"
#include "ru.pb.h"
#include "terminal.pb.h"

ValidationResult RUValidator::validate(const mailbox::MailboxRequest &p_request) {
    ValidationResult commonResult = this->validateCommon(p_request);
    if (!commonResult.isSuccess()) {
        return commonResult;
    }

    if (p_request.payload().Is<ru::RURequest>()) {
        ru::RURequest ruReq;
        if (!p_request.payload().UnpackTo(&ruReq)) {
            return ValidationResult{false, mailbox::INVALID_PAYLOAD,
                                    "Failed to unpack RURequest payload"};
        }
        if (ruReq.command().empty()) {
            return ValidationResult{false, mailbox::MISSING_REQUIRED_FIELD,
                                    "RURequest missing command field"};
        }
        return ValidationResult::success();
    }

    if (p_request.payload().Is<terminal::TerminalRequest>()) {
        terminal::TerminalRequest termReq;
        if (!p_request.payload().UnpackTo(&termReq)) {
            return ValidationResult{false, mailbox::INVALID_PAYLOAD,
                                    "Failed to unpack TerminalRequest payload"};
        }
        if (termReq.action().empty() && termReq.raw_command().empty()) {
            return ValidationResult{false, mailbox::MISSING_REQUIRED_FIELD,
                                    "TerminalRequest missing command or action"};
        }
        return ValidationResult::success();
    }

    return ValidationResult{false, mailbox::INVALID_PAYLOAD,
                            "Unsupported payload type for RU: expected RURequest or TerminalRequest"};
}
