#ifndef I_SUPPORT_CHECKER_H
#define I_SUPPORT_CHECKER_H

#include "mailbox.pb.h"

/**
 * @brief Defines the interface for node-specific request support checks.
 */
class ISupportChecker {
public:
  /** @brief Destroys a support checker through an interface pointer. */
  virtual ~ISupportChecker() = default;

  /**
   * @brief Determines whether a mailbox request is supported.
   *
   * @param p_request Request to evaluate.
   * @return true when the request can be handled by the implementation.
   */
  virtual bool isSupported(const mailbox::MailboxRequest &p_request) = 0;
};

#endif // I_SUPPORT_CHECKER_H
