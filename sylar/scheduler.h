
#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include "sylar/fiber.h"
#include "sylar/thread.h"

namespace sylar {

/**
 * @brief 协程调度器
 * @details 封装了一个N:M 协程调度器, 内部有一个线程池，支持协程在线程之间切换
 * */
class Scheduler {
public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex                      MutexType;

  /**
   * @brief 构造函数
   * @param[in] threads 线程数量
   * @param[in] use_caller 是否使用当前调用线程
   * @param[in] name 协程调度器名称
   */
  Scheduler(size_t             threads      = 1,
            bool               user_callers = true,
            const std::string &name         = "");
  /**
   * @brief 析构函数
   * */
  virtual ~Scheduler();

  /**
   * @brief 返回协程调度器名称
   * @return const std::string&
   * */
  const std::string &getName() const {
    return m_name;
  }

  /**
   * @brief 返回当前调度协程
   * @return Scheduler*
   * */
  static Scheduler *GetThis();

  /**
   * @brief  返回当前协程调度器的调度协程
   * @return Fiber*
   * */
  static Fiber *GetMainFiber();

  /**
   * @brief 启动协程调度器
   * */
  void start();

  /**
   * @brief 停止协程调度器
   * */
  void stop();

  /**
   * @brief  调度协程
   * @tparam FiberOrCb
   * @param  cb  协程或函数
   * @param  thr 协程执行的线程id，-1表示任意线程
   * */
  template <typename FiberOrCb>
  void schedule(FiberOrCb cb, int thr = -1) {
    bool need_tickle = false;
    {
      MutexType::Lock lock{m_mutex};
      need_tickle = ScheduleNoLock(cb, thr);
    }

    if (need_tickle) {
      tickle();
    }
  }

  /**
   * @brief  批量调度协程
   * @tparam InputIterator
   * @param  begin  协程数组的开始
   * @param  end    协程数组的结束
   * */
  template <typename InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_tickle = false;
    {
      MutexType::Lock lock{m_mutex};
      while (begin != end) {
        need_tickle = ScheduleNoLock(&*begin, -1) || need_tickle;
      }
    }

    if (need_tickle) {
      tickle();
    }
  }

  void          switchTo(int thread = -1);
  std::ostream &dump(std::ostream &os);

protected:
  /**
   * @brief 通知协程调度器存在任务
   * */
  virtual void tickle();

  /**
   * @brief 协程调度函数
   * */
  void run();

  /**
   * @brief 返回是否可以停止
   * @return true
   * @return false
   * */
  virtual bool stopping();

  /**
   * @brief 协程无任务可调度时执行idle协程
   * */
  virtual void idle();

  /**
   * @brief 设置当前的协程调度器
   * */
  void setThis();

  /**
   * @brief 是否有空闲协程
   * @return true
   * @return false
   * */
  bool hasIdleThread() {
    return m_idleThreadCount > 0;
  }

private:
  /**
   * @brief  协程调度启动(无锁)
   * @details 将FiberAndThread放入协程队列
   * @tparam FiberOrCb
   * */
  template <typename FiberOrCb>
  bool ScheduleNoLock(FiberOrCb cb, int thr) {
    bool           need_tickle = m_fibers.empty();
    FiberAndThread ft(cb, thr);
    if (ft.m_cb || ft.m_fiber) {
      m_fibers.push_back(ft);
    }

    return need_tickle;
  }

private:
  /// @brief 协程/函数/线程
  struct FiberAndThread {
    /// @brief 协程
    Fiber::ptr m_fiber;
    /**
     * @brief 函数
     * */
    std::function<void()> m_cb;
    /**
     * @brief 线程id
     * */
    int m_thread;

    /**
     * @brief 构造函数
     * @param[in] f 协程
     * @param[in] thr 线程id
     */
    FiberAndThread(Fiber::ptr f, int thr) : m_fiber(f), m_thread(thr) {
    }

    /**
     * @brief 构造函数
     * @param[in] f 协程指针
     * @param[in] thr 线程id
     * @post *f = nullptr
     */
    FiberAndThread(Fiber::ptr *f, int thr) : m_thread(thr) {
      m_fiber.swap(*f);
    }

    /**
     * @brief 构造函数
     * @param[in] f 协程执行函数
     * @param[in] thr 线程id
     */
    FiberAndThread(std::function<void()> cb, int thr)
        : m_cb(cb), m_thread(thr) {
    }

    /**
     * @brief 构造函数
     * @param[in] f 协程执行函数指针
     * @param[in] thr 线程id
     * @post *f = nullptr
     */
    FiberAndThread(std::function<void()> *cb, int thr) : m_thread(thr) {
      m_cb.swap(*cb);
    }

    /**
     * @brief 无参构造函数
     */
    FiberAndThread() : m_thread(-1) {
    }

    void reset() {
      m_thread = -1;
      m_fiber  = nullptr;
      m_cb     = nullptr;
    }
  };

private:
  /**
   * @brief mutex
   * */
  MutexType m_mutex;
  /**
   * @brief 线程池
   * */
  std::vector<Thread::ptr> m_threads;
  /**
   * @brief 待执行的协程队列
   * */
  std::list<FiberAndThread> m_fibers;
  /**
   * @brief use_caller为true时有效, 调度协程
   * */
  Fiber::ptr m_rootFiber;
  /**
   * @brief 协程调度器名称
   * */
  std::string m_name;

protected:
  /**
   * @brief 协程下的线程组id
   * */
  std::vector<int> m_threadIds;
  /**
   * @brief 线程数量
   * */
  size_t m_threadCount = 0;

  /**
   * @brief 工作线程数量
   * */
  std::atomic<size_t> m_activeThreadCount = 0;
  /**
   * @brief 空闲线程数量
   * */
  std::atomic<size_t> m_idleThreadCount = 0;
  /**
   * @brief 是否正在停止
   * */
  bool m_stopping = true;
  /**
   * @brief 是否自动停止
   * */
  bool m_autoStop = false;

  /**
   * @brief 主线程id(use_caller)
   * */
  int m_rootThread = 0;
};

class SchedulerSwitcher : public Noncopyable {
public:
  SchedulerSwitcher(Scheduler *target = nullptr);
  ~SchedulerSwitcher();

private:
  Scheduler *m_caller;
};

}  // namespace sylar

#endif
