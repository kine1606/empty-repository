#include "MailboxWorker.h"
#include <utility>

#include "Logger.h"

MailboxWorker::MailboxWorker(Mailbox &p_mailbox, IMailboxManager &p_manager,
                             SendResponse p_sendResponse)
    : m_mailbox(p_mailbox), m_manager(p_manager),
      m_sendResponse(std::move(p_sendResponse)) {}

MailboxWorker::~MailboxWorker() { stop(); }

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
    auto response = this->m_manager.processNextMessage();

    if (!response.has_value()) {
      INFO("[Worker] No response generated for the request");
    } else if (this->m_sendResponse) {
      INFO("[Worker] Response generated for the request");
      this->m_sendResponse(response.value());
      // sendtoclient
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