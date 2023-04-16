#include "sylar/fiber.h"

#include <atomic>

#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/macro.h"
#include "sylar/scheduler.h"

namespace sylar {

static sylar::Logger::ptr g_logger = LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber     *t_fiber       = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size",
                             128 * 1024,
                             "fiber stack size");

class MallocStackAllocator {
public:
  static void *Alloc(size_t size) {
    return malloc(size);
  }

  static void Dealloc(void *ptr, size_t size) {
    return free(ptr);
  }
};

using StackAlloc = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
  if (t_fiber) {
    return t_fiber->getId();
  }

  return 0;
}

Fiber::Fiber() {
  m_state = EXEC;
  SetThis(this);

  if (getcontext(&m_ctx)) {
    ASSERT2(false, "getcontext");
  }
  ++s_fiber_count;
  LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool user_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
  ++s_fiber_count;
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

  m_stack = StackAlloc::Alloc(m_stacksize);
  if (getcontext(&m_ctx)) {
    ASSERT2(false, "getcontext");
  }

  m_ctx.uc_link          = nullptr;
  m_ctx.uc_stack.ss_sp   = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  if (!user_caller) {
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
  } else {
    makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
  }

  LOG_DEBUG(g_logger) << "Fiber::Fiber id= " << m_id;
}

Fiber::~Fiber() {
  --s_fiber_count;
  if (m_stack) {
    LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                        << " total=" << s_fiber_count << " state=" << m_state;
    ASSERT(TERM == m_state || EXCEPT == m_state || INIT == m_state);
    StackAlloc::Dealloc(m_stack, m_stacksize);
  } else {
    ASSERT(!m_cb);
    ASSERT(m_state == EXEC);

    Fiber *curr = t_fiber;
    if (curr == this) {
      SetThis(nullptr);
    }
  }
  LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                      << " total=" << s_fiber_count;
}

/// @brief 重置协程函数，并重置状态 INIT，TERM, EXCEPT
/// @param cb
void Fiber::reset(std::function<void()> cb) {
  ASSERT(m_stack);
  ASSERT(TERM == m_state || EXCEPT == m_state || INIT == m_state);
  m_cb = cb;
  if (getcontext(&m_ctx)) {
    ASSERT2(false, "getcontext");
  }

  m_ctx.uc_link          = nullptr;
  m_ctx.uc_stack.ss_sp   = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  m_state = INIT;
}

void Fiber::call() {
  SetThis(this);
  m_state = EXEC;

  if (swapcontext(&(t_threadFiber->m_ctx), &m_ctx)) {
    ASSERT2(false, "swapcontext");
  }
}

void Fiber::back() {
  SetThis(t_threadFiber.get());

  if (swapcontext(&m_ctx, &(t_threadFiber->m_ctx))) {
    ASSERT2(false, "swapcontext");
  }
}

/// @brief 切换协程执行
void Fiber::swapIn() {
  SetThis(this);
  ASSERT(m_state != EXEC);
  m_state = EXEC;

  if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
    // if (swapcontext(&(t_threadFiber->m_ctx), &m_ctx)) {
    ASSERT2(false, "swapcontext");
  }
}

/// @brief 切换到后台执行
void Fiber::swapOut() {
  SetThis(t_threadFiber.get());

  if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
    // if (swapcontext(&m_ctx, &(t_threadFiber->m_ctx))) {
    ASSERT2(false, "swapcontext");
  }
}

/// @brief  设置协程
/// @param f
void Fiber::SetThis(Fiber *f) {
  t_fiber = f;
}

/// @brief 返回当前协程
/// @return
Fiber::ptr Fiber::GetThis() {
  if (t_fiber) {
    return t_fiber->shared_from_this();
  }

  Fiber::ptr main_fiber(new Fiber);
  ASSERT(main_fiber.get() == t_fiber);
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}

/// @brief 协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur->m_state == EXEC);
  cur->m_state = READY;
  cur->swapOut();
}

/// @brief 协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThis();
  ASSERT(cur->m_state == EXEC);
  cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
  return s_fiber_count;
}

void Fiber::MainFunc() {
  Fiber::ptr curr = GetThis();
  ASSERT(curr);
  try {
    curr->m_cb();
    curr->m_cb    = nullptr;
    curr->m_state = TERM;
  } catch (const std::exception &ex) {
    curr->m_state = EXCEPT;
    LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                        << " fiber_id=" << curr->getId() << std::endl
                        << sylar::BackTraceToString();
  } catch (...) {
    curr->m_state = EXCEPT;
    LOG_ERROR(g_logger) << "Fiber Except"
                        << " fiber_id=" << curr->getId() << std::endl
                        << sylar::BackTraceToString();
  }

  auto raw_ptr = curr.get();
  curr.reset();
  raw_ptr->swapOut();
  ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

void Fiber::CallerMainFunc() {
  Fiber::ptr curr = GetThis();
  ASSERT(curr);

  try {
    curr->m_cb();
    curr->m_cb    = nullptr;
    curr->m_state = TERM;
  } catch (const std::exception &ex) {
    curr->m_state = EXCEPT;
    LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                        << " fiber_id=" << curr->getId() << std::endl
                        << sylar::BackTraceToString();
  } catch (...) {
    curr->m_state = EXCEPT;
    LOG_ERROR(g_logger) << "Fiber Except"
                        << " fiber_id=" << curr->getId() << std::endl
                        << sylar::BackTraceToString();
  }

  auto raw_ptr = curr.get();
  curr.reset();
  raw_ptr->back();
  ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}  // namespace sylar
