#include "test_base.hpp"

namespace x11bench {

// =============================================================================
// XRender / Composite Tests
// =============================================================================
class TestAlphaRectangles : public TestBase {
public:
    std::string name() const override { return "alpha_rectangles"; }
    std::string description() const override { return "Semi-transparent overlapping rectangles (XRender)"; }

    void render(Display& display) override {
        if (!display.has_xrender()) {
            // Fallback: just draw solid rectangles with a marker
            display.set_foreground(128, 128, 128);
            display.draw_rectangle(0, 0, width(), height(), true);
            display.set_foreground(255, 0, 0);
            display.draw_rectangle(10, 10, 50, 20, true);  // "No XRender" marker
            return;
        }

        // White background using XRender
        display.render_fill_rectangle(0, 0, width(), height(), 255, 255, 255, 255);

        // Semi-transparent red rectangle
        display.render_fill_rectangle(50, 50, 150, 150, 255, 0, 0, 128);

        // Semi-transparent blue rectangle (overlapping)
        display.render_fill_rectangle(100, 100, 150, 150, 0, 0, 255, 128);

        // Semi-transparent green rectangle (overlapping both)
        display.render_fill_rectangle(75, 75, 100, 100, 0, 255, 0, 128);
    }

    int tolerance() const override { return 2; }  // Alpha blending may have rounding
};
REGISTER_TEST(TestAlphaRectangles)

class TestAlphaGradient : public TestBase {
public:
    std::string name() const override { return "alpha_gradient"; }
    std::string description() const override { return "Alpha transparency gradient (XRender)"; }

    void render(Display& display) override {
        if (!display.has_xrender()) {
            display.set_foreground(128, 128, 128);
            display.draw_rectangle(0, 0, width(), height(), true);
            return;
        }

        // Checkerboard background to show transparency
        int cell = 16;
        for (uint32_t y = 0; y < height(); y += cell) {
            for (uint32_t x = 0; x < width(); x += cell) {
                bool white = ((x / cell) + (y / cell)) % 2 == 0;
                display.render_fill_rectangle(x, y, cell, cell,
                    white ? 255 : 200, white ? 255 : 200, white ? 255 : 200, 255);
            }
        }

        // Red rectangle with varying alpha
        for (uint32_t x = 0; x < width(); x++) {
            uint8_t alpha = (x * 255) / (width() - 1);
            display.render_fill_rectangle(x, 80, 1, 96, 255, 0, 0, alpha);
        }
    }

    int tolerance() const override { return 3; }
};
REGISTER_TEST(TestAlphaGradient)

class TestLayeredAlpha : public TestBase {
public:
    std::string name() const override { return "layered_alpha"; }
    std::string description() const override { return "Multiple layered alpha blends (XRender)"; }

    void render(Display& display) override {
        if (!display.has_xrender()) {
            display.set_foreground(128, 128, 128);
            display.draw_rectangle(0, 0, width(), height(), true);
            return;
        }

        // White background
        display.render_fill_rectangle(0, 0, width(), height(), 255, 255, 255, 255);

        // Build up layers with different alpha values
        int layers = 10;
        for (int i = 0; i < layers; i++) {
            int margin = i * 10;
            int w = width() - 2 * margin;
            int h = height() - 2 * margin;
            if (w <= 0 || h <= 0) break;

            // Alternate colors
            uint8_t r = (i % 3 == 0) ? 255 : 0;
            uint8_t g = (i % 3 == 1) ? 255 : 0;
            uint8_t b = (i % 3 == 2) ? 255 : 0;

            display.render_fill_rectangle(margin, margin, w, h, r, g, b, 50);
        }
    }

    int tolerance() const override { return 3; }
};
REGISTER_TEST(TestLayeredAlpha)

class TestRenderFillColors : public TestBase {
public:
    std::string name() const override { return "render_fill_colors"; }
    std::string description() const override { return "XRender color fill accuracy"; }

    void render(Display& display) override {
        if (!display.has_xrender()) {
            display.set_foreground(128, 128, 128);
            display.draw_rectangle(0, 0, width(), height(), true);
            return;
        }

        // Test that XRender fills produce correct colors
        int cols = 4;
        int rows = 4;
        int cell_w = width() / cols;
        int cell_h = height() / rows;

        struct Color { uint8_t r, g, b, a; };
        Color colors[] = {
            {255, 0, 0, 255},     // Pure red
            {0, 255, 0, 255},     // Pure green
            {0, 0, 255, 255},     // Pure blue
            {255, 255, 0, 255},   // Yellow
            {255, 0, 255, 255},   // Magenta
            {0, 255, 255, 255},   // Cyan
            {255, 128, 0, 255},   // Orange
            {128, 0, 128, 255},   // Purple
            {0, 0, 0, 255},       // Black
            {255, 255, 255, 255}, // White
            {128, 128, 128, 255}, // Gray
            {64, 64, 64, 255},    // Dark gray
            {192, 192, 192, 255}, // Light gray
            {128, 0, 0, 255},     // Dark red
            {0, 128, 0, 255},     // Dark green
            {0, 0, 128, 255},     // Dark blue
        };

        for (int i = 0; i < 16; i++) {
            int x = (i % cols) * cell_w;
            int y = (i / cols) * cell_h;
            display.render_fill_rectangle(x, y, cell_w, cell_h,
                colors[i].r, colors[i].g, colors[i].b, colors[i].a);
        }
    }

    int tolerance() const override { return 1; }
};
REGISTER_TEST(TestRenderFillColors)

class TestAlphaBlendModes : public TestBase {
public:
    std::string name() const override { return "alpha_blend"; }
    std::string description() const override { return "Alpha blending with specific values"; }

    void render(Display& display) override {
        if (!display.has_xrender()) {
            display.set_foreground(128, 128, 128);
            display.draw_rectangle(0, 0, width(), height(), true);
            return;
        }

        // Solid color background
        display.render_fill_rectangle(0, 0, width(), height(), 0, 0, 255, 255);  // Blue

        // 50% white should produce light blue
        display.render_fill_rectangle(0, 0, width() / 2, height() / 2, 255, 255, 255, 128);

        // 50% red over blue should produce purple
        display.render_fill_rectangle(width() / 2, 0, width() / 2, height() / 2, 255, 0, 0, 128);

        // 50% green over blue should produce cyan
        display.render_fill_rectangle(0, height() / 2, width() / 2, height() / 2, 0, 255, 0, 128);

        // 50% black over blue should produce dark blue
        display.render_fill_rectangle(width() / 2, height() / 2, width() / 2, height() / 2, 0, 0, 0, 128);
    }

    int tolerance() const override { return 2; }
};
REGISTER_TEST(TestAlphaBlendModes)

class TestPixelPerfect : public TestBase {
public:
    std::string name() const override { return "pixel_perfect"; }
    std::string description() const override { return "Single pixel accuracy test"; }
    uint32_t width() const override { return 64; }
    uint32_t height() const override { return 64; }

    void render(Display& display) override {
        // Black background
        display.set_foreground(0, 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Draw individual pixels in a pattern
        for (uint32_t y = 0; y < height(); y++) {
            for (uint32_t x = 0; x < width(); x++) {
                // Create a deterministic pattern
                if ((x + y) % 2 == 0) {
                    uint8_t r = (x * 4) & 0xFF;
                    uint8_t g = (y * 4) & 0xFF;
                    uint8_t b = ((x + y) * 2) & 0xFF;
                    display.set_foreground(r, g, b);
                    display.draw_rectangle(x, y, 1, 1, true);
                }
            }
        }
    }

    int tolerance() const override { return 0; }  // Must be exact
};
REGISTER_TEST(TestPixelPerfect)

} // namespace x11bench
