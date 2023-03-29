load("@bazel_skylib//lib:versions.bzl", "versions")

def _GetPath(ctx, path):
    if ctx.label.workspace_root:
        return ctx.label.workspace_root + "/" + path
    else:
        return path

def _IsNewExternal(ctx):
    # Bazel 0.4.4 and older have genfiles paths that look like:
    #   bazel-out/local-fastbuild/genfiles/external/repo/foo
    # After the exec root rearrangement, they look like:
    #   ../repo/bazel-out/local-fastbuild/genfiles/foo
    return ctx.label.workspace_root.startswith("../")

def _GenDir(ctx):
    if _IsNewExternal(ctx):
        # We are using the fact that Bazel 0.4.4+ provides repository-relative paths
        # for ctx.genfiles_dir.
        return ctx.genfiles_dir.path + (
            "/" + ctx.attr.includes[0] if ctx.attr.includes and ctx.attr.includes[0] else ""
        )

    # This means that we're either in the old version OR the new version in the local repo.
    # Either way, appending the source path to the genfiles dir works.
    return ctx.var["GENDIR"] + "/" + _SourceDir(ctx)

def _SourceDir(ctx):
    if not ctx.attr.includes:
        return ctx.label.workspace_root
    if not ctx.attr.includes[0]:
        return _GetPath(ctx, ctx.label.package)
    if not ctx.label.package:
        return _GetPath(ctx, ctx.attr.includes[0])
    return _GetPath(ctx, ctx.label.package + "/" + ctx.attr.includes[0])

def _ReplaceProtoExt(srcs, ext):
    # ret = [s.replace("proto", ext) for s in srcs]
    ret = [s[:-len(".proto")] + "." + ext for s in srcs]
    return ret

def _CcHdrs(srcs, use_plugin = False, gen_mock = False, ext = "", gen_pbcc = True):  #replace whit protocal name:sylar,nrpc...
    ret1 = []
    ret2 = []
    if gen_pbcc:
        ret1 = [s[:-len(".proto")] + ".pb.h" for s in srcs]
    if use_plugin:
        if gen_mock:
            tmp = ext
            tmp = tmp + ".pb.mock.h"
            ret2 += _ReplaceProtoExt(srcs, tmp)
        ext = ext + ".pb.h"
        ret2 += _ReplaceProtoExt(srcs, ext)
    return struct(
        raw = ret1,
        plugin = ret2,
    )

def _CcSrcs(srcs, use_plugin = False, ext = "", gen_pbcc = True):  #replace whit protocal name:sylar,nrpc...
    ret1 = []
    ret2 = []
    if gen_pbcc:
        ret1 = [s[:-len(".proto")] + ".pb.cc" for s in srcs]
    if use_plugin:
        ext = ext + ".pb.cc"
        ret2 += _ReplaceProtoExt(srcs, ext)
    return struct(
        raw = ret1,
        plugin = ret2,
    )

def _CcOuts(srcs, use_plugin = False, gen_mock = False, ext = "", gen_pbcc = True):  #replace param...
    hdrs = _CcHdrs(srcs, use_plugin, gen_mock, ext, gen_pbcc)
    srcs = _CcSrcs(srcs, use_plugin, ext, gen_pbcc)
    return hdrs.raw + hdrs.plugin + srcs.raw + srcs.plugin

def _PyOuts(srcs, use_plugin = False, ext = ""):
    ret = [s[:-len(".proto")] + "_pb2.py" for s in srcs]
    if use_plugin:
        ext = "_pb2_" + ext + ".py"
        ret += _ReplaceProtoExt(srcs, ext)
    return ret

def _RelativeOutputPath(path, include, dest = ""):
    if include == None:
        return path

    if not path.startswith(include):
        fail("Include path %s isn't part of the path %s." % (include, path))

    if include and include[-1] != "/":
        include = include + "/"
    if dest and dest[-1] != "/":
        dest = dest + "/"

    path = path[len(include):]
    return dest + path

sylarProtoCollectorInfo = provider(
    "Proto file collector by aspect",
    fields = {
        "proto": "depset of pb files collected from proto_library",
        "proto_deps": "depset of pb files being directly depented by cc_proto_library",
        "import_flags": "depset of import flags(resolve the external deps not found issue)",
    },
)

def _proto_collector_aspect_impl(target, ctx):
    proto = []
    import_flags = []
    proto_deps = []
    if ProtoInfo in target:
        for src in ctx.rule.attr.srcs:
            for pbfile in src.files.to_list():
                proto.append(pbfile)
                proto_deps.append(pbfile)

                # 通过@这类外部依赖引入的，需要设置额外的include path
                if len(target.label.workspace_root) > 0:
                    import_flags.append("-I" + target.label.workspace_root)

                    # com_google_protobuf将pb文件生成在了bin目录下，业务使用protobuf的，得包含这样的路径
                    import_flags.append("-I" + ctx.bin_dir.path + "/" + target.label.workspace_root)

                    # 在protobuf3.14等较新版本中，import的路径发生改变，因此，需要同步加上下面的前缀
                    if target.label.workspace_root == "external/com_google_protobuf":
                        import_flags.append("-I" + target.label.workspace_root + "/src")

    for dep in ctx.rule.attr.deps:
        proto += [pbfile for pbfile in dep[sylarProtoCollectorInfo].proto.to_list()]
        import_flags += [flag for flag in dep[sylarProtoCollectorInfo].import_flags.to_list()]
        if CcInfo in target and ProtoInfo in dep:
            # cc_proto_library仅能依赖一个proto_library，因此下面赋值仅发生一次
            for proto_dep in dep[sylarProtoCollectorInfo].proto_deps.to_list():
                proto_deps.append(proto_dep)

    return [sylarProtoCollectorInfo(
        proto = depset(proto),
        proto_deps = depset(proto_deps),
        import_flags = depset(import_flags),
    )]

proto_collector_aspect = aspect(
    implementation = _proto_collector_aspect_impl,
    attr_aspects = ["deps"],
)

def _proto_gen_impl(ctx):
    """General implementation for generating protos"""
    srcs = ctx.files.srcs
    deps = []
    deps += ctx.files.srcs

    # 当srcs和deps为空时，标识为为native_proto_deps和native_cc_proto_deps生成sylar桩代码
    # 兼容当前srcs不存在，但是deps有多个sylar_proto_library的场景
    pblibdep_gen_sylar_stub = (len(ctx.attr.srcs) == 0 and len(ctx.attr.deps) == 0)

    # 当前仅支持sylar_proto_library
    if ctx.executable.plugin == None or "sylar_cpp_plugin" != ctx.executable.plugin.basename:
        pblibdep_gen_sylar_stub = False

    source_dir = _SourceDir(ctx)
    gen_dir = _GenDir(ctx).rstrip("/")
    if source_dir:
        import_flags = ["-I" + source_dir, "-I" + gen_dir]
    else:
        import_flags = ["-I."]

    for dep in ctx.attr.deps:
        # 兼容同时存在高版本bazel-4.2以及高protobuf-3.20时候，dep为depset类型的问题
        # 注意：protobuf-3.20只能在bazel-4以上才能编译通过
        if type(dep.proto.import_flags) == "depset":
            import_flags += dep.proto.import_flags.to_list()
        else:
            import_flags += dep.proto.import_flags

        if type(dep.proto.deps) == "depset":
            deps += dep.proto.deps.to_list()
        else:
            deps += dep.proto.deps

    for dep in ctx.attr.native_cc_proto_deps:
        deps += dep[sylarProtoCollectorInfo].proto.to_list()
        import_flags += dep[sylarProtoCollectorInfo].import_flags.to_list()

    # 让pb文件能通过当前文件夹引入
    import_flags_dirs = ["-I" + dep.dirname for dep in deps]

    # 去重
    deps = depset(deps).to_list()
    import_flags = depset(import_flags).to_list()
    import_flags_dirs = depset(import_flags_dirs).to_list()

    if not ctx.attr.gen_cc and not ctx.attr.gen_py and not ctx.executable.plugin:
        return struct(
            proto = struct(
                srcs = srcs,
                import_flags = import_flags,
                deps = deps,
            ),
        )
    actual_srcs = [src for src in srcs]
    if pblibdep_gen_sylar_stub:
        pb_dep_files = ctx.attr.native_cc_proto_deps[0][sylarProtoCollectorInfo].proto_deps.to_list()
        if len(pb_dep_files) != 1:
            fail("Target of native_proto_deps must have only one .proto file. Or native_cc_proto_deps can only dep on proto_library which has only one .proto file")
        actual_srcs += pb_dep_files

    for src in actual_srcs:
        args = []

        in_gen_dir = src.root.path == gen_dir
        if in_gen_dir:
            import_flags_real = []
            for f in depset(import_flags).to_list():
                path = f.replace("-I", "")
                import_flags_real.append("-I$(realpath -s %s)" % path)
            for f in import_flags_dirs:
                path = f.replace("-I", "")
                import_flags_real.append("-I$(realpath -s %s)" % path)

        outs = []
        use_plugin = (len(ctx.attr.plugin_language) > 0 and ctx.attr.plugin)  #expand to sylar

        path_tpl = "$(realpath %s)" if in_gen_dir else "%s"
        if ctx.attr.gen_cc:
            if ctx.attr.gen_pbcc:
                args += [("--cpp_out=" + path_tpl) % gen_dir]
            if ctx.attr.generate_new_mock_code:
                outs.extend(_CcOuts([src.basename], use_plugin, ctx.attr.generate_new_mock_code, ctx.attr.plugin_language, ctx.attr.gen_pbcc))
            else:
                outs.extend(_CcOuts([src.basename], use_plugin, ctx.attr.generate_mock_code, ctx.attr.plugin_language, ctx.attr.gen_pbcc))
        if ctx.attr.gen_py:
            args += [("--python_out=" + path_tpl) % gen_dir]
            outs.extend(_PyOuts([src.basename], use_plugin, ctx.attr.plugin_language))

        if pblibdep_gen_sylar_stub:
            # 不填sibling字段，在本package下生成桩代码
            filename = ctx.label.name[:-len("_genproto")]
            include_suffix = [".sylar.pb.h", ".sylar.pb.cc"]
            if ctx.attr.generate_new_mock_code or ctx.attr.generate_mock_code:
                include_suffix.append(".sylar.pb.mock.h")
            outs = [ctx.actions.declare_file(filename + suffix) for suffix in include_suffix]
        else:
            outs = [ctx.actions.declare_file(out, sibling = src) for out in outs]

        inputs = [src] + deps
        tools = []
        if ctx.executable.plugin:
            plugin = ctx.executable.plugin
            lang = ctx.attr.plugin_language
            if not lang and plugin.basename.startswith("protoc-gen-"):
                lang = plugin.basename[len("protoc-gen-"):]
            if not lang:
                fail("cannot infer the target language of plugin", "plugin_language")

            outdir = "." if in_gen_dir else gen_dir

            if ctx.attr.plugin_options:
                outdir = ",".join(ctx.attr.plugin_options) + ":" + outdir
            args += [("--plugin=protoc-gen-%s=" + path_tpl) % (lang, plugin.path)]

            # 若generate_new_mock_code和generate_mock_code同为true，则优先使用gmock的新版本MockMethod
            if ctx.attr.generate_new_mock_code:
                outdir = "generate_new_mock_code=true:" + outdir
            elif ctx.attr.generate_mock_code:
                outdir = "generate_mock_code=true:" + outdir

            if ctx.attr.enable_explicit_link_proto:
                if ctx.attr.generate_new_mock_code or ctx.attr.generate_mock_code:
                    outdir = "enable_explicit_link_proto=true," + outdir
                else:
                    outdir = "enable_explicit_link_proto=true:" + outdir

            if pblibdep_gen_sylar_stub:
                # 为传递参数给插件以生成合适路径的桩代码
                filename = ctx.label.name[:-len("_genproto")]
                generate_sylar_stub_path = ctx.label.package + "/" + filename
                params = "generate_sylar_stub_path=" + generate_sylar_stub_path

                # 当生成桩代码使用native_proto_library中的proto_library, 并且proto_library使用strip_import_prefix时
                # 需要调整include头文件路径为项目根目录（就算strip_import_prefix语义为去除子目录，但依然pb文件还是被生成到根目录，不知道是bazel实现bug还是语义描述有误）
                # 只有特殊场景才需要使用到该选项
                if ctx.attr.proto_include_prefix != "ProtoIncludePrefixNotValid":
                    params = params + ",proto_include_prefix=" + ctx.attr.proto_include_prefix

                # 解决当不存在mock代码生成时，参数传递错误的问题
                if ctx.attr.generate_new_mock_code or ctx.attr.generate_mock_code:
                    outdir = params + "," + outdir
                else:
                    outdir = params + ":" + outdir

            args += ["--%s_out=%s" % (lang, outdir)]

            # 如果 inputs += [plugin] 这样把 plugin 加到 inputs ，会导致如下错误
            # [root@songliao-0 /data/work/sylar-cpp/sylar-cpp]# ./build.sh
            # INFO: Invocation ID: be2d17aa-dea8-4359-b1b6-5a6ffb10ee4b
            # ERROR: /data/work/sylar-cpp/sylar-cpp/sylar/codec/sso/BUILD:151:18: in proto_gen rule //sylar/codec/sso:hello_genproto:
            # Traceback (most recent call last):
            #         File "/data/work/sylar-cpp/sylar-cpp/sylar/codec/sso/BUILD", line 151, column 18, in proto_gen(name = 'hello_genproto')
            #                 sso_proto_library(
            #         File "/data/work/sylar-cpp/sylar-cpp/sylar/sylar.bzl", line 158, column 28, in _proto_gen_impl
            #                 ctx.actions.run(
            # Error in run: Found tool(s) 'bazel-out/host/bin/sylar/tools/sso_cpp_plugin/sso_cpp_plugin' in inputs. A tool is an input with executable=True set. All tools should be passed using the 'tools' argument instead of 'inputs' in order to make their runfiles available to the action. This safety check will not be performed once the action is modified to take a 'tools' argument. To temporarily disable this check, set --incompatible_no_support_tools_in_action_inputs=false.
            # INFO: Repository remotejdk11_linux instantiated at:
            #   no stack (--record_rule_instantiation_callstack not enabled)
            # Repository rule http_archive defined at:
            #   /root/.cache/bazel/_bazel_root/a33a4927e9f130c89f400594de6cf4ed/external/bazel_tools/tools/build_defs/repo/http.bzl:336:31: in <toplevel>
            # ERROR: Analysis of target '//sylar/codec/sso:sso_client_codec_test' failed; build aborted: Analysis failed
            # INFO: Elapsed time: 1.331s
            # INFO: 0 processes.
            # FAILED: Build did NOT complete successfully (210 packages loaded, 4588 targets configured)
            #     Fetching @TegMonitorApi; Cloning tags/v2.2.2 of http://git.woa.com/teg-devops/monitor.git

            # inputs += [plugin]
            tools = [plugin]

        if not in_gen_dir:
            ctx.actions.run(
                inputs = inputs,
                tools = tools,
                outputs = outs,
                progress_message = "Invoking Protoc " + ctx.label.name,
                arguments = args + import_flags + import_flags_dirs + [src.path],
                executable = ctx.executable.protoc,
                mnemonic = "ProtoCompile",
                use_default_shell_env = True,
            )
        else:
            for out in outs:
                orig_command = " ".join(
                    ["$(realpath %s)" % ctx.executable.protoc.path] + args +
                    import_flags_real + ["-I.", src.basename],
                )
                command = ";".join([
                    'CMD="%s"' % orig_command,
                    "cd %s" % src.dirname,
                    "${CMD}",
                    "cd -",
                ])
                generated_out = "/".join([gen_dir, out.basename])
                if generated_out != out.path:
                    command += ";mv %s %s" % (generated_out, out.path)
                ctx.actions.run_shell(
                    inputs = inputs + [ctx.executable.protoc],
                    outputs = [out],
                    command = command,
                    mnemonic = "ProtoCompile",
                    use_default_shell_env = True,
                )

    return struct(
        proto = struct(
            srcs = srcs,
            import_flags = import_flags,
            deps = deps,
        ),
    )

proto_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(providers = ["proto"]),
        "native_cc_proto_deps": attr.label_list(providers = [CcInfo, OutputGroupInfo], aspects = [proto_collector_aspect]),
        "proto_include_prefix": attr.string(default = "ProtoIncludePrefixNotValid"),
        "includes": attr.string_list(),
        "protoc": attr.label(
            cfg = "host",
            executable = True,
            allow_single_file = True,
            mandatory = True,
        ),
        "plugin": attr.label(
            cfg = "host",
            allow_files = True,
            executable = True,
        ),
        "plugin_language": attr.string(),
        "plugin_options": attr.string_list(),
        "gen_cc": attr.bool(),
        "gen_pbcc": attr.bool(default = True),
        "gen_py": attr.bool(),
        "outs": attr.output_list(),
        "generate_mock_code": attr.bool(default = False),
        "generate_new_mock_code": attr.bool(default = False),
        "enable_explicit_link_proto": attr.bool(default = False),
    },
    output_to_genfiles = True,
    implementation = _proto_gen_impl,
)
"""Generates codes from Protocol Buffers definitions.

This rule helps you to implement Skylark macros specific to the target
language. tc_cc_proto_library is based cc_proto_library and surport not only grpc, but 
any IDL with it's plugin. you should wrraper it with a micro to specify the plugin and libs
you relied on.
Args:
  srcs: Protocol Buffers definition files (.proto) to run the protocol compiler
    against.
  deps: a list of dependency labels; must be other proto libraries.
  includes: a list of include paths to .proto files.
  protoc: the label of the protocol compiler to generate the sources.
  plugin: the label of the protocol compiler plugin to be passed to the protocol
    compiler.
  plugin_language: the language of the generated sources 
  plugin_options: a list of options to be passed to the plugin
  gen_cc: generates C++ sources in addition to the ones from the plugin.
  gen_py: generates Python sources in addition to the ones from the plugin.
  outs: a list of labels of the expected outputs from the protocol compiler.
"""

def tc_cc_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        # 增加这个选项是因为当proto_library使用了strip_import_prefix之后，pb桩代码将会生成在项目根路径下，导致sylar.pb.h头文件找不到的问题
        # 为了防止和业务代码冲突，这里显示设置默认值为`ProtoIncludePrefixNotValid`
        # 为了语义清晰，如果为空字符串，代表生成pb文件在根目录
        # 如果是其他目录，比如test/proto目录下，请改为"test/proto"
        # 一般，没使用strip_import_prefix的，不需要设置该值，请删掉这个选项
        proto_include_prefix = "ProtoIncludePrefixNotValid",
        cc_libs = [],
        include = None,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = False,
        use_plugin = False,
        plugin_language = "",
        generate_mock_code = False,
        generate_new_mock_code = False,
        plugin = "",
        default_runtime = "@com_google_protobuf//:protobuf",
        gen_pbcc = True,
        enable_explicit_link_proto = False,
        **kargs):
    """Bazel rule to create a C++ protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the cc_proto_library.
      srcs: the .proto files of the cc_proto_library.
      deps: a list of dependency labels; must be cc_proto_library.
      cc_libs: a list of other cc_library targets depended by the generated
          cc_library.
      include: a string indicating the include path of the .proto files.
      protoc: the label of the protocol compiler to generate the sources.
      internal_bootstrap_hack: a flag indicate the cc_proto_library is used only
          for bootstraping. When it is set to True, no files will be generated.
          The rule will simply be a provider for .proto files, so that other
          cc_proto_library can depend on it.
      use_plugin: a flag to indicate whether to call the C++ plugin
      plugin_language: indicate which RPC protocol to use,like sylar, grpc, nrpc...
          when processing the proto files.
      default_runtime: the implicitly default runtime which will be depended on by
          the generated cc_library target.
      **kargs: other keyword arguments that are passed to cc_library.

    """

    includes = []
    if include != None:
        includes = [include]

    # 检查为pb文件生成桩代码的场景, 前置参数检查
    stub_gen_hdrs = []
    stub_gen_srcs = []

    # 当srcs和deps为空时，标识为为native_proto_deps和native_cc_proto_deps生成sylar桩代码
    # 兼容当前srcs不存在，但是deps有多个sylar_proto_library的场景
    pblibdep_gen_sylar_stub = (len(srcs) == 0 and len(deps) == 0)

    # 当前仅支持sylar_proto_library
    if plugin == None or plugin.find("sylar_cpp_plugin") < 0:
        pblibdep_gen_sylar_stub = False
    if pblibdep_gen_sylar_stub:
        if len(native_proto_deps + native_cc_proto_deps) != 1:
            fail("Keep only one dep in native_proto_deps either native_cc_proto_deps when you want to generate stub code")

        # 生成的sylar桩代码文件名统一使用target的名字(因为不能在该宏定义阶段得到使用了哪个pb文件)
        stub_gen_hdrs = [name + ".sylar.pb.h"]
        stub_gen_srcs = [name + ".sylar.pb.cc"]
        if generate_mock_code or generate_new_mock_code:
            stub_gen_hdrs.append(name + ".sylar.pb.mock.h")

    # 将proto_library和cc_proto_library的依赖转化为cc_proto_library的依赖
    acutal_native_proto_deps = []
    pblib_target_suffix = "_genpblibproto"
    for dep in native_proto_deps:
        if dep.startswith(":"):
            dep_name = dep[1:] + pblib_target_suffix
        elif dep.startswith("@") or dep.startswith("/"):
            dep_name = Label(dep).name + pblib_target_suffix
        else:
            dep_name = dep
        acutal_native_proto_deps.append(dep_name)
        if dep_name not in native.existing_rules():
            native.cc_proto_library(
                name = dep_name,
                deps = [dep],
            )

    if internal_bootstrap_hack:
        # For pre-checked-in generated files, we add the internal_bootstrap_hack
        # which will skip the codegen action.
        proto_gen(
            name = name + "_genproto",
            srcs = srcs,
            deps = [s + "_genproto" for s in deps],
            native_cc_proto_deps = acutal_native_proto_deps + native_cc_proto_deps,
            proto_include_prefix = proto_include_prefix,
            includes = includes,
            protoc = protoc,
            gen_pbcc = gen_pbcc,
            visibility = ["//visibility:public"],
        )

        # An empty cc_library to make rule dependency consistent.
        native.cc_library(
            name = name,
            **kargs
        )
        return

    gen_srcs = _CcSrcs(srcs, use_plugin, plugin_language, gen_pbcc)

    # 设置gen_hdrs默认值
    gen_hdrs = _CcHdrs(srcs, use_plugin, False, plugin_language, gen_pbcc)

    # 若generate_new_mock_code和generate_mock_code为true，则优先使用gmock的新版本MockMethod
    if generate_new_mock_code:
        gen_hdrs = _CcHdrs(srcs, use_plugin, generate_new_mock_code, plugin_language, gen_pbcc)
    elif generate_mock_code:
        gen_hdrs = _CcHdrs(srcs, use_plugin, generate_mock_code, plugin_language, gen_pbcc)

    outs = gen_srcs.raw + gen_srcs.plugin + gen_hdrs.raw + gen_hdrs.plugin

    if pblibdep_gen_sylar_stub:
        outs = stub_gen_hdrs + stub_gen_srcs

    proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        deps = [s + "_genproto" for s in deps],
        native_cc_proto_deps = acutal_native_proto_deps + native_cc_proto_deps,
        proto_include_prefix = proto_include_prefix,
        includes = includes,
        protoc = protoc,
        plugin = plugin,
        plugin_language = plugin_language,
        generate_mock_code = generate_mock_code,
        generate_new_mock_code = generate_new_mock_code,
        gen_cc = 1,
        outs = outs,
        visibility = ["//visibility:public"],
        gen_pbcc = gen_pbcc,
        enable_explicit_link_proto = enable_explicit_link_proto,
    )

    if not enable_explicit_link_proto:
        native.cc_library(
            name = name + "_raw",
            srcs = gen_srcs.raw,
            hdrs = gen_hdrs.raw,
            deps = deps + [default_runtime] + acutal_native_proto_deps + native_cc_proto_deps,
            includes = includes,
            **kargs
        )
    else:
        # 本地不存在proto文件，直接通过原生proto规则（proto_library/cc_proto_library）拉取远程proto文件，生成的桩代码的场景需要加载pb.cc文件
        remote_pb_srcs = []
        if len(srcs) == 0 and (len(acutal_native_proto_deps) + len(native_cc_proto_deps)) == 1:
            if len(acutal_native_proto_deps) == 1:
                remote_pb_srcs.append(acutal_native_proto_deps[0])
            elif len(native_cc_proto_deps) == 1:
                remote_pb_srcs.append(native_cc_proto_deps[0])
        native.cc_library(
            name = name + "_raw",
            alwayslink = True,  # 尽管pb.h里定义的接口/数据类型也没有被使用，仍然加载pb.cc（proto的options获取需要加载pb.cc内的proto文件内容）
            srcs = gen_srcs.raw + remote_pb_srcs,
            hdrs = gen_hdrs.raw,
            deps = deps + [default_runtime] + acutal_native_proto_deps + native_cc_proto_deps,
            includes = includes,
            **kargs
        )

    native.cc_library(
        name = name,
        srcs = gen_srcs.plugin if not pblibdep_gen_sylar_stub else stub_gen_srcs,
        hdrs = gen_hdrs.plugin if not pblibdep_gen_sylar_stub else stub_gen_hdrs,
        deps = cc_libs + [name + "_raw"],
        **kargs
    )

def internal_copied_filegroup(name, srcs, strip_prefix, dest, **kwargs):
    """Macro to copy files to a different directory and then create a filegroup.

    This is used by the //:protobuf_python py_proto_library target to work around
    an issue caused by Python source files that are part of the same Python
    package being in separate directories.

    Args:
      srcs: The source files to copy and add to the filegroup.
      strip_prefix: Path to the root of the files to copy.
      dest: The directory to copy the source files into.
      **kwargs: extra arguments that will be passesd to the filegroup.
    """
    outs = [_RelativeOutputPath(s, strip_prefix, dest) for s in srcs]

    native.genrule(
        name = name + "_genrule",
        srcs = srcs,
        outs = outs,
        cmd = " && ".join(
            ["cp $(location %s) $(location %s)" %
             (s, _RelativeOutputPath(s, strip_prefix, dest)) for s in srcs],
        ),
    )

    native.filegroup(
        name = name,
        srcs = outs,
        **kwargs
    )

def check_protobuf_required_bazel_version():
    """For WORKSPACE files, to check the installed version of bazel.

    This ensures bazel supports our approach to proto_library() depending on a
    copied filegroup. (Fixed in bazel 0.5.4)
    """
    versions.check(minimum_bazel_version = "0.5.4")

def sylar_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        proto_include_prefix = "ProtoIncludePrefixNotValid",
        include = None,
        internal_bootstrap_hack = False,
        use_sylar_plugin = False,
        generate_mock_code = False,
        generate_new_mock_code = False,
        rootpath = "",
        gen_pbcc = True,
        plugin = None,
        # 仅启用链接proto选项的才会生成获取option的桩代码，因为获取option要求必须能链接pb.cc
        # 在复杂场景下可能不能链接成功，需要针对性的排查，默认关闭
        enable_explicit_link_proto = False,
        **kargs):
    sylar_libs = []
    plugin_language = ""
    if (use_sylar_plugin):
        sylar_libs = [
            "%s//sylar/client:rpc_service_proxy" % rootpath,
            "%s//sylar/server:rpc_async_method_handler" % rootpath,
            "%s//sylar/server:rpc_method_handler" % rootpath,
            "%s//sylar/server:rpc_service_impl" % rootpath,
            "%s//sylar/server:stream_rpc_async_method_handler" % rootpath,
            "%s//sylar/server:stream_rpc_method_handler" % rootpath,
            "%s//sylar/stream:stream" % rootpath,
        ]
        plugin_language = "sylar"
        if plugin == None:
            plugin = "%s//sylar/tools/sylar_cpp_plugin:sylar_cpp_plugin" % rootpath
    tc_cc_proto_library(
        name = name,
        srcs = srcs,
        deps = deps,
        native_proto_deps = native_proto_deps,
        native_cc_proto_deps = native_cc_proto_deps,
        proto_include_prefix = proto_include_prefix,
        cc_libs = sylar_libs,
        include = include,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = internal_bootstrap_hack,
        use_plugin = use_sylar_plugin,
        plugin_language = plugin_language,
        plugin = plugin,
        generate_mock_code = generate_mock_code,
        generate_new_mock_code = generate_new_mock_code,
        default_runtime = "@com_google_protobuf//:protobuf",
        gen_pbcc = gen_pbcc,
        enable_explicit_link_proto = enable_explicit_link_proto,
        **kargs
    )

def nrpc_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        include = None,
        internal_bootstrap_hack = False,
        use_nrpc_plugin = False,
        rootpath = "",
        **kargs):
    nrpc_libs = []
    plugin_language = ""
    plugin = None
    if (use_nrpc_plugin):
        nrpc_libs = [
            "%s//sylar/server:rpc_service_impl" % rootpath,
            "%s//sylar/server:rpc_method_handler" % rootpath,
            "%s//sylar/client:rpc_service_proxy" % rootpath,
            #"%s//sylar/client/nrpc:nrpc_service_proxy" % rootpath,
        ]
        plugin_language = "nrpc"
        plugin = "%s//sylar/tools/nrpc_cpp_plugin:nrpc_cpp_plugin" % rootpath
    tc_cc_proto_library(
        name = name,
        srcs = srcs,
        deps = deps,
        native_proto_deps = native_proto_deps,
        native_cc_proto_deps = native_cc_proto_deps,
        cc_libs = nrpc_libs,
        include = include,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = internal_bootstrap_hack,
        use_plugin = use_nrpc_plugin,
        plugin_language = plugin_language,
        plugin = plugin,
        default_runtime = "@com_google_protobuf//:protobuf",
        **kargs
    )

def oidb_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        include = None,
        internal_bootstrap_hack = False,
        use_oidb_plugin = False,
        rootpath = "",
        **kargs):
    oidb_libs = []
    plugin_language = ""
    plugin = None
    if (use_oidb_plugin):
        oidb_libs = [
            "%s//sylar/server:rpc_service_impl" % rootpath,
            "%s//sylar/server:rpc_method_handler" % rootpath,
            "%s//sylar/client:rpc_service_proxy" % rootpath,
            #"%s//sylar/client/oidb:oidb_service_proxy" % rootpath,
        ]
        plugin_language = "oidb"
        plugin = "%s//sylar/tools/oidb_cpp_plugin:oidb_cpp_plugin" % rootpath
    tc_cc_proto_library(
        name = name,
        srcs = srcs,
        deps = deps,
        native_proto_deps = native_proto_deps,
        native_cc_proto_deps = native_cc_proto_deps,
        cc_libs = oidb_libs,
        include = include,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = internal_bootstrap_hack,
        use_plugin = use_oidb_plugin,
        plugin_language = plugin_language,
        plugin = plugin,
        default_runtime = "@com_google_protobuf//:protobuf",
        **kargs
    )

def sso_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        include = None,
        internal_bootstrap_hack = False,
        use_sso_plugin = False,
        rootpath = "",
        **kargs):
    sso_libs = []
    plugin_language = ""
    plugin = None
    if (use_sso_plugin):
        sso_libs = [
            "%s//sylar/server:rpc_service_impl" % rootpath,
            "%s//sylar/server:rpc_method_handler" % rootpath,
            "%s//sylar/client/sso:sso_service_proxy" % rootpath,
        ]
        plugin_language = "sso"
        plugin = "%s//sylar/tools/sso_cpp_plugin:sso_cpp_plugin" % rootpath
    tc_cc_proto_library(
        name = name,
        srcs = srcs,
        deps = deps,
        native_proto_deps = native_proto_deps,
        native_cc_proto_deps = native_cc_proto_deps,
        cc_libs = sso_libs,
        include = include,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = internal_bootstrap_hack,
        use_plugin = use_sso_plugin,
        plugin_language = plugin_language,
        plugin = plugin,
        default_runtime = "@com_google_protobuf//:protobuf",
        **kargs
    )

def tafpb_proto_library(
        name,
        srcs = [],
        deps = [],
        native_proto_deps = [],
        native_cc_proto_deps = [],
        include = None,
        internal_bootstrap_hack = False,
        use_tafpb_plugin = True,
        gen_pbcc = False,
        generate_mock_code = False,
        generate_new_mock_code = False,
        rootpath = "",
        **kargs):
    protoc = "@com_google_protobuf//:protoc"
    default_runtime = "@com_google_protobuf//:protobuf"
    tafpb_libs = []
    plugin_language = ""
    plugin = None
    if (use_tafpb_plugin):
        tafpb_libs = [
            "%s//sylar/client/taf:taf_service_proxy" % rootpath,
        ]
        plugin_language = "tafpb"
        plugin = "%s//sylar/tools/tafpb_cpp_plugin:tafpb_cpp_plugin" % rootpath
    tc_cc_proto_library(
        name = name,
        srcs = srcs,
        deps = deps,
        native_proto_deps = native_proto_deps,
        native_cc_proto_deps = native_cc_proto_deps,
        cc_libs = tafpb_libs,
        include = include,
        protoc = "@com_google_protobuf//:protoc",
        internal_bootstrap_hack = internal_bootstrap_hack,
        use_plugin = use_tafpb_plugin,
        plugin_language = plugin_language,
        plugin = plugin,
        default_runtime = "@com_google_protobuf//:protobuf",
        gen_pbcc = gen_pbcc,
        **kargs
    )
