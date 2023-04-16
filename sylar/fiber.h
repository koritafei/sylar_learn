#ifndef __FIBER_H__
#define __FIBER_H__

#include <ucontext.h>

#include <functional>
#include <memory>

namespace sylar {

class Scheduler;

// @brief 协程类
class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;

public:
  typedef std::shared_ptr<Fiber> ptr;

  /// @brief 协程状态
  enum State {
    /// @brief 初始化状态
    INIT,
    /// @brief 暂停状态
    HOLD,
    /// @brief 执行中状态
    EXEC,
    /// @brief 结束状态
    TERM,
    /// @brief 可执行状态
    READY,
    /// @brief 异常状态
    EXCEPT
  };

private:
  /**
   * @brief 无参构造函数
   * @attention 每个线程的第一个构造函数
   * */
  Fiber();

public:
  /**
   * @brief 构造函数
   * @param  cb               协程执行函数
   * @param  stacksize        协程运行栈大小
   * @param  user_callser     是否在mainfiber上调度
   * */
  Fiber(std::function<void()> cb,
        size_t                stacksize    = 0,
        bool                  user_callser = false);

  /**
   * @brief 析构函数
   * */
  ~Fiber();

  /**
   * @brief 重置协程执行函数,并设置状态
   * @pre getState() 为 INIT, TERM, EXCEPT
   * @post getState() = INIT
   */
  void reset(std::function<void()> cb);

  /**
   * @brief 将当前协程切换到运行状态
   * @pre getState() != EXEC
   * @post getState() = EXEC
   */
  void swapIn();

  /**
   * @brief 将当前协程切换到后台
   */
  void swapOut();

  /**
   * @brief 将当前线程切换到执行状态
   * @pre 执行的为当前线程的主协程
   */
  void call();

  /**
   * @brief 将当前线程切换到后台
   * @pre 执行的为该协程
   * @post 返回到线程的主协程
   */
  void back();

  /**
   * @brief 返回协程id
   */
  uint64_t getId() const {
    return m_id;
  }

  /**
   * @brief 返回协程状态
   */
  State getState() const {
    return m_state;
  }

public:
  /**
   * @brief 设置当前线程的运行协程
   * @param[in] f 运行协程
   */
  static void SetThis(Fiber *f);

  /**
   * @brief 返回当前所在的协程
   */
  static Fiber::ptr GetThis();

  /**
   * @brief 将当前协程切换到后台,并设置为READY状态
   * @post getState() = READY
   */
  static void YieldToReady();

  /**
   * @brief 将当前协程切换到后台,并设置为READY状态
   * @post getState() = READY
   */
  static void YieldToHold();

  /**
   * @brief 返回当前协程的总数量
   */
  static uint64_t TotalFibers();

  /**
   * @brief 协程执行函数
   * @post 执行完成返回到线程主协程
   */
  static void MainFunc();

  /**
   * @brief 协程执行函数
   * @post 执行完成返回到线程调度协程
   */
  static void CallerMainFunc();

  /**
   * @brief 获取当前协程的id
   */
  static uint64_t GetFiberId();

private:
  /// @brief 协程ID
  uint64_t m_id = 0;
  /// @brief 协程运行栈大小
  uint32_t m_stacksize = 0;
  /// @brief 协程状态
  State m_state = INIT;
  /**
   * @brief 协程上下文
   * */
  ucontext_t m_ctx;
  /// @brief 协程运行栈指针
  void *m_stack = nullptr;
  /// @brief 协程运行函数
  std::function<void()> m_cb;
};

}  // namespace sylar

#endif