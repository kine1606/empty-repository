#include "Exception.h"

Exception::Exception(const std::string &p_message) : m_message(p_message) {}

const char *Exception::what() const noexcept { return m_message.c_str(); }