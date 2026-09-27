#ifndef MAILBOX_WORKER_H
#define MAILBOX_WORKER_H

#include <atomic>
#include <functional>
#include <thread>

#include "IMailboxManager.h"
#include "Mailbox.h"

using SendResponse = std::function<void(const mailbox::MailboxRequest &)>;
/**
 * @brief Worker responsible for processing mailbox requests.
 *
 * Runs a dedicated worker thread that retrieves requests from a Mailbox and
 * forwards them to an IMailboxManager for processing.
 */
class MailboxWorker {

public:
  /**
   * @brief Constructs a mailbox worker.
   *
   * @param p_mailbox Mailbox from which requests are consumed.
   * @param p_manager Manager responsible for processing consumed requests.
   * @param p_sendResponse Function to send responses back to the client.
   */
  MailboxWorker(Mailbox &p_mailbox, IMailboxManager &p_manager,
                SendResponse p_sendResponse);

  /**
   * @brief Stops and destroys the mailbox worker.
   */
  ~MailboxWorker();

  /**
   * @brief Starts the mailbox worker thread.
   */
  void start();

  /**
   * @brief Stops the mailbox worker thread.
   */
  void stop();

private:
  /**
   * @brief Continuously retrieves and processes mailbox requests.
   *
   * Runs on the worker thread until the worker is stopped.
   */
  void workerLoop();

private:
  /**
   * @brief Mailbox consumed by the worker.
   */
  Mailbox &m_mailbox;

  /**
   * @brief Manager used to process mailbox requests.
   */
  IMailboxManager &m_manager;

  /**
   * @brief Thread executing the worker loop.
   */
  std::thread m_worker;

  /**
   * @brief Function to send responses back to the client.
   */
  SendResponse m_sendResponse;

  /**
   * @brief Indicates whether the worker is running.
   */
  std::atomic<bool> m_running{false};
};

#endif // MAILBOX_WORKER_H
