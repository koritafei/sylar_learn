/**
 * @file noncopyable.h
 * @author koritafei (koritafei@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-07-28
 *
 * @copyright Copyright (c) 2023 by koritafei
 *
 */

#ifndef __NONCOPYABLE__H__
#define __NONCOPYABLE__H__

namespace sylar {

class Noncopyable {
public:
  Noncopyable()          = default;
  virtual ~Noncopyable() = default;

  Noncopyable(const Noncopyable &)    = delete;
  Noncopyable(Noncopyable &&)         = delete;
  void operator=(const Noncopyable &) = delete;
};

}  // namespace sylar

#endif /* __NONCOPYABLE__H__ */
