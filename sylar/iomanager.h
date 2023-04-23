#ifndef __IOMANAGER__H__
#define __IOMANAGER__H__

#include "sylar/scheduler.h"
#include "sylar/timer.h"

namespace sylar {

/// @brief 基于EPOLL的IO协程调度器
class IOManager : public Scheduler, public TimerManager {
public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex                    RWMutexType;

  /// @brief IO事件
  enum Event {
    /**
     * @brief 无事件
     * */
    NONE = 0x00,
    /**
     * @brief 读事件(EPOLLIN)
     * */
    READ = 0x01,
    /**
     * @brief 写事件(EPOLLOUT)
     * */
    WRITE = 0x02,
  };

private:
  /**
   * @brief Scoket事件上线文类
   * */
  struct FdContext {
    typedef Mutex MutexType;
    /**
     * @brief Socket事件上线文类
     * */
    struct EventContext {
      /// @brief 事件执行的调度器
      Scheduler* m_scheduler = nullptr;
      /// @brief 事件协程
      Fiber::ptr m_fiber;
      /// @brief 事件回调函数
      std::function<void()> m_cb;
    };

    /**
     * @brief 获取事件上下文类
     * @param  event  事件类型
     * @return EventContext 返回对应事件的上线文
     * */
    EventContext& getContext(Event event);
    /**
     * @brief 重置事件上下文
     * @param[in, out] ctx 待重置的上下文类
     */
    void resetContext(EventContext& ctx);
    /**
     * @brief 触发事件
     * @param[in] event 事件类型
     */
    void triggerContext(Event event);

    /// @brief 读事件上下文
    EventContext m_read;
    /// @brief 写时间上下文
    EventContext m_write;
    /// @brief 事件关联句柄
    int m_fd = 0;
    /// @brief 当前的事件
    Event m_events = NONE;
    /// @brief mutex
    MutexType m_mutex;
  };

public:
  /**
   * @brief 构造函数
   * @param[in] threads 线程数量
   * @param[in] use_caller 是否将调用线程包含进去
   * @param[in] name 调度器的名称
   */
  IOManager(size_t             threads    = 1,
            bool               use_caller = true,
            const std::string& name       = "");
  /**
   * @brief 析构函数
   * */
  ~IOManager();

  /**
   * @brief  事件句柄
   * @param  fd    socket句柄
   * @param  event 事件类型
   * @param  cb    事件回调函数
   * @return int   添加成功返回0，失败返回-1
   * */
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

  /**
   * @brief  删除事件
   * @param  fd     socket句柄
   * @param  event  时间类型
   * @attention 不会触发事件
   * @return true
   * @return false
   * */
  bool delEvent(int fd, Event event);

  /**
   * @brief  取消事件
   * @param  fd    socket句柄
   * @param  event 事件类型
   * @attention 如果存在事件则触发
   * @return true
   * @return false
   * */
  bool cancelEvent(int fd, Event event);

  /**
   * @brief 取消所有事件
   * @param  fd  socket句柄
   * @return true
   * @return false
   * */
  bool cancelAll(int fd);

  /**
   * @brief 返回当前的IOManager
   * @return IOManager*
   * */
  static IOManager* GetThis();

protected:
  void tickle() override;
  bool stopping() override;
  void idle() override;
  void onTimerInsertedAtFront() override;

  /// @brief 重置socket句柄上下文的容器大小
  /// @param size 容量大小
  void contextResize(size_t size);

  /// @brief 判断是否可以停止
  /// @param timeout 最近要触发的定时器时间间隔
  /// @return 返回是否可以停止
  bool stopping(uint64_t& timeout);

private:
  /// @brief epoll文件句柄
  int m_epfd = 0;
  /// @brief pipe 文件句柄
  int m_tickleFds[2];
  /// @brief 当前等待执行的时间数量
  std::atomic<size_t> m_pendingEventCount = 0;
  /// @brief IOManager的mutex
  RWMutexType m_mutex;
  /// @brief socket上下文容器
  std::vector<FdContext*> m_fdContexts;
};

}  // namespace sylar

#endif /* __IOMANAGER__H__ */
