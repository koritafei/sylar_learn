#include "sylar/config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>

#include "sylar/env.h"

namespace sylar {

static sylar::Logger::ptr g_logger = LOG_NAME("system");

ConfigVarBase::ptr Config::LookupBase(const std::string &name) {
  auto it = GetDatas().find(name);
  return it == GetDatas().end() ? nullptr : it->second;
}

//"A.B", 10
// A:
//  B: 10
//  C: str
static void listAllMember(
    const std::string                                   &prefix,
    const YAML::Node                                    &node,
    std::list<std::pair<std::string, const YAML::Node>> &output) {
  if (prefix.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678") !=
      std::string::npos) {
    LOG_ERROR(g_logger) << "Config invalid name: " << prefix << " : " << node;
    return;
  }

  output.push_back(std::make_pair(prefix, node));
  if (node.IsMap()) {
    for (auto it = node.begin(); it != node.end(); ++it) {
      listAllMember(prefix.empty() ? it->first.Scalar()
                                   : prefix + "." + it->first.Scalar(),
                    it->second,
                    output);
    }
  }
}

void Config::LoadFromYaml(const YAML::Node &root) {
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  listAllMember("", root, all_nodes);

  for (auto &i : all_nodes) {
    std::string key = i.first;
    if (key.empty()) {
      continue;
    }

    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);
    if (var) {
      if (i.second.IsScalar()) {
        var->fromString(i.second.Scalar());
      } else {
        std::stringstream ss;
        ss << i.second;
        var->fromString(ss.str());
      }
    }
  }
}

static std::map<std::string, uint64_t> s_file2modifytime;

void Config::LoadFromConfDir(const std::string &path, bool force) {
  std::string abspath = sylar::EnvMgr::GetInstance()->getAbsolutePath(path);
  std::vector<std::string> files;
  FSUtil::ListAllFile(files, abspath, ".yml");

  for (auto &i : files) {
    {
      struct stat st;
      lstat(i.c_str(), &st);
      if (!force && s_file2modifytime[i] == (uint64_t)st.st_mtime) {
        continue;
      }

      s_file2modifytime[i] = st.st_mtime;
    }

    try {
      YAML::Node root = YAML::Load(i);
      LoadFromYaml(root);
      LOG_INFO(g_logger) << "LoadConfFile file=" << i << " ok";
    } catch (...) {
      LOG_ERROR(g_logger) << "LoadConfFile file=" << i << " failed";
    }
  }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  ConfigVarMap maps = GetDatas();
  for (auto it = maps.begin(); it != maps.end(); it++) {
    cb(it->second);
  }
}

}  // namespace sylar
