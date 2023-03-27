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
  deps = [":singleton"]
 )

cc_binary (
  name="log_unittest",
  srcs= ["log_unittest.cc"],
  deps = [
    ":log",
    "@com_google_googletest//:gtest_main",
  ],
)

