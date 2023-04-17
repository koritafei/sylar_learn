#ifndef __MUTEX__H__
#define __MUTEX__H__

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

#include "sylar/fiber.h"
#include "sylar/noncopyable.h"

namespace sylar {

/// @brief 信号量
class Semaphore {
public:
  /**
   * @brief 构造函数
   * @param[in] count 信号量值的大小
   */
  Semaphore(uint32_t count = 0);
  /**
   * @brief 析构函数
   */
  ~Semaphore();
  /**
   * @brief 获取信号量
   */
  void wait();
  /**
   * @brief 释放信号量
   */
  void notify();

private:
  sem_t m_semaphore;
};

/**
 * @brief 局部锁的模板实现
 */
template <typename T>
class ScopedLockImpl {
public:
  /**
   * @brief 构造函数
   * @param[in] mutex Mutex
   */
  ScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.lock();
    m_locked = true;
  }

  /**
   * @brief 析构函数,自动释放锁
   */
  ~ScopedLockImpl() {
    unlock();
  }

  /**
   * @brief 加锁
   */
  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  /**
   * @brief 解锁
   */
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  /// @brief mutex
  T &m_mutex;
  /// @brief 是否已上锁
  bool m_locked;
};

/**
 * @brief 局部读锁模板实现
 */
template <typename T>
class ReadScopedLockImpl {
public:
  /**
   * @brief 构造函数
   * @param[in] mutex 读写锁
   */
  ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    m_locked = true;
  }
  /**
   * @brief 析构函数,自动释放锁
   */
  ~ReadScopedLockImpl() {
    unlock();
  }
  /**
   * @brief 上读锁
   */
  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  /**
   * @brief 释放锁
   */
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  /// @brief mutex
  T &m_mutex;
  /// @brief 是否上锁
  bool m_locked;
};

/**
 * @brief 局部写锁模板实现
 */
template <typename T>
class WriteScopedLockImpl {
public:
  /**
   * @brief 构造函数
   * @param[in] mutex 读写锁
   */
  WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    m_locked = true;
  }
  /**
   * @brief 析构函数
   */
  ~WriteScopedLockImpl() {
    unlock();
  }
  /**
   * @brief 上写锁
   */
  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }
  /**
   * @brief 解锁
   */
  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  /// @brief mutex
  T &m_mutex;
  /// @brief 是否上锁
  bool m_locked;
};

/// @brief 互斥锁
class Mutex : Noncopyable {
public:
  /// @brief 局部锁
  typedef ScopedLockImpl<Mutex> Lock;

  /**
   * @brief 构造函数
   */
  Mutex() {
    pthread_mutex_init(&m_mutex, nullptr);
  }

  /**
   * @brief 析构函数
   */
  ~Mutex() {
    pthread_mutex_destroy(&m_mutex);
  }

  /// @brief 加锁
  void lock() {
    pthread_mutex_lock(&m_mutex);
  }

  /// @brief 解锁
  void unlock() {
    pthread_mutex_unlock(&m_mutex);
  }

private:
  /// @brief mutex
  pthread_mutex_t m_mutex;
};

/// @brief 空锁用于调试
class NullMutex : Noncopyable {
public:
  /// @brief 局部锁
  typedef ScopedLockImpl<NullMutex> Lock;

  NullMutex() {
  }

  ~NullMutex() {
  }

  void lock() {
  }

  void unlock() {
  }
};

/// @brief 读写互斥量
class RWMutex : Noncopyable {
public:
  /// @brief 局部读锁
  typedef ReadScopedLockImpl<RWMutex> ReadLock;
  /// @brief 局部写锁
  typedef WriteScopedLockImpl<RWMutex> WriteLock;
  /**
   * @brief 构造函数
   */
  RWMutex() {
    pthread_rwlock_init(&m_lock, nullptr);
  }
  /**
   * @brief 析构函数
   */
  ~RWMutex() {
    pthread_rwlock_destroy(&m_lock);
  }
  /**
   * @brief 上读锁
   */
  void rdlock() {
    pthread_rwlock_rdlock(&m_lock);
  }
  /**
   * @brief 上写锁
   */
  void wrlock() {
    pthread_rwlock_wrlock(&m_lock);
  }
  /**
   * @brief 解锁
   */
  void unlock() {
    pthread_rwlock_unlock(&m_lock);
  }

private:
  /// @brief 读写锁
  pthread_rwlock_t m_lock;
};

/**
 * @brief 空读写锁(用于调试)
 */
class NullRWMutex : Noncopyable {
public:
  NullRWMutex() {
  }
  ~NullRWMutex() {
  }

  void rdlock() {
  }
  void wrlock() {
  }
  void unlock() {
  }
};

/// @brief 自旋锁
class SpinLock : Noncopyable {
public:
  typedef ScopedLockImpl<SpinLock> Lock;
  /**
   * @brief 构造函数
   */
  SpinLock() {
    pthread_spin_init(&m_lock, 0);
  }
  /**
   * @brief 析构函数
   */
  ~SpinLock() {
    pthread_spin_destroy(&m_lock);
  }
  /**
   * @brief 上锁
   */
  void lock() {
    pthread_spin_lock(&m_lock);
  }
  /**
   * @brief 解锁
   */
  void unlock() {
    pthread_spin_unlock(&m_lock);
  }

private:
  /// @brief 自旋锁
  pthread_spinlock_t m_lock;
};

class CASLock : Noncopyable {
public:
  /// @brief 局部锁
  typedef ScopedLockImpl<CASLock> Lock;
  /**
   * @brief 构造函数
   */
  CASLock() {
    m_mutex.clear();
  }
  /**
   * @brief 析构函数
   */
  ~CASLock() {
  }

  /**
   * @brief 上锁
   */
  void lock() {
    while (std::atomic_flag_test_and_set_explicit(&m_mutex,
                                                  std::memory_order_acquire))
      ;
  }

  /**
   * @brief 解锁
   */
  void unlock() {
    std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
  }

private:
  /// @brief 原子状态
  volatile std::atomic_flag m_mutex;
};

class Scheduler;

class FiberSemaphore : Noncopyable {
public:
  typedef SpinLock MutexType;

  FiberSemaphore(size_t initial_concurrency = 0);
  ~FiberSemaphore();

  bool tryWait();
  void wait();
  void notify();

  size_t getConcurrency() const {
    return m_concurrency;
  }

  void reset() {
    m_concurrency = 0;
  }

private:
  MutexType                                     m_mutex;
  std::list<std::pair<Scheduler *, Fiber::ptr>> m_waiters;
  size_t                                        m_concurrency;
};

}  // namespace sylar

#endif /* __MUTEX__H__ */
