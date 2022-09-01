#ifndef __LOG__H__
#define __LOG__H__

#include <inttypes.h>
#include <stdint.h>

#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace sylar {

class Logger;

/**
 * @brief 日志级别
 * */
class LogLevel {
public:
  /**
   * @brief 日志级别枚举
   * */
  enum Level {
    // 未知级别
    UNKONW = 0,
    // DEBUG
    DEBUG = 1,
    // INFO
    INFO = 2,
    // WARN
    WARN = 3,
    // ERROR
    ERROR = 4,
    // FATAL
    FATAL = 5
  };

  /**
   * @brief 日志级别转string输出
   * @param  level
   * @return const char*
   * */
  static const char *ToString(LogLevel::Level level);

  /**
   * @brief  文本转日志级别
   * @param  str
   * @return LogLevel::Level
   * */
  static LogLevel::Level FromString(const std::string &str);
};

/**
 * @brief 日志事件
 * */
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
           uint32_t                espace,
           uint32_t                thread_id,
           uint32_t                fiber_id,
           uint64_t                time,
           const std::string      &thread_name);

  /**
   * @brief 返回文件名
   * @return const char*
   * */
  const char *getFile() const {
    return m_file;
  }

  /**
   * @brief 返回行号
   * @return int32_t
   * */
  int32_t getLine() const {
    return m_line;
  }

  /**
   * @brief 返回耗时
   * @return uint32_t
   * */
  uint32_t getEspace() const {
    return m_elapse;
  }

  /**
   * @brief 返回线程id
   * @return uint32_t
   * */
  uint32_t getThreadId() const {
    return m_threadId;
  }

  /**
   * @brief 返回协程id
   * @return uint32_t
   * */
  uint32_t getFiberId() const {
    return m_fiberId;
  }

  /**
   * @brief 返回时间
   * @return uint64_t
   * */
  uint64_t getTime() const {
    return m_time;
  }

  /**
   * @brief 返回线程名称
   * @return const std::string&
   * */
  const std::string &getThreadName() const {
    return m_threadName;
  }

  /**
   * @brief 返回日志内容
   * @return std::string
   * */
  std::string getContent() const {
    return m_ss.str();
  }

  /**
   * @brief 返回日志器
   * @return std::shared_ptr<Logger>
   * */
  std::shared_ptr<Logger> getLogger() const {
    return m_logger;
  }

  /**
   * @brief 返回日志级别
   * @return LogLevel::Level
   * */
  LogLevel::Level getLogLevel() const {
    return m_level;
  }

  /**
   * @brief 返回日志内容的字符串流
   * @return std::stringstream
   * */
  std::stringstream &getSS() {
    return m_ss;
  }

  void format(const char *fmt, ...);
  void format(const char *fmt, va_list al);

private:
  const char             *m_file     = nullptr;  // 文件名
  int32_t                 m_line     = 0;        // 行号
  uint32_t                m_elapse   = 0;  // 程序启动到现在的毫秒数
  uint32_t                m_threadId = 0;  // 线程id
  uint32_t                m_fiberId  = 0;  // 协程id
  uint64_t                m_time     = 0;  // 时间戳
  std::string             m_threadName;    // 线程名称
  std::stringstream       m_ss;            // 日志内容流
  std::shared_ptr<Logger> m_logger;        //日志器
  LogLevel::Level         m_level;         // 日志级别
};

// 日志格式化
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

  std::string format(LogEvent::ptr event);

  class FormatItem {
  public:
    typedef std::shared_ptr<FormatItem> ptr;
    virtual ~FormatItem() {
    }

    /**
     * @brief   格式化日志到流
     * @param  os               os 日志输出流
     * @param  logger           logger 日志器
     * @param  level            level 日志等级
     * @param  event            event 日志事件
     * */
    virtual void format(std::ostream           &os,
                        std::shared_ptr<Logger> logger,
                        LogLevel::Level         level,
                        LogEvent::ptr           event) = 0;
  };

  /**
   * @brief 初始化，解析日志模板
   * */
  void init();

private:
  std::string                  m_pattern;  // 日志格式模板
  std::vector<FormatItem::ptr> m_items;    // 解析后的日志格式
  bool                         m_error;    // 是否存在错误
};

// 日志输出地
class LogAppender {
public:
  typedef std::shared_ptr<LogAppender> ptr;

  virtual ~LogAppender() {
  }

  virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

  void setFormatter(LogFormatter::ptr formatter) {
    m_formatter = formatter;
  }

  LogFormatter::ptr GetFormatter() const {
    return m_formatter;
  }

protected:
  LogLevel::Level m_level        = LogLevel::DEBUG;  //日志级别
  bool            m_hasFormatter = false;  // 是否有自己的日志格式器
  LogFormatter::ptr m_formatter;
};

// 日志输出器
class Logger {
public:
  typedef std::shared_ptr<Logger> ptr;
  Logger(const std::string &name = "root");

  void Log(LogLevel::Level level, LogEvent::ptr event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);

  void delAppender(LogAppender::ptr appender);

  LogLevel::Level GetLevel() const {
    return m_level;
  }

  void setLevel(LogLevel::Level level) {
    m_level = level;
  }

private:
  std::string                 m_name;       // 日志名称
  LogLevel::Level             m_level;      // 日志级别
  std::list<LogAppender::ptr> m_appenders;  // 目的地集合
};

/**
 * @brief 输出到控制台
 * */
class StdoutLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;

  virtual void log(LogLevel::Level level, LogEvent::ptr event) override;

private:
};

/**
 * @brief 输出到文件
 * */
class FileLogAppender : public LogAppender {
public:
  typedef std::shared_ptr<FileLogAppender> ptr;
  FileLogAppender(const std::string &filename);
  virtual void log(LogLevel::Level level, LogEvent::ptr event) override;

  bool reopen();

private:
  std::string   m_filename;
  std::ofstream m_filestream;
};

}  // namespace sylar

#endif /* __LOG__H__ */
