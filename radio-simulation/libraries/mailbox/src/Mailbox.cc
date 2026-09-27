#include "Mailbox.h"

#include "Logger.h"

#include <utility>
#include <vector>

bool Mailbox::enqueue(const mailbox::MailboxRequest &p_request) {

  {
    std::lock_guard<std::mutex> lock(this->m_queueMutex);

    if (this->m_shutdown) {
      return false;
    }

    this->m_messages.push(p_request);
  }

  this->m_condition.notify_one();
  INFO("[Mailbox] Enqueue %1", p_request.request_id());
  return true;
}

std::optional<mailbox::MailboxRequest> Mailbox::dequeue() {
  std::unique_lock<std::mutex> lock(this->m_queueMutex);

  this->m_condition.wait(
      lock, [this] { return this->m_shutdown || !this->m_messages.empty(); });

  if (this->m_shutdown) {
    return std::nullopt;
  }

  mailbox::MailboxRequest request = std::move(this->m_messages.front());
  this->m_messages.pop();

  INFO("[Mailbox] Dequeue %1", request.request_id());
  return request;
}

void Mailbox::shutdown() {
  {
    std::lock_guard<std::mutex> lock(this->m_queueMutex);

    if (this->m_shutdown) {
      return;
    }

    this->m_shutdown = true;

    while (!this->m_messages.empty()) {
      this->m_messages.pop();
    }
  }

  this->m_condition.notify_all();
}
