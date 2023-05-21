#ifndef __FD_MANAGER__H__
#define __FD_MANAGER__H__

#include <memory>
#include <vector>

#include "sylar/singleton.h"
#include "sylar/thread.h"

namespace sylar {

/**
 * @brief 文件句柄上下文类
 * @details 管理文件句柄类型(是否socket), 是否阻塞，是否关闭，读写超时时间
 * */
class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
  typedef std::shared_ptr<FdCtx> ptr;

  /**
   * @brief  通过文件句柄构造FdCtx
   * @param  fd
   * */
  FdCtx(int fd);

  /**
   * @brief 析构函数
   * */
  ~FdCtx();

  /**
   * @brief 是否初始化完成
   * @return true
   * @return false
   * */
  bool isInit() const {
    return m_isInit;
  }

  /**
   * @brief 是否socket
   * @return true
   * @return false
   * */
  bool isSocket() const {
    return m_isSocket;
  }

  /**
   * @brief 是否已关闭
   * @return true
   * @return false
   * */
  bool isClose() const {
    return m_isClosed;
  }

  /**
   * @brief  设置用户主动设置非阻塞
   * @param  v  是否阻塞
   * */
  void setUserNonBlock(bool v) {
    m_userNonBlock = v;
  }

  /**
   * @brief 获取用户是否设置非阻塞
   * @return true
   * @return false
   * */
  bool getUserNonBlock() const {
    return m_userNonBlock;
  }

  /**
   * @brief 设置系统非阻塞
   * @param  v  是否阻塞
   * */
  void setSysNonBlock(bool v) {
    m_sysNonblock = v;
  }

  /**
   * @brief 获取系统非阻塞
   * @return true
   * @return false
   * */
  bool getSysNonBlock() const {
    return m_sysNonblock;
  }

  /**
   * @brief 设置超时时间
   * @param  type  类型SO_RCVTIMEO(读超时)，SENDTIMEO(写超时)
   * @param  v  时间毫秒
   * */
  void setTimeout(int type, uint64_t v);

  /**
   * @brief 获取超时时间
   * @param  type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
   * */
  uint64_t getTimeout(int type);

private:
  /**
   * @brief 初始化
   * @return true
   * @return false
   * */
  bool init();

private:
  /// @brief 是否初始化
  bool m_isInit : 1;
  /// @brief 是否socket
  bool m_isSocket : 1;
  /// @brief 是否hook非阻塞
  bool m_sysNonblock : 1;
  /// @brief 是否用户主动设置非阻塞
  bool m_userNonBlock : 1;
  /// @brief 是否关闭
  bool m_isClosed : 1;
  /// @brief 文件句柄
  int m_fd;
  /// @brief 读超时时间毫秒
  uint64_t m_recvTimeout;
  /// @brief 写超时时间毫秒
  uint64_t m_sendTimeout;
};

/**
 * @brief 文件管理句柄类
 * */
class FdManager {
public:
  typedef RWMutex RWMutexType;

  /// @brief 无参构造函数
  FdManager();
  /**
   * @brief   获取/创建文件句柄类
   * @param  fd           文件句柄
   * @param  auto_create  是否自动创建
   * @return FdCtx::ptr
   * */
  FdCtx::ptr get(int fd, bool auto_create = false);

  /**
   * @brief 删除文件句柄类
   * @param  fd  文件句柄
   * */
  void del(int fd);

private:
  /// @brief 读写锁
  RWMutexType m_mutex;
  /// @brief 文件句柄集合
  std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

}  // namespace sylar

#endif /* __FD_MANAGER__H__ */
