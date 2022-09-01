#ifndef __UTIL__H__
#define __UTIL__H__

#include <cxxabi.h>
#include <google/protobuf/message.h>
#include <json/json.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <yaml-cpp/yaml.h>

#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace sylar {

class TypeUtil {
public:
  static int8_t  ToChar(const std::string &str);
  static int64_t Atoi(const std::string &str);
  static double  Atof(const std::string &str);
  static int8_t  ToChar(const char *str);
  static int64_t Atoi(const char *str);
  static double  Atof(const char *str);
};

}  // namespace sylar

#endif /* __UTIL__H__ */
