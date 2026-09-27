#ifndef I_MAILBOX_MANAGER_H
#define I_MAILBOX_MANAGER_H

#include "mailbox.pb.h"
#include <optional>

/**
 * @brief Defines the processing interface for mailbox consumers.
 */
class IMailboxManager {
public:
  /** @brief Destroys the mailbox manager through an interface pointer. */
  virtual ~IMailboxManager() = default;

  /**
   * @brief Retrieves and processes the next available mailbox request.
   */
  virtual std::optional<mailbox::MailboxRequest> processNextMessage() = 0;
};

#endif // I_MAILBOX_MANAGER_H
