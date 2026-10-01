# wst-toys

C library and playground: polynomial math (expression parsing, arithmetic,
solving, calculus), dense matrices, sorting, type-safe stacks, a dynamic
array, and its own libc-style string ops. All comes with shared logging and CLI
parsing infrastructure, with tests and examples.

API docs (doxygen, rebuilt automatically on every push to `main`):
<https://olegos2.github.io/wst-toys/>

## Dependencies

- C compiler, `meson >= 1.1.0`, `ninja`
- `raylib` — only for the graphical calculator example

## Build

```sh
# requires meson >= 1.1.0 and ninja
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

Without examples (skips raylib dependency) or without tests:

```sh
meson setup builddir -Dexamples=false
meson setup builddir -Dtests=false
```

For build-system-less builds:

```sh
./build.sh      # Simpler and slower
./build_ext.sh  # Still simple, but slightly faster
```

## Library (`include/toys`)

- `argparse.h` — command line parser: switches, int/string options, positional
  args, `--help` generation
- `debug.h` — leveled logging (`LOG_V`/`LOG_D`/`LOG_W`/`LOG_E`/`LOG_I`),
  terminal colors (`auto`/`always`/`never`), log redirection to a file,
  verbose assert
- `vector.h` — stb-style dynamic array (`vec_push`/`vec_len`/`vec_cap`/`vec_free`)
- `expr.h` — parse math expressions or raw coefficients into polynomials
- `math.h` — float helpers (`my_iszero`, `my_isnan`, `my_isinf`)
- `mtx.h` — dense and symmetric matrices: arithmetic, `mul`, `det`,
  optional short-name aliases (`WST_MTX_SHORT_NAMES`)
- `poly.h` — polynomial type, arithmetic (`add`/`sub`/`mul`/`scale`),
  `eval`, `deriv`, `integ`, `solve` (up to quadratic)
- `sort.h` — generic `qsort`/`bsort` over a comparator plus byte radix sort
  for NUL-terminated strings
- `stk.h` — type-safe stacks (`WstStkInt`, `WstStkDouble`), see below
- `string.h` — libc-style string and memory ops (`strcpy`, `strtok`,
  `memswp`, …)

## Stack (`stk.h`)

Stacks are stamped per element type from one shared implementation, so they
stay fully type-safe without macro-written bodies. Every operation verifies
the stack first: length/capacity invariants, an 8-byte canary tail after the
buffer, and a checksum over the struct fields. Failures are reported with
`WstStkErr` (`NOMEM`/`EMPTY`/`CORRUPT`/`OVERFLOW`, see `wst_stk_err_str`).
A refused operation never mutates the stack — not even `free`, since a
damaged data pointer can't be trusted.

```c
#include "toys/stk.h"

WstStkDouble s = { 0 };
wst_stk_double_init(&s, 16);    // reinit + reserve 16 elements

wst_stk_double_push(&s, 1.5);

double v = 0;
wst_stk_double_pop(&s, &v);     // pass NULL to discard instead

wst_stk_double_verify(&s);      // explicit check, also runs inside every op
wst_stk_double_reserve(&s, 64);
wst_stk_double_free(&s);        // frees stack, zeroes the struct, safe to call twice
```

Large elements avoid by-value copies: stamp with `STK_REF` defined and
`push` takes a pointer instead (`pop` already does) — see `test_stk.c`
for a stamped example.

## Tests

```sh
meson test -C builddir
```

Covers: `argparse`, `ds`, `expr`, `string`, `mtx`, `poly`,
`sort`, `stk`. Each is a standalone binary under `builddir/tests/`.

## Examples

```sh
# solve interactively
./toys_solve

# get help about available flags and subcommands
./toys_solve --help

# solve an expression, get derivative and integrate
./toys_solve --pretty --expr solve "0.08 * x ^ 2 + 0.3 * x - 5"

# plot with audio
# Press `space` to start playback
# Press `g` to toggle grid
# Tap and move to pan
# Scroll to scale plot in both axes
# Hold `shift` and scroll to scale plot along X axis
./toys_solve --pretty --expr plot \
  "1e-07*x^11 + 5.3e-06*x^10 + 0.000104*x^9 + 0.0008002*x^8 - 0.0008451*x^7 - 0.0497787*x^6 - 0.250217*x^5 - 0.137462*x^4 + 1.52641*x^3 + 1.60167*x^2 - 2.69069*x" , \
  "0.2 * x ^ 3 - x ^ 2 + 3" , \
  "-x" , \
  "(x + 5) ** 2 - 4"
```

```sh
# Sort a poem in 2 ways, then catenate original contents to output
./file_sort --verbose --logfile log.txt poem.txt output.txt
```
