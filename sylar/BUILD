package(default_visibility = ["//visibility:public"])


# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
  name = "noncopyable",
  hdrs = ["noncopyable.h"],
)

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
  name = "singleton",
  hdrs = ["singleton.h"],
)

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
  name = "log",
  srcs = ["log.cc"],
  hdrs = ["log.h"],
  deps = [
    ":singleton",
    ":util",
    "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ]
 )



# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
  name = "env",
  srcs = ["env.cc"],
  hdrs = ["env.h"],
  deps = [
    ":log"
  ],
)


# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
  name = "util",
  srcs = ["util.cc"],
  hdrs = ["util.h"],
  deps = [],
)


# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
  name = "config",
  srcs = ["config.cc"],
  hdrs = ["config.h"],
  deps = [
    ":log",
    ":env",
    ":util",
    "@com_github_jbeder_yaml_cpp//:yaml-cpp",
    ],
)

