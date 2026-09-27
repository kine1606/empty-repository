#ifndef LOGGER_TPP
#define LOGGER_TPP

template <typename T> std::string Logger::serialize(const T &p_value) {
  std::ostringstream stream;
  stream << p_value;
  return stream.str();
}

template <typename T>
std::string Logger::serialize(const std::vector<T> &p_value) {
  std::ostringstream stream;
  bool first = true;

  for (const auto &parameter : p_value) {
    if (!first) {
      stream << ' ';
    }

    stream << serialize(parameter);
    first = false;
  }

  return stream.str();
}

template <typename... Args>
void Logger::log(LogLevel p_logLevel, const char *p_file, int p_line,
                 const std::string &p_format, const Args &...p_args) {

  ActiveLogGuard guard(*this);

  if (!guard.accepted()) {
    this->m_droppedEntries.fetch_add(1, std::memory_order_relaxed);
    return;
  }

  if (p_logLevel >= LogLevel::INVALID) {
    // [TODO] Replace with custom exception
    throw std::invalid_argument("Invalid log level used.");
  }

  {
    std::lock_guard<std::mutex> lock(this->m_enabledLevelsMutex);

    if (!this->m_enabledLevels[static_cast<std::size_t>(p_logLevel)]) {
      return;
    }
  }

  std::vector<std::string> arguments{serialize(p_args)...};
  const std::string message = formatMessage(p_format, arguments);

  processLog(p_logLevel, p_file, p_line, message);
}

#endif