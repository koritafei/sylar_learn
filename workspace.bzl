load("//third_party:repo.bzl", "sylar_http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def clean_dep(dep):
    return str(Label(dep))

def sylar_workspace(path_prefix = "", tf_repo_name = "", **kwargs):
    """构建tbrpc工程规则.
    NOTE:主要是通过传入参数来决定工程构建过程中所需依赖软件的版本
    Args:
        path_prefix:路径前缀
        tf_repo_name:
        kwargs:关键字参数，字典类型，主要用于指定依赖软件的版本或者sha256的值,其中关键字的key构造由名称+ver,例如:protobuf_ver,zlib_ver等.
    Example:
        sylar_workspace(path_prefix="", tf_repo_name="", protobuf_ver="xxx", protobuf_sha256="xxx", ...)
        其中xxx就是具体指定的版本，如果没有通过key指定版本，就默认采用缺省值，例如:protobuf_ver = kwargs.get("protobuf_ver", "3.8.0")
    """

    # bazel_skylib 版本 摘要
    bazel_skylib_ver = kwargs.get("bazel_skylib_ver", "1.0.2")
    bazel_skylib_sha256 = kwargs.get("bazel_skylib_sha256", "e5d90f0ec952883d56747b7604e2a15ee36e288bb556c3d0ed33e818a4d971f2")
    bazel_skylib_urls = [
        "https://github.com/bazelbuild/bazel-skylib/archive/{ver}.tar.gz".format(ver = bazel_skylib_ver),
    ]
    if not native.existing_rule("bazel_skylib"):
        http_archive(
            name = "bazel_skylib",
            sha256 = bazel_skylib_sha256,
            strip_prefix = "bazel-skylib-{ver}".format(ver = bazel_skylib_ver),
            urls = bazel_skylib_urls,
        )

    #zlib 版本 摘要
    zlib_ver = kwargs.get("zlib_ver", "1.2.11")
    zlib_sha256 = kwargs.get("zlib_sha256", "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff")
    zlib_urls = [
        "https://github.com/madler/zlib/archive/v{ver}.tar.gz".format(ver = zlib_ver),
    ]
    if not native.existing_rule("zlib"):
        http_archive(
            name = "zlib",
            build_file = clean_dep("//third_party/zlib:zlib.BUILD"),
            sha256 = zlib_sha256,
            strip_prefix = "zlib-{ver}".format(ver = zlib_ver),
            urls = zlib_urls,
        )

    # rules_cc 版本 摘要（高版本protobuf需要这个依赖）
    rules_cc_ver = kwargs.get("rules_cc_ver", "818289e5613731ae410efb54218a4077fb9dbb03")
    rules_cc_sha256 = kwargs.get("rules_cc_sha256", "9d48151ea71b3e225adfb6867e6d2c7d0dce46cbdc8710d9a9a628574dfd40a0")
    rules_cc_urls = [
        "https://github.com/bazelbuild/rules_cc/archive/{ver}.tar.gz".format(ver = rules_cc_ver),
    ]
    if not native.existing_rule("rules_cc"):
        http_archive(
            name = "rules_cc",
            sha256 = rules_cc_sha256,
            strip_prefix = "rules_cc-{ver}".format(ver = rules_cc_ver),
            urls = rules_cc_urls,
        )

    # rules_python 版本 摘要（高版本protobuf需要这个依赖）
    rules_python_ver = kwargs.get("rules_python_ver", "4b84ad270387a7c439ebdccfd530e2339601ef27")
    rules_python_sha256 = kwargs.get("rules_python_sha256", "e5470e92a18aa51830db99a4d9c492cc613761d5bdb7131c04bd92b9834380f6")
    rules_python_urls = [
        "https://github.com/bazelbuild/rules_python/archive/{ver}.tar.gz".format(ver = rules_python_ver),
    ]
    if not native.existing_rule("rules_python"):
        http_archive(
            name = "rules_python",
            sha256 = rules_python_sha256,
            strip_prefix = "rules_python-{ver}".format(ver = rules_python_ver),
            urls = rules_python_urls,
        )

    # protobuf 版本 摘要
    protobuf_ver = kwargs.get("protobuf_ver", "3.8.0")
    protobuf_sha256 = kwargs.get("protobuf_sha256", "03d2e5ef101aee4c2f6ddcf145d2a04926b9c19e7086944df3842b1b8502b783")
    protobuf_strip_prefix = "protobuf-{ver}".format(ver = protobuf_ver)
    protobuf_urls = [
        "https://github.com/protocolbuffers/protobuf/archive/v{ver}.tar.gz".format(ver = protobuf_ver),
    ]
    sylar_http_archive(
        name = "protobuf_archive",
        sha256 = protobuf_sha256,
        strip_prefix = protobuf_strip_prefix,
        system_build_file = clean_dep("//third_party/systemlibs:protobuf.BUILD"),
        system_link_files = {
            "//third_party/systemlibs:protobuf.bzl": "protobuf.bzl",
        },
        urls = protobuf_urls,
    )

    # We need to import the protobuf library under the names com_google_protobuf
    # and com_google_protobuf_cc to enable proto_library support in bazel.
    # Unfortunately there is no way to alias http_archives at the moment.
    sylar_http_archive(
        name = "com_google_protobuf",
        sha256 = protobuf_sha256,
        strip_prefix = protobuf_strip_prefix,
        system_build_file = clean_dep("//third_party/systemlibs:protobuf.BUILD"),
        system_link_files = {
            "//third_party/systemlibs:protobuf.bzl": "protobuf.bzl",
        },
        urls = protobuf_urls,
    )

    sylar_http_archive(
        name = "com_google_protobuf_cc",
        sha256 = protobuf_sha256,
        strip_prefix = protobuf_strip_prefix,
        system_build_file = clean_dep("//third_party/systemlibs:protobuf.BUILD"),
        system_link_files = {
            "//third_party/systemlibs:protobuf.bzl": "protobuf.bzl",
        },
        urls = protobuf_urls,
    )

    # ===== gtest =====
    # com_google_googletest 版本 摘要
    com_google_googletest_ver = kwargs.get("com_google_googletest_ver", "1.10.0")
    com_google_googletest_sha256 = kwargs.get("com_google_googletest_sha256", "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb")
    com_google_googletest_urls = [
        "https://github.com/google/googletest/archive/release-{ver}.tar.gz".format(ver = com_google_googletest_ver),
    ]
    http_archive(
        name = "com_google_googletest",
        sha256 = com_google_googletest_sha256,
        strip_prefix = "googletest-release-{ver}".format(ver = com_google_googletest_ver),
        build_file = clean_dep("//third_party/gtest:BUILD"),
        urls = com_google_googletest_urls,
    )

    # com_github_gflags_gflags 版本 摘要
    com_github_gflags_gflags_ver = kwargs.get("com_github_gflags_gflags_ver", "2.2.2")
    com_github_gflags_gflags_sha256 = kwargs.get("com_github_gflags_gflags_sha256", "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf")
    com_github_gflags_gflags_urls = [
        "https://github.com/gflags/gflags/archive/v{ver}.tar.gz".format(ver = com_github_gflags_gflags_ver),
    ]
    http_archive(
        name = "com_github_gflags_gflags",
        sha256 = com_github_gflags_gflags_sha256,
        strip_prefix = "gflags-{ver}".format(ver = com_github_gflags_gflags_ver),
        urls = com_github_gflags_gflags_urls,
    )

    # com_github_jbeder_yaml_cpp 版本 摘要
    com_github_jbeder_yaml_cpp_ver = kwargs.get("com_github_jbeder_yaml_cpp_ver", "0.6.2")
    com_github_jbeder_yaml_cpp_sha256 = kwargs.get("com_github_jbeder_yaml_cpp_sha256", "e4d8560e163c3d875fd5d9e5542b5fd5bec810febdcba61481fe5fc4e6b1fd05")
    com_github_jbeder_yaml_cpp_urls = [
        "https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-{ver}.tar.gz".format(ver = com_github_jbeder_yaml_cpp_ver),
    ]
    sylar_http_archive(
        name = "com_github_jbeder_yaml_cpp",
        build_file = clean_dep("//third_party/yaml-cpp:yaml-cpp.BUILD"),
        sha256 = com_github_jbeder_yaml_cpp_sha256,
        strip_prefix = "yaml-cpp-yaml-cpp-{ver}".format(ver = com_github_jbeder_yaml_cpp_ver),
        urls = com_github_jbeder_yaml_cpp_urls,
    )

    # com_github_tencent_rapidjson 版本 摘要
    com_github_tencent_rapidjson_ver = kwargs.get("com_github_tencent_rapidjson_ver", "1.1.0")
    com_github_tencent_rapidjson_sha256 = kwargs.get("com_github_tencent_rapidjson_sha256", "8e00c38829d6785a2dfb951bb87c6974fa07dfe488aa5b25deec4b8bc0f6a3ab")
    com_github_tencent_rapidjson_urls = [
        "https://github.com/Tencent/rapidjson/archive/v{ver}.zip".format(ver = com_github_tencent_rapidjson_ver),
    ]
    sylar_http_archive(
        name = "com_github_tencent_rapidjson",
        build_file = clean_dep("//third_party/rapidjson:BUILD"),
        sha256 = com_github_tencent_rapidjson_sha256,
        strip_prefix = "rapidjson-{ver}".format(ver = com_github_tencent_rapidjson_ver),
        urls = com_github_tencent_rapidjson_urls,
    )

    # com_github_opentracing_cpp 版本 摘要
    com_github_opentracing_cpp_ver = kwargs.get("com_github_opentracing_cpp_ver", "1.6.0")
    com_github_opentracing_cpp_sha256 = kwargs.get("com_github_opentracing_cpp_sha256", "6d48ba5f5c19cfe82174848dd4bc8e85c3406bb15fc40f82e065cac35debddb3")
    com_github_opentracing_cpp_urls = [
        "https://github.com/opentracing/opentracing-cpp/archive/v{ver}.zip".format(ver = com_github_opentracing_cpp_ver),
    ]
    http_archive(
        name = "com_github_opentracing_cpp",
        sha256 = com_github_opentracing_cpp_sha256,
        strip_prefix = "opentracing-cpp-{ver}".format(ver = com_github_opentracing_cpp_ver),
        urls = com_github_opentracing_cpp_urls,
    )

    # prometheus_cpp
    com_github_jupp0r_prometheus_cpp_ver = kwargs.get("com_github_jupp0r_prometheus_cpp_ver", "1.0.0")
    com_github_jupp0r_prometheus_cpp_sha256 = kwargs.get("com_github_jupp0r_prometheus_cpp_sha256", "07018db604ea3e61f5078583e87c80932ea10c300d979061490ee1b7dc8e3a41")
    com_github_jupp0r_prometheus_cpp_urls = [
        "https://github.com/jupp0r/prometheus-cpp/archive/v{ver}.tar.gz".format(ver = com_github_jupp0r_prometheus_cpp_ver),
    ]
    http_archive(
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = com_github_jupp0r_prometheus_cpp_sha256,
        strip_prefix = "prometheus-cpp-{ver}".format(ver = com_github_jupp0r_prometheus_cpp_ver),
        urls = com_github_jupp0r_prometheus_cpp_urls,
    )

    civetweb_ver = kwargs.get("civetweb_ver", "1.15")
    civetweb_sha256 = kwargs.get("civetweb_sha256", "90a533422944ab327a4fbb9969f0845d0dba05354f9cacce3a5005fa59f593b9")
    civetweb_urls = [
        "https://github.com/civetweb/civetweb/archive/v{ver}.tar.gz".format(ver = civetweb_ver),
    ]
    http_archive(
        name = "civetweb",
        build_file = clean_dep("@com_github_jupp0r_prometheus_cpp//bazel:civetweb.BUILD"),
        sha256 = civetweb_sha256,
        strip_prefix = "civetweb-{ver}".format(ver = civetweb_ver),
        urls = civetweb_urls,
    )

    cppcodec_ver = kwargs.get("cppcodec_ver", "0.2")
    cppcodec_sha256 = kwargs.get("cppcodec_sha256", "0edaea2a9d9709d456aa99a1c3e17812ed130f9ef2b5c2d152c230a5cbc5c482")
    cppcodec_urls = [
        "https://github.com/tplgy/cppcodec/archive/v{ver}.tar.gz".format(ver = cppcodec_ver),
    ]
    http_archive(
        name = "com_github_tplgy_cppcodec",
        build_file = clean_dep("@com_github_jupp0r_prometheus_cpp//bazel:cppcodec.BUILD"),
        sha256 = cppcodec_sha256,
        strip_prefix = "cppcodec-{ver}".format(ver = cppcodec_ver),
        urls = cppcodec_urls,
    )

    http_archive(
        name = "net_zlib_zlib",
        build_file = clean_dep("@com_github_jupp0r_prometheus_cpp//bazel:zlib.BUILD"),
        sha256 = zlib_sha256,
        strip_prefix = "zlib-{ver}".format(ver = zlib_ver),
        urls = zlib_urls,
    )

    native.new_local_repository(
        name = "local_curl",
        path = "/usr",
        build_file = "@PcgMonitorApi//third_party/bazel:curl.BUILD",
    )

    native.new_local_repository(
        name = "local_crypto",
        build_file = "@rainbow_sdk//third_party/crypto:BUILD.crypto",
        path = "/usr",
    )

    # fmtlib
    fmt_ver = kwargs.get("fmt_ver", "6.2.0")
    fmt_sha256 = kwargs.get("fmt_sha256", "fe6e4ff397e01c379fc4532519339c93da47404b9f6674184a458a9967a76575")
    fmt_urls = [
        "https://github.com/fmtlib/fmt/archive/{ver}.tar.gz".format(ver = fmt_ver),
    ]
    http_archive(
        name = "fmtlib",
        sha256 = fmt_sha256,
        strip_prefix = "fmt-{ver}".format(ver = fmt_ver),
        build_file = clean_dep("//third_party/fmtlib:fmtlib.BUILD"),
        urls = fmt_urls,
    )

    # OpenSSL installed on local file system, prefix: `/usr/`.
    native.new_local_repository(
        name = "openssl",
        path = "/usr",
        build_file = clean_dep("//third_party/openssl:BUILD"),
    )

    # jsoncpp
    jsoncpp_ver = kwargs.get("jsoncpp_ver", "1.9.3")
    jsoncpp_sha256 = kwargs.get("jsoncpp_sha256", "8593c1d69e703563d94d8c12244e2e18893eeb9a8a9f8aa3d09a327aa45c8f7d")
    jsoncpp_name = "jsoncpp-{ver}".format(ver = jsoncpp_ver)
    jsoncpp_urls = [
        "https://github.com/open-source-parsers/jsoncpp/archive/{ver}.tar.gz".format(ver = jsoncpp_ver),
    ]
    http_archive(
        name = "jsoncpp",
        strip_prefix = jsoncpp_name,
        sha256 = jsoncpp_sha256,
        urls = jsoncpp_urls,
        build_file = clean_dep("//third_party/jsoncpp:jsoncpp.BUILD"),
    )

    #flatbuffers 版本 摘要
    fbs_ver = kwargs.get("fbs_ver", "1.12.0")
    fbs_sha256 = kwargs.get("fbs_sha256", "62f2223fb9181d1d6338451375628975775f7522185266cd5296571ac152bc45")
    fbs_name = "flatbuffers-{ver}".format(ver = fbs_ver)
    fbs_urls = [
        "https://github.com/google/flatbuffers/archive/v{ver}.tar.gz".format(ver = fbs_ver),
    ]
    http_archive(
        name = "com_github_google_flatbuffers",
        strip_prefix = fbs_name,
        sha256 = fbs_sha256,
        urls = fbs_urls,
    )

    opentelemetry_cpp_ver = kwargs.get("opentelemetry_cpp_ver", "0.7.0")
    opentelemetry_cpp_sha256 = kwargs.get("opentelemetry_cpp_sha256", "147be991aeb7b3f8c2a2d65fe53a97577d10c6b286991fa8715b321d1d7f2b69")
    opentelemetry_cpp_urls = [
        "https://github.com/open-telemetry/opentelemetry-cpp/archive/v{ver}.tar.gz".format(ver = opentelemetry_cpp_ver),
    ]
    http_archive(
        name = "opentelemetry_cpp",
        sha256 = opentelemetry_cpp_sha256,
        strip_prefix = "opentelemetry-cpp-{ver}".format(ver = opentelemetry_cpp_ver),
        urls = opentelemetry_cpp_urls,
    )

    new_git_repository(
        name = "com_github_opentelemetry_proto",
        build_file = clean_dep("@tps_sdk_cpp//bazel:opentelemetry_proto.BUILD"),
        remote = "https://github.com/open-telemetry/opentelemetry-proto",
        tag = "v0.8.0",
    )

    # libnghttp2 版本 摘要
    # url: https://github.com/nghttp2/nghttp2/releases/download/v1.40.0/nghttp2-1.40.0.tar.gz
    # sha256: eb9d9046495a49dd40c7ef5d6c9907b51e5a6b320ea6e2add11eb8b52c982c47
    # 注意：此处的nghttp2 版本和框架使用的北极星SDK中使用的nghttp2保持一致
    # 如果要升级此处版本或者升级北极星SDK时，请二次确认下两处版本保持一致。
    nghttp2_ver = kwargs.get("nghttp2_ver", "1.40.0")
    nghttp2_sha256 = kwargs.get("nghttp2_sha256", "eb9d9046495a49dd40c7ef5d6c9907b51e5a6b320ea6e2add11eb8b52c982c47")
    nghttp2_name = "nghttp2-{ver}".format(ver = nghttp2_ver)
    nghttp2_urls = [
        "https://github.com/nghttp2/nghttp2/releases/download/v{ver}/nghttp2-{ver}.tar.gz".format(ver = nghttp2_ver),
    ]
    sylar_http_archive(
        name = "nghttp2",
        build_file = clean_dep("//third_party/nghttp2:nghttp2.BUILD"),
        sha256 = nghttp2_sha256,
        strip_prefix = nghttp2_name,
        urls = nghttp2_urls,
    )

    # url parser 版本 摘要
    # url: https://github.com/play-co/url-parser/archive/refs/tags/v0.1.0.tar.gz
    # sha256: d8e7a5e31d03656253400bcb8b5683aee6ebd51213b4e36ba95a3b09d1a08d83
    urlparser_ver = kwargs.get("urlparser_ver", "0.1.0")
    urlparser_sha256 = kwargs.get("urlparser_sha256", "d8e7a5e31d03656253400bcb8b5683aee6ebd51213b4e36ba95a3b09d1a08d83")
    urlparser_name = "url-parser-{ver}".format(ver = urlparser_ver)
    urlparser_urls = [
        "https://github.com/play-co/url-parser/archive/refs/tags/v{ver}.tar.gz".format(ver = urlparser_ver),
    ]
    sylar_http_archive(
        name = "urlparser",
        build_file = clean_dep("//third_party/urlparser:urlparser.BUILD"),
        sha256 = urlparser_sha256,
        strip_prefix = urlparser_name,
        urls = urlparser_urls,
    )

    # snappy 版本 摘要
    com_github_google_snappy_ver = kwargs.get("com_github_google_snappy_ver", "1.1.8")
    com_github_google_snappy_sha256 = kwargs.get("com_github_google_snappy_sha256", "16b677f07832a612b0836178db7f374e414f94657c138e6993cbfc5dcc58651f")
    com_github_google_snappy_name = "snappy-{ver}".format(ver = com_github_google_snappy_ver)
    com_github_google_snappy_urls = [
        "https://github.com/google/snappy/archive/refs/tags/{ver}.tar.gz".format(ver = com_github_google_snappy_ver),
    ]
    sylar_http_archive(
        name = "com_github_google_snappy",
        build_file = clean_dep("//third_party/snappy:snappy.BUILD"),
        sha256 = com_github_google_snappy_sha256,
        strip_prefix = com_github_google_snappy_name,
        urls = com_github_google_snappy_urls,
    )

    # liburing 版本 摘要
    # url: https://github.com/axboe/liburing/archive/refs/tags/liburing-2.1.zip
    # sha256: 9669db5d9d48dafba0f3d033e0dce15aeee561bbd6b06eef4f392b122f75c33d
    # 注意：当前使用的是最新版liburing，主要是为了避免之前版本存在未修复的bug，
    #      并不代表当前版本的所有liburing功能，tkernel4都支持。
    #      需要根据当前内核确认是否支持该功能。
    liburing_ver = kwargs.get("liburing_ver", "2.1")
    liburing_sha256 = kwargs.get("liburing_sha256", "9669db5d9d48dafba0f3d033e0dce15aeee561bbd6b06eef4f392b122f75c33d")
    liburing_name = "liburing-liburing-{ver}".format(ver = liburing_ver)
    liburing_urls = [
        "https://github.com/axboe/liburing/archive/refs/tags/liburing-{ver}.zip".format(ver = liburing_ver),
    ]
    sylar_http_archive(
        name = "liburing",
        build_file = clean_dep("//third_party/liburing:liburing.BUILD"),
        sha256 = liburing_sha256,
        strip_prefix = liburing_name,
        urls = liburing_urls,
    )
