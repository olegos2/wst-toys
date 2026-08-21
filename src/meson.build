subdir('poly')

toys_argparse_lib = static_library(
    'toys_argparse',
    sources: files(
        'argparse.c',
    ),
    include_directories: inc,
    install: false,
)

toys_argparse_dep = declare_dependency(
    link_with: toys_argparse_lib,
    include_directories: inc,
)

toys_ds_lib = static_library(
    'toys_ds',
    sources: files(
        'ds.c',
    ),
    include_directories: inc,
    install: false,
)

toys_ds_dep = declare_dependency(
    link_with: toys_ds_lib,
    include_directories: inc,
)
