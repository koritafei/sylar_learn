#ifndef __SINGLETON__H__
#define __SINGLETON__H__

#include <memory>

namespace sylar {

namespace {

template <typename T, typename X, int N>
T &GetInstanceX() {
  static T x;
  return x;
}

template <typename T, typename X, int N>
std::shared_ptr<T> GetInstance() {
  static std::shared_ptr<T> v(new T);
  return v;
}

}  // namespace

template <typename T, typename X = void, int N = 0>
class Singleton {
public:
  static T *GetInstance() {
    static T v;
    return &v;
  }
};

template <typename T, typename X = void, int N = 0>
class SingletonPtr {
public:
  static std::shared_ptr<T> GetInstance() {
    static std::shared_ptr<T> v(new T);
    return v;
  }
};

}  // namespace sylar

#endif /* __SINGLETON__H__ */
