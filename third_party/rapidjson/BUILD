licenses(["notice"])

# RapidJSON (rapidjson.org) library.
# from https://github.com/Tencent/rapidjson

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "rapidjson",
    hdrs = glob(["include/rapidjson/**/*.h"]),
    defines = ["RAPIDJSON_HAS_STDSTRING=1"],
    includes = ["include"],
)
