#include "test_base.hpp"

namespace x11bench {

// Global test registry
std::vector<TestInfo>& get_test_registry() {
    static std::vector<TestInfo> registry;
    return registry;
}

void register_test(const std::string& name, TestFactory factory) {
    get_test_registry().push_back({name, factory});
}

// =============================================================================
// Solid Color Fill Test
// =============================================================================
class TestSolidRed : public TestBase {
public:
    std::string name() const override { return "solid_red"; }
    std::string description() const override { return "Solid red fill"; }

    void render(Display& display) override {
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestSolidRed)

class TestSolidGreen : public TestBase {
public:
    std::string name() const override { return "solid_green"; }
    std::string description() const override { return "Solid green fill"; }

    void render(Display& display) override {
        display.set_foreground(0, 255, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestSolidGreen)

class TestSolidBlue : public TestBase {
public:
    std::string name() const override { return "solid_blue"; }
    std::string description() const override { return "Solid blue fill"; }

    void render(Display& display) override {
        display.set_foreground(0, 0, 255);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestSolidBlue)

// =============================================================================
// Rectangle Tests
// =============================================================================
class TestFilledRectangle : public TestBase {
public:
    std::string name() const override { return "filled_rectangle"; }
    std::string description() const override { return "Filled rectangle on white background"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Red rectangle in center
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(width() / 4, height() / 4,
                               width() / 2, height() / 2, true);
    }
};
REGISTER_TEST(TestFilledRectangle)

class TestOutlinedRectangle : public TestBase {
public:
    std::string name() const override { return "outlined_rectangle"; }
    std::string description() const override { return "Outlined rectangle on white background"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Blue outlined rectangle
        display.set_foreground(0, 0, 255);
        display.draw_rectangle(width() / 4, height() / 4,
                               width() / 2, height() / 2, false);
    }
};
REGISTER_TEST(TestOutlinedRectangle)

class TestNestedRectangles : public TestBase {
public:
    std::string name() const override { return "nested_rectangles"; }
    std::string description() const override { return "Multiple nested rectangles"; }

    void render(Display& display) override {
        // Black background
        display.set_foreground(0, 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Nested colored rectangles
        uint8_t colors[][3] = {
            {255, 0, 0},    // Red
            {0, 255, 0},    // Green
            {0, 0, 255},    // Blue
            {255, 255, 0},  // Yellow
            {255, 0, 255},  // Magenta
            {0, 255, 255},  // Cyan
        };

        int margin = 20;
        for (int i = 0; i < 6; i++) {
            int x = margin * (i + 1);
            int y = margin * (i + 1);
            int w = width() - 2 * x;
            int h = height() - 2 * y;
            if (w > 0 && h > 0) {
                display.set_foreground(colors[i][0], colors[i][1], colors[i][2]);
                display.draw_rectangle(x, y, w, h, true);
            }
        }
    }
};
REGISTER_TEST(TestNestedRectangles)

// =============================================================================
// Line Tests
// =============================================================================
class TestHorizontalLines : public TestBase {
public:
    std::string name() const override { return "horizontal_lines"; }
    std::string description() const override { return "Horizontal lines pattern"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Black horizontal lines
        display.set_foreground(0, 0, 0);
        for (uint32_t y = 0; y < height(); y += 10) {
            display.draw_line(0, y, width() - 1, y);
        }
    }
};
REGISTER_TEST(TestHorizontalLines)

class TestVerticalLines : public TestBase {
public:
    std::string name() const override { return "vertical_lines"; }
    std::string description() const override { return "Vertical lines pattern"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Black vertical lines
        display.set_foreground(0, 0, 0);
        for (uint32_t x = 0; x < width(); x += 10) {
            display.draw_line(x, 0, x, height() - 1);
        }
    }
};
REGISTER_TEST(TestVerticalLines)

class TestDiagonalLines : public TestBase {
public:
    std::string name() const override { return "diagonal_lines"; }
    std::string description() const override { return "Diagonal lines pattern"; }

    void render(Display& display) override {
        // Gray background
        display.set_foreground(128, 128, 128);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Red diagonal from top-left to bottom-right
        display.set_foreground(255, 0, 0);
        display.draw_line(0, 0, width() - 1, height() - 1);

        // Blue diagonal from top-right to bottom-left
        display.set_foreground(0, 0, 255);
        display.draw_line(width() - 1, 0, 0, height() - 1);

        // Green cross in center
        display.set_foreground(0, 255, 0);
        int cx = width() / 2;
        int cy = height() / 2;
        display.draw_line(cx - 50, cy, cx + 50, cy);
        display.draw_line(cx, cy - 50, cx, cy + 50);
    }
};
REGISTER_TEST(TestDiagonalLines)

// =============================================================================
// Arc/Circle Tests
// =============================================================================
class TestCircle : public TestBase {
public:
    std::string name() const override { return "circle"; }
    std::string description() const override { return "Circle outline"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Black circle in center
        display.set_foreground(0, 0, 0);
        int radius = std::min(width(), height()) / 3;
        int cx = width() / 2 - radius;
        int cy = height() / 2 - radius;
        display.draw_arc(cx, cy, radius * 2, radius * 2, 0, 360 * 64);
    }
};
REGISTER_TEST(TestCircle)

class TestConcentricCircles : public TestBase {
public:
    std::string name() const override { return "concentric_circles"; }
    std::string description() const override { return "Concentric circles"; }

    void render(Display& display) override {
        // Black background
        display.set_foreground(0, 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Multiple colored circles
        uint8_t colors[][3] = {
            {255, 0, 0}, {255, 128, 0}, {255, 255, 0},
            {0, 255, 0}, {0, 255, 255}, {0, 0, 255}, {255, 0, 255}
        };

        int max_radius = std::min(width(), height()) / 2 - 10;
        int step = max_radius / 7;

        for (int i = 0; i < 7; i++) {
            int radius = max_radius - i * step;
            if (radius <= 0) break;

            display.set_foreground(colors[i][0], colors[i][1], colors[i][2]);
            int cx = width() / 2 - radius;
            int cy = height() / 2 - radius;
            display.draw_arc(cx, cy, radius * 2, radius * 2, 0, 360 * 64);
        }
    }
};
REGISTER_TEST(TestConcentricCircles)

// =============================================================================
// Checkerboard Pattern
// =============================================================================
class TestCheckerboard : public TestBase {
public:
    std::string name() const override { return "checkerboard"; }
    std::string description() const override { return "Black and white checkerboard"; }

    void render(Display& display) override {
        int cell_size = 32;

        for (uint32_t y = 0; y < height(); y += cell_size) {
            for (uint32_t x = 0; x < width(); x += cell_size) {
                bool white = ((x / cell_size) + (y / cell_size)) % 2 == 0;
                display.set_foreground(white ? 255 : 0, white ? 255 : 0, white ? 255 : 0);
                display.draw_rectangle(x, y, cell_size, cell_size, true);
            }
        }
    }
};
REGISTER_TEST(TestCheckerboard)

} // namespace x11bench
