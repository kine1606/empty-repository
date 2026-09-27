#include "NodeAManager.h"

#include <chrono>

#include "Logger.h"

NodeAManager::NodeAManager(Mailbox &p_mailbox)
    : BaseMailboxManager(p_mailbox), m_supportChecker("NodeA") {
}

std::optional<mailbox::MailboxRequest>
NodeAManager::validateMessage(const mailbox::MailboxRequest &p_request) {
    if (p_request.payload().Is<mailbox::ValidationResponse>()) {
        mailbox::ValidationResponse valResp;
        p_request.payload().UnpackTo(&valResp);

        INFO("[NodeAManager] >>> Received VALIDATION RESPONSE from %1 for RequestId=%2: "
             "is_valid=%3, msg='%4'",
             p_request.source(), valResp.request_id(), valResp.is_valid(), valResp.message());

        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_receivedValidation = true;
            this->m_lastValidation = valResp;
        }
        this->m_cv.notify_all();
    }
    // Node A is receiving responses; it does not send any response back
    return std::nullopt;
}

std::optional<mailbox::MailboxRequest>
NodeAManager::processBusinessLogic(const mailbox::MailboxRequest &p_request) {
    if (p_request.payload().Is<mailbox::NodeAResponse>()) {
        mailbox::NodeAResponse bizResp;
        p_request.payload().UnpackTo(&bizResp);

        INFO("[NodeAManager] >>> Received BUSINESS RESPONSE from %1 for RequestId=%2: "
             "status='%3', code=%4, details='%5'",
             p_request.source(), bizResp.request_id(), bizResp.status(),
             bizResp.result_code(), bizResp.result_details());

        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_receivedBusiness = true;
            this->m_lastBusiness = bizResp;
        }
        this->m_cv.notify_all();
    }
    return std::nullopt;
}

mailbox::MailboxRequest
NodeAManager::buildResponse(const mailbox::MailboxRequest &) {
    return mailbox::MailboxRequest();
}

bool NodeAManager::waitForValidation(int p_timeoutMs) {
    std::unique_lock<std::mutex> lock(this->m_mutex);
    return this->m_cv.wait_for(lock, std::chrono::milliseconds(p_timeoutMs), [this]() {
        return this->m_receivedValidation;
    });
}

bool NodeAManager::waitForBusinessResponse(int p_timeoutMs) {
    std::unique_lock<std::mutex> lock(this->m_mutex);
    return this->m_cv.wait_for(lock, std::chrono::milliseconds(p_timeoutMs), [this]() {
        return this->m_receivedBusiness;
    });
}

bool NodeAManager::hasReceivedValidation() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_receivedValidation;
}

bool NodeAManager::hasReceivedBusinessResponse() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_receivedBusiness;
}

mailbox::ValidationResponse NodeAManager::getLastValidation() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_lastValidation;
}

mailbox::NodeAResponse NodeAManager::getLastBusinessResponse() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_lastBusiness;
}
