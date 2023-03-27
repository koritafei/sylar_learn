#ifndef __NONCOPYABLE__H__
#define __NONCOPYABLE__H__

namespace sylar {

class Noncopyable {
public:
  Noncopyable()  = default;
  ~Noncopyable() = default;

  Noncopyable(const Noncopyable &)            = delete;
  Noncopyable &operator=(const Noncopyable &) = delete;

private:
};

}  // namespace sylar

#endif /* __NONCOPYABLE__H__ */
