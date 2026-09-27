#include "TerminalValidator.h"

#include "du.pb.h"
#include "mailbox.pb.h"
#include "terminal.pb.h"

ValidationResult TerminalValidator::validate(const mailbox::MailboxRequest &p_request) {
    ValidationResult commonResult = this->validateCommon(p_request);
    if (!commonResult.isSuccess()) {
        return commonResult;
    }

    if (p_request.payload().Is<terminal::TerminalResponse>() ||
        p_request.payload().Is<du::DUResponse>() ||
        p_request.payload().Is<mailbox::ValidationResponse>()) {
        return ValidationResult::success();
    }

    return ValidationResult{false, mailbox::INVALID_PAYLOAD,
                            "Unsupported response payload type for Terminal"};
}
