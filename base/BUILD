# package //base
package(default_visibility = ["//visibility:public"])

cc_library(
    name="base",
    srcs=glob(
        include=[
            "*.h",
            "*.cc",
        ],
        exclude=[
            "*_test.cc",
            "gtest_main.cc",
        ],
    ),
    hdrs=glob(include=["*.h"]),
    linkopts=[
        "-lgflags",
        "-lglog",
        "-lmpfr",
        "-lgmp",
        "-lgmpxx",
        "-lboost_regex",
    ],
)

cc_library(
    name="test",
    srcs=["gtest_main.cc"],
    deps=[":base"],
    linkopts=[
        "-L/W/github.com/google/googletest/bazel-bin",
        "-lgtest",
    ],
)

cc_test(
    name="base_test",
    srcs=glob(
        include=["*_test.cc"],
    ),
    deps=[":base", ":test"],
)
