#include "NodeClient.h"
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

NodeClient::NodeClient(std::shared_ptr<grpc::Channel> p_channel)
    : m_stub(mailbox::MailboxService::NewStub(std::move(p_channel))) {}

void NodeClient::sendMessage(const mailbox::MailboxRequest &p_request) {

  grpc::ClientContext context;
  google::protobuf::Empty response;

  const grpc::Status status =
      this->m_stub->SendMessage(&context, p_request, &response);
}