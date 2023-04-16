#include "sylar/thread.h"

#include <unistd.h>

#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/util.h"

sylar::Logger::ptr g_logger = LOG_ROOT();

int          count = 0;
sylar::Mutex s_mutex;

void func1() {
  LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                     << " this.name: " << sylar::Thread::GetThis()->getName()
                     << " id: " << sylar::GetThreadId()
                     << " this.id: " << sylar::Thread::GetThis()->getId();
  for (int i = 0; i < 100000; ++i) {
    // sylar::RWMutex::WriteLock lock(s_mutex);
    sylar::Mutex::Lock lock(s_mutex);
    ++count;
  }
}

void func2() {
  while (true) {
    LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  }
}

void func3() {
  while (true) {
    LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  }
}

int main(int argc, char** argv) {
  LOG_INFO(g_logger) << "thread test begin";
  YAML::Node root =
      YAML::LoadFile("/home/koritafei/code/sylar_l/bin/conf/log.yml");
  sylar::Config::LoadFromYaml(root);

  std::vector<sylar::Thread::ptr> thrs;
  for (int i = 0; i < 1; ++i) {
    sylar::Thread::ptr thr(
        new sylar::Thread(&func1, "name_" + std::to_string(i * 2)));
    // sylar::Thread::ptr thr2(
    // new sylar::Thread(&func3, "name_" + std::to_string(i * 2 + 1)));
    thrs.push_back(thr);
    // thrs.push_back(thr2);
  }

  for (size_t i = 0; i < thrs.size(); ++i) {
    thrs[i]->join();
  }
  LOG_INFO(g_logger) << "thread test end";
  LOG_INFO(g_logger) << "count=" << count;

  return 0;
}