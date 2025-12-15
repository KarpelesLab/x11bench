# x11bench

A visual test framework for validating X11 client library rendering. Creates windows, renders specific patterns, captures the output via X11, and compares against reference images.

Designed for testing X11 client library implementations by verifying pixel-perfect (or within tolerance) rendering output.

## Features

- **52 visual tests** covering core X11 functionality
- **PNG reference images** for comparison
- **Fuzzy matching** with configurable tolerance for anti-aliased content
- **CLI interface** with filtering, regeneration, and failure debugging
- Works with real X servers or Xvfb for headless testing

## Test Categories

### Basic Shapes
Rectangles, lines, circles, arcs, and polygons.

| Filled Rectangle | Checkerboard | Concentric Circles |
|:---:|:---:|:---:|
| ![filled_rectangle](reference/filled_rectangle.png) | ![checkerboard](reference/checkerboard.png) | ![concentric_circles](reference/concentric_circles.png) |

### Colors & Gradients
Color accuracy, gradients, and color space tests.

| Color Bars | Grayscale Ramp | Color Wheel |
|:---:|:---:|:---:|
| ![color_bars](reference/color_bars.png) | ![grayscale_ramp](reference/grayscale_ramp.png) | ![color_wheel](reference/color_wheel.png) |

### Text Rendering
Font rendering with Xft at various sizes and colors.

| Basic Text | Font Sizes | Colored Text |
|:---:|:---:|:---:|
| ![basic_text](reference/basic_text.png) | ![font_sizes](reference/font_sizes.png) | ![colored_text](reference/colored_text.png) |

### XRender Compositing
Alpha blending and transparency tests using the XRender extension.

| Alpha Rectangles | Alpha Gradient | Layered Alpha |
|:---:|:---:|:---:|
| ![alpha_rectangles](reference/alpha_rectangles.png) | ![alpha_gradient](reference/alpha_gradient.png) | ![layered_alpha](reference/layered_alpha.png) |

### GC Raster Operations
XOR, AND, OR, invert, and other graphics context functions.

| XOR Draw | GC Functions | GC Invert |
|:---:|:---:|:---:|
| ![xor_draw](reference/xor_draw.png) | ![gc_functions](reference/gc_functions.png) | ![gc_invert](reference/gc_invert.png) |

### Line Styles
Dashed lines, cap styles, join styles, and line widths.

| Dashed Lines | Line Caps | Line Joins |
|:---:|:---:|:---:|
| ![dashed_lines](reference/dashed_lines.png) | ![line_cap_styles](reference/line_cap_styles.png) | ![line_join_styles](reference/line_join_styles.png) |

### Stipple & Tile Patterns
Fill patterns using stipples and tiles.

| Stipple Fill | Stipple Patterns | Tile Fill |
|:---:|:---:|:---:|
| ![stipple_fill](reference/stipple_fill.png) | ![stipple_patterns](reference/stipple_patterns.png) | ![tile_fill](reference/tile_fill.png) |

### Clipping & Masking
Clip masks, clip rectangles, and plane masks.

| Clip Mask | Clip Rectangles | Plane Mask |
|:---:|:---:|:---:|
| ![clip_mask](reference/clip_mask.png) | ![clip_rectangles](reference/clip_rectangles.png) | ![plane_mask](reference/plane_mask.png) |

### Fill Rules
Polygon fill rules for self-intersecting shapes.

| EvenOdd Rule | Winding Rule |
|:---:|:---:|
| ![fill_rule_evenodd](reference/fill_rule_evenodd.png) | ![fill_rule_winding](reference/fill_rule_winding.png) |

## Building

### Dependencies

- CMake 3.16+
- C++17 compiler
- libX11, libXext, libXrender, libXft
- libpng

On Debian/Ubuntu:
```bash
apt install build-essential cmake libx11-dev libxext-dev libxrender-dev libxft-dev libpng-dev
```

On Fedora:
```bash
dnf install gcc-c++ cmake libX11-devel libXext-devel libXrender-devel libXft-devel libpng-devel
```

### Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage

```bash
# Run all tests
./x11bench

# List available tests
./x11bench --list

# Run specific tests (filter by name)
./x11bench --filter xor
./x11bench --filter stipple

# Regenerate reference images
./x11bench --regenerate

# Save failure images for debugging
./x11bench --save-failures

# Verbose output
./x11bench -v

# Use specific X display
./x11bench --display :1
```

### Headless Testing with Xvfb

```bash
# Start Xvfb
Xvfb :99 -screen 0 1024x768x24 &

# Run tests
DISPLAY=:99 ./x11bench
```

## Test Output

```
Running X11 visual tests
============================================================

alpha_blend                         [PASS]
alpha_gradient                      [PASS]
checkerboard                        [PASS]
xor_draw                            [PASS]
...

============================================================
Summary:
  Passed:  52
  Failed:  0
  Total:   52
```

## Adding New Tests

Tests are defined in `src/tests/` and automatically registered using the `REGISTER_TEST` macro:

```cpp
class TestMyPattern : public TestBase {
public:
    std::string name() const override { return "my_pattern"; }
    std::string description() const override { return "Description of test"; }

    // Optional: custom window size (default 256x256)
    uint32_t width() const override { return 256; }
    uint32_t height() const override { return 256; }

    // Optional: comparison tolerance (default 0 = exact match)
    int tolerance() const override { return 0; }

    void render(Display& display) override {
        // Draw your pattern using display methods
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestMyPattern)
```

Run with `--regenerate` to create the reference image, then subsequent runs will compare against it.

## Project Structure

```
x11bench/
├── CMakeLists.txt
├── src/
│   ├── main.cpp           # Test runner
│   ├── display.hpp/cpp    # X11 Display/Window wrapper (RAII)
│   ├── image.hpp/cpp      # RGBA image buffer with PNG I/O
│   ├── capture.hpp/cpp    # Window capture via XGetImage
│   ├── compare.hpp/cpp    # Image comparison
│   └── tests/
│       ├── test_base.hpp      # Test interface
│       ├── test_shapes.cpp    # Shape tests
│       ├── test_colors.cpp    # Color tests
│       ├── test_text.cpp      # Text rendering tests
│       ├── test_composite.cpp # XRender tests
│       └── test_advanced.cpp  # GC ops, stipples, clips, etc.
└── reference/             # Reference PNG images
```

## License

MIT
