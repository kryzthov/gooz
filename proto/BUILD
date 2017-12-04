# package //proto
package(default_visibility = ["//visibility:public"])

proto_library(
    name="protolib",
    srcs=["store.proto"],
)

cc_proto_library(
    name="proto",
    deps=[":protolib"],
)
