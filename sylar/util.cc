#include "sylar/util.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <execinfo.h>
#include <ifaddrs.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "sylar/fiber.h"
#include "sylar/log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = LOG_NAME("system");

pid_t GetThreadId() {
  return syscall(SYS_gettid);
}

uint32_t GetFiberId() {
  return sylar::Fiber::GetFiberId();
}

static std::string demangle(const char *str) {
  size_t      size   = 0;
  int         status = 0;
  std::string rt;
  rt.resize(256);
  if (1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {
    char *v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
    if (v) {
      std::string result(v);
      free(v);
      return result;
    }
  }
  if (1 == sscanf(str, "%255s", &rt[0])) {
    return rt;
  }

  return str;
}

void BackTrace(std::vector<std::string> &bt, int size, int skip) {
  void **array = (void **)malloc(sizeof(void *) * size);
  size_t s     = ::backtrace(array, size);

  char **strings = backtrace_symbols(array, size);
  if (nullptr == strings) {
    LOG_ERROR(g_logger) << "backtrace_synbols error";
    return;
  }

  for (size_t i = skip; i < s; ++i) {
    bt.push_back(demangle(strings[i]));
  }

  free(strings);
  free(array);
}

std::string BackTraceToString(int size, int skip, const std::string &prefix) {
  std::vector<std::string> vec;
  BackTrace(vec, size, skip);
  std::stringstream ss;
  for (std::size_t i = 0; i < vec.size(); ++i) {
    ss << prefix << vec[i] << std::endl;
  }

  return ss.str();
}

std::string FSUtil::Dirname(const std::string &filename) {
  if (filename.empty()) {
    return ".";
  }

  auto pos = filename.rfind('/');
  if (0 == pos) {
    return "/";
  } else if (pos == std::string::npos) {
    return ".";
  } else {
    return filename.substr(0, pos);
  }
}

static int __lstat(const char *file, struct stat *st = nullptr) {
  struct stat lst;
  int         ret = lstat(file, &lst);
  if (st) {
    *st = lst;
  }

  return ret;
}

static int __mkdir(const char *path) {
  if (access(path, F_OK) == 0) {
    return 0;
  }

  return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

bool FSUtil::Mkdir(const std::string &dirname) {
  if (0 == __lstat(dirname.c_str())) {
    return true;
  }

  char *path = strdup(dirname.c_str());
  char *ptr  = strchr(path + 1, '/');

  do {
    for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
      *ptr = '\0';
      if (0 != __mkdir(path)) {
        break;
      }
    }
    if (ptr != nullptr) {
      break;
    } else if (0 != __mkdir(path)) {
      break;
    }

    free(path);
    return true;

  } while (0);
  free(path);

  return false;
}

void FSUtil::ListAllFile(std::vector<std::string> &files,
                         const std::string        &path,
                         const std::string        &subfix) {
  if (access(path.c_str(), 0) != 0) {
    return;
  }

  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }

  struct dirent *dp = nullptr;
  while (nullptr != (dp = readdir(dir))) {
    if (DT_DIR == dp->d_type) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
        continue;
      }
      ListAllFile(files, path + "/" + dp->d_name, subfix);
    } else if (DT_REG == dp->d_type) {
      std::string filename(dp->d_name);
      if (subfix.empty()) {
        files.push_back(path + "/" + filename);
      } else {
        if (filename.size() < subfix.size()) {
          continue;
        }

        if (subfix == filename.substr(filename.length() - subfix.size())) {
          files.push_back(path + "/" + filename);
        }
      }
    }
  }

  closedir(dir);
}

bool FSUtil::OpenForWrite(std::ofstream          &ofs,
                          const std::string      &filename,
                          std::ios_base::openmode mode) {
  ofs.open(filename.c_str(), mode);
  if (!ofs.is_open()) {
    std::string dir = Dirname(filename);
    Mkdir(dir);
    ofs.open(filename.c_str(), mode);
  }
  return ofs.is_open();
}

uint64_t GetCurrentMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUs() {
  struct timeval tm;
  gettimeofday(&tm, NULL);
  return tm.tv_sec * 1000 * 1000ul + tm.tv_usec;
}

}  // namespace sylar
