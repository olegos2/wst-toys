# tests_common = static_library(
#     'tests_common',
#     sources: files(
#         'common.c',
#     ),
#     include_directories: [
#         inc,
#     ],
#     dependencies: toys_poly_dep,
#     install: false,
# )

# tests_common_dep = declare_dependency(
#     link_with: tests_common_lib,
#     include_directories: inc,
#     dependencies: toys_poly_dep,
# )

test_poly = executable(
    'test_poly',
    'test_poly.c',
    dependencies: [
        toys_poly_dep,
        m_dep,
    ],
)

test('poly', test_poly)

test_argparse = executable(
    'test_argparse',
    'test_argparse.c',
    dependencies: [
        toys_common_dep,
    ],
)

test('argparse', test_argparse)

test_ds = executable(
    'test_ds',
    'test_ds.c',
    dependencies: [
        toys_common_dep,
    ],
)

test('ds', test_ds)

test_expr = executable(
    'test_expr',
    'test_expr.c',
    dependencies: [
        toys_poly_dep,
    ],
)

test('expr', test_expr)

test_string = executable(
    'test_string',
    'test_string.c',
    dependencies: [
        toys_common_dep,
    ],
)

test('string', test_string)

