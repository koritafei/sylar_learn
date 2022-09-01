package(
    default_visibility = ["//visibility:public"],
)

config_setting(
    name = "darwin",
    values = {
        "cpu": "darwin_x86_64",
    },
)

cc_library(
    name = "libcrypto",
    hdrs = glob(["include/openssl/*.h"]),
    srcs = glob(["lib64/libcrypto.so"]),
    includes = ["include"],
    linkopts = select({
        ":darwin": [],
        "//conditions:default": ["-lpthread", "-ldl"],
    }),
    visibility = ["//visibility:public"],
)

cc_library(
    name = "libssl",
    deps = [":libcrypto"],
    hdrs = glob(["include/openssl/*.h"]),
    srcs = glob(["lib64/libssl.so"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)