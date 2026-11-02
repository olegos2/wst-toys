#!/usr/bin/env bash

set -euo pipefail

HOME_DIR=$(dirname "$(realpath "$0")")
BUILD_DIR="$HOME_DIR/builddir"
OBJ_DIR="$BUILD_DIR/obj"

CC=${CC:-gcc}
LD=${LD:-gcc}
AR=${AR:-ar}
CLAUNCHER=${CLAUNCHER-ccache}

DEFAULT_CFLAGS="-Wall -Wextra -Wconversion -Wfloat-equal -Wshadow"
DEFAULT_CFLAGS+=" -Wpointer-arith -Wno-unused-function -O2 -g"
DEFAULT_CPPFLAGS="-DWST_DEBUG"

GRAPHVIZ_CFLAGS=$(pkg-config --cflags libgvc)
GRAPHVIZ_LIBS=$(pkg-config --libs libgvc)

declare -A SOURCE_CFLAGS=(
    [src/graphviz.c]="$GRAPHVIZ_CFLAGS"
)

if ! command -v "$CLAUNCHER" >/dev/null 2>&1; then
    echo "warning: '$CLAUNCHER' not found, building without it" >&2
    CLAUNCHER=
fi

run() {
    printf '%q ' "$@"
    printf '\n'
    "$@"
}

my_cc() {
    local -a cmd=()
    if [[ -n $CLAUNCHER ]]; then
        cmd+=("$CLAUNCHER")
    fi
    cmd+=("$CC")
    cmd+=(${CFLAGS:-$DEFAULT_CFLAGS} ${CPPFLAGS:-$DEFAULT_CPPFLAGS})
    cmd+=(-I"$HOME_DIR/include")
    cmd+=("$@")
    run "${cmd[@]}"
}

my_ld() {
    local -a cmd=("$LD")
    cmd+=(${LDFLAGS:-})
    cmd+=("$@")
    run "${cmd[@]}"
}

compile() {
    local src="$1"
    shift
    local obj="$OBJ_DIR/${src%.c}.o"
    mkdir -p "$(dirname "$obj")"
    my_cc ${SOURCE_CFLAGS[$src]:-} "$@" -c "$HOME_DIR/$src" -o "$obj"
    last_obj="$obj"
}

# build_archive output sources...
build_archive() {
    local out="$BUILD_DIR/$1"
    shift
    local -a objs=()
    local src
    for src in "$@"; do
        compile "$src"
        objs+=("$last_obj")
    done
    mkdir -p "$(dirname "$out")"
    rm -f "$out"
    run "$AR" rcs "$out" "${objs[@]}"
}

# build_exe output sources... -- link flags...
build_exe() {
    local out="$BUILD_DIR/$1"
    shift
    local -a objs=()
    while [[ $# -gt 0 && $1 != "--" ]]; do
        compile "$1"
        objs+=("$last_obj")
        shift
    done
    if [[ ${1:-} == "--" ]]; then
        shift
    fi
    mkdir -p "$(dirname "$out")"
    my_ld "${objs[@]}" "$@" -o "$out"
}

mkdir -p "$BUILD_DIR"
cd "$HOME_DIR"

build_archive libtoys_common.a src/*.c src/debug/*.c
build_archive libtoys_poly.a src/poly/*.c

declare -a common_libs=("$BUILD_DIR/libtoys_common.a" -lm)
declare -a poly_libs=(
    "$BUILD_DIR/libtoys_poly.a"
    "$BUILD_DIR/libtoys_common.a"
    -lm
)
declare -a graph_libs=("$BUILD_DIR/libtoys_common.a" $GRAPHVIZ_LIBS -lm)

build_exe examples/calc/toys_solve \
    examples/calc/solve.c examples/calc/plot.c \
    -- "${poly_libs[@]}" -lraylib

build_exe examples/file_sort/file_sort \
    examples/file_sort/line.c examples/file_sort/file_sort.c \
    -- "${common_libs[@]}"

build_exe examples/list_trace/list_trace \
    examples/list_trace/list_trace.c \
    -- "${graph_libs[@]}"

build_exe tests/test_argparse tests/test_argparse.c \
    -- "${common_libs[@]}"
build_exe tests/test_debug tests/test_debug.c \
    -- "${graph_libs[@]}"
build_exe tests/test_expr tests/test_expr.c \
    -- "${poly_libs[@]}"
build_exe tests/test_list tests/test_list.c \
    -- "${graph_libs[@]}"
build_exe tests/test_list_idx tests/test_list_idx.c \
    -- "${graph_libs[@]}"
build_exe tests/test_mtx tests/test_mtx.c \
    -- "${common_libs[@]}"
build_exe tests/test_poly tests/test_poly.c \
    -- "${poly_libs[@]}"
build_exe tests/test_sort tests/test_sort.c \
    -- "${common_libs[@]}"
build_exe tests/test_string tests/test_string.c \
    -- "${common_libs[@]}"
build_exe tests/test_vec tests/test_vec.c \
    -- "${common_libs[@]}"

echo "Build finished"
