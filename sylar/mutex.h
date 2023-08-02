#ifndef __MUTEX__H__
#define __MUTEX__H__

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include <atomic>
#include <functional>
#include <list>
#include <memory>

#include "sylar/noncopyable.h"

namespace sylar {

/// @brief  信号量
class Semaphore : Noncopyable {
public:
  /// @brief 构造函数
  /// @param count 信号量
  Semaphore(uint32_t count = 0);

  /// @brief 析构函数
  ~Semaphore();

  /// @brief 等待信号量
  void wait();

  /// @brief 释放信号量
  void notify();

private:
  sem_t m_semaphore;
};

}  // namespace sylar

#endif /* __MUTEX__H__ */
