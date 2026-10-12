
toys_common_lib = library(
    'toys_common',
    sources: files(
        'argparse.c',
        'debug.c',
        'file.c',
        'mtx.c',
        'sort.c',
        'string.c',
        'vec.c',
    ),
    include_directories: inc,
    install: false,
)

toys_common_dep = declare_dependency(
    link_with: toys_common_lib,
    include_directories: inc,
)

subdir('poly')
