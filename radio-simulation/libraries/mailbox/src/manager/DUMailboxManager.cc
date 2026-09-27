#include "DUMailboxManager.h"

#include <algorithm>
#include <sstream>

#include "Logger.h"

namespace {
std::string toUpper(std::string p_str) {
    std::transform(p_str.begin(), p_str.end(), p_str.begin(), ::toupper);
    return p_str;
}
}

DUMailboxManager::DUMailboxManager(Mailbox &p_mailbox)
    : BaseMailboxManager(p_mailbox) {
}

std::optional<mailbox::MailboxRequest>
DUMailboxManager::validateMessage(const mailbox::MailboxRequest &p_request) {
    INFO("[DUMailboxManager] Validating request RequestId=%1 from %2",
         p_request.request_id(), p_request.source());

    if (p_request.payload().Is<ru::RUResponse>() ||
        p_request.payload().Is<mailbox::ValidationResponse>()) {
        return std::nullopt;
    }

    if (!this->m_supportChecker.isSupported(p_request)) {
        WARN("[DUMailboxManager] Unsupported destination: %1 (expected DU/DU01)",
             p_request.destination());
        return this->buildValidationResponse(p_request, false,
                                             mailbox::INVALID_ADDRESS,
                                             "Destination node unsupported for DU");
    }

    ValidationResult result = this->m_validator.validate(p_request);
    if (!result.isSuccess()) {
        WARN("[DUMailboxManager] Validation failed for RequestId=%1: %2",
             p_request.request_id(), result.getErrorMessage());
        return this->buildValidationResponse(p_request, false,
                                             result.getErrorCode(),
                                             result.getErrorMessage());
    }

    std::string validMsg = "DU validated request successfully";
    if (p_request.payload().Is<terminal::TerminalRequest>()) {
        terminal::TerminalRequest termReq;
        p_request.payload().UnpackTo(&termReq);
        if (0 == termReq.target_node().compare("RU") ||
            termReq.raw_command().rfind("ru ", 0) == 0 ||
            termReq.raw_command().rfind("RU ", 0) == 0) {
            validMsg = "DU validated request; routing to RU";
        }
    }

    INFO("[DUMailboxManager] RequestId=%1 validated successfully", p_request.request_id());
    return this->buildValidationResponse(p_request, true, mailbox::OK, validMsg);
}

std::optional<mailbox::MailboxRequest>
DUMailboxManager::processBusinessLogic(const mailbox::MailboxRequest &p_request) {
    INFO("[DUMailboxManager] Processing business logic for RequestId=%1", p_request.request_id());

    mailbox::MailboxRequest responseEnvelope;
    responseEnvelope.set_request_id(p_request.request_id());
    responseEnvelope.set_source("DU");
    responseEnvelope.set_destination(p_request.source());

    // 1. If payload is from Terminal
    if (p_request.payload().Is<terminal::TerminalRequest>()) {
        terminal::TerminalRequest termReq;
        p_request.payload().UnpackTo(&termReq);

        std::string action = toUpper(termReq.action());
        if (action.empty() && !termReq.raw_command().empty()) {
            std::istringstream iss(termReq.raw_command());
            iss >> action;
            action = toUpper(action);
        }

        bool isTargetingRU = (0 == termReq.target_node().compare("RU") ||
                              0 == action.compare("RU") ||
                              termReq.raw_command().rfind("ru ", 0) == 0 ||
                              termReq.raw_command().rfind("RU ", 0) == 0);
        if (isTargetingRU) {
            INFO("[DUMailboxManager] Routing TerminalRequest RequestId=%1 to RU",
                 p_request.request_id());
            mailbox::MailboxRequest ruEnvelope;
            ruEnvelope.set_request_id(p_request.request_id());
            ruEnvelope.set_source("DU");
            ruEnvelope.set_destination("RU");

            std::string subAction = action;
            std::string subParams = termReq.parameters();
            if (0 == action.compare("RU")) {
                std::istringstream iss(termReq.raw_command());
                std::string prefix;
                iss >> prefix >> subAction;
                subAction = toUpper(subAction);
                std::getline(iss, subParams);
                size_t first = subParams.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    subParams = subParams.substr(first);
                } else {
                    subParams.clear();
                }
            }

            ru::RURequest ruReq;
            ruReq.set_command(subAction);
            ruReq.set_additional_params(subParams);
            ruEnvelope.mutable_payload()->PackFrom(ruReq);
            return ruEnvelope;
        }

        terminal::TerminalResponse termResp;
        termResp.set_request_id(p_request.request_id());

        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        if (0 == action.compare("CONFIG") || 0 == action.compare("SET_CONFIG")) {
            if (!termReq.parameters().empty()) {
                this->m_carrierFreq = termReq.parameters();
            }
            this->m_state = "CONFIGURED";
            termResp.set_is_success(true);
            termResp.set_return_code(0);
            termResp.set_output_text("DU configured successfully with parameters: " +
                                     this->m_carrierFreq + " [State=" + this->m_state + "]");
        } else if (0 == action.compare("START") || 0 == action.compare("START_CELL")) {
            this->m_state = "RUNNING";
            termResp.set_is_success(true);
            termResp.set_return_code(0);
            termResp.set_output_text("DU Radio Cell [" + this->m_cellId +
                                     "] started successfully. [State=RUNNING, Freq=" +
                                     this->m_carrierFreq + "]");
        } else if (0 == action.compare("STOP") || 0 == action.compare("STOP_CELL")) {
            this->m_state = "STOPPED";
            termResp.set_is_success(true);
            termResp.set_return_code(0);
            termResp.set_output_text("DU Radio Cell [" + this->m_cellId +
                                     "] stopped. [State=STOPPED]");
        } else if (0 == action.compare("STATUS")) {
            termResp.set_is_success(true);
            termResp.set_return_code(0);
            std::ostringstream oss;
            oss << "DU Status:\n"
                << "  Cell ID:      " << this->m_cellId << "\n"
                << "  State:        " << this->m_state << "\n"
                << "  Carrier Freq: " << this->m_carrierFreq << "\n"
                << "  Bandwidth:    " << this->m_bandwidthMhz << " MHz\n"
                << "  Tx Power:     " << this->m_txPowerDbm << " dBm";
            termResp.set_output_text(oss.str());
        } else if (0 == action.compare("HELP")) {
            termResp.set_is_success(true);
            termResp.set_return_code(0);
            termResp.set_output_text(
                "DU Terminal Commands:\n"
                "  status               - Query DU radio cell status\n"
                "  config <params>      - Configure radio carrier and power\n"
                "  start                - Activate and start DU transmission\n"
                "  stop                 - Halt DU transmission\n"
                "  help                 - Display command help\n"
                "  exit / quit          - Exit terminal");
        } else {
            termResp.set_is_success(false);
            termResp.set_return_code(1);
            termResp.set_output_text("Unknown command: '" + termReq.raw_command() +
                                     "'. Type 'help' for available commands.");
        }

        responseEnvelope.mutable_payload()->PackFrom(termResp);
        return responseEnvelope;
    }

    // 2. If payload is DURequest
    if (p_request.payload().Is<du::DURequest>()) {
        du::DURequest duReq;
        p_request.payload().UnpackTo(&duReq);

        du::DUResponse duResp;
        duResp.set_request_id(p_request.request_id());
        duResp.set_status_code(200);
        duResp.set_status_message("SUCCESS");

        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        if (!duReq.carrier_freq().empty()) {
            this->m_carrierFreq = duReq.carrier_freq();
        }
        if (duReq.bandwidth_mhz() > 0.0) {
            this->m_bandwidthMhz = duReq.bandwidth_mhz();
        }
        if (duReq.tx_power_dbm() > 0.0) {
            this->m_txPowerDbm = duReq.tx_power_dbm();
        }
        duResp.set_active_state(this->m_state);
        duResp.set_details("DU executed command: " + duReq.command());

        responseEnvelope.mutable_payload()->PackFrom(duResp);
        return responseEnvelope;
    }

    // 3. If payload is RUResponse from RU
    if (p_request.payload().Is<ru::RUResponse>()) {
        ru::RUResponse ruResp;
        p_request.payload().UnpackTo(&ruResp);

        INFO("[DUMailboxManager] Relaying RUResponse for RequestId=%1 to Terminal",
             ruResp.request_id());

        terminal::TerminalResponse termResp;
        termResp.set_request_id(ruResp.request_id());
        termResp.set_is_success(ruResp.status_code() == 200);
        termResp.set_return_code(ruResp.status_code());
        termResp.set_output_text("[RU via DU] " + ruResp.details());

        mailbox::MailboxRequest termEnvelope;
        termEnvelope.set_request_id(ruResp.request_id());
        termEnvelope.set_source("DU");
        termEnvelope.set_destination("Terminal");
        termEnvelope.mutable_payload()->PackFrom(termResp);
        return termEnvelope;
    }

    return std::nullopt;
}

mailbox::MailboxRequest
DUMailboxManager::buildResponse(const mailbox::MailboxRequest &p_request) {
    auto response = this->processBusinessLogic(p_request);
    if (response.has_value()) {
        return response.value();
    }
    mailbox::MailboxRequest fallback;
    fallback.set_request_id(p_request.request_id());
    fallback.set_source("DU");
    fallback.set_destination(p_request.source());
    return fallback;
}

std::string DUMailboxManager::getState() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_state;
}

std::string DUMailboxManager::getCarrierFreq() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_carrierFreq;
}

double DUMailboxManager::getBandwidthMhz() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_bandwidthMhz;
}

double DUMailboxManager::getTxPowerDbm() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_txPowerDbm;
}
