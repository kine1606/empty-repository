#ifndef MAILBOX_H
#define MAILBOX_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>

#include "mailbox.pb.h"

/**
 * @brief Thread-safe queue for mailbox requests.
 *
 * Provides synchronized enqueue and dequeue operations for communication
 * between mailbox producers and consumers.
 */
class Mailbox {
public:
  /**
   * @brief Adds a request to the mailbox.
   *
   * Wakes a waiting consumer after the request has been added.
   *
   * @param p_request Request to enqueue.
   */
  bool enqueue(const mailbox::MailboxRequest &p_request);

  /**
   * @brief Retrieves the next request from the mailbox.
   *
   * Waits until a request becomes available or the mailbox is shut down.
   *
   * @return The next mailbox request if available, otherwise std::nullopt when
   *         the mailbox has been shut down.
   */
  std::optional<mailbox::MailboxRequest> dequeue();

  /**
   * @brief Immediately shuts down the mailbox.
   *
   * Rejects subsequent enqueue operations, discards all pending requests,
   * and wakes all consumers waiting in dequeue(). After shutdown,
   * dequeue() returns std::nullopt.
   *
   * Calling shutdown() multiple times has no additional effect.
   */
  void shutdown();

private:
  /**
   * @brief Protects access to the request queue and shutdown state.
   */
  std::mutex m_queueMutex;

  /**
   * @brief Notifies consumers when requests arrive or shutdown is requested.
   */
  std::condition_variable m_condition;

  /**
   * @brief Queue containing pending mailbox requests.
   */
  std::queue<mailbox::MailboxRequest> m_messages;

  /**
   * @brief Indicates whether the mailbox is shutting down.
   */
  bool m_shutdown{false};
};

#endif
