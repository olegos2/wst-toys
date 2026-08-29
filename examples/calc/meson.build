raylib_dep = dependency('raylib')

toys_solve_deps = [
    toys_poly_dep,
    toys_common_dep,
    raylib_dep,
]

executable(
    'toys_solve',
    sources: files(
        # 'font.c',
        'plot.c',
        'solve.c',
    ),
    include_directories: inc,
    dependencies: toys_solve_deps,
    install: true,
)
