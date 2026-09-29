#!/usr/bin/env bash

HOME_DIR=$(dirname "$(realpath "$0")")
BUILD_DIR="$HOME_DIR/builddir"

export CC="${CC:=gcc}"
export PATH="/usr/lib/ccache/bin:$PATH"

declare -a common_src=(
    "$HOME_DIR/src/poly/expr.c"
    "$HOME_DIR/src/poly/poly.c"
    "$HOME_DIR/src/argparse.c"
    "$HOME_DIR/src/debug.c"
    "$HOME_DIR/src/mtx.c"
    "$HOME_DIR/src/sort.c"
    "$HOME_DIR/src/stk.c"
    "$HOME_DIR/src/string.c"
    "$HOME_DIR/src/vector.c"
)

# should add more flags?
declare -a common_flags=(
    -lm -I"$HOME_DIR/include" -I"$HOME_DIR/src/inculde"
    ${CFLAGS:="-Wall -Wextra -Wconversion -Wfloat-equal -Wshadow -Wpointer-arith -Wno-unused-function -O2 -g"}
    -DWST_DEBUG
)

mkdir -p "$BUILD_DIR" &&
pushd "$BUILD_DIR" &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/examples/calc/solve.c" \
    "$HOME_DIR/examples/calc/plot.c" \
    -lraylib \
    "${common_flags[@]}" -o toys_solve &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/examples/file_sort/line.c" \
    "$HOME_DIR/examples/file_sort/file_sort.c" \
    "${common_flags[@]}" -o file_sort &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_argparse.c" \
    "${common_flags[@]}" -o test_argparse &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_ds.c" \
    "${common_flags[@]}" -o test_ds &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_expr.c" \
    "${common_flags[@]}" -o test_expr &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_mtx.c" \
    "${common_flags[@]}" -o test_mtx &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_poly.c" \
    "${common_flags[@]}" -o test_poly &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_sort.c" \
    "${common_flags[@]}" -o test_sort &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_stk.c" \
    "${common_flags[@]}" -o test_stk &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_string.c" \
    "${common_flags[@]}" -o test_string &&
popd &&
echo "Finished"
