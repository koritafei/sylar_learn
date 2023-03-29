#include "sylar/log.h"

#include <iostream>

// #include "gtest/gtest.h"

int main() {
  std::cout << sylar::LogLevel::toString(sylar::LogLevel::DEBUG) << std::endl;
  std::cout << sylar::LogLevel::fromString("debug") << std::endl;

  sylar::Logger::ptr logger(new sylar::Logger);
  logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

  sylar::FileLogAppender::ptr file_appender(
      new sylar::FileLogAppender("./log.txt"));
  sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
  file_appender->setFormatter(fmt);
  file_appender->setLogLevel(sylar::LogLevel::ERROR);

  logger->addAppender(file_appender);

  // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__, __LINE__, 0,
  // sylar::GetThreadId(), sylar::GetFiberId(), time(0))); event->getSS() <<
  // "hello sylar log"; logger->log(sylar::LogLevel::DEBUG, event);
  std::cout << "hello sylar log" << std::endl;

  LOG_INFO(logger) << "test macro";
  LOG_ERROR(logger) << "test macro error";

  LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");

  auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
  LOG_INFO(l) << "xxx";
}
