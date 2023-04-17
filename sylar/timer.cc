#include "sylar/timer.h"

#include "sylar/util.h"

namespace sylar {

bool Timer::Comparator::operator()(const Timer::ptr &lhs,
                                   const Timer::ptr &rhs) const {
  if (!lhs && !rhs) {
    return false;
  }
  if (!lhs) {
    return true;
  }
  if (!rhs) {
    return false;
  }
  if (lhs->m_next < rhs->m_next) {
    return true;
  }

  if (rhs->m_next < lhs->m_next) {
    return false;
  }

  return lhs.get() < rhs.get();
}

Timer::Timer(uint64_t              ms,
             std::function<void()> cb,
             bool                  recurring,
             TimerManager         *manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
  m_next = sylar::GetCurrentMs() + m_ms;
}

Timer::Timer(uint64_t next) : m_next(next) {
}

bool Timer::cancel() {
  TimerManager::RWMutexType::WriteLock lock{m_manager->m_mutex};
  if (m_cb) {
    m_cb    = nullptr;
    auto it = m_manager->m_times.find(shared_from_this());
    m_manager->m_times.erase(it);
    return true;
  }

  return false;
}

bool Timer::refresh() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    return false;
  }

  auto it = m_manager->m_times.find(shared_from_this());
  if (it == m_manager->m_times.end()) {
    return false;
  }
  m_manager->m_times.erase(it);
  m_next = sylar::GetCurrentMs() + m_ms;
  m_manager->m_times.insert(shared_from_this());
  return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
  if (ms == m_ms && !from_now) {
    return true;
  }
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    return false;
  }

  auto it = m_manager->m_times.find(shared_from_this());
  if (it == m_manager->m_times.end()) {
    return false;
  }
  m_manager->m_times.erase(it);
  uint64_t start = 0;
  if (from_now) {
    start = sylar::GetCurrentMs();
  } else {
    start = m_next - ms;
  }

  m_ms   = ms;
  m_next = start + m_ms;
  m_manager->addTimer(shared_from_this(), lock);
  return true;
}

TimerManager::TimerManager() {
  m_previouseTime = sylar::GetCurrentMs();
}

TimerManager::~TimerManager() {
}

Timer::ptr TimerManager::addTimer(uint64_t              ms,
                                  std::function<void()> cb,
                                  bool                  recurring) {
  Timer::ptr             timer(new Timer(ms, cb, recurring, this));
  RWMutexType::WriteLock lock{m_mutex};
  addTimer(timer, lock);
  return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
  std::shared_ptr<void> tmp = weak_cond.lock();
  if (tmp) {
    cb();
  }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t              ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void>   weak_cond,
                                           bool                  recurring) {
  return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
  RWMutexType::ReadLock lock{m_mutex};
  m_tickled = false;
  if (m_times.empty()) {
    return ~0ull;
  }

  const Timer::ptr &next   = *m_times.begin();
  uint64_t          now_ms = sylar::GetCurrentMs();
  if (now_ms >= next->m_next) {
    return 0;
  } else {
    return now_ms - next->m_next;
  }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs) {
  uint64_t                now_ms = sylar::GetCurrentMs();
  std::vector<Timer::ptr> expires;
  {
    RWMutexType::ReadLock lock{m_mutex};
    if (m_times.empty()) {
      return;
    }
  }

  RWMutexType::WriteLock lock{m_mutex};
  if (m_times.empty()) {
    return;
  }

  bool rollover = detectClockRollover(now_ms);
  if (!rollover && ((*m_times.begin())->m_next > now_ms)) {
    return;
  }

  Timer::ptr now_timer(new Timer(now_ms));
  auto       it = rollover ? m_times.end() : m_times.lower_bound(now_timer);
  while (it != m_times.end() && (*it)->m_next == now_ms) {
    ++it;
  }

  expires.insert(expires.begin(), m_times.begin(), it);
  m_times.erase(m_times.begin(), it);
  expires.reserve(expires.size());

  for (auto &timer : expires) {
    cbs.push_back(timer->m_cb);
    if (timer->m_recurring) {
      timer->m_next = now_ms + timer->m_ms;
      m_times.insert(timer);
    } else {
      timer->m_cb = nullptr;
    }
  }
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock &lock) {
  auto it       = m_times.insert(val).first;
  bool at_front = (it == m_times.end()) && !m_tickled;

  if (at_front) {
    m_tickled = true;
  }
  lock.unlock();

  if (at_front) {
    onTimerInsertedAtFront();
  }
}

bool TimerManager::detectClockRollover(uint64_t now_ms) {
  bool rollover = false;
  if (now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
    rollover = true;
  }
  m_previouseTime = now_ms;
  return rollover;
}

bool TimerManager::hasTimer() {
  RWMutexType::ReadLock lock{m_mutex};
  return !m_times.empty();
}

}  // namespace sylar
