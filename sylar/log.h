#ifndef __LOG__H__
#define __LOG__H__

#include <yaml-cpp/yaml.h>

#include <cstdarg>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <tuple>
#include <vector>

#include "sylar/mutex.h"
#include "sylar/singleton.h"
#include "sylar/thread.h"
#include "sylar/util.h"

#define LOG_LEVEL(logger, level)                                               \
  if (level >= logger->getLevel())                                             \
  sylar::LogEventWrap(                                                         \
      sylar::LogEvent::ptr(new sylar::LogEvent(logger,                         \
                                               level,                          \
                                               __FILE__,                       \
                                               __LINE__,                       \
                                               0,                              \
                                               time(0),                        \
                                               sylar::GetThreadId(),           \
                                               sylar::GetFiberId(),            \
                                               sylar::Thread::GetName())))     \
      .getSS()

#define LOG_DEBUG(logger) LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define LOG_INFO(logger)  LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define LOG_WARN(logger)  LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define LOG_LEVEL_FORMAT(logger, level, fmt, ...)                              \
  if (level >= logger->getLevel())                                             \
  sylar::LogEventWrap(                                                         \
      sylar::LogEvent::ptr(new sylar::LogEvent(logger,                         \
                                               level,                          \
                                               __FILE__,                       \
                                               __LINE__,                       \
                                               0,                              \
                                               time(0),                        \
                                               sylar::GetThreadId(),           \
                                               sylar::GetFiberId(),            \
                                               sylar::thread::GetName())))     \
      .getLogEvent()                                                           \
      ->format(fmt, __VA_ARGS__)
#define LOG_FMT_DEBUG(logger, fmt, ...)                                        \
  LOG_LEVEL_FORMAT(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...)                                         \
  LOG_LEVEL_FORMAT(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...)                                         \
  LOG_LEVEL_FORMAT(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...)                                        \
  LOG_LEVEL_FORMAT(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...)                                        \
  LOG_LEVEL_FORMAT(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_ROOT()     sylar::LoggerMgr::GetInstance()->getRoot()
#define LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)
namespace sylar {

class Logger;
class LoggerManager;

/// @brief 日志级别
class LogLevel {
public:
  enum Level {
    UNKNOWN = 0,
    DEBUG   = 1,
    INFO    = 2,
    WARN    = 3,
    ERROR   = 4,
    FATAL   = 5,
  };
  /// @brief string转loglevel
  /// @param str
  /// @return
  static LogLevel::Level fromString(const std::string &str);

  /// @brief loglevel 转string
  /// @param level
  /// @return
  static const char *toString(LogLevel::Level level);
};

class LogEvent {
public:
  typedef std::shared_ptr<LogEvent> ptr;

  /**
   * @brief 构造函数
   * @param[in] logger 日志器
   * @param[in] level 日志级别
   * @param[in] file 文件名
   * @param[in] line 文件行号
   * @param[in] elapse 程序启动依赖的耗时(毫秒)
   * @param[in] thread_id 线程id
   * @param[in] fiber_id 协程id
   * @param[in] time 日志事件(秒)
   * @param[in] thread_name 线程名称
   */
  LogEvent(std::shared_ptr<Logger> logger,
           LogLevel::Level         level,
           const char             *file,
           int32_t                 line,
           uint64_t                elapse,
           uint64_t                time,
           uint32_t                threadid,
           uint32_t                fiberid,
           std::string             threadname);

  std::shared_ptr<Logger> getLogger() const {
    return m_logger;
  }

  LogLevel::Level getLevel() const {
    return m_level;
  }

  const char *getFile() const {
    return m_file;
  }

  int32_t getLine() const {
    return m_line;
  }

  uint64_t getElapse() const {
    return m_elapse;
  }

  uint64_t getTime() const {
    return m_time;
  }

  uint32_t getThreadId() const {
    return m_threadId;
  }

  uint32_t getFiberId() const {
    return m_fiberId;
  }

  std::string getThreadName() const {
    return m_threadName;
  }

  std::stringstream &getSS() {
    return m_ss;
  }

  std::string getContent() const {
    return m_ss.str();
  }

  void format(const char *fmt, ...);

  void format(const char *fmt, va_list va);

private:
  std::shared_ptr<Logger> m_logger;
  LogLevel::Level         m_level;
  const char             *m_file;
  int32_t                 m_line     = 0;
  uint64_t                m_elapse   = 0;
  uint64_t                m_time     = 0;
  uint32_t                m_threadId = 0;
  uint32_t                m_fiberId  = 0;
  std::string             m_threadName;
  std::stringstream       m_ss;
};

class LogEventWrap {
public:
  LogEventWrap(LogEvent::ptr e);
  ~LogEventWrap();

  LogEvent::ptr getLogEvent() const {
    return m_event;
  }

  std::stringstream &getSS();

private:
  LogEvent::ptr m_event;
};

class LogFormatter {
public:
  typedef std::shared_ptr<LogFormatter> ptr;

  /**
   * @brief 构造函数
   * @param[in] pattern 格式模板
   * @details
   *  %m 消息
   *  %p 日志级别
   *  %r 累计毫秒数
   *  %c 日志名称
   *  %t 线程id
   *  %n 换行
   *  %d 时间
   *  %f 文件名
   *  %l 行号
   *  %T 制表符
   *  %F 协程id
   *  %N 线程名称
   *
   *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
   */
  LogFormatter(const std::string &pattern);

  std::string   format(std::shared_ptr<Logger> logger,
                       LogLevel::Level         level,
                       LogEvent::ptr           event);
  std::ostream &format(std::ostream           &ofs,
                       std::shared_ptr<Logger> logger,
                       LogLevel::Level         level,
                       LogEvent::ptr           event);

public:
  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;

    ~FormatItem() {
    }

    /**
     * @brief 格式化日志到流
     * @param[in, out] os 日志输出流
     * @param[in] logger 日志器
     * @param[in] level 日志等级
     * @param[in] event 日志事件
     */
    virtual void format(std::ostream           &os,
                        std::shared_ptr<Logger> logger,
                        LogLevel::Level         level,
                        LogEvent::ptr           event) = 0;
  };

  void init();

  bool isError() const {
    return m_error;
  }

  std::string getPattern() const {
    return m_pattern;
  }

private:
  std::string                  m_pattern;
  std::vector<FormatItem::ptr> m_patterns;
  bool                         m_error = false;
};

class LogAppender {
  friend class Logger;

public:
  typedef std::shared_ptr<LogAppender> ptr;
  typedef SpinLock                     MutexType;

  virtual ~LogAppender() {
  }

  virtual void        logger(std::shared_ptr<Logger> logger,
                             LogLevel::Level         level,
                             LogEvent::ptr           event) = 0;
  virtual std::string toYamlString()              = 0;

  LogLevel::Level getLogLevel() const {
    return m_level;
  }

  void setLogLevel(LogLevel::Level level) {
    m_level = level;
  }

  void setFormatter(LogFormatter::ptr val);

  LogFormatter::ptr getFormatter() const;

protected:
  LogLevel::Level m_level        = LogLevel::DEBUG;
  bool            m_hasFormatter = false;
  /// Mutex
  MutexType         m_mutex;
  LogFormatter::ptr m_formatter;
};

class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;
  void        logger(std::shared_ptr<Logger> logger,
                     LogLevel::Level         level,
                     LogEvent::ptr           event) override;
  std::string toYamlString() override;
};

class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;
  FileLogAppender(const std::string &filename);

  void        logger(std::shared_ptr<Logger> logger,
                     LogLevel::Level         level,
                     LogEvent::ptr           event) override;
  std::string toYamlString() override;

  bool reopen();

private:
  std::string   m_filename;
  std::ofstream m_filestream;
  uint64_t      lasttime = 0;
};

class Logger : public std::enable_shared_from_this<Logger> {
  friend class LoggerManager;

public:
  typedef std::shared_ptr<Logger> ptr;
  typedef SpinLock                MutexType;

  Logger(const std::string &name = "root");

  void logger(LogLevel::Level level, LogEvent::ptr event);

  void        debug(LogEvent::ptr event);
  void        info(LogEvent::ptr event);
  void        warn(LogEvent::ptr event);
  void        error(LogEvent::ptr event);
  void        fatal(LogEvent::ptr event);
  std::string toYamlString();

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  void clearAppenders();

  LogLevel::Level getLevel() const {
    return m_level;
  }

  void setLevel(LogLevel::Level level) {
    m_level = level;
  }

  std::string getName() const {
    return m_name;
  }

  void setFormatter(LogFormatter::ptr formatter);
  void setFormatter(const std::string &name);

  LogFormatter::ptr getFormatter();

private:
  /// @brief 日志名称
  std::string     m_name;
  LogLevel::Level m_level = LogLevel::DEBUG;
  /// Mutex
  MutexType                   m_mutex;
  std::list<LogAppender::ptr> m_appender;
  LogFormatter::ptr           m_formatter;
  Logger::ptr                 m_root;
};

class LoggerManager {
public:
  typedef SpinLock MutexType;
  LoggerManager();

  Logger::ptr getLogger(const std::string &name);

  void init();

  Logger::ptr getRoot() {
    return m_root;
  }
  std::string toYamlString();

private:
  /// Mutex
  MutexType                          m_mutex;
  std::map<std::string, Logger::ptr> m_loggers;
  Logger::ptr                        m_root;
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;

}  // namespace sylar

#endif /* __LOG__H__ */
