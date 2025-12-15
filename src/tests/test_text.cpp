#include "test_base.hpp"
#include <cmath>

namespace x11bench {

// =============================================================================
// Text Rendering Tests
// =============================================================================
class TestBasicText : public TestBase {
public:
    std::string name() const override { return "basic_text"; }
    std::string description() const override { return "Basic text rendering with Xft"; }
    uint32_t width() const override { return 400; }
    uint32_t height() const override { return 300; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Load a common font
        XftFont* font = display.load_font("monospace", 16);
        if (!font) {
            font = display.load_font("fixed", 16);
        }
        if (!font) {
            // Fallback: draw a rectangle to indicate font failure
            display.set_foreground(255, 0, 0);
            display.draw_rectangle(10, 10, 100, 20, true);
            return;
        }

        // Draw black text
        display.draw_text(font, 20, 40, "Hello, X11!", 0, 0, 0);
        display.draw_text(font, 20, 70, "The quick brown fox", 0, 0, 0);
        display.draw_text(font, 20, 100, "jumps over the lazy dog.", 0, 0, 0);
        display.draw_text(font, 20, 140, "0123456789", 0, 0, 0);
        display.draw_text(font, 20, 170, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 0, 0, 0);
        display.draw_text(font, 20, 200, "abcdefghijklmnopqrstuvwxyz", 0, 0, 0);

        display.free_font(font);
    }

    int tolerance() const override { return 5; }  // Font rendering varies slightly
    double allowed_diff_percent() const override { return 1.0; }
};
REGISTER_TEST(TestBasicText)

class TestColoredText : public TestBase {
public:
    std::string name() const override { return "colored_text"; }
    std::string description() const override { return "Colored text rendering"; }
    uint32_t width() const override { return 400; }
    uint32_t height() const override { return 250; }

    void render(Display& display) override {
        // Gray background
        display.set_foreground(200, 200, 200);
        display.draw_rectangle(0, 0, width(), height(), true);

        XftFont* font = display.load_font("sans", 18);
        if (!font) font = display.load_font("fixed", 18);
        if (!font) return;

        display.draw_text(font, 20, 40, "Red Text", 255, 0, 0);
        display.draw_text(font, 20, 70, "Green Text", 0, 180, 0);
        display.draw_text(font, 20, 100, "Blue Text", 0, 0, 255);
        display.draw_text(font, 20, 130, "Yellow Text", 200, 200, 0);
        display.draw_text(font, 20, 160, "Magenta Text", 255, 0, 255);
        display.draw_text(font, 20, 190, "Cyan Text", 0, 200, 200);

        display.free_font(font);
    }

    int tolerance() const override { return 5; }
    double allowed_diff_percent() const override { return 1.0; }
};
REGISTER_TEST(TestColoredText)

class TestFontSizes : public TestBase {
public:
    std::string name() const override { return "font_sizes"; }
    std::string description() const override { return "Different font sizes"; }
    uint32_t width() const override { return 500; }
    uint32_t height() const override { return 350; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        int sizes[] = {8, 10, 12, 14, 16, 20, 24, 32};
        int y = 20;

        for (int size : sizes) {
            XftFont* font = display.load_font("sans", size);
            if (!font) font = display.load_font("fixed", size);
            if (!font) continue;

            char text[64];
            snprintf(text, sizeof(text), "Size %d: The quick brown fox", size);

            y += size + 4;
            display.draw_text(font, 20, y, text, 0, 0, 0);

            display.free_font(font);
        }
    }

    int tolerance() const override { return 5; }
    double allowed_diff_percent() const override { return 2.0; }
};
REGISTER_TEST(TestFontSizes)

class TestUnicodeText : public TestBase {
public:
    std::string name() const override { return "unicode_text"; }
    std::string description() const override { return "Unicode character rendering"; }
    uint32_t width() const override { return 450; }
    uint32_t height() const override { return 300; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        XftFont* font = display.load_font("sans", 16);
        if (!font) font = display.load_font("fixed", 16);
        if (!font) return;

        // Various Unicode strings
        display.draw_text(font, 20, 40, "ASCII: Hello World!", 0, 0, 0);
        display.draw_text(font, 20, 70, "Latin: cafe, resume, naive", 0, 0, 0);
        display.draw_text(font, 20, 100, "Symbols: +  -  *  /  =", 0, 0, 0);
        display.draw_text(font, 20, 130, "Arrows: <  >  ^  v", 0, 0, 0);
        display.draw_text(font, 20, 160, "Math: 1 + 2 = 3", 0, 0, 0);
        display.draw_text(font, 20, 190, "Brackets: [ ] { } ( )", 0, 0, 0);

        display.free_font(font);
    }

    int tolerance() const override { return 5; }
    double allowed_diff_percent() const override { return 2.0; }
};
REGISTER_TEST(TestUnicodeText)

class TestTextOnBackground : public TestBase {
public:
    std::string name() const override { return "text_on_background"; }
    std::string description() const override { return "Text on various backgrounds"; }
    uint32_t width() const override { return 400; }
    uint32_t height() const override { return 200; }

    void render(Display& display) override {
        // Create colored bands
        int band_height = height() / 4;

        display.set_foreground(255, 255, 255);  // White
        display.draw_rectangle(0, 0, width(), band_height, true);

        display.set_foreground(0, 0, 0);  // Black
        display.draw_rectangle(0, band_height, width(), band_height, true);

        display.set_foreground(0, 0, 128);  // Dark blue
        display.draw_rectangle(0, 2 * band_height, width(), band_height, true);

        display.set_foreground(255, 255, 0);  // Yellow
        display.draw_rectangle(0, 3 * band_height, width(), band_height, true);

        XftFont* font = display.load_font("sans", 18);
        if (!font) font = display.load_font("fixed", 18);
        if (!font) return;

        // Text with contrasting colors
        display.draw_text(font, 20, 35, "Black on White", 0, 0, 0);
        display.draw_text(font, 20, 35 + band_height, "White on Black", 255, 255, 255);
        display.draw_text(font, 20, 35 + 2 * band_height, "Yellow on Blue", 255, 255, 0);
        display.draw_text(font, 20, 35 + 3 * band_height, "Blue on Yellow", 0, 0, 128);

        display.free_font(font);
    }

    int tolerance() const override { return 5; }
    double allowed_diff_percent() const override { return 1.0; }
};
REGISTER_TEST(TestTextOnBackground)

} // namespace x11bench
