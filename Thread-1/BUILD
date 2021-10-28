##load("@rules_cc//cc:defs.bzl", "cc_test")  #load the test target

cc_binary(
    name = "q0",
    srcs = [
        "q0.cc"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q0*"]),
)

cc_binary(
    name = "format",
    srcs = [
        "format.cc"
            ],
)

cc_binary(
    name = "q1",
    srcs = [
        "q1.cc"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q1*"]),
)

cc_binary(
    name = "q2",
    srcs = [
        "q2.cc"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q2*"]),
)

cc_binary(
    name = "q3",
    srcs = [
        "q3.cc"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q3*"]),
)

cc_binary(
    name = "q4",
    srcs = [
        "q4.cc"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib"
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q4*"]),
)

cc_test(
  name = "benchmark",
  size = "small",
  srcs = ["benchmark.cc"],
  deps = [
      "@gbench//:benchmark",
      ],
  copts = [
        "-O3",
        "-std=c++11",
  ],
  data = glob(["data/*"]),
)

cc_binary(
    name = "q1_test",
    srcs = [
        "q1_test.cc",
        "q1.h"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib",
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q1*"]),
)

cc_binary(
    name = "q2_test",
    srcs = [
        "q2_test.cc",
        "q2.h"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib",
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q2*"]),
)

cc_binary(
    name = "q3_test",
    srcs = [
        "q3_test.cc",
        "q3.h"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib",
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q3*"]),
)

cc_binary(
    name = "q4_test",
    srcs = [
        "q4_test.cc",
        "q4.h"
            ],
    deps = [
        "//lib:embedding_lib",
        "//lib:instruction_lib",
        "//lib:utils_lib",
        "//lib:model_lib",
    ],
    copts = [
        "-std=c++11",
    ],
    data = glob(["data/q4*"]),
)