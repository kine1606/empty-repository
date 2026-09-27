#ifndef BASE_MAILBOX_MANAGER_H
#define BASE_MAILBOX_MANAGER_H

#include <string>

#include "IMailboxManager.h"

#include "IMailboxValidator.h"
#include "ISupportChecker.h"

#include "Mailbox.h"

/**
 * @brief Implements the common mailbox request processing workflow.
 *
 * Derived managers provide validation, support checking, and a log label for
 * their node type.
 */
class BaseMailboxManager : public IMailboxManager {
public:
  /**
   * @brief Constructs a manager that retrieves requests from @p mailbox.
   *
   * @param mailbox Mailbox consumed by this manager.
   */
  explicit BaseMailboxManager(Mailbox &mailbox);

  /** @brief Destroys the manager through a base-class pointer. */
  virtual ~BaseMailboxManager() = default;

  /**
   * @brief Retrieves, validates, checks, and processes one mailbox request.
   *
   * Returns without processing when the mailbox is shut down, validation
   * fails, the request is unsupported, or an exception occurs.
   */
  std::optional<mailbox::MailboxRequest> processNextMessage() override;

protected:
  /**
   * @brief Returns the manager label used in log messages.
   *
   * @return Human-readable manager name.
   */
  virtual std::string getManagerName() const = 0;

  /**
   * @brief Returns the validator used before processing a request.
   *
   * @return Request validator owned by the derived manager.
   */
  virtual IMailboxValidator &getValidator() = 0;

  /**
   * @brief Returns the support checker used after validation.
   *
   * @return Support checker owned by the derived manager.
   */
  virtual ISupportChecker &getSupportChecker() = 0;

  virtual mailbox::MailboxRequest
  buildResponse(const mailbox::MailboxRequest &p_request) = 0;

protected:
  /** @brief Mailbox from which requests are retrieved. */
  Mailbox &m_mailbox;
};

#endif // BASE_MAILBOX_MANAGER_H
