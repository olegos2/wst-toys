# wst-toys

Small C library and playground. It has polynomial math, matrices,
sorting, type-safe vectors and its own string functions.
Everything shares the same logging and command line parsing code. Tests
and examples are included.

API docs (doxygen, rebuilt on every push to `main`):
<https://olegos2.github.io/wst-toys/>

## What you need

- A C compiler, `meson >= 1.1.0`, `ninja`
- `graphviz` development files (`libgvc`) for graph rendering
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

No build system at hand. Use the plain build script instead.

```sh
./build.sh      # Plain bash build, no Meson
```

## What is inside (`include/toys`)

- `argparse.h` — parses flags and args. Switches, int and string options,
  positional args and `--help` output
- `debug.h` — logging with levels (`LOG_V` `LOG_D` `LOG_W` `LOG_E`
  `LOG_I`). Text or HTML files, images and a chatty assert included
- `graph.h` — small Graphviz DOT builder with escaped labels and attributes
- `graphviz.h` — reusable Graphviz context that renders DOT to SVG
- `expr.h` — turns math expressions or plain coefficients into polynomials
- `file.h` — maps a whole file into memory (`mmap`, malloc and read
  fallback)
- `math.h` — small float helpers (`my_iszero`, `my_isnan`, `my_isinf`)
- `mtx.h` — plain and symmetric matrices. Add, scale, multiply and
  determinant. Short names with `WST_MTX_SHORT_NAMES`
- `poly.h` — polynomials. Basic math (`add` `sub` `mul` `scale`) plus
  `eval` `deriv` `integ` and `solve` for linear and quadratic equations
- `sort.h` — generic `qsort` and `bsort` that take a comparator, plus a
  byte radix sort for plain strings
- `vec.h` — type-safe vectors (`WstVecInt`, `WstVecDouble`, `WstVecU8`),
  details below
- `string.h` — string and memory functions in libc style (`strcpy`,
  `strtok`, `memswp` and the rest)

## Vectors (`vec.h`)

One shared core stamps out a vector per element type. You get full type
safety with no macro-written function bodies. Each call checks the vector
first. It looks at length and capacity, at a canary byte block on both
sides of the buffer, and at a checksum over the struct fields and its
contents. Problems come back as `WstVecErr` (`NOMEM` `EMPTY` `CORRUPT`
`OVERFLOW` `NULL`, read them with `wst_vec_err_str`). A failed call never
touches the vector. Not even `free`, since a broken data pointer can not
be trusted. Each check has its own switch: `VEC_USE_CANARY`,
`VEC_USE_HASH` and `VEC_USE_CONTENT_HASH`. They follow `WST_DEBUG` unless
you set them to 0 or 1 by hand. `vec_dump` prints fields, checksum
values, canary bytes and buffer contents when you need to see what went
wrong.

```c
#include "toys/vec.h"

WstVecDouble s = { 0 };
wst_vec_double_init(&s, 16);    // reinit + reserve 16 elements

wst_vec_double_push(&s, 1.5);

double v = 0;
wst_vec_double_pop(&s, &v);     // pass NULL to discard instead

wst_vec_double_verify(&s);      // explicit check, also runs inside every op
wst_vec_double_reserve(&s, 64);
wst_vec_double_free(&s);        // frees vector, zeroes the struct, safe to call twice

wst_vec_double_set(&s, 0, 2.5); // indexed write, reseals afterwards
wst_vec_double_ensure(&s, 128); // grow length, zero-filling the new part
```

Big structs skip by-value copies. Stamp with `VEC_REF` and `push` takes
a pointer instead (`pop` already does). `test_vec.c` shows a full example.

## Lists

There are two doubly linked list backends. Both use the same stamped header
style and expose the same names, so switching is mostly a matter of swapping
the include.

- `impl/list_internal.h` keeps nodes in heap blocks. Node addresses stay stable
  across insertions, which makes node handles safe to hold for a long time.
- `impl/list_idx_internal.h` keeps nodes in one flat array. Links are 32-bit
  indices, which makes nodes smaller and more cache friendly. Slot 0 is a
  sentinel that links the head and tail, so insertion and removal don't need
  head/tail special cases. Insertions may grow and move the array, so fetch a
  node again after growth instead of holding on to its pointer.

Both backends share the typed API template in `impl/list_api_internal.h`.
Only their storage primitives and lifecycle functions differ. Both accept a
zero-initialized list. Stamp the chosen backend in one source file with
`LIST_IMPL`, and include it without `LIST_IMPL` where only the type and
declarations are needed:

```c
#define LIST_ELEM int
#define LIST_T MyIntList
#define LIST_F(name) my_int_list_ ## name
#define LIST_FMT "%d"
#define LIST_IMPL
#include "toys/impl/list_idx_internal.h"

MyIntList list = { 0 };
my_int_list_append(&list, 7);
my_int_list_free(&list);
```

`test_list.c` and `test_list_idx.c` show the full API.

## Logs and graphs

Logs go to `stderr` by default. Pass a directory to get a timestamped file,
and pick HTML when you want escaped text, colored levels and images:

```c
wst_log_open("logs", WST_LOG_FORMAT_HTML);
LOG_I("started");
wst_log_close();
```

HTML mode uses the same `LOG_*` macros. Text is escaped, priorities and source
locations get their own CSS classes, and the file ends with a valid HTML
footer even when the program exits without an explicit close.

`toys/graph.h` builds DOT without string plumbing. `toys/graphviz.h` renders
it in-process through a reusable Graphviz context:

```c
WstGraph *graph = wst_graph_new("demo");
wst_graph_node(graph, "a", "first", NULL);
wst_graph_node(graph, "b", "second", NULL);
wst_graph_edge(graph, "a", "b", "style=dashed");

const char *svg = wst_log_graphviz(wst_graph_dot(graph), "demo");
wst_log_image(svg, "demo graph");
wst_graph_free(graph);
```

`list_dump` uses the same path to print aligned live, pooled and raw slot
tables and to embed a graph of the live chain. The `.dot` and `.svg` files
stay next to the log for later inspection.

`examples/list_trace/list_trace.c` demonstrates this end to end. It dumps both
list backends into one HTML log and performs an append, insert, remove and slot
reuse after each dump so the graphs change visibly:

```sh
./builddir/examples/list_trace/list_trace logs
```

## Tests

```sh
meson test -C builddir
```

Covers: `argparse`, `debug`, `expr`, `string`, `mtx`, `poly`,
`sort`, `vec`, `list`, `list_idx`. Each is a standalone binary under
`builddir/tests/`.

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
./file_sort --verbose --logdir logs --log-html poem.txt output.txt
```
