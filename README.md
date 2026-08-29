# wst-toys

C library for polynomial math: expression parsing, arithmetic, solving, calculus.

## Dependencies

- `raylib` -- for graphical calculator example

## Build

```sh
# requires meson >= 1.1.0 and ninja
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

To build without raylib (tests and libs only):

```sh
meson setup builddir -Dexamples=false
```

For build-system-less build:

```sh
./build.sh
```

## Library

- `poly.h` -- polynomial type, arithmetic (`add`/`sub`/`mul`/`scale`), `eval`, `deriv`, `integ`, `solve`
- `expr.h` -- parse math expressions or raw coefficients into polynomials
- `argparse.h` -- command line option parser
- `ds.h` -- dynamic array (stb-style)
- `debug.h` -- logging with priority levels

## Example

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
