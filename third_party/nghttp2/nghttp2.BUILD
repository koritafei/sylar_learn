licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "nghttp2",
    srcs = ["libnghttp2.a"],
    hdrs = glob([
        "lib/includes/**/*.h",
    ]),
    includes = ["lib/includes/"],
)

genrule(
    name = "nghttp2-build",
    srcs = glob(
        ["**/*"],
        exclude = ["bazel-*"],
    ),
    outs = [
        "libnghttp2.a",
        "include",
    ],
    cmd = """
        CONFIG_LOG=$$(mktemp)
        MAKE_LOG=$$(mktemp)
        NGHTTP2_ROOT=$$(dirname $(location configure))
        echo $$NGHTTP2_ROOT
        pushd $$NGHTTP2_ROOT > /dev/null
            if ! ./configure --enable-lib-only > $$CONFIG_LOG; then
                cat $$CONFIG_LOG
            fi
            if ! make -j 4 > $$MAKE_LOG; then
                cat $$MAKE_LOG
            fi
        popd > /dev/null
        pwd && ls
        echo $$NGHTTP2_ROOT
        cp $$NGHTTP2_ROOT/lib/.libs/libnghttp2.a $(location libnghttp2.a)
        cp -r $$NGHTTP2_ROOT/lib/includes $(location include)
    """,
)
