#include "NodeServiceImpl.h"

#include <utility>

NodeServiceImpl::NodeServiceImpl(std::shared_ptr<Mailbox> p_mailbox)
    : m_mailbox(std::move(p_mailbox)) {}

grpc::Status
NodeServiceImpl::SendMessage(grpc::ServerContext *,
                             const mailbox::MailboxRequest *p_request,
                             google::protobuf::Empty *p_response) {

  if (!m_mailbox->enqueue(*p_request)) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, "Mailbox is shut down");
  }
  // p_response->set_request_id(p_request->request_id());
  // p_response->set_success(accepted);
  // p_response->set_error_code(accepted ? mailbox::OK :
  // mailbox::INTERNAL_ERROR);

  // p_response->set_message(accepted ? "Request accepted"
  //                                  : "Mailbox is shut down");

  return grpc::Status::OK;
}