package(
    default_visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "dagger",
    hdrs = ["dagger.h"],
    srcs = ["dagger.cpp"],
)

cc_test(
    name = "dagger_test",
    srcs = ["dagger_test.cpp"],
    deps = [
        ":dagger",
        "@com_google_googletest//:gtest_main",
    ],
)
