#include "log.h"

namespace sylar {

class MessageFormatItem : public LogFormatter::FormatItem {
public:
  MessageFormatItem(const std::string &str = "") {
  }

  void format(std::ostream           &os,
              std::shared_ptr<Logger> logger,
              LogLevel::Level         level,
              LogEvent::ptr           event) override {
    os << event;
  }
};

Logger::Logger(const std::string &name) : m_name(name) {
}

void Logger::addAppender(LogAppender::ptr appender) {
  m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
  for (auto iter = m_appenders.begin(); iter != m_appenders.end(); ++iter) {
    if (iter != m_appenders.end()) {
      m_appenders.erase(iter);
      break;
    }
  }
}

void Logger::Log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    for (auto &iter : m_appenders) {
      iter->log(level, event);
    }
  }
}

void Logger::debug(LogEvent::ptr event) {
  debug(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event) {
  debug(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event) {
  debug(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event) {
  debug(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event) {
  debug(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string &filename)
    : m_filename(filename) {
}

void FileLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    m_filestream << m_formatter->format(event);
  }
}

bool FileLogAppender::reopen() {
  if (m_filestream) {
    m_filestream.close();
  }
  m_filestream.open(m_filename);

  return !!m_filestream;
}

void StdoutLogAppender::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    std::cout << m_formatter->format(event);
  }
}

std::string LogFormatter::format(LogEvent::ptr event) {
  std::stringstream ss;
  for (auto &iter : m_items) {
    iter->format(ss, event);
  }

  return ss.str();
}

LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) {
}

/**
 * @brief ?????????????????????%xxx %xxx{xxx} %%
 *        ?????????%m%p???{}??????????????????
 * */
void LogFormatter::init() {
  // str format type??? ????????? str??????????????????format
  std::vector<std::tuple<std::string, std::string, int>> vec;

  std::string nstr;  // ?????????????????????
  for (size_t i = 0; i < m_pattern.size(); ++i) {
    if (isspace(m_pattern[i])) {  // ????????????
      continue;
    }
    if ('%' != m_pattern[i]) {
      nstr.append(1, m_pattern[i]);
      continue;
    }

    if ((i + 1) < m_pattern.size()) {
      if ('%' == m_pattern[i + 1]) {
        nstr.append(1, '%');
        continue;
      }
    }

    size_t n = i + 1;
    int    fmt_status =
        0;  // fmt??????????????????0--???????????????1--?????????????????????{}????????????
    size_t fmt_begin = 0;  // ???????????????????????????

    std::string str;  // str????????????
    std::string fmt;  // str???????????????
    while (n < m_pattern.size()) {
      if (!fmt_status && isspace(m_pattern[n])) {  // ????????????
        ++n;
        continue;
      }

      if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                          m_pattern[n] != '}')) {
        str = m_pattern.substr(i + 1, n - i - 1);  // ???????????????%??????????????????
        break;
      }

      if (0 == fmt_status) {
        if ('{' == m_pattern[n]) {
          str = m_pattern.substr(i + 1, n - i - 1);  // %??????,{ ?????????????????????
          fmt_status = 1;                            // ????????????
          fmt_begin  = n;
          n++;
          continue;
        }
      } else if (1 == fmt_status) {
        if ('}' == m_pattern[n]) {  // ????????????{}???????????????
          fmt        = m_pattern.substr(fmt_begin + 1,
                                 n - fmt_begin - 1);  // ?????????????????????
          fmt_status = 0;
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

    if (0 == fmt_status) {
      if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
        nstr.clear();
      }

      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n - 1;
    } else if (1 == fmt_status) {
      std::cout << "pattern parse error: " << m_pattern << " - "
                << m_pattern.substr(i) << std::endl;
      m_error = true;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    }
  }

  if (nstr.empty()) {
    vec.push_back(std::make_tuple(nstr, "", 0));
  }

  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string &str)>>
      s_format_tems = {
#define XX(str, C)                                                             \
  {                                                                            \
#str, [](const std::string fmt) {                                          \
      return FormatItem::ptr(new C(fmt));                                      \
    }                                                                          \
  }
          XX(m, MessageFormatItem),     // m:??????
          XX(p, LevelFormatItem),       // p:????????????
          XX(r, ElapseFormatItem),      // r:???????????????
          XX(c, NameFormatItem),        // c:????????????
          XX(t, ThreadIdFormatItem),    // t:??????id
          XX(n, NewLineFormatItem),     // n:??????
          XX(d, DateTimeFormatItem),    // d:??????
          XX(f, FilenameFormatItem),    // f:?????????
          XX(l, LineFormatItem),        // l:??????
          XX(T, TabFormatItem),         // T:Tab
          XX(F, FiberIdFormatItem),     // F:??????id
          XX(N, ThreadNameFormatItem),  // N:????????????
#undef XX
      };

  for (auto &iter : vec) {
    if (0 == std::get<2>(iter)) {
      m_items.push_back(
          FormatItem::ptr(new StringFormatItem(std::get<0>(iter))));
    } else {
      auto it = s_format_tems.find(std::get<0>(iter));
      if (it == s_format_tems.end()) {
        m_items.push_back(FormatItem::ptr(new StringFormatItem(
            "<<error_format %" + std::get<0>(iter) + " >>")));
        m_error = true;
      } else {
        m_items.push_back(it->second(std::get<1>(iter)));
      }
    }
  }
}

}  // namespace sylar
