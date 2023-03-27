package(
    default_visibility = ["//visibility:public"],
)

proto_library(
    name = "cls_structedlog",
    srcs = ["cls-cpp-sdk/src/cls_structedlog.proto"],
)

cc_proto_library(
    name = "cls_structedlog_cc",
    deps = [":cls_structedlog"],
)

cc_library(
    name = "cls_api",
    srcs = glob(
        include = [
            "cls-cpp-sdk/src/*.cpp",
            "cls-cpp-sdk/src/*.c",
        ],
    ),
    hdrs = glob(["cls-cpp-sdk/src/*.h"]),
    defines = [
        'CLS_SDK_VERSION=\\"1.0.1\\"',
    ],
    includes = ["cls-cpp-sdk/src/"],
    linkstatic = True,
    deps = [
        ":cls_structedlog_cc",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_protobuf//:protobuf",
        "@openssl//:libcrypto",
        "@openssl//:libssl",
    ],
)
