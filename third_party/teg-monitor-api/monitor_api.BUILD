package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "digestible",
    hdrs = glob(["third-party/digestible/*.h"]),
)

cc_library(
    name = "monitor_api",
    srcs = glob(
        include = [
            "src/**/*.h",
            "src/**/*.cc",
            "third-party/zhiyan_metric_yaml/src/**/*.cc",
            "third-party/zhiyan_metric_yaml/include/**/*.h",
            "third-party/zhiyan_metric_yaml/src/**/*.h",
        ],
    ),
    hdrs = glob(["include/*.h"]),
    copts = [
        "-Iexternal/TegMonitorApi/third-party/zhiyan_metric_yaml/include",
        "-Iexternal/TegMonitorApi/third-party",
    ],
    defines = [
        'TEG_MONITOR_SDK_VERSION=\\"2.0.26\\"',
        'REVERSION=\\"V001D002\\"',
        "CMD_BATCH_MODE",
        "__STDC_FORMAT_MACROS",
    ],
    includes = ["include"],
    deps = [
        ":digestible",
    ],
)
