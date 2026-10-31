toys_impl_hdrs = files(
    'impl/list_api_internal.h',
    'impl/list_common_internal.h',
    'impl/list_idx_internal.h',
    'impl/list_internal.h',
    'impl/vec_internal.h',
)

toys_hdrs = files(
    'argparse.h',
    'debug.h',
    'expr.h',
    'file.h',
    'list.h',
    'math.h',
    'mtx.h',
    'poly.h',
    'sort.h',
    'string.h',
    'vec.h',
)

install_headers(toys_hdrs, subdir: 'toys')
install_headers(toys_impl_hdrs, subdir: 'toys/impl')
