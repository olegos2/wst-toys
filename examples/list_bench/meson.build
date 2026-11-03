list_bench = files('list_bench.c')

executable(
    'list_bench_block',
    sources: list_bench,
    include_directories: inc,
    dependencies: [
        toys_common_dep,
    ],
)

executable(
    'list_bench_index',
    sources: list_bench,
    include_directories: inc,
    dependencies: [
        toys_common_dep,
    ],
    c_args: ['-DLIST_BENCH_INDEX'],
)
