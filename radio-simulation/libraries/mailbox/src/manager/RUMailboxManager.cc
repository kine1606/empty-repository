#include "RUMailboxManager.h"

#include <algorithm>
#include <sstream>

#include "Logger.h"

namespace {
std::string toUpper(std::string p_str) {
    std::transform(p_str.begin(), p_str.end(), p_str.begin(), ::toupper);
    return p_str;
}

void parseKeyValueParams(const std::string &p_params,
                         double &p_txPower,
                         double &p_centerFreq,
                         int32_t &p_antennas) {
    if (p_params.empty()) {
        return;
    }
    std::istringstream iss(p_params);
    std::string token;
    while (iss >> token) {
        size_t eqPos = token.find('=');
        if (eqPos == std::string::npos) {
            continue;
        }
        std::string key = token.substr(0, eqPos);
        std::string val = token.substr(eqPos + 1);
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        try {
            if (0 == key.compare("tx_power") || 0 == key.compare("power")) {
                p_txPower = std::stod(val);
            } else if (0 == key.compare("freq") || 0 == key.compare("center_freq")) {
                p_centerFreq = std::stod(val);
            } else if (0 == key.compare("antennas") || 0 == key.compare("ports")) {
                p_antennas = std::stoi(val);
            }
        } catch (...) {
        }
    }
}
}

RUMailboxManager::RUMailboxManager(Mailbox &p_mailbox)
    : BaseMailboxManager(p_mailbox) {
}

std::optional<mailbox::MailboxRequest>
RUMailboxManager::validateMessage(const mailbox::MailboxRequest &p_request) {
    INFO("[RUMailboxManager] Validating request RequestId=%1 from %2",
         p_request.request_id(), p_request.source());

    if (!this->m_supportChecker.isSupported(p_request)) {
        WARN("[RUMailboxManager] Unsupported destination: %1 (expected RU/RU01)",
             p_request.destination());
        return this->buildValidationResponse(p_request, false,
                                             mailbox::INVALID_ADDRESS,
                                             "Destination node unsupported for RU");
    }

    ValidationResult result = this->m_validator.validate(p_request);
    if (!result.isSuccess()) {
        WARN("[RUMailboxManager] Validation failed for RequestId=%1: %2",
             p_request.request_id(), result.getErrorMessage());
        return this->buildValidationResponse(p_request, false,
                                             result.getErrorCode(),
                                             result.getErrorMessage());
    }

    INFO("[RUMailboxManager] RequestId=%1 validated successfully", p_request.request_id());
    return this->buildValidationResponse(p_request, true, mailbox::OK,
                                         "RU validated request successfully");
}

std::optional<mailbox::MailboxRequest>
RUMailboxManager::processBusinessLogic(const mailbox::MailboxRequest &p_request) {
    INFO("[RUMailboxManager] Processing business logic for RequestId=%1", p_request.request_id());

    mailbox::MailboxRequest responseEnvelope;
    responseEnvelope.set_request_id(p_request.request_id());
    responseEnvelope.set_source("RU");
    responseEnvelope.set_destination(p_request.source());

    if (p_request.payload().Is<ru::RURequest>()) {
        ru::RURequest ruReq;
        p_request.payload().UnpackTo(&ruReq);

        std::string command = toUpper(ruReq.command());
        ru::RUResponse ruResp;
        ruResp.set_request_id(p_request.request_id());
        ruResp.set_status_code(200);
        ruResp.set_status_message("SUCCESS");

        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        if (!ruReq.additional_params().empty()) {
            parseKeyValueParams(ruReq.additional_params(),
                                this->m_txPowerDbm,
                                this->m_centerFreqMhz,
                                this->m_antennaPorts);
        }
        if (ruReq.tx_power_dbm() > 0.0) {
            this->m_txPowerDbm = ruReq.tx_power_dbm();
        }
        if (ruReq.center_freq_mhz() > 0.0) {
            this->m_centerFreqMhz = ruReq.center_freq_mhz();
        }
        if (ruReq.antenna_ports() > 0) {
            this->m_antennaPorts = ruReq.antenna_ports();
        }

        if (0 == command.compare("START") || 0 == command.compare("ENABLE_RF")) {
            this->m_rfState = "TX_ACTIVE";
            ruResp.set_details("RU RF frontend [" + this->m_unitId +
                               "] activated (TX_ACTIVE). [Power=" +
                               std::to_string(static_cast<int>(this->m_txPowerDbm)) + " dBm, Freq=" +
                               std::to_string(static_cast<int>(this->m_centerFreqMhz)) + " MHz, Ports=" +
                               std::to_string(this->m_antennaPorts) + "]");
        } else if (0 == command.compare("STOP") || 0 == command.compare("DISABLE_RF")) {
            this->m_rfState = "STANDBY";
            ruResp.set_details("RU RF frontend [" + this->m_unitId + "] disabled. [State=STANDBY]");
        } else if (0 == command.compare("CONFIG") || 0 == command.compare("SET_CONFIG")) {
            ruResp.set_details("RU configured: Power=" +
                               std::to_string(static_cast<int>(this->m_txPowerDbm)) + " dBm, Freq=" +
                               std::to_string(static_cast<int>(this->m_centerFreqMhz)) + " MHz, Ports=" +
                               std::to_string(this->m_antennaPorts));
        } else if (0 == command.compare("STATUS")) {
            std::ostringstream oss;
            oss << "RU Hardware Status:\n"
                << "  Unit ID:        " << this->m_unitId << "\n"
                << "  RF State:       " << this->m_rfState << "\n"
                << "  Tx Power:       " << this->m_txPowerDbm << " dBm\n"
                << "  Center Freq:    " << this->m_centerFreqMhz << " MHz\n"
                << "  Antenna Ports:  " << this->m_antennaPorts;
            ruResp.set_details(oss.str());
        } else {
            ruResp.set_details("RU command '" + ruReq.command() + "' executed successfully.");
        }

        ruResp.set_rf_state(this->m_rfState);
        ruResp.set_actual_tx_power(this->m_txPowerDbm);
        ruResp.set_actual_freq_mhz(this->m_centerFreqMhz);

        responseEnvelope.mutable_payload()->PackFrom(ruResp);
        return responseEnvelope;
    }

    if (p_request.payload().Is<terminal::TerminalRequest>()) {
        terminal::TerminalRequest termReq;
        p_request.payload().UnpackTo(&termReq);

        std::string action = toUpper(termReq.action());
        if (action.empty() && !termReq.raw_command().empty()) {
            std::istringstream iss(termReq.raw_command());
            iss >> action;
            action = toUpper(action);
        }

        ru::RUResponse ruResp;
        ruResp.set_request_id(p_request.request_id());
        ruResp.set_status_code(200);
        ruResp.set_status_message("SUCCESS");

        std::lock_guard<std::mutex> lock(this->m_stateMutex);
        if (!termReq.parameters().empty()) {
            parseKeyValueParams(termReq.parameters(),
                                this->m_txPowerDbm,
                                this->m_centerFreqMhz,
                                this->m_antennaPorts);
        }

        if (0 == action.compare("START") || 0 == action.compare("ENABLE")) {
            this->m_rfState = "TX_ACTIVE";
            ruResp.set_details("RU RF frontend [" + this->m_unitId +
                               "] activated (TX_ACTIVE). [Power=" +
                               std::to_string(static_cast<int>(this->m_txPowerDbm)) + " dBm, Freq=" +
                               std::to_string(static_cast<int>(this->m_centerFreqMhz)) + " MHz, Ports=" +
                               std::to_string(this->m_antennaPorts) + "]");
        } else if (0 == action.compare("STOP") || 0 == action.compare("DISABLE")) {
            this->m_rfState = "STANDBY";
            ruResp.set_details("RU RF frontend [" + this->m_unitId + "] stopped. [State=STANDBY]");
        } else if (0 == action.compare("STATUS")) {
            std::ostringstream oss;
            oss << "RU Hardware Status:\n"
                << "  Unit ID:        " << this->m_unitId << "\n"
                << "  RF State:       " << this->m_rfState << "\n"
                << "  Tx Power:       " << this->m_txPowerDbm << " dBm\n"
                << "  Center Freq:    " << this->m_centerFreqMhz << " MHz\n"
                << "  Antenna Ports:  " << this->m_antennaPorts;
            ruResp.set_details(oss.str());
        } else {
            ruResp.set_details("RU executed action '" + action + "' successfully.");
        }

        ruResp.set_rf_state(this->m_rfState);
        ruResp.set_actual_tx_power(this->m_txPowerDbm);
        ruResp.set_actual_freq_mhz(this->m_centerFreqMhz);

        responseEnvelope.mutable_payload()->PackFrom(ruResp);
        return responseEnvelope;
    }

    return std::nullopt;
}

mailbox::MailboxRequest
RUMailboxManager::buildResponse(const mailbox::MailboxRequest &p_request) {
    auto response = this->processBusinessLogic(p_request);
    if (response.has_value()) {
        return response.value();
    }
    mailbox::MailboxRequest fallback;
    fallback.set_request_id(p_request.request_id());
    fallback.set_source("RU");
    fallback.set_destination(p_request.source());
    return fallback;
}

std::string RUMailboxManager::getRfState() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_rfState;
}

double RUMailboxManager::getTxPowerDbm() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_txPowerDbm;
}

double RUMailboxManager::getCenterFreqMhz() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_centerFreqMhz;
}

int32_t RUMailboxManager::getAntennaPorts() const {
    std::lock_guard<std::mutex> lock(this->m_stateMutex);
    return this->m_antennaPorts;
}
