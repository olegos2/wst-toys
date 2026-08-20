toys_quad_lib = static_library(
    'toys_quad',
    sources: files(
        'quad.c',
    ),
    include_directories: [
        inc,
    ],
    dependencies: [
        m_dep,
    ],
    install: false,
)

toys_quad_dep = declare_dependency(
    link_with: toys_quad_lib,
    include_directories: inc,
)
