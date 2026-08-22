
toys_common_lib = static_library(
    'toys_common',
    sources: files(
        'argparse.c',
        'ds.c',
            'debug.c',
    ),
    include_directories: inc,
    install: false,
)

toys_common_dep = declare_dependency(
    link_with: toys_common_lib,
    include_directories: inc,
)

subdir('poly')
