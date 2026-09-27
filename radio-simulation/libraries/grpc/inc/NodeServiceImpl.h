#ifndef NODE_SERVICE_IMPL_H
#define NODE_SERVICE_IMPL_H

#include <memory>

#include "IMailboxManager.h"
#include "Mailbox.h"
#include "mailbox.grpc.pb.h"

/**
 * @brief Implements the gRPC mailbox service for a local node.
 *
 * Accepted requests are placed on the node's mailbox for asynchronous
 * processing by its mailbox worker.
 */
class NodeServiceImpl final : public mailbox::MailboxService::Service {
public:
  /**
   * @brief Creates a service that enqueues requests for a mailbox manager.
   *
   * @param p_mailbox Mailbox that stores accepted requests.
   */
  explicit NodeServiceImpl(std::shared_ptr<Mailbox> p_mailbox);

  /**
   * @brief Accepts an incoming mailbox request.
   *
   * Enqueues the request and returns an acknowledgement to the client.
   *
   * @param p_context Context of the gRPC request.
   * @param p_request Request received from the client.
   * @param p_response Acknowledgement returned to the client.
   * @return gRPC status for the request.
   */
  grpc::Status SendMessage(grpc::ServerContext *p_context,
                           const mailbox::MailboxRequest *p_request,
                           google::protobuf::Empty *p_response) override;

private:
  /** @brief Mailbox into which accepted requests are enqueued. */
  std::shared_ptr<Mailbox> m_mailbox;

  /** @brief Manager associated with the mailbox service. */
  std::shared_ptr<IMailboxManager> m_mailboxManager;
};

#endif // NODE_SERVICE_IMPL_H
