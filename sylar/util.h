#ifndef __UTIL__H__
#define __UTIL__H__

#include <cxxabi.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/lexical_cast.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "sylar/fiber.h"

namespace sylar {

/**
 * @brief 返回当前线程的ID
 */
pid_t GetThreadId();

/**
 * @brief 返回当前协程的ID
 */
uint32_t GetFiberId();

/**
 * @brief 获取当前的调用栈
 * @param[out] bt 保存调用栈
 * @param[in] size 最多返回层数
 * @param[in] skip 跳过栈顶的层数
 */
void BackTrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串
 * @param[in] size 栈的最大层数
 * @param[in] skip 跳过栈顶的层数
 * @param[in] prefix 栈信息前输出的内容
 */
std::string BackTraceToString(int                size   = 64,
                              int                skip   = 2,
                              const std::string &prefix = "");

/**
 * @brief  获取当前时间的毫秒
 * @return uint64_t
 * */
uint64_t GetCurrentMs();

/**
 * @brief  获取当前时间的微秒
 * @return uint64_t
 * */
uint64_t GetCurrentUs();

template <class T>
const char *TypeToName() {
  static const char *s_name =
      abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
  return s_name;
}

class FSUtil {
public:
  static void ListAllFile(std::vector<std::string> &files,
                          const std::string        &path,
                          const std::string        &subfix);
  static bool OpenForWrite(std::ofstream          &ofs,
                           const std::string      &filename,
                           std::ios_base::openmode mode);

  static bool        Mkdir(const std::string &dirname);
  static std::string Dirname(const std::string &filename);
};

}  // namespace sylar

#endif /* __UTIL__H__ */
