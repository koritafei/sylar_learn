#ifndef __JSON_UTIL__H__
#define __JSON_UTIL__H__

#include <json/json.h>

#include <iostream>
#include <string>

namespace sylar {

class JsonUtil {
public:
  static bool        NeedEscape(const std::string &v);
  static std::string Escape(const std::string &v);
  static std::string GetString(const Json::Value &json,
                               const std::string &name,
                               const std::string &default_value = "");
  static double      GetDouble(const Json::Value &json,
                               const std::string &name,
                               double             defalut_value = 0.0);
  static int32_t     GetInt32(const Json::Value &json,
                              const std::string &name,
                              int32_t            default_value = 0);
  static uint32_t    GetUInt32(const Json::Value &json,
                               const std::string &name,
                               uint32_t           default_value = 0);
  static int64_t     GetInt64(const Json::Value &json,
                              const std::string &name,
                              int64_t            default_value = 0);
  static uint64_t    GetUint64(const Json::Value &json,
                               const std::string &name,
                               uint64_t           defalut_value = 0);
  static bool        FromString(Json::Value &json, const std::string &v);
  static std::string ToString(const Json::Value &json);
};

}  // namespace sylar

#endif /* __JSON_UTIL__H__ */
