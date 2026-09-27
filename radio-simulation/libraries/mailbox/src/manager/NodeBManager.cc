#include "NodeBManager.h"

#include <string>

#include "Logger.h"

NodeBManager::NodeBManager(Mailbox &p_mailbox)
    : BaseMailboxManager(p_mailbox), m_supportChecker("NodeB") {
}

std::optional<mailbox::MailboxRequest>
NodeBManager::validateMessage(const mailbox::MailboxRequest &p_request) {
    INFO("[NodeBManager] Step 1: Validating incoming request RequestId=%1 from %2",
         p_request.request_id(), p_request.source());

    if (!this->m_supportChecker.isSupported(p_request)) {
        WARN("[NodeBManager] Unsupported destination: %1 (expected NodeB)",
             p_request.destination());
        return this->buildValidationResponse(
            p_request, false, mailbox::ErrorCode::INVALID_ADDRESS,
            "Destination is not NodeB");
    }

    ValidationResult baseValidation = this->m_validator.validate(p_request);
    if (!baseValidation.isSuccess()) {
        WARN("[NodeBManager] Envelope validation failed: %1", baseValidation.getErrorMessage());
        return this->buildValidationResponse(
            p_request, false, baseValidation.getErrorCode(), baseValidation.getErrorMessage());
    }

    if (!p_request.payload().Is<mailbox::NodeBRequest>()) {
        WARN("[NodeBManager] Payload does not contain NodeBRequest");
        return this->buildValidationResponse(
            p_request, false, mailbox::ErrorCode::INVALID_PAYLOAD,
            "Payload type mismatch: expected NodeBRequest");
    }

    INFO("[NodeBManager] Validation SUCCESS for RequestId=%1. Returning ValidationResponse.",
         p_request.request_id());

    return this->buildValidationResponse(
        p_request, true, mailbox::ErrorCode::OK,
        "NodeB validated NodeBRequest successfully");
}

mailbox::MailboxRequest
NodeBManager::buildResponse(const mailbox::MailboxRequest &p_request) {
    INFO("[NodeBManager] Step 2: Processing business logic inside Node B for RequestId=%1",
         p_request.request_id());

    mailbox::NodeBRequest nodeBReq;
    p_request.payload().UnpackTo(&nodeBReq);

    INFO("[NodeBManager] Unpacked NodeBRequest: command='%1', tx_id=%2, data='%3'",
         nodeBReq.command(), nodeBReq.transaction_id(), nodeBReq.data());

    std::string processedDetails = "NodeB completed command [" +
                                   nodeBReq.command() + "] (tx_id=" +
                                   std::to_string(nodeBReq.transaction_id()) +
                                   ") with data: " + nodeBReq.data();

    INFO("[NodeBManager] Business logic complete. Building NodeAResponse.");

    mailbox::NodeAResponse nodeAResp;
    nodeAResp.set_request_id(p_request.request_id());
    nodeAResp.set_status("SUCCESS");
    nodeAResp.set_result_code(200);
    nodeAResp.set_result_details(processedDetails);

    mailbox::MailboxRequest responseEnvelope;
    responseEnvelope.set_request_id(p_request.request_id());
    responseEnvelope.set_source("NodeB");
    responseEnvelope.set_destination(p_request.source());
    responseEnvelope.mutable_payload()->PackFrom(nodeAResp);

    return responseEnvelope;
}
