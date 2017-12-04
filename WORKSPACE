workspace(name="gooz")

git_repository(
    name="com_google_protobuf",
    remote="https://github.com/google/protobuf",
    tag="v3.5.0",
)

# --------------------------------------------------------------------------------------------------
# GoogleTest

git_repository(
    name="com_google_googletest",
    remote="https://github.com/google/googletest",
    commit="247a3d8e5e5d403f7fcacdb8ccc71e5059f15daa",  # master as of 2017-12-01
    # tag="release-1.8.0",  # v1.8.0 is missing Bazel rules
)

bind(
    name = "gtest",
    actual = "@com_google_googletest//:gtest",
)

bind(
    name = "gtest_main",
    actual = "@com_google_googletest//:gtest_main",
)

# --------------------------------------------------------------------------------------------------
# GoogleFlags

git_repository(
    name="com_google_gflags",
    remote="https://github.com/gflags/gflags",
    tag="v2.2.1",
)
