/**
 * @file singleton.h
 * @author koritafei (koritafei@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-08-02
 *
 * @copyright Copyright (c) 2023 by koritafei
 *
 */

#ifndef __SINGLETON__H__
#define __SINGLETON__H__

#include <memory>

namespace sylar {

namespace {

template <typename T, typename X, int N>
T& GetInstanceX() {
  static T v;
  return v;
}

template <typename T, typename X, int N>
std::shared_ptr<T> GetInstancePtr() {
  static std::shared_ptr<T> v(new T);
  return v;
}

}  // namespace

/**
 * @brief 单例模式封装类
 *
 * @tparam T 类型
 * @tparam X 为了创造多个实例对应的Tag
 * @tparam N 同一个Tag创造多个实例索引
 */
template <typename T, typename X = void, int N = 0>
class Singleton {
public:
  static T* GetInstance() {
    static T v;
    return &v;
  }
};

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 *          X 为了创造多个实例对应的Tag
 *          N 同一个Tag创造多个实例索引
 */
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
