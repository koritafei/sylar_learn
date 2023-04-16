#include "sylar/scheduler.h"

#include "sylar/fiber.h"
#include "sylar/log.h"

static sylar::Logger::ptr g_logger = LOG_ROOT();

void test_fiber() {
  static int s_count = 1;
  LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

  sleep(1);
  if (--s_count >= 0) {
    sylar::Scheduler::GetThis()->schedule(&test_fiber, sylar::GetThreadId());
  }
}

int main(int argc, char** argv) {
  LOG_INFO(g_logger) << "main";
  sylar::Scheduler sc(5, false, "test");
  sc.start();
  sleep(2);
  LOG_INFO(g_logger) << "schedule";
  sc.schedule(&test_fiber);
  sc.stop();
  LOG_INFO(g_logger) << "over";
  return 0;
}