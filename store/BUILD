# package //store
package(default_visibility = ["//visibility:public"])

cc_library(
    name="store",
    srcs=glob(
        include=[
            "arity.cc",
            "array.cc",
            "atom.cc",
            "boolean.cc",
            "bytecode.cc",
            "cell.cc",
            "closure.cc",
            "compiler.cc",
            "engine.cc",
            "environment.cc",
            "float.cc",
            "heap_value.cc",
            "integer.cc",
            "list.cc",
            "literal.cc",
            "moved_value.cc",
            "name.cc",
            "open_record.cc",
            "ozvalue.cc",
            "record.cc",
            "store.cc",
            "string.cc",
            "thread.cc",
            "tuple.cc",
            "value.cc",
            "variable.cc",
            "*.h",
#            "*.cc",
        ],
        exclude=[
            "compiler.*",
            "*_test.cc",
        ],
    ),
    hdrs=glob(include=["*.h"]),
    deps=[
        "//base",
        "//proto",
    ],
)

# cc_test(
#     name="base_test",
#     srcs=glob(
#         include=["*_test.cc"],
#     ),
#     deps=[":base"],
#     linkopts=[
#         "-L/W/github.com/google/googletest/bazel-bin",
#         "-lgflags",
#         "-lgtest",
#         "-lmpfr",
#         "-lglog",
#         "-lboost_regex",
#     ],
# )
