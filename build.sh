#!/usr/bin/env bash

HOME_DIR=$(dirname "$(realpath "$0")")
BUILD_DIR="$HOME_DIR/builddir"

export CC="${CC:=gcc}"

declare -a common_src=(
    "$HOME_DIR/src/poly/expr.c"
    "$HOME_DIR/src/poly/poly.c"
    "$HOME_DIR/src/argparse.c"
    "$HOME_DIR/src/debug.c"
    "$HOME_DIR/src/ds.c"
)

# коллективный разум закреп
declare -a common_flags=(
    -lm -I"$HOME_DIR/include" -I"$HOME_DIR/src/inculde"
    ${CFLAGS:="-Wall -Wextra -Wconversion -Wfloat-equal -O2 -g"}
    -DWST_DEBUG
)

mkdir -p "$BUILD_DIR" &&
pushd "$BUILD_DIR" &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/examples/solve.c" \
    "$HOME_DIR/examples/plot.c" \
    "${common_flags[@]}" -o toys_solve &&
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
    "$HOME_DIR/tests/test_poly.c" \
    "${common_flags[@]}" -o test_poly &&
"$CC" \
    "${common_src[@]}" \
    "$HOME_DIR/tests/test_expr.c" \
    "${common_flags[@]}" -o test_expr &&
popd &&
echo "Finished"
