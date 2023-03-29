#ifndef __ENV__H__
#define __ENV__H__

#include <map>
#include <string>
#include <vector>

#include "sylar/singleton.h"

namespace sylar {

class Env {
public:
  bool init(int argc, char **argv);

  void add(const std::string &key, const std::string &value);
  bool has(const std::string &key);
  void del(const std::string &key);

  std::string get(const std::string &key,
                  const std::string &default_value = "");

  void addHelp(const std::string &key, const std::string &desc);
  void removeHelp(const std::string &key);
  void printHelp();

  const std::string getExe() const {
    return m_exe;
  }

  const std::string getCwd() const {
    return m_cwd;
  }

  bool        setEnv(const std::string &key, const std::string &value);
  std::string getEnv(const std::string &key,
                     const std::string &default_value = "");

  std::string getAbsolutePath(const std::string &path) const;
  // std::string getAbsoluteWorkPath(const std::string &path) const;

  std::string getConfigPath();

private:
  std::map<std::string, std::string>               m_args;
  std::vector<std::pair<std::string, std::string>> m_helpers;

  std::string m_program;
  std::string m_exe;
  std::string m_cwd;
};

typedef sylar::Singleton<Env> EnvMgr;

}  // namespace sylar

#endif /* __ENV__H__ */
