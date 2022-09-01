package(
    default_visibility = ["//sylar:internal"],
)

package_group(
    name = "internal",
    packages = [
        "//sylar/...",
    ],
)

filegroup(
    name = "all_files",
    srcs = glob(
        ["**/*"],
    ),
)

cc_library(
    name = "function_traits",
    hdrs = ["function_traits.h"],
)
