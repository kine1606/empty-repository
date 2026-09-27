#include <iostream>

#include "BaseMailboxManager.h"

#include "Logger.h"

#include <exception>

BaseMailboxManager::BaseMailboxManager(Mailbox &p_mailbox)
    : m_mailbox(p_mailbox) {}

std::optional<mailbox::MailboxRequest>
BaseMailboxManager::processNextMessage() {

  try {
    auto request = this->m_mailbox.dequeue();

    if (!request.has_value()) {

      INFO("[%1] Mailbox shutdown or empty", getManagerName());

      return std::nullopt;
    }

    const mailbox::MailboxRequest &message = request.value();
    INFO("[%1] Processing RequestId=%2", getManagerName(),
         message.request_id());

    //  if (message.message_type() != mailbox::MessageType::REQUEST) {
    //   WARN("[%1] Not a request message type. RequestId=%2", getManagerName(),
    //        message.request_id());
    //   return std::nullopt;
    // }
    if (!getSupportChecker().isSupported(message)) {
      WARN("[%1] Unsupported message. RequestId=%2", getManagerName(),
           message.request_id());
      return std::nullopt;
    }
    ValidationResult validationResult = getValidator().validate(message);

    if (!validationResult.isSuccess()) {
      WARN("[%1] Validation failed. RequestId=%2", getManagerName(),
           message.request_id());

      return std::nullopt;
    }

    INFO("[%1] Message processed successfully. RequestId=%2", getManagerName(),
         message.request_id());
    return buildResponse(message);
  } catch (const std::exception &ex) {
    std::cerr << "[" << getManagerName() << "] Exception: " << ex.what()
              << std::endl;
    return std::nullopt;
  }
}