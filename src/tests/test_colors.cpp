#include "test_base.hpp"
#include <cmath>

namespace x11bench {

// =============================================================================
// Color Accuracy Tests
// =============================================================================
class TestColorBars : public TestBase {
public:
    std::string name() const override { return "color_bars"; }
    std::string description() const override { return "Standard color bars (SMPTE-like)"; }

    void render(Display& display) override {
        // Standard color bars: white, yellow, cyan, green, magenta, red, blue, black
        uint8_t colors[][3] = {
            {255, 255, 255},  // White
            {255, 255, 0},    // Yellow
            {0, 255, 255},    // Cyan
            {0, 255, 0},      // Green
            {255, 0, 255},    // Magenta
            {255, 0, 0},      // Red
            {0, 0, 255},      // Blue
            {0, 0, 0},        // Black
        };

        int bar_width = width() / 8;

        for (int i = 0; i < 8; i++) {
            display.set_foreground(colors[i][0], colors[i][1], colors[i][2]);
            display.draw_rectangle(i * bar_width, 0, bar_width, height(), true);
        }
    }
};
REGISTER_TEST(TestColorBars)

class TestGrayscaleRamp : public TestBase {
public:
    std::string name() const override { return "grayscale_ramp"; }
    std::string description() const override { return "Grayscale gradient ramp"; }

    void render(Display& display) override {
        // Horizontal grayscale gradient
        for (uint32_t x = 0; x < width(); x++) {
            uint8_t gray = (x * 255) / (width() - 1);
            display.set_foreground(gray, gray, gray);
            display.draw_line(x, 0, x, height() - 1);
        }
    }
};
REGISTER_TEST(TestGrayscaleRamp)

class TestRGBRamps : public TestBase {
public:
    std::string name() const override { return "rgb_ramps"; }
    std::string description() const override { return "Separate R, G, B gradient ramps"; }

    void render(Display& display) override {
        int section_height = height() / 3;

        // Red ramp
        for (uint32_t x = 0; x < width(); x++) {
            uint8_t val = (x * 255) / (width() - 1);
            display.set_foreground(val, 0, 0);
            display.draw_line(x, 0, x, section_height - 1);
        }

        // Green ramp
        for (uint32_t x = 0; x < width(); x++) {
            uint8_t val = (x * 255) / (width() - 1);
            display.set_foreground(0, val, 0);
            display.draw_line(x, section_height, x, 2 * section_height - 1);
        }

        // Blue ramp
        for (uint32_t x = 0; x < width(); x++) {
            uint8_t val = (x * 255) / (width() - 1);
            display.set_foreground(0, 0, val);
            display.draw_line(x, 2 * section_height, x, height() - 1);
        }
    }
};
REGISTER_TEST(TestRGBRamps)

class TestColorWheel : public TestBase {
public:
    std::string name() const override { return "color_wheel"; }
    std::string description() const override { return "HSV color wheel approximation"; }
    uint32_t width() const override { return 256; }
    uint32_t height() const override { return 256; }

    void render(Display& display) override {
        // Black background
        display.set_foreground(0, 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);

        int cx = width() / 2;
        int cy = height() / 2;
        int radius = std::min(width(), height()) / 2 - 10;

        // Draw filled colored segments
        for (int angle = 0; angle < 360; angle++) {
            double rad = angle * 3.14159265 / 180.0;

            // HSV to RGB (simplified, S=1, V=1)
            double h = angle / 60.0;
            int hi = static_cast<int>(h) % 6;
            double f = h - static_cast<int>(h);
            uint8_t v = 255;
            uint8_t q = static_cast<uint8_t>(255 * (1 - f));
            uint8_t t = static_cast<uint8_t>(255 * f);

            uint8_t r, g, b;
            switch (hi) {
                case 0: r = v; g = t; b = 0; break;
                case 1: r = q; g = v; b = 0; break;
                case 2: r = 0; g = v; b = t; break;
                case 3: r = 0; g = q; b = v; break;
                case 4: r = t; g = 0; b = v; break;
                default: r = v; g = 0; b = q; break;
            }

            display.set_foreground(r, g, b);

            // Draw line from center
            int x2 = cx + static_cast<int>(radius * cos(rad));
            int y2 = cy + static_cast<int>(radius * sin(rad));
            display.draw_line(cx, cy, x2, y2);
        }
    }

    int tolerance() const override { return 2; }  // Allow slight anti-aliasing differences
};
REGISTER_TEST(TestColorWheel)

class TestColorGrid : public TestBase {
public:
    std::string name() const override { return "color_grid"; }
    std::string description() const override { return "Grid of distinct colors"; }

    void render(Display& display) override {
        // 16x16 grid of colors varying in R and G, B fixed
        int cell_w = width() / 16;
        int cell_h = height() / 16;

        for (int gy = 0; gy < 16; gy++) {
            for (int gx = 0; gx < 16; gx++) {
                uint8_t r = gx * 17;  // 0, 17, 34, ..., 255
                uint8_t g = gy * 17;
                uint8_t b = 128;

                display.set_foreground(r, g, b);
                display.draw_rectangle(gx * cell_w, gy * cell_h, cell_w, cell_h, true);
            }
        }
    }
};
REGISTER_TEST(TestColorGrid)

class TestSpecificColors : public TestBase {
public:
    std::string name() const override { return "specific_colors"; }
    std::string description() const override { return "Specific color value accuracy test"; }

    void render(Display& display) override {
        // Test specific color values for accuracy
        struct ColorSample {
            uint8_t r, g, b;
            const char* name;
        };

        ColorSample colors[] = {
            {0, 0, 0, "Black"},
            {255, 255, 255, "White"},
            {255, 0, 0, "Pure Red"},
            {0, 255, 0, "Pure Green"},
            {0, 0, 255, "Pure Blue"},
            {128, 128, 128, "Mid Gray"},
            {64, 64, 64, "Dark Gray"},
            {192, 192, 192, "Light Gray"},
            {255, 128, 0, "Orange"},
            {128, 0, 128, "Purple"},
            {0, 128, 128, "Teal"},
            {128, 128, 0, "Olive"},
        };

        int num_colors = sizeof(colors) / sizeof(colors[0]);
        int rows = 3;
        int cols = 4;
        int cell_w = width() / cols;
        int cell_h = height() / rows;

        for (int i = 0; i < num_colors; i++) {
            int gx = i % cols;
            int gy = i / cols;

            display.set_foreground(colors[i].r, colors[i].g, colors[i].b);
            display.draw_rectangle(gx * cell_w, gy * cell_h, cell_w, cell_h, true);
        }
    }
};
REGISTER_TEST(TestSpecificColors)

} // namespace x11bench
