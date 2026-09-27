#include "BaseMailboxManager.h"

#include <exception>
#include <iostream>

#include "Logger.h"

BaseMailboxManager::BaseMailboxManager(Mailbox &p_mailbox)
    : m_mailbox(p_mailbox)
{
}

std::optional<mailbox::MailboxRequest>
BaseMailboxManager::validateMessage(const mailbox::MailboxRequest &p_request) {
    try {
        INFO("[%1] Validating RequestId=%2 from Source=%3",
             this->getManagerName(), p_request.request_id(), p_request.source());

        if (!this->getSupportChecker().isSupported(p_request)) {
            WARN("[%1] Unsupported message. RequestId=%2",
                 this->getManagerName(), p_request.request_id());
            return this->buildValidationResponse(
                p_request, false, mailbox::ErrorCode::INVALID_PAYLOAD, "Unsupported message");
        }

        ValidationResult validationResult = this->getValidator().validate(p_request);
        if (!validationResult.isSuccess()) {
            WARN("[%1] Validation failed. RequestId=%2, Error=%3",
                 this->getManagerName(), p_request.request_id(), validationResult.getErrorMessage());
            return this->buildValidationResponse(
                p_request, false, validationResult.getErrorCode(), validationResult.getErrorMessage());
        }

        INFO("[%1] Validation passed. RequestId=%2",
             this->getManagerName(), p_request.request_id());
        return this->buildValidationResponse(
            p_request, true, mailbox::ErrorCode::OK, "Request validated successfully");
    } catch (const std::exception &ex) {
        std::cerr << "[" << this->getManagerName() << "] Exception in validateMessage: "
                  << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<mailbox::MailboxRequest>
BaseMailboxManager::buildValidationResponse(const mailbox::MailboxRequest &p_request,
                                            bool p_isValid,
                                            mailbox::ErrorCode p_errorCode,
                                            const std::string &p_message) {
    mailbox::MailboxRequest response;
    response.set_request_id(p_request.request_id());
    response.set_source(this->getManagerName());
    response.set_destination(p_request.source());

    mailbox::ValidationResponse valPayload;
    valPayload.set_request_id(p_request.request_id());
    valPayload.set_is_valid(p_isValid);
    valPayload.set_error_code(p_errorCode);
    valPayload.set_message(p_message);

    response.mutable_payload()->PackFrom(valPayload);
    return response;
}

std::optional<mailbox::MailboxRequest>
BaseMailboxManager::processBusinessLogic(const mailbox::MailboxRequest &p_request) {
    try {
        if (!this->getSupportChecker().isSupported(p_request)) {
            return std::nullopt;
        }

        ValidationResult validationResult = this->getValidator().validate(p_request);
        if (!validationResult.isSuccess()) {
            return std::nullopt;
        }

        INFO("[%1] Processing business logic for RequestId=%2",
             this->getManagerName(), p_request.request_id());
        return this->buildResponse(p_request);
    } catch (const std::exception &ex) {
        std::cerr << "[" << this->getManagerName() << "] Exception in processBusinessLogic: "
                  << ex.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<mailbox::MailboxRequest>
BaseMailboxManager::processNextMessage() {
    try {
        auto request = this->m_mailbox.dequeue();

        if (!request.has_value()) {
            INFO("[%1] Mailbox shutdown or empty", this->getManagerName());
            return std::nullopt;
        }

        return this->processBusinessLogic(request.value());
    } catch (const std::exception &ex) {
        std::cerr << "[" << this->getManagerName() << "] Exception: " << ex.what()
                  << std::endl;
        return std::nullopt;
    }
}