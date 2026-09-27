#ifndef ERROR_MESSAGES_H
#define ERROR_MESSAGES_H

#include <string>

/**
 * @brief Defines reusable error messages for mailbox validation and handling.
 */
namespace ErrorMessages {

/** @name Request identifier errors */
/** @{ */
/** @brief Indicates that a request identifier was not provided. */
inline constexpr char REQUEST_ID_EMPTY[] = "RequestId is required";

/** @name Payload errors */
/** @{ */
/** @brief Indicates that a request payload was not provided. */
inline constexpr char PAYLOAD_EMPTY[] = "Payload is required";

/** @brief Indicates that a sender was not provided. */
inline constexpr char SENDER_EMPTY[] = "Sender is required";

/** @name Support errors */
/** @{ */
/** @brief Indicates that a request targets an unsupported destination. */
inline constexpr char UNSUPPORTED_DESTINATION[] =
    "Destination is not supported";

} // namespace ErrorMessages

#endif // ERROR_MESSAGES_H
