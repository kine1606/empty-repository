#ifndef NODE_SUPPORT_CHECKER_H
#define NODE_SUPPORT_CHECKER_H

#include <string>
#include <utility>

#include "ISupportChecker.h"

/**
 * @brief Checks if a request is supported by matching the expected destination.
 */
class NodeSupportChecker : public ISupportChecker {
public:
    /**
     * @brief Constructs a support checker expecting requests for @p p_nodeName.
     *
     * @param p_nodeName Expected destination node name.
     */
    explicit NodeSupportChecker(std::string p_nodeName)
        : m_nodeName(std::move(p_nodeName)) {}

    /**
     * @brief Evaluates whether the request is supported for this node.
     *
     * @param p_request Mailbox request to inspect.
     * @return true if destination matches this node, false otherwise.
     */
    bool isSupported(const mailbox::MailboxRequest &p_request) override {
        return p_request.destination() == this->m_nodeName;
    }

private:
    std::string m_nodeName;
};

#endif
