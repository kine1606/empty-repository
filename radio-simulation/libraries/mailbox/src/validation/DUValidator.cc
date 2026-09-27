#include "DUValidator.h"

#include "ErrorMessages.h"
#include "du.pb.h"
#include "ru.pb.h"
#include "terminal.pb.h"

ValidationResult DUValidator::validate(const mailbox::MailboxRequest &p_request) {
    ValidationResult commonResult = this->validateCommon(p_request);
    if (!commonResult.isSuccess()) {
        return commonResult;
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

    if (p_request.payload().Is<du::DURequest>()) {
        du::DURequest duReq;
        if (!p_request.payload().UnpackTo(&duReq)) {
            return ValidationResult{false, mailbox::INVALID_PAYLOAD,
                                    "Failed to unpack DURequest payload"};
        }
        if (duReq.command().empty()) {
            return ValidationResult{false, mailbox::MISSING_REQUIRED_FIELD,
                                    "DURequest missing command field"};
        }
        return ValidationResult::success();
    }

    if (p_request.payload().Is<ru::RUResponse>() ||
        p_request.payload().Is<mailbox::ValidationResponse>()) {
        return ValidationResult::success();
    }

    return ValidationResult{false, mailbox::INVALID_PAYLOAD,
                            "Unsupported payload type for DU: expected TerminalRequest, DURequest, or RUResponse"};
}
