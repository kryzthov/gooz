# package //combinators
package(default_visibility = ["//visibility:public"])

cc_library(
    name="stream",
    srcs=["stream.cc"],
    hdrs=["stream.h"],
    deps=[
        "//base",
    ],
)

cc_library(
    name="base",
    srcs=["base.cc"],
    hdrs=["base.h"],
    deps=[
        ":stream",
        "//base",
    ],
)

cc_test(
    name="base_test",
    srcs=["base_test.cc"],
    deps=[
        ":base",
        "//base:test",
    ],
)

cc_library(
    name="combinators",
    srcs=[
        "ozlexer.cc",
        "oznode.cc",
        "oznode_check_visitor.cc",
        "oznode_compile_visitor.cc",
        "oznode_default_visitor.cc",
        "oznode_dump_visitor.cc",
        "oznode_eval_visitor.cc",
        "oznode_format_visitor.cc",
        "ozparser.cc",
    ],
    hdrs=[
        "ozlexer.h",
        "oznode.h",
        "ozparser.h",
        "oznode_check_visitor.h",
        "oznode_compile_visitor.h",
        "oznode_default_visitor.h",
        "oznode_dump_visitor.h",
        "oznode_eval_visitor.h",
        "oznode_format_visitor.h",
    ],
    deps=[
        ":stream",
        ":base",
        "//store",
        "//base",
    ],
)

cc_test(
    name="ozlexer_test",
    srcs=["ozlexer_test.cc"],
    deps=[
        ":combinators",
        "//base:test",
    ],
)

cc_test(
    name="ozparser_test",
    srcs=["ozparser_test.cc"],
    deps=[
        ":combinators",
        "//base:test",
    ],
)

cc_test(
    name="visitor_test",
    srcs=[
        #"oznode_check_visitor.h",
        "oznode_compile_visitor_test.cc",
        #"oznode_default_visitor.h",
        #"oznode_dump_visitor.h",
        "oznode_eval_visitor_test.cc",
        #"oznode_format_visitor.h",
    ],
    deps=[
        ":combinators",
        "//base:test",
    ],
)

# cc_library(
#     name="bytecode",
#     srcs=["bytecode.cc", "bytecode.h"],
#     deps=[":combinators"],
# )
#
# cc_test(
#     name="bytecode_test",
#     srcs=[
#         "bytecode_test.cc",
#     ],
#     deps=[
#         ":bytecode",
#         "//base:test",
#     ],
# )
