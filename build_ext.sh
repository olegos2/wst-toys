#!/usr/bin/env bash

set -uo pipefail

HOME_DIR=$(dirname "$(realpath "$0")")
BUILD_DIR="$HOME_DIR/builddir"

# if [[ -d $BUILD_DIR ]]; then
#     echo "$BUILD_DIR already exists"
#     exit 1
# fi

mkdir -p "$BUILD_DIR"

# # files_list dir file..
# files_list() {
#     for i in "${@:2}"; do
#         echo "$1/$i"
#     done
# }

declare -a common_inc=(
    "$HOME_DIR/include"
    "$HOME_DIR/src/include"
)

DEFAULT_CFLAGS="-Wall -Wextra -Wconversion -Wfloat-equal -Wshadow-all -O2 -g"
DEFAULT_CPPFLAGS="-DWST_DEBUG"
DEFAULT_LDFLAGS=""
export CC=${CC:=gcc}
export CXX=${CXX:=g++}
export LD=${LD:=gcc}
export AR=${AR:=ar}

print_eval() {
    echo "$@"
    eval "$@"
}

my_cc() {
    declare -a args=(${CFLAGS:="$DEFAULT_CFLAGS"} ${CPPFLAGS:="$DEFAULT_CPPFLAGS"})
    for i in "${common_inc[@]}"; do
        args+=(-I"$i")
    done
    print_eval "$CC" "${args[@]}" "$@"
}

my_ld() {
    print_eval "$LD" ${LDFLAGS:="$DEFAULT_LDFLAGS"} "$@"
}

build_common() {
    my_cc -c "$HOME_DIR/src/argparse.c" -o toys_argparse.o &&
    my_cc -c "$HOME_DIR/src/debug.c" -o toys_debug.o &&
    my_cc -c "$HOME_DIR/src/ds.c" -o toys_ds.o &&
    "$AR" rcs libtoys_common.a toys_argparse.o toys_debug.o toys_ds.o
}

build_poly() {
    my_cc -c "$HOME_DIR/src/poly/expr.c" -o toys_expr.o &&
    my_cc -c "$HOME_DIR/src/poly/poly.c" -o toys_poly.o &&
    "$AR" rcs libtoys_poly.a toys_expr.o toys_poly.o
}

build_solve() {
    declare -a solve_deps=(
        libtoys_poly.a
        libtoys_common.a
    )
    declare -a solve_libs=(
        m
        raylib
    )

    for i in "${solve_libs[@]}"; do
        solve_deps+=(-l"$i")
    done

    my_cc -c "$HOME_DIR/examples/calc/solve.c" -o calc_solve.o &&
    my_cc -c "$HOME_DIR/examples/calc/plot.c" -o calc_plot.o &&
    my_ld calc_solve.o calc_plot.o "${solve_deps[@]}" -o toys_solve
}

build_test_poly() {
    my_cc -c "$HOME_DIR/tests/test_poly.c" -o test_poly.o &&
    my_ld test_poly.o libtoys_poly.a libtoys_common.a -lm -o test_poly
}

build_test_ds() {
    my_cc -c "$HOME_DIR/tests/test_ds.c" -o test_ds.o &&
    my_ld test_ds.o libtoys_common.a -o test_ds
}

build_test_argparse() {
    my_cc -c "$HOME_DIR/tests/test_argparse.c" -o test_argparse.o &&
    my_ld test_argparse.o libtoys_common.a -o test_argparse
}

build_test_expr() {
    my_cc -c "$HOME_DIR/tests/test_expr.c" -o test_expr.o &&
    my_ld test_expr.o libtoys_poly.a libtoys_common.a -lm -o test_expr
}

pushd "$BUILD_DIR" &&
build_common &&
build_poly &&
build_solve &&
build_test_argparse &&
build_test_ds &&
build_test_poly &&
build_test_expr &&
popd &&
echo "Build finished"
