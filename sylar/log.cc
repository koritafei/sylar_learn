#include "log.h"

#include <sstream>

namespace sylar {

LogLevel::Level LogLevel::fromString(const std::string& str) {
#define XX(level, v)                                                           \
  if (str == #v) {                                                             \
    return LogLevel::level;                                                    \
  }

  XX(UNKNOWN, UNKNOWN);
  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);

  XX(UNKNOWN, unknown);
  XX(DEBUG, debug);
  XX(INFO, info);
  XX(WARN, warn);
  XX(ERROR, error);
  XX(FATAL, fatal);

  return LogLevel::UNKNOWN;
#undef XX
}

const char* LogLevel::toString(LogLevel::Level level) {
  switch (level) {
#define XX(name)                                                               \
  case LogLevel::name:                                                         \
    return #name;                                                              \
    break;

    XX(UNKNOWN);
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);

#undef XX

    default:
      return "UNKNOWN";
      break;
  }

  return "UNKNOWN";
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger,
                   LogLevel::Level         level,
                   const char*             file,
                   int32_t                 line,
                   uint64_t                elapse,
                   uint64_t                time,
                   uint32_t                threadid,
                   uint32_t                fiberid,
                   std::string             threadname)
    : m_logger(logger),
      m_level(level),
      m_file(file),
      m_line(line),
      m_elapse(elapse),
      m_time(time),
      m_threadId(threadid),
      m_fiberId(fiberid),
      m_threadName(threadname) {
}

void LogEvent::format(const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  format(fmt, va);
  va_end(va);
}

void LogEvent::format(const char* fmt, va_list al) {
  char* buff = nullptr;
  int   len  = vasprintf(&buff, fmt, al);
  if (-1 != len) {
    m_ss << std::string(buff, len);
    free(buff);
  }
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {
}

LogEventWrap::~LogEventWrap() {
  m_event->getLogger()->logger(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWrap::getSS() {
  return m_event->getSS();
}

LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern) {
  init();
}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
  MessageFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
  LevelFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getLevel();
  }
};
class ElapseFormatItem : public LogFormatter::FormatItem {
public:
  ElapseFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getElapse();
  }
};
class NameFormatItem : public LogFormatter::FormatItem {
public:
  NameFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getLogger()->getName();
  }
};
class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
  ThreadIdFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getThreadId();
  }
};
class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
  FiberIdFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getFiberId();
  }
};
class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
  ThreadNameFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getThreadName();
  }
};
class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
  DateTimeFormatItem(const std::string& str = "%Y-%m-%d %H:%M:%S")
      : m_format(str) {
    if (m_format.empty()) {
      m_format = "%Y-%m-%d %H:%M:%S";
    }
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    struct tm tm;
    time_t    time = event->getTime();

    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), m_format.c_str(), &tm);
    os << buf;
  }

private:
  std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
  FilenameFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
  LineFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
  NewLineFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
  StringFormatItem(const std::string& str) : m_str(str) {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << m_str;
  }

private:
  std::string m_str;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
  TabFormatItem(const std::string& str = "") {
  }
  void format(std::ostream&           os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << "\t";
  }
};

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level         level,
                                 LogEvent::ptr           event) {
  std::stringstream ss;
  for (auto& i : m_patterns) {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

std::ostream& LogFormatter::format(std::ostream&           ofs,
                                   std::shared_ptr<Logger> logger,
                                   LogLevel::Level         level,
                                   LogEvent::ptr           event) {
  for (auto& i : m_patterns) {
    i->format(ofs, logger, level, event);
  }
  return ofs;
}

void LogFormatter::init() {
  /// string fmt type
  std::vector<std::tuple<std::string, std::string, int>> vec;
  std::string                                            nstr;

  for (std::size_t i = 0; i < m_pattern.size(); i++) {
    if (m_pattern[i] != '%') {
      nstr.append(1, m_pattern[i]);
      continue;
    }

    if ((i + 1) < m_pattern.size()) {
      if (m_pattern[i + 1] == '%') {
        nstr.append(1, '%');
        continue;
      }
    }

    std::string str;
    std::string fmt;
    int         fmt_status = 0;
    int         fmt_begin  = 0;
    std::size_t n          = i + 1;

    while (n < m_pattern.size()) {
      if (!fmt_status && (!isalpha(m_pattern[n]) && '{' != m_pattern[n] &&
                          '}' != m_pattern[n])) {
        str = m_pattern.substr(i + 1, n - i - 1);
        break;
      }

      if (fmt_status == 0) {
        if ('{' == m_pattern[n]) {
          str        = m_pattern.substr(i + 1, n - i - 1);
          fmt_status = 1;
          fmt_begin  = n;
          ++n;
          continue;
        }
      } else if (1 == fmt_status) {
        if ('}' == m_pattern[n]) {
          fmt        = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          fmt_status = 0;
          ++n;
          break;
        }
      }
      ++n;
      if (n == m_pattern.size()) {
        if (str.empty()) {
          str = m_pattern.substr(i + 1);
        }
      }
    }

    if (fmt_status == 0) {
      if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
        nstr.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n - 1;
    } else if (fmt_status == 1) {
      std::cout << "pattern parse error: " << m_pattern << " - "
                << m_pattern.substr(i) << std::endl;
      m_error = true;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    }
  }

  if (!nstr.empty()) {
    vec.push_back(std::make_tuple(nstr, "", 0));
  }

  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string& str)>>
      s_format_items = {
#define XX(str, C)                                                             \
  {                                                                            \
#str, [](const std::string& fmt) {                                         \
      return FormatItem::ptr(new C(fmt));                                      \
    }                                                                          \
  }

          XX(d, DateTimeFormatItem),
          XX(f, FilenameFormatItem),
          XX(t, ThreadIdFormatItem),
          XX(T, TabFormatItem),
          XX(m, MessageFormatItem),
          XX(p, LevelFormatItem),
          XX(r, ElapseFormatItem),
          XX(c, NameFormatItem),
          XX(n, NewLineFormatItem),
          XX(N, ThreadNameFormatItem),
          XX(F, FiberIdFormatItem),
          XX(l, LineFormatItem),

#undef XX

      };
  for (auto& i : vec) {
    if (std::get<2>(i) == 0) {
      m_patterns.push_back(
          FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    } else {
      auto iter = s_format_items.find(std::get<0>(i));
      if (iter == s_format_items.end()) {
        m_patterns.push_back(FormatItem::ptr(
            new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
        m_error = true;
      } else {
        m_patterns.push_back(iter->second(std::get<1>(i)));
      }
    }
  }
}

LogFormatter::ptr LogAppender::getFormatter() const {
  return m_formatter;
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
  m_formatter = val;
  if (m_formatter) {
    m_hasFormatter = true;
  } else {
    m_hasFormatter = false;
  }
}

void StdoutLogAppender::logger(std::shared_ptr<Logger> logger,
                               LogLevel::Level         level,
                               LogEvent::ptr           event) {
  if (level >= m_level) {
    m_formatter->format(std::cout, logger, level, event);
  }
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename) {
  if (!m_filename.empty()) {
    reopen();
  }
}

bool FileLogAppender::reopen() {
  if (m_filestream.is_open()) {
    m_filestream.close();
  }
  m_filestream.open(m_filename, std::ios::app);
  return m_filestream.is_open();
}

void FileLogAppender::logger(std::shared_ptr<Logger> logger,
                             LogLevel::Level         level,
                             LogEvent::ptr           event) {
  if (level >= m_level) {
    uint64_t now = event->getTime();
    if (now >= (lasttime + 3)) {
      reopen();
      lasttime = now;
    }

    if (!m_formatter->format(m_filestream, logger, level, event)) {
      std::cout << "error" << std::endl;
    }
  }
}

Logger::Logger(const std::string& name)
    : m_name(name),
      m_level(LogLevel::DEBUG),
      m_formatter(new LogFormatter(
          "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n")) {
}

LogFormatter::ptr Logger::getFormatter() {
  return m_formatter;
}

void Logger::setFormatter(LogFormatter::ptr formatter) {
  m_formatter = formatter;
  for (auto& i : m_appender) {
    if (!i->m_hasFormatter) {
      i->m_hasFormatter = true;
    }
  }
}

void Logger::setFormatter(const std::string& val) {
  std::cout << "---" << val << std::endl;
  LogFormatter::ptr new_val(new LogFormatter(val));

  if (new_val->isError()) {
    std::cout << "Logger setFormatter name=" << m_name << " value=" << val
              << " invalid formatter" << std::endl;
    return;
  }

  setFormatter(new_val);
}

void Logger::addAppender(LogAppender::ptr appender) {
  if (!appender->getFormatter()) {
    appender->m_formatter = m_formatter;
  }

  m_appender.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
  for (auto&& it = m_appender.begin(); it != m_appender.end(); it++) {
    if (*it == appender) {
      m_appender.erase(it);
      break;
    }
  }
}

void Logger::clearAppenders() {
  m_appender.clear();
}

void Logger::logger(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    auto self = shared_from_this();
    if (!m_appender.empty()) {
      for (auto& i : m_appender) {
        i->logger(self, level, event);
      }
    } else {
      m_root->logger(level, event);
    }
  }
}

void Logger::debug(LogEvent::ptr event) {
  logger(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
  logger(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
  logger(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
  logger(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
  logger(LogLevel::FATAL, event);
}

LoggerManager::LoggerManager() {
  m_root.reset(new Logger);
  m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
  m_loggers[m_root->m_name] = m_root;

  init();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
  auto iter = m_loggers.find(name);
  if (iter != m_loggers.end()) {
    return iter->second;
  }

  Logger::ptr logger(new Logger(name));
  logger->m_root  = m_root;
  m_loggers[name] = logger;

  return logger;
}

void LoggerManager::init() {
}

}  // namespace sylar
