# wst-toys

Small C library and playground. It has polynomial math, matrices,
sorting, type-safe stacks, a dynamic array and its own string functions.
Everything shares the same logging and command line parsing code. Tests
and examples are included.

API docs (doxygen, rebuilt on every push to `main`):
<https://olegos2.github.io/wst-toys/>

## What you need

- A C compiler, `meson >= 1.1.0`, `ninja`
- `raylib`, only for the graphical calculator example

## How to build

```sh
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

Leave out examples to skip raylib. Leave out tests to skip test builds.

```sh
meson setup builddir -Dexamples=false
meson setup builddir -Dtests=false
```

No build system at hand. Use one of the plain scripts instead.

```sh
./build.sh      # Simpler and slower
./build_ext.sh  # Still simple, but slightly faster
```

## What is inside (`include/toys`)

- `argparse.h` — parses flags and args. Switches, int and string options,
  positional args and `--help` output
- `debug.h` — logging with levels (`LOG_V` `LOG_D` `LOG_W` `LOG_E`
  `LOG_I`). Colored output, file redirect and a chatty assert included
- `vector.h` — growable array in stb style (`vec_push` `vec_len`
  `vec_cap` `vec_free`)
- `expr.h` — turns math expressions or plain coefficients into polynomials
- `math.h` — small float helpers (`my_iszero`, `my_isnan`, `my_isinf`)
- `mtx.h` — plain and symmetric matrices. Add, scale, multiply and
  determinant. Short names with `WST_MTX_SHORT_NAMES`
- `poly.h` — polynomials. Basic math (`add` `sub` `mul` `scale`) plus
  `eval` `deriv` `integ` and `solve` for linear and quadratic equations
- `sort.h` — generic `qsort` and `bsort` that take a comparator, plus a
  byte radix sort for plain strings
- `stk.h` — type-safe stacks (`WstStkInt`, `WstStkDouble`), details below
- `string.h` — string and memory functions in libc style (`strcpy`,
  `strtok`, `memswp` and the rest)

## Stacks (`stk.h`)

One shared core stamps out a stack per element type. You get full type
safety with no macro-written function bodies. Each call checks the stack
first. It looks at length and capacity, at a canary byte block on both
sides of the buffer, and at a checksum over the struct fields and its
contents. Problems come back as `WstStkErr` (`NOMEM` `EMPTY` `CORRUPT`
`OVERFLOW` `NULL`, read them with `wst_stk_err_str`). A failed call never
touches the stack. Not even `free`, since a broken data pointer can not
be trusted. Each check has its own switch: `STK_USE_CANARY`,
`STK_USE_HASH` and `STK_USE_CONTENT_HASH`. They follow `WST_DEBUG` unless
you set them to 0 or 1 by hand. `stk_dump` prints fields, checksum
values, canary bytes and buffer contents when you need to see what went
wrong.

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

Big structs skip by-value copies. Stamp with `STK_REF` and `push` takes
a pointer instead (`pop` already does). `test_stk.c` shows a full example.

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
