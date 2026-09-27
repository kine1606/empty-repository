#include "MailboxWorker.h"

#include <utility>

#include "Logger.h"

MailboxWorker::MailboxWorker(Mailbox &p_mailbox, IMailboxManager &p_manager,
                             SendResponse p_sendResponse)
    : m_mailbox(p_mailbox), m_manager(p_manager) {
    this->m_sendResponse = std::move(p_sendResponse);
}

MailboxWorker::~MailboxWorker() {
    this->stop();
}

void MailboxWorker::start() {
    if (this->m_running) {
        return;
    }

    this->m_running = true;

    this->m_worker = std::thread(&MailboxWorker::workerLoop, this);
}

void MailboxWorker::workerLoop() {
    INFO("[Worker] Loop started");

    while (this->m_running) {
        auto requestOpt = this->m_mailbox.dequeue();
        if (!requestOpt.has_value()) {
            INFO("[Worker] Mailbox shutdown or empty");
            break;
        }

        const auto &request = requestOpt.value();
        INFO("[Worker] Dequeued RequestId=%1 from Source=%2",
             request.request_id(), request.source());

        // 1. First distinct response: Validation response sent back to caller
        auto validationResponse = this->m_manager.validateMessage(request);
        if (validationResponse.has_value() && this->m_sendResponse) {
            INFO("[Worker] Sending validation response for RequestId=%1 to Destination=%2",
                 request.request_id(), validationResponse.value().destination());
            this->m_sendResponse(validationResponse.value());
        }

        // 2. Second distinct response: Business logic response sent after node done processing inside
        auto businessResponse = this->m_manager.processBusinessLogic(request);
        if (businessResponse.has_value() && this->m_sendResponse) {
            INFO("[Worker] Sending business logic response for RequestId=%1 to Destination=%2",
                 request.request_id(), businessResponse.value().destination());
            this->m_sendResponse(businessResponse.value());
        }
    }
}

void MailboxWorker::stop() {
    if (!this->m_running) {
        return;
    }

    this->m_running = false;

    this->m_mailbox.shutdown();

    if (this->m_worker.joinable()) {
        this->m_worker.join();
    }
}