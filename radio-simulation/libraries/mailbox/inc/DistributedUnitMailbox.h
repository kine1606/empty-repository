#ifndef DISTRIBUTED_UNIT_MAILBOX_H
#define DISTRIBUTED_UNIT_MAILBOX_H

#include "Mailbox.h"

/**
 * @brief Thread-safe mailbox queue dedicated to the Distributed Unit (DU).
 */
class DistributedUnitMailbox : public Mailbox {
public:
    DistributedUnitMailbox() = default;
    ~DistributedUnitMailbox() = default;
};

#endif
