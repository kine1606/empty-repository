#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>

/**
 * @brief Exception class for handling errors in the application.
 */
class Exception : public std::exception {
protected:
  /**
   * @brief Error message associated with the exception.
   */
  std::string m_message;

public:
  /**
   * @brief Constructs an Exception with a given error message.
   * @param p_message The error message to be associated with the exception.
   */
  explicit Exception(const std::string &p_message);

  /**
   * @brief Returns the error message associated with the exception.
   * @return The error message as a C-style string.
   */
  const char *what() const noexcept override;
};

#endif