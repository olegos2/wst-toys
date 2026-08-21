#!/usr/bin/env bash

set -uo pipefail

HOME_DIR=$(dirname "$(realpath "$0")")
BUILD_DIR="$HOME_DIR/builddir"

if [[ -d $BUILD_DIR ]]; then
    echo "$BUILD_DIR already exists"
    exit 1
fi

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

DEFAULT_CFLAGS="-Wall -Wextra -Wconversion -Wfloat-equal"
DEFAULT_LDFLAGS=""
export CC=${CC:=gcc}
export CXX=${CXX:=g++}
export LD=${LD:=gcc}

print_eval() {
    echo "$@"
    eval "$@"
}

my_cc() {
    declare -a args=(${CFLAGS:="$DEFAULT_CFLAGS"})
    for i in "${common_inc[@]}"; do
        args+=(-I"$i")
    done
    print_eval "$CC" "${args[@]}" "$@"
}

my_ld() {
    print_eval "$LD" ${LDFLAGS:="$DEFAULT_LDFLAGS"} "$@"
}

build_poly() {
    my_cc -c "$HOME_DIR/src/poly/expr.c -o toys_expr.o &&
    my_cc -c "$HOME_DIR/src/poly/poly.c -o toys_poly.o
}

build_argparse() {
    my_cc -c "$HOME_DIR/src/argparse.c" -o toys_argparse.o
}

build_solve() {
    declare -a solve_deps=(
        toys_argparse.o
        toys_expr.o
        toys_poly.o
        toys_solve.o
    )
    declare -a solve_libs=(
        m
    )

    for i in "${solve_libs[@]}"; do
        solve_deps+=(-l"$i")
    done

    my_cc -c "$HOME_DIR/examples/solve.c" -o toys_solve.o &&
    my_ld "${solve_deps[@]}" -o toys_solve
}

# build_test_poly() {
#     declare -a test_poly_src=(
#         tests/test_poly.c
#     )
# }

pushd "$BUILD_DIR" &&
build_poly &&
build_argparse &&
build_solve &&
popd

