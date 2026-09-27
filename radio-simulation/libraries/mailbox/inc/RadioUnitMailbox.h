#ifndef RADIO_UNIT_MAILBOX_H
#define RADIO_UNIT_MAILBOX_H

#include "Mailbox.h"

/**
 * @brief Thread-safe mailbox queue dedicated to the Radio Unit (RU).
 */
class RadioUnitMailbox : public Mailbox {
public:
    RadioUnitMailbox() = default;
    ~RadioUnitMailbox() = default;
};

#endif
