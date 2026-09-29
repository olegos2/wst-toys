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
export CLAUNCHER=${CLAUNCHER:=ccache}

if ! command -v "$CLAUNCHER" &>/dev/null; then
    echo "Failed to find CLAUNCHER binary: $CLAUNCHER"
    CLAUNCHER=""
fi

print_eval() {
    echo "$@"
    eval "$@"
}

my_cc() {
    declare -a args=(${CFLAGS:="$DEFAULT_CFLAGS"} ${CPPFLAGS:="$DEFAULT_CPPFLAGS"})
    for i in "${common_inc[@]}"; do
        args+=(-I"$i")
    done
    if [[ -z $CLAUNCHER ]]; then
        print_eval "$CC" "${args[@]}" "$@"
    else
        print_eval "$CLAUNCHER" "$CC" "${args[@]}" "$@"
    fi
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
        "$BUILD_DIR/libtoys_poly.a"
        "$BUILD_DIR/libtoys_common.a"
    )
    declare -a solve_libs=(
        m
        raylib
    )

    for i in "${solve_libs[@]}"; do
        solve_deps+=(-l"$i")
    done

    mkdir -p examples/calc &&
    pushd examples/calc || return

    my_cc -c "$HOME_DIR/examples/calc/solve.c" -o calc_solve.o &&
    my_cc -c "$HOME_DIR/examples/calc/plot.c" -o calc_plot.o &&
    my_ld calc_solve.o calc_plot.o "${solve_deps[@]}" -o toys_solve || {
        popd
        return 1
    }

    popd
}

build_file_sort() {
    declare -a file_sort_deps=(
        "$BUILD_DIR/libtoys_common.a"
    )

    mkdir -p examples/file_sort &&
    pushd examples/file_sort || return

    my_cc -c "$HOME_DIR/examples/file_sort/file_sort.c" -o file_sort.o &&
    my_cc -c "$HOME_DIR/examples/file_sort/line.c" -o line.o &&
    my_ld file_sort.o line.o "${file_sort_deps[@]}" -o file_sort || {
        popd
        return 1
    }

    popd
}

build_tests() {
    mkdir -p tests &&
    pushd tests || return

    my_cc -c "$HOME_DIR/tests/test_argparse.c" -o test_argparse.o &&
    my_ld test_argparse.o "$BUILD_DIR/libtoys_common.a" -o test_argparse &&

    my_cc -c "$HOME_DIR/tests/test_ds.c" -o test_ds.o &&
    my_ld test_ds.o "$BUILD_DIR/libtoys_common.a" -o test_ds &&

    my_cc -c "$HOME_DIR/tests/test_expr.c" -o test_expr.o &&
    my_ld test_expr.o "$BUILD_DIR/libtoys_poly.a" "$BUILD_DIR/libtoys_common.a" -lm -o test_expr &&

    my_cc -c "$HOME_DIR/tests/test_mtx.c" -o test_mtx.o &&
    my_ld test_mtx.o "$BUILD_DIR/libtoys_common.a" -o test_mtx &&

    my_cc -c "$HOME_DIR/tests/test_poly.c" -o test_poly.o &&
    my_ld test_poly.o "$BUILD_DIR/libtoys_poly.a" "$BUILD_DIR/libtoys_common.a" -lm -o test_poly &&

    my_cc -c "$HOME_DIR/tests/test_sort.c" -o test_sort.o &&
    my_ld test_sort.o "$BUILD_DIR/libtoys_common.a" -o test_sort &&

    my_cc -c "$HOME_DIR/tests/test_stk.c" -o test_stk.o &&
    my_ld test_stk.o "$BUILD_DIR/libtoys_common.a" -o test_stk &&

    my_cc -c "$HOME_DIR/tests/test_string.c" -o test_string.o &&
    my_ld test_string.o "$BUILD_DIR/libtoys_common.a" -o test_string || {
        popd
        return 1
    }

    popd    
}

pushd "$BUILD_DIR" &&
build_archive src libtoys_common.a \
    argparse \
    debug \
    mtx \
    sort \
    stk \
    string \
    vector &&
build_archive src/poly libtoys_poly.a \
    expr \
    poly &&
build_solve &&
build_file_sort &&
build_tests &&
popd &&
echo "Build finished"
