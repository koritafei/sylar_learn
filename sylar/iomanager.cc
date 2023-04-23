#include "sylar/iomanager.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "sylar/log.h"
#include "sylar/macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = LOG_NAME("system");

enum EpollCtlOp {};

static std::ostream &operator<<(std::ostream &os, const EpollCtlOp &op) {
  switch ((int)(op)) {
#define XX(ctl)                                                                \
  case (ctl):                                                                  \
    return os << #ctl;
    XX(EPOLL_CTL_ADD);
    XX(EPOLL_CTL_DEL);
    XX(EPOLL_CTL_MOD);
    default:
      return os << (int)op;
#undef XX
  }
}

static std::ostream &operator<<(std::ostream &os, EPOLL_EVENTS events) {
  if (!events) {
    return os << "0";
  }
  bool first = true;
#define XX(E)                                                                  \
  if (events & E) {                                                            \
    if (!first) {                                                              \
      os << "/";                                                               \
    }                                                                          \
    os << #E;                                                                  \
    first = false;                                                             \
  }
  XX(EPOLLIN);
  XX(EPOLLPRI);
  XX(EPOLLOUT);
  XX(EPOLLRDNORM);
  XX(EPOLLRDBAND);
  XX(EPOLLWRNORM);
  XX(EPOLLWRBAND);
  XX(EPOLLMSG);
  XX(EPOLLERR);
  XX(EPOLLHUP);
  XX(EPOLLRDHUP);
  XX(EPOLLONESHOT);
  XX(EPOLLET);
#undef XX
  return os;
}

IOManager::FdContext::EventContext &IOManager::FdContext::getContext(
    IOManager::Event event) {
  switch (event) {
    case IOManager::READ:
      return m_read;
    case IOManager::WRITE:
      return m_write;
    default:
      ASSERT2(false, "getContext");
  }
  throw std::invalid_argument("getContext invlid argument");
}

void IOManager::FdContext::resetContext(EventContext &ctx) {
  ctx.m_scheduler = nullptr;
  ctx.m_fiber.reset();
  ctx.m_cb = nullptr;
}

void IOManager::FdContext::triggerContext(IOManager::Event event) {
  LOG_INFO(g_logger) << "fd=" << m_fd << " triggerEvent event=" << event
                     << " events=" << m_events;
  ASSERT(m_events & event);

  if (UNLIKELY(!(event & event))) {
    return;
  }

  m_events          = (Event)(m_events & ~event);
  EventContext &ctx = getContext(event);
  if (ctx.m_cb) {
    ctx.m_scheduler->schedule(&ctx.m_cb);
  } else {
    ctx.m_scheduler->schedule(&ctx.m_fiber);
  }

  ctx.m_scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threads, bool user_caller, const std::string &name)
    : Scheduler(threads, user_caller, name) {
  m_epfd = epoll_create(5000);
  ASSERT(m_epfd > 0);
  int rt = pipe(m_tickleFds);
  ASSERT(!rt);

  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events  = EPOLLIN | EPOLLOUT;
  event.data.fd = m_tickleFds[0];

  rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
  ASSERT(!rt);
  rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
  ASSERT(!rt);

  contextResize(32);

  start();
}

IOManager::~IOManager() {
  stop();
  close(m_epfd);
  close(m_tickleFds[0]);
  close(m_tickleFds[1]);

  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (m_fdContexts[i]) {
      delete m_fdContexts[i];
    }
  }
}

void IOManager::contextResize(size_t size) {
  m_fdContexts.resize(size);

  for (size_t i = 0; i < m_fdContexts.size(); ++i) {
    if (!m_fdContexts[i]) {
      m_fdContexts[i]       = new FdContext;
      m_fdContexts[i]->m_fd = i;
    }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
  FdContext            *fd_ctx = nullptr;
  RWMutexType::ReadLock lock{m_mutex};
  if ((int)m_fdContexts.size() > fd) {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    lock.unlock();
    RWMutexType::WriteLock lock2{m_mutex};
    contextResize(fd * 1.5);
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutexType::Lock lock2{fd_ctx->m_mutex};
  if (UNLIKELY(fd_ctx->m_events & event)) {
    LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                        << " event=" << (EPOLL_EVENTS)event
                        << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->m_events;

    ASSERT(!(fd_ctx->m_events & event));
  }

  int         op = fd_ctx->m_events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events   = EPOLLET | fd_ctx->m_events | event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << (EpollCtlOp)op
                        << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events
                        << "):" << rt << " (" << errno << ") ("
                        << strerror(errno) << ") fd_ctx->m_events="
                        << (EPOLL_EVENTS)fd_ctx->m_events;
    return -1;
  }

  ++m_pendingEventCount;
  fd_ctx->m_events                   = (Event)(fd_ctx->m_events | event);
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  ASSERT(!event_ctx.m_scheduler && !event_ctx.m_fiber && !event_ctx.m_cb);
  event_ctx.m_scheduler = Scheduler::GetThis();

  if (cb) {
    event_ctx.m_cb.swap(cb);
  } else {
    event_ctx.m_fiber = Fiber::GetThis();
    ASSERT2(event_ctx.m_fiber->getState() == Fiber::EXEC,
            "state= " << event_ctx.m_fiber->getState());
  }

  return 0;
}

bool IOManager::delEvent(int fd, Event event) {
  RWMutexType::ReadLock lock{m_mutex};
  if ((int)m_fdContexts.size() <= fd) {
    return false;
  }

  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2{fd_ctx->m_mutex};
  if (UNLIKELY(!(fd_ctx->m_events & event))) {
    return false;
  }

  Event       new_event = (Event)(fd_ctx->m_events & ~event);
  int         op        = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epevent;
  epevent.events   = EPOLLET | new_event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << (EpollCtlOp)op
                        << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events
                        << "):" << rt << " (" << errno << ") ("
                        << strerror(errno) << ")";
    return false;
  }

  --m_pendingEventCount;
  fd_ctx->m_events                   = new_event;
  FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
  fd_ctx->resetContext(event_ctx);

  return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
  RWMutexType::ReadLock lock{m_mutex};
  if ((int)m_fdContexts.size() <= fd) {
    return false;
  }

  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();

  FdContext::MutexType::Lock lock2{fd_ctx->m_mutex};
  if (UNLIKELY(!(fd_ctx->m_events & event))) {
    return false;
  }

  Event       new_event = (Event)(fd_ctx->m_events & ~event);
  int         op        = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events   = EPOLLET | new_event;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << (EpollCtlOp)op
                        << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events
                        << "):" << rt << " (" << errno << ") ("
                        << strerror(errno) << ")";

    return false;
  }

  fd_ctx->triggerContext(event);
  --m_pendingEventCount;
  return true;
}

bool IOManager::cancelAll(int fd) {
  RWMutexType::ReadLock lock{m_mutex};
  if ((int)m_fdContexts.size() <= fd) {
    return false;
  }

  FdContext *fd_ctx = m_fdContexts[fd];
  lock.unlock();
  FdContext::MutexType::Lock lock2{fd_ctx->m_mutex};
  if (!fd_ctx->m_events) {
    return false;
  }

  int         op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events   = 0;
  epevent.data.ptr = fd_ctx;

  int rt = epoll_ctl(m_epfd, op, fd, &epevent);
  if (rt) {
    LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << (EpollCtlOp)op
                        << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events
                        << "):" << rt << " (" << errno << ") ("
                        << strerror(errno) << ")";
    return false;
  }

  if (fd_ctx->m_events & READ) {
    fd_ctx->triggerContext(READ);
    --m_pendingEventCount;
  }

  if (fd_ctx->m_events & WRITE) {
    fd_ctx->triggerContext(WRITE);
    --m_pendingEventCount;
  }

  ASSERT(fd_ctx->m_events == 0);
  return true;
}

IOManager *IOManager::GetThis() {
  return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

void IOManager::tickle() {
  if (!hasIdleThread()) {
    return;
  }

  int rt = write(m_tickleFds[1], "T", 1);
  assert(rt == 1);
}

bool IOManager::stopping(uint64_t &time_out) {
  time_out = getNextTimer();
  return time_out == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

bool IOManager::stopping() {
  uint64_t time_out = 0;
  return stopping(time_out);
}

void IOManager::onTimerInsertedAtFront() {
  tickle();
}

void IOManager::idle() {
  LOG_DEBUG(g_logger) << "idle";
  const uint64_t               MAX_EVENTS = 256;
  epoll_event                 *events     = new epoll_event[MAX_EVENTS];
  std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr) {
    delete[] ptr;
  });

  while (true) {
    uint64_t next_timeout = 0;
    if (UNLIKELY(stopping(next_timeout))) {
      LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
      break;
    }

    int rt = 0;
    do {
      static const int MAX_TIMEOUT = 3000;
      if (next_timeout != ~0ull) {
        next_timeout =
            (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
      } else {
        next_timeout = MAX_TIMEOUT;
      }

      rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
      if (rt < 0 && errno == EINTR) {
      } else {
        break;
      }
    } while (true);

    std::vector<std::function<void()>> cbs;
    listExpiredCb(cbs);
    if (!cbs.empty()) {
      LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
      schedule(cbs.begin(), cbs.end());
      cbs.clear();
    }

    for (int i = 0; i < rt; ++i) {
      epoll_event &event = events[i];
      if (event.data.fd == m_tickleFds[0]) {
        uint8_t dummy[256];
        while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
          ;
        continue;
      }

      FdContext                 *fd_ctx = (FdContext *)event.data.ptr;
      FdContext::MutexType::Lock lock{fd_ctx->m_mutex};

      if (event.events & (EPOLLERR | EPOLLHUP)) {
        event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->m_events;
      }

      int real_events = NONE;
      if (event.events & EPOLLIN) {
        real_events |= READ;
      }
      if (event.events & EPOLLOUT) {
        real_events |= WRITE;
      }

      if (NONE == (fd_ctx->m_events & real_events)) {
        continue;
      }
      int left_events = (fd_ctx->m_events & ~real_events);
      int op          = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events    = EPOLLOUT | left_events;

      int rt2 = epoll_ctl(m_epfd, op, fd_ctx->m_fd, &event);
      if (rt2) {
        LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << (EpollCtlOp)op << ", "
            << fd_ctx->m_fd << ", " << (EPOLL_EVENTS)event.events << "):" << rt2
            << " (" << errno << ") (" << strerror(errno) << ")";
        continue;
      }

      if (real_events & READ) {
        fd_ctx->triggerContext(READ);
        --m_pendingEventCount;
      }

      if (real_events & WRITE) {
        fd_ctx->triggerContext(WRITE);
        --m_pendingEventCount;
      }
    }

    Fiber::ptr cur     = Fiber::GetThis();
    auto       raw_ptr = cur.get();
    cur.reset();

    raw_ptr->swapOut();
  }
}

}  // namespace sylar
