#ifndef TERMINAL_MAILBOX_H
#define TERMINAL_MAILBOX_H

#include "Mailbox.h"

/**
 * @brief Thread-safe mailbox queue dedicated to the Terminal.
 */
class TerminalMailbox : public Mailbox {
public:
    TerminalMailbox() = default;
    ~TerminalMailbox() = default;
};

#endif
