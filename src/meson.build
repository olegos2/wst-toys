
graphviz_dep = dependency('libgvc')

toys_common_lib = library(
    'toys_common',
    sources: files(
        'argparse.c',
        'debug/assert.c',
        'debug/files.c',
        'debug/html.c',
        'debug/image.c',
        'debug/log.c',
        'file.c',
        'graph.c',
        'graphviz.c',
        'list.c',
        'list_graph.c',
        'list_idx.c',
        'mtx.c',
        'sort.c',
        'string.c',
        'vec.c',
    ),
    include_directories: inc,
    dependencies: graphviz_dep,
    install: false,
)

toys_common_dep = declare_dependency(
    link_with: toys_common_lib,
    include_directories: inc,
    dependencies: graphviz_dep,
)

subdir('poly')
