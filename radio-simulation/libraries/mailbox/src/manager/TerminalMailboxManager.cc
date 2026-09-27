#include "TerminalMailboxManager.h"

#include <chrono>

#include "Logger.h"

TerminalMailboxManager::TerminalMailboxManager(Mailbox &p_mailbox)
    : BaseMailboxManager(p_mailbox) {
}

std::optional<mailbox::MailboxRequest>
TerminalMailboxManager::validateMessage(const mailbox::MailboxRequest &p_request) {
    if (p_request.payload().Is<mailbox::ValidationResponse>()) {
        mailbox::ValidationResponse valResp;
        p_request.payload().UnpackTo(&valResp);

        INFO("[TerminalMailboxManager] Received ValidationResponse: valid=%1, msg='%2'",
             valResp.is_valid(), valResp.message());

        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_receivedValidation = true;
            this->m_lastValidation = valResp;
        }
        if (this->m_outputCallback) {
            std::string status = valResp.is_valid() ? "[ACK] " : "[NACK] ";
            this->m_outputCallback(status + valResp.message());
        }
        this->m_cv.notify_all();
    }
    return std::nullopt;
}

std::optional<mailbox::MailboxRequest>
TerminalMailboxManager::processBusinessLogic(const mailbox::MailboxRequest &p_request) {
    if (p_request.payload().Is<terminal::TerminalResponse>()) {
        terminal::TerminalResponse termResp;
        p_request.payload().UnpackTo(&termResp);

        INFO("[TerminalMailboxManager] Received TerminalResponse for RequestId=%1: code=%2",
             termResp.request_id(), termResp.return_code());

        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_receivedExecution = true;
            this->m_lastTerminalResponse = termResp;
        }
        if (this->m_outputCallback) {
            this->m_outputCallback(termResp.output_text());
        }
        this->m_cv.notify_all();
    } else if (p_request.payload().Is<du::DUResponse>()) {
        du::DUResponse duResp;
        p_request.payload().UnpackTo(&duResp);

        INFO("[TerminalMailboxManager] Received DUResponse for RequestId=%1: status=%2",
             duResp.request_id(), duResp.status_code());

        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_receivedExecution = true;
            this->m_lastDUResponse = duResp;
        }
        if (this->m_outputCallback) {
            this->m_outputCallback(duResp.details());
        }
        this->m_cv.notify_all();
    }
    return std::nullopt;
}

void TerminalMailboxManager::setOutputCallback(outputCallback p_callback) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_outputCallback = std::move(p_callback);
}

bool TerminalMailboxManager::waitForResponse(int p_timeoutMs) {
    std::unique_lock<std::mutex> lock(this->m_mutex);
    return this->m_cv.wait_for(lock, std::chrono::milliseconds(p_timeoutMs), [this]() {
        return this->m_receivedExecution;
    });
}

bool TerminalMailboxManager::hasReceivedValidation() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_receivedValidation;
}

bool TerminalMailboxManager::hasReceivedExecution() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_receivedExecution;
}

mailbox::ValidationResponse TerminalMailboxManager::getLastValidation() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_lastValidation;
}

terminal::TerminalResponse TerminalMailboxManager::getLastTerminalResponse() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_lastTerminalResponse;
}

du::DUResponse TerminalMailboxManager::getLastDUResponse() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_lastDUResponse;
}

void TerminalMailboxManager::reset() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_receivedValidation = false;
    this->m_receivedExecution = false;
    this->m_lastValidation.Clear();
    this->m_lastTerminalResponse.Clear();
    this->m_lastDUResponse.Clear();
}

mailbox::MailboxRequest
TerminalMailboxManager::buildResponse(const mailbox::MailboxRequest &p_request) {
    mailbox::MailboxRequest emptyResp;
    emptyResp.set_request_id(p_request.request_id());
    emptyResp.set_source("Terminal");
    emptyResp.set_destination(p_request.source());
    return emptyResp;
}
