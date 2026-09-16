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

DEFAULT_CFLAGS="-Wall -Wextra -Wconversion -Wfloat-equal -Wshadow -Wpointer-arith -Wno-unused-function -O2 -g"
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

# build_archive dir ar_filename sources
build_archive() {
    mkdir -p "$BUILD_DIR/$1" || return
    declare -a objs=()
    local src_name
    for src_name in "${@:3}"; do
        my_cc -c "$HOME_DIR/$1/$src_name.c" -o "$1/$src_name.o" || return
        objs+=("$1/$src_name.o")
    done
    "$AR" rcs "$2" "${objs[@]}"
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

build_tests() {
    my_cc -c "$HOME_DIR/tests/test_argparse.c" -o test_argparse.o &&
    my_ld test_argparse.o libtoys_common.a -o test_argparse &&

    my_cc -c "$HOME_DIR/tests/test_ds.c" -o test_ds.o &&
    my_ld test_ds.o libtoys_common.a -o test_ds &&

    my_cc -c "$HOME_DIR/tests/test_expr.c" -o test_expr.o &&
    my_ld test_expr.o libtoys_poly.a libtoys_common.a -lm -o test_expr &&

    my_cc -c "$HOME_DIR/tests/test_mtx.c" -o test_mtx.o &&
    my_ld test_mtx.o libtoys_common.a -o test_mtx &&

    my_cc -c "$HOME_DIR/tests/test_poly.c" -o test_poly.o &&
    my_ld test_poly.o libtoys_poly.a libtoys_common.a -lm -o test_poly &&

    my_cc -c "$HOME_DIR/tests/test_sort.c" -o test_sort.o &&
    my_ld test_sort.o libtoys_common.a -o test_sort &&

    my_cc -c "$HOME_DIR/tests/test_string.c" -o test_string.o &&
    my_ld test_string.o libtoys_common.a -o test_string
}

pushd "$BUILD_DIR" &&
build_archive src libtoys_common.a \
    argparse \
    debug \
    ds \
    mtx \
    sort \
    string &&
build_archive src/poly libtoys_poly.a \
    expr \
    poly &&
build_solve &&
build_tests &&
popd &&
echo "Build finished"
