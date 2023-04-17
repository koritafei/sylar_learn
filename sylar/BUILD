package(default_visibility = ["//visibility:public"])

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
  name = "sylar",
  srcs = [
    "log.cc",
    "env.cc",
    "util.cc",
    "config.cc",
    "mutex.cc",
    "thread.cc",
    "fiber.cc",
    "scheduler.cc",
    "timer.cc",
    ],
  hdrs = [ 
          "noncopyable.h",
          "singleton.h",
          "log.h",
          "env.h",
          "util.h",
          "config.h",
          "mutex.h",
          "thread.h",
          "macro.h",
          "scheduler.h",
          "fiber.h",
          "timer.h",
        ],
  deps = [
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
  ],
)