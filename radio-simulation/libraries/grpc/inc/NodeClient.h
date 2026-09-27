#ifndef NODE_CLIENT_H
#define NODE_CLIENT_H

#include <chrono>
#include <memory>

#include <grpcpp/grpcpp.h>

#include "mailbox.grpc.pb.h"

/**
 * @brief Sends mailbox requests to a remote node through gRPC.
 */
class NodeClient {
public:
  /**
   * @brief Creates a client that uses the supplied gRPC channel.
   *
   * @param p_channel Channel connected to the remote mailbox service.
   */
  explicit NodeClient(std::shared_ptr<grpc::Channel> p_channel);

  /**
   * @brief Sends a request to the remote mailbox service.
   *
   * @param p_request Request to send.
   * @return Service response, or an INTERNAL_ERROR response when the RPC fails.
   */
  void sendMessage(const mailbox::MailboxRequest &p_request);

  /**
   * @brief Sends a request to the remote mailbox service.
   *
   * @param p_request Request to send.
   * @return Service response, or an INTERNAL_ERROR response when the RPC fails.
   */
  // void sendMessage(const mailbox::MailboxResponse &p_request);

private:
  /** @brief Stub used to invoke the remote mailbox service. */
  std::unique_ptr<mailbox::MailboxService::Stub> m_stub;
};

#endif // NODE_CLIENT_H
