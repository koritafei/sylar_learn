#ifndef __THREAD__H__
#define __THREAD__H__

#include <sys/syscall.h>

#include "sylar/mutex.h"
#include "sylar/util.h"

namespace sylar {

/**
 * @brief 线程类
 */
class Thread : Noncopyable {
public:
  typedef std::shared_ptr<Thread> ptr;

  /**
   * @brief 构造函数
   * @param[in] cb 线程执行函数
   * @param[in] name 线程名称
   */
  Thread(std::function<void()> cb, const std::string& name);

  ~Thread();

  pid_t getId() const {
    return m_id;
  }

  const std::string& getName() const {
    return m_name;
  }

  void join();

  static Thread*            GetThis();
  static const std::string& GetName();

  static void SetName(const std::string& name);

private:
  /**
   * @brief 线程执行函数
   */
  static void* run(void* args);

private:
  /// @brief 线程id
  pid_t m_id = -1;
  /// @brief 线程结构
  pthread_t m_thread = 0;
  /// @brief 线程执行函数
  std::function<void()> m_cb;
  /// @brief 线程名称
  std::string m_name;
  /// @brief 信号量
  Semaphore m_semaphore;
};

}  // namespace sylar

#endif /* __THREAD__H__ */
