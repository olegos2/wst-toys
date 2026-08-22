m_dep = cc.find_library('m', required: false)

toys_poly_deps = [
    m_dep,
    toys_common_dep,
]

toys_poly_lib = static_library(
    'toys_poly',
    sources: files(
        'poly.c',
        'expr.c',
    ),
    include_directories: [
        inc,
    ],
    dependencies: toys_poly_deps,
    install: false,
)

toys_poly_dep = declare_dependency(
    link_with: toys_poly_lib,
    include_directories: inc,
    dependencies: toys_poly_deps,
)
