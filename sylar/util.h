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

namespace sylar {

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
