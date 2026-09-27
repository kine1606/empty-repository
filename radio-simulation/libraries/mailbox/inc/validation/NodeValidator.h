#ifndef NODE_VALIDATOR_H
#define NODE_VALIDATOR_H

#include "BaseValidator.h"

/**
 * @brief Validates common fields for Node requests.
 */
class NodeValidator : public BaseValidator {
public:
    NodeValidator() = default;
    ~NodeValidator() override = default;
};

#endif
