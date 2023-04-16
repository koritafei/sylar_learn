#include "sylar/scheduler.h"

// #include "sylar/hook.h"
#include "sylar/log.h"
#include "sylar/macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = LOG_NAME("system");

static thread_local Scheduler *t_scheduler           = nullptr;
static thread_local Fiber     *t_scheduler_mainfiber = nullptr;

Scheduler::Scheduler(size_t threads, bool user_callers, const std::string &name)
    : m_name(name) {
  ASSERT(threads > 0);

  /// 调用caller线程
  if (user_callers) {
    sylar::Fiber::GetThis();
    --threads;

    ASSERT(GetThis() == nullptr);
    t_scheduler = this;
    /// 创建调度协程
    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    sylar::Thread::SetName(m_name);

    t_scheduler_mainfiber = m_rootFiber.get();
    m_rootThread          = sylar::GetThreadId();
    m_threadIds.push_back(m_rootThread);

  } else {
    m_rootThread = -1;
  }

  m_threadCount = threads;
}

Scheduler::~Scheduler() {
  ASSERT(m_stopping);
  if (GetThis() == nullptr) {
    t_scheduler = nullptr;
  }
}

Scheduler *Scheduler::GetThis() {
  return t_scheduler;
}

Fiber *Scheduler::GetMainFiber() {
  return t_scheduler_mainfiber;
}

void Scheduler::start() {
  MutexType::Lock lock{m_mutex};
  if (!m_stopping) {
    return;
  }

  /// 创建线程池
  m_threads.resize(m_threadCount);
  for (size_t i = 0; i < m_threadCount; i++) {
    m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                  m_name + "_" + std::to_string(i)));
    m_threadIds.push_back(m_threads[i]->getId());
  }
  lock.unlock();
}

void Scheduler::stop() {
  m_autoStop = true;
  if (m_rootFiber && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::INIT ||
       m_rootFiber->getState() == Fiber::TERM)) {
    LOG_INFO(g_logger) << this << " stopped";
    m_stopping = true;

    if (stopping()) {
      return;
    }
  }

  if (-1 != m_rootThread) {
    ASSERT(GetThis() == this);
  } else {
    ASSERT(GetThis() != this);
  }

  m_stopping = true;
  for (size_t i = 0; i < m_threadCount; i++) {
    tickle();
  }

  if (m_rootFiber) {
    tickle();
  }

  if (m_rootFiber) {
    if (!stopping()) {
      m_rootFiber->call();
    }
  }

  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock{m_mutex};
    m_threads.swap(thrs);
  }

  for (auto &t : thrs) {
    t->join();
  }
}

void Scheduler::setThis() {
  t_scheduler = this;
}

void Scheduler::run() {
  LOG_DEBUG(g_logger) << m_name << " run";
  // set_hook_enable(true);
  setThis();
  if (m_rootThread != sylar::GetThreadId()) {
    t_scheduler_mainfiber = Fiber::GetThis().get();
  }

  Fiber::ptr     idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr     cb_fiber;
  FiberAndThread ft;
  while (true) {
    ft.reset();
    bool tickle_me = false;
    bool is_active = false;
    {
      MutexType::Lock lock{m_mutex};
      auto            iter = m_fibers.begin();
      while (iter != m_fibers.end()) {
        if (iter->m_thread != -1 && iter->m_thread != sylar::GetThreadId()) {
          ++iter;
          tickle_me = true;
          continue;
        }

        ASSERT(iter->m_fiber || iter->m_cb);
        if (iter->m_fiber && iter->m_fiber->getState() == Fiber::EXEC) {
          iter++;
          continue;
        }

        ft = *iter;
        m_fibers.erase(iter++);
        ++m_activeThreadCount;
        is_active = true;
        break;
      }

      tickle_me |= iter != m_fibers.end();
    }

    if (tickle_me) {
      tickle();
    }

    if (ft.m_fiber && (Fiber::TERM != ft.m_fiber->getState() &&
                       Fiber::EXCEPT != ft.m_fiber->getState())) {
      ft.m_fiber->swapIn();
      --m_activeThreadCount;

      if (Fiber::READY == ft.m_fiber->getState()) {
        schedule(ft.m_fiber);
      } else if (Fiber::TERM != ft.m_fiber->getState() &&
                 Fiber::EXCEPT != ft.m_fiber->getState()) {
        ft.m_fiber->m_state = Fiber::HOLD;
      }
      ft.reset();
    } else if (ft.m_cb) {
      if (cb_fiber) {
        cb_fiber->reset(ft.m_cb);
      } else {
        cb_fiber.reset(new Fiber(ft.m_cb));
      }

      ft.reset();
      cb_fiber->swapIn();
      --m_activeThreadCount;
      if (Fiber::READY == cb_fiber->getState()) {
        schedule(cb_fiber);
        cb_fiber.reset();
      } else if (Fiber::TERM == cb_fiber->getState() ||
                 Fiber::EXCEPT == cb_fiber->getState()) {
        cb_fiber->reset(nullptr);
      } else {
        cb_fiber->m_state = Fiber::HOLD;
        cb_fiber.reset();
      }
    } else {
      if (is_active) {
        --m_activeThreadCount;
        continue;
      }

      if (idle_fiber->getState() == Fiber::TERM) {
        LOG_INFO(g_logger) << "idle fiber term";
        break;
      }
      ++m_idleThreadCount;
      idle_fiber->swapIn();
      --m_idleThreadCount;

      if (Fiber::TERM != idle_fiber->getState() &&
          Fiber::EXCEPT != idle_fiber->getState()) {
        idle_fiber->m_state = Fiber::HOLD;
      }
    }
  }
}

void Scheduler::tickle() {
  LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
  MutexType::Lock lock{m_mutex};
  return m_autoStop && m_stopping && m_fibers.empty() &&
         m_activeThreadCount == 0;
}

void Scheduler::idle() {
  LOG_INFO(g_logger) << "idle";
  while (!stopping()) {
    sylar::Fiber::YieldToHold();
  }
}

void Scheduler::switchTo(int thread) {
  ASSERT(Scheduler::GetThis() != nullptr);
  if (this != Scheduler::GetThis()) {
    if (thread == -1 || thread == sylar::GetThreadId()) {
      return;
    }
  }

  schedule(Fiber::GetThis(), thread);
  Fiber::YieldToHold();
}

std::ostream &Scheduler::dump(std::ostream &os) {
  os << "[Scheduler name=" << m_name << " size=" << m_threadCount
     << " active_count=" << m_activeThreadCount
     << " idle_count=" << m_idleThreadCount << " stopping=" << m_stopping
     << " ]" << std::endl
     << "    ";
  for (size_t i = 0; i < m_threadIds.size(); ++i) {
    if (i) {
      os << ", ";
    }
    os << m_threadIds[i];
  }
  return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler *target) {
  m_caller = Scheduler::GetThis();
  if (target) {
    target->switchTo();
  }
}

SchedulerSwitcher::~SchedulerSwitcher() {
  if (m_caller) {
    m_caller->switchTo();
  }
}

}  // namespace sylar
