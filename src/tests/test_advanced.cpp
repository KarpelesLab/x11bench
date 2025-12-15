#include "test_base.hpp"
#include <cstring>
#include <cmath>

namespace x11bench {

// =============================================================================
// GC Function (Raster Operation) Tests
// =============================================================================
class TestXorDraw : public TestBase {
public:
    std::string name() const override { return "xor_draw"; }
    std::string description() const override { return "XOR drawing mode (GXxor)"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Draw a red rectangle
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(50, 50, 150, 150, true);

        // Switch to XOR mode and draw overlapping blue rectangle
        display.set_function(GXxor);
        display.set_foreground(0, 0, 255);
        display.draw_rectangle(100, 100, 150, 150, true);

        // Reset to normal copy mode
        display.set_function(GXcopy);
    }
};
REGISTER_TEST(TestXorDraw)

class TestXorLines : public TestBase {
public:
    std::string name() const override { return "xor_lines"; }
    std::string description() const override { return "XOR mode with crossing lines"; }

    void render(Display& display) override {
        // Gray background
        display.set_foreground(128, 128, 128);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Draw grid in XOR mode - intersections should show original color
        display.set_function(GXxor);
        display.set_foreground(255, 255, 255);

        // Horizontal lines
        for (int y = 20; y < (int)height(); y += 20) {
            display.draw_line(0, y, width() - 1, y);
        }

        // Vertical lines
        for (int x = 20; x < (int)width(); x += 20) {
            display.draw_line(x, 0, x, height() - 1);
        }

        display.set_function(GXcopy);
    }
};
REGISTER_TEST(TestXorLines)

class TestGcFunctions : public TestBase {
public:
    std::string name() const override { return "gc_functions"; }
    std::string description() const override { return "Various GC raster operations"; }

    void render(Display& display) override {
        // Create a pattern: 4 rows showing different operations
        int row_height = height() / 4;

        // Row 1: GXcopy (normal)
        display.set_function(GXcopy);
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(0, 0, width(), row_height, true);
        display.set_foreground(0, 0, 255);
        display.draw_rectangle(width() / 4, 0, width() / 2, row_height, true);

        // Row 2: GXxor
        display.set_function(GXcopy);
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(0, row_height, width(), row_height, true);
        display.set_function(GXxor);
        display.set_foreground(0, 0, 255);
        display.draw_rectangle(width() / 4, row_height, width() / 2, row_height, true);

        // Row 3: GXand
        display.set_function(GXcopy);
        display.set_foreground(255, 255, 0);
        display.draw_rectangle(0, 2 * row_height, width(), row_height, true);
        display.set_function(GXand);
        display.set_foreground(255, 0, 255);
        display.draw_rectangle(width() / 4, 2 * row_height, width() / 2, row_height, true);

        // Row 4: GXor
        display.set_function(GXcopy);
        display.set_foreground(0, 128, 0);
        display.draw_rectangle(0, 3 * row_height, width(), row_height, true);
        display.set_function(GXor);
        display.set_foreground(128, 0, 128);
        display.draw_rectangle(width() / 4, 3 * row_height, width() / 2, row_height, true);

        display.set_function(GXcopy);
    }
};
REGISTER_TEST(TestGcFunctions)

class TestGcInvert : public TestBase {
public:
    std::string name() const override { return "gc_invert"; }
    std::string description() const override { return "GXinvert raster operation"; }

    void render(Display& display) override {
        // Create gradient background
        for (uint32_t x = 0; x < width(); x++) {
            uint8_t gray = (x * 255) / (width() - 1);
            display.set_foreground(gray, gray, gray);
            display.draw_line(x, 0, x, height() - 1);
        }

        // Invert center rectangle
        display.set_function(GXinvert);
        display.draw_rectangle(width() / 4, height() / 4,
                               width() / 2, height() / 2, true);

        display.set_function(GXcopy);
    }
};
REGISTER_TEST(TestGcInvert)

// =============================================================================
// Line Style Tests
// =============================================================================
class TestDashedLines : public TestBase {
public:
    std::string name() const override { return "dashed_lines"; }
    std::string description() const override { return "Dashed line styles"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        display.set_foreground(0, 0, 0);

        // LineSolid (default)
        display.set_line_attributes(2, LineSolid, CapButt, JoinMiter);
        display.draw_line(20, 30, width() - 20, 30);

        // LineOnOffDash
        display.set_line_attributes(2, LineOnOffDash, CapButt, JoinMiter);
        const char dash1[] = {10, 10};
        display.set_dashes(0, dash1, 2);
        display.draw_line(20, 60, width() - 20, 60);

        // LineOnOffDash with different pattern
        const char dash2[] = {20, 5, 5, 5};
        display.set_dashes(0, dash2, 4);
        display.draw_line(20, 90, width() - 20, 90);

        // LineDoubleDash
        display.set_line_attributes(2, LineDoubleDash, CapButt, JoinMiter);
        display.set_foreground(255, 0, 0);
        display.set_background(0, 0, 255);
        const char dash3[] = {15, 15};
        display.set_dashes(0, dash3, 2);
        display.draw_line(20, 120, width() - 20, 120);

        // Dotted line
        display.set_foreground(0, 128, 0);
        display.set_line_attributes(1, LineOnOffDash, CapRound, JoinRound);
        const char dots[] = {1, 4};
        display.set_dashes(0, dots, 2);
        display.draw_line(20, 150, width() - 20, 150);

        // Reset
        display.set_line_attributes(0, LineSolid, CapButt, JoinMiter);
    }
};
REGISTER_TEST(TestDashedLines)

class TestLineCapStyles : public TestBase {
public:
    std::string name() const override { return "line_cap_styles"; }
    std::string description() const override { return "Line cap styles (Butt, Round, Projecting)"; }

    void render(Display& display) override {
        // Gray background
        display.set_foreground(200, 200, 200);
        display.draw_rectangle(0, 0, width(), height(), true);

        display.set_foreground(0, 0, 128);

        // CapButt - lines end exactly at endpoint
        display.set_line_attributes(20, LineSolid, CapButt, JoinMiter);
        display.draw_line(50, 50, width() - 50, 50);

        // CapRound - lines have rounded ends
        display.set_line_attributes(20, LineSolid, CapRound, JoinMiter);
        display.draw_line(50, 100, width() - 50, 100);

        // CapProjecting - lines extend beyond endpoint by half line width
        display.set_line_attributes(20, LineSolid, CapProjecting, JoinMiter);
        display.draw_line(50, 150, width() - 50, 150);

        // Draw reference lines to show cap differences
        display.set_foreground(255, 0, 0);
        display.set_line_attributes(1, LineSolid, CapButt, JoinMiter);
        display.draw_line(50, 30, 50, 170);
        display.draw_line(width() - 50, 30, width() - 50, 170);

        // Reset
        display.set_line_attributes(0, LineSolid, CapButt, JoinMiter);
    }
};
REGISTER_TEST(TestLineCapStyles)

class TestLineJoinStyles : public TestBase {
public:
    std::string name() const override { return "line_join_styles"; }
    std::string description() const override { return "Line join styles (Miter, Round, Bevel)"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        display.set_foreground(0, 0, 200);

        // JoinMiter - sharp corners
        display.set_line_attributes(15, LineSolid, CapButt, JoinMiter);
        XPoint miter[] = {{30, 80}, {80, 30}, {130, 80}};
        XDrawLines(display.x_display(), display.x_window(), display.gc(),
                   miter, 3, CoordModeOrigin);

        // JoinRound - rounded corners
        display.set_line_attributes(15, LineSolid, CapButt, JoinRound);
        XPoint round[] = {{160, 80}, {210, 30}, {260, 80}};
        XDrawLines(display.x_display(), display.x_window(), display.gc(),
                   round, 3, CoordModeOrigin);

        // JoinBevel - flat corners
        display.set_line_attributes(15, LineSolid, CapButt, JoinBevel);
        XPoint bevel[] = {{30, 180}, {80, 130}, {130, 180}};
        XDrawLines(display.x_display(), display.x_window(), display.gc(),
                   bevel, 3, CoordModeOrigin);

        // Reset
        display.set_line_attributes(0, LineSolid, CapButt, JoinMiter);
    }
};
REGISTER_TEST(TestLineJoinStyles)

class TestLineWidths : public TestBase {
public:
    std::string name() const override { return "line_widths"; }
    std::string description() const override { return "Various line widths"; }

    void render(Display& display) override {
        display.set_foreground(240, 240, 240);
        display.draw_rectangle(0, 0, width(), height(), true);

        display.set_foreground(0, 0, 0);

        int widths[] = {0, 1, 2, 3, 5, 8, 12, 20};
        int y = 20;

        for (int w : widths) {
            display.set_line_attributes(w, LineSolid, CapRound, JoinRound);
            display.draw_line(30, y, width() - 30, y);
            y += 30;
        }

        display.set_line_attributes(0, LineSolid, CapButt, JoinMiter);
    }
};
REGISTER_TEST(TestLineWidths)

// =============================================================================
// Stipple Pattern Tests
// =============================================================================
class TestStippleFill : public TestBase {
public:
    std::string name() const override { return "stipple_fill"; }
    std::string description() const override { return "Stippled fill pattern"; }

    void render(Display& display) override {
        // White background
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Create a simple stipple pattern (8x8 checkerboard)
        static const unsigned char stipple_data[] = {
            0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA
        };

        Pixmap stipple = XCreateBitmapFromData(display.x_display(),
                                                display.x_window(),
                                                reinterpret_cast<const char*>(stipple_data), 8, 8);

        display.set_foreground(0, 0, 255);
        display.set_stipple(stipple);
        display.set_fill_style(FillStippled);
        display.draw_rectangle(20, 20, width() / 2 - 30, height() - 40, true);

        // OpaqueStippled with background color
        display.set_foreground(255, 0, 0);
        display.set_background(255, 255, 0);
        display.set_fill_style(FillOpaqueStippled);
        display.draw_rectangle(width() / 2 + 10, 20, width() / 2 - 30, height() - 40, true);

        // Reset
        display.set_fill_style(FillSolid);
        display.free_pixmap(stipple);
    }
};
REGISTER_TEST(TestStippleFill)

class TestStipplePatterns : public TestBase {
public:
    std::string name() const override { return "stipple_patterns"; }
    std::string description() const override { return "Various stipple patterns"; }

    void render(Display& display) override {
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Diagonal lines pattern
        static const unsigned char diag_data[] = {
            0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
        };

        // Horizontal lines pattern
        static const unsigned char horiz_data[] = {
            0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00
        };

        // Vertical lines pattern
        static const unsigned char vert_data[] = {
            0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
        };

        // Dots pattern
        static const unsigned char dots_data[] = {
            0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00, 0x88
        };

        int cell_w = width() / 2;
        int cell_h = height() / 2;

        struct PatternInfo {
            const unsigned char* data;
            int x, y;
            uint8_t r, g, b;
        };

        PatternInfo patterns[] = {
            {diag_data, 0, 0, 255, 0, 0},
            {horiz_data, cell_w, 0, 0, 255, 0},
            {vert_data, 0, cell_h, 0, 0, 255},
            {dots_data, cell_w, cell_h, 128, 0, 128},
        };

        for (const auto& p : patterns) {
            Pixmap stipple = XCreateBitmapFromData(display.x_display(),
                                                    display.x_window(),
                                                    reinterpret_cast<const char*>(p.data), 8, 8);
            display.set_foreground(p.r, p.g, p.b);
            display.set_stipple(stipple);
            display.set_fill_style(FillStippled);
            display.draw_rectangle(p.x + 5, p.y + 5, cell_w - 10, cell_h - 10, true);
            display.free_pixmap(stipple);
        }

        display.set_fill_style(FillSolid);
    }
};
REGISTER_TEST(TestStipplePatterns)

// =============================================================================
// Tile Pattern Tests
// =============================================================================
class TestTileFill : public TestBase {
public:
    std::string name() const override { return "tile_fill"; }
    std::string description() const override { return "Tiled fill pattern"; }

    void render(Display& display) override {
        // Create an 8x8 tile with a gradient pattern
        Pixmap tile = display.create_pixmap(8, 8, display.depth());
        GC tile_gc = display.create_gc_for_pixmap(tile);

        // Draw pattern into tile
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                uint8_t val = ((x + y) * 32) & 0xFF;
                XSetForeground(display.x_display(), tile_gc,
                               display.alloc_color(val, 255 - val, 128));
                XDrawPoint(display.x_display(), tile, tile_gc, x, y);
            }
        }

        // Use tile to fill window
        display.set_tile(tile);
        display.set_fill_style(FillTiled);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Draw a solid rectangle in center to show contrast
        display.set_fill_style(FillSolid);
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(width() / 4, height() / 4,
                               width() / 2, height() / 2, true);

        display.free_gc(tile_gc);
        display.free_pixmap(tile);
    }
};
REGISTER_TEST(TestTileFill)

// =============================================================================
// Clip Mask Tests
// =============================================================================
class TestClipMask : public TestBase {
public:
    std::string name() const override { return "clip_mask"; }
    std::string description() const override { return "Clip mask limiting drawing area"; }

    void render(Display& display) override {
        // Create circular clip mask
        Pixmap mask = display.create_bitmap(width(), height());
        GC mask_gc = display.create_gc_for_pixmap(mask);

        // Clear mask to 0 (transparent)
        XSetForeground(display.x_display(), mask_gc, 0);
        XFillRectangle(display.x_display(), mask, mask_gc, 0, 0, width(), height());

        // Draw circle in mask (1 = visible)
        XSetForeground(display.x_display(), mask_gc, 1);
        int radius = std::min(width(), height()) / 2 - 20;
        XFillArc(display.x_display(), mask, mask_gc,
                 width() / 2 - radius, height() / 2 - radius,
                 radius * 2, radius * 2, 0, 360 * 64);

        // Apply clip mask
        display.set_clip_mask(mask);

        // Draw gradient - only visible within circle
        for (uint32_t y = 0; y < height(); y++) {
            uint8_t r = (y * 255) / height();
            uint8_t b = 255 - r;
            display.set_foreground(r, 128, b);
            display.draw_line(0, y, width() - 1, y);
        }

        // Clear clip mask
        display.set_clip_mask(None);

        // Draw border around where circle should be
        display.set_foreground(0, 0, 0);
        display.draw_arc(width() / 2 - radius, height() / 2 - radius,
                         radius * 2, radius * 2, 0, 360 * 64);

        display.free_gc(mask_gc);
        display.free_pixmap(mask);
    }
};
REGISTER_TEST(TestClipMask)

class TestClipRectangles : public TestBase {
public:
    std::string name() const override { return "clip_rectangles"; }
    std::string description() const override { return "Clip rectangles limiting drawing"; }

    void render(Display& display) override {
        // Define clip regions
        XRectangle clips[] = {
            {20, 20, 80, 80},
            {120, 20, 80, 80},
            {20, 120, 80, 80},
            {120, 120, 80, 80},
        };

        display.set_clip_rectangles(0, 0, clips, 4, Unsorted);

        // Draw diagonal gradient - only visible in clip regions
        for (int i = 0; i < (int)(width() + height()); i += 2) {
            uint8_t c = (i * 255) / (width() + height());
            display.set_foreground(c, c, 255 - c);
            display.draw_line(i, 0, 0, i);
        }

        // Clear clip
        display.set_clip_mask(None);

        // Draw outlines of clip regions
        display.set_foreground(255, 0, 0);
        for (const auto& r : clips) {
            display.draw_rectangle(r.x, r.y, r.width, r.height, false);
        }
    }
};
REGISTER_TEST(TestClipRectangles)

// =============================================================================
// Plane Mask Tests
// =============================================================================
class TestPlaneMask : public TestBase {
public:
    std::string name() const override { return "plane_mask"; }
    std::string description() const override { return "Plane mask affecting color channels"; }

    void render(Display& display) override {
        // Draw full color gradient
        for (uint32_t x = 0; x < width(); x++) {
            uint8_t c = (x * 255) / (width() - 1);
            display.set_foreground(c, c, c);
            display.draw_line(x, 0, x, height() / 3);
        }

        // Red channel only (mask out green and blue)
        display.set_plane_mask(0xFF0000);
        for (uint32_t x = 0; x < width(); x++) {
            display.set_foreground(255, 255, 255);
            display.draw_line(x, height() / 3, x, 2 * height() / 3);
        }

        // Blue channel only
        display.set_plane_mask(0x0000FF);
        for (uint32_t x = 0; x < width(); x++) {
            display.set_foreground(255, 255, 255);
            display.draw_line(x, 2 * height() / 3, x, height());
        }

        // Reset plane mask
        display.set_plane_mask(AllPlanes);
    }
};
REGISTER_TEST(TestPlaneMask)

// =============================================================================
// Fill Rule Tests
// =============================================================================
class TestFillRuleEvenOdd : public TestBase {
public:
    std::string name() const override { return "fill_rule_evenodd"; }
    std::string description() const override { return "EvenOdd fill rule for polygons"; }

    void render(Display& display) override {
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Create a star polygon (self-intersecting)
        XPoint star[10];
        int cx = width() / 2;
        int cy = height() / 2;
        int outer_r = std::min(width(), height()) / 2 - 20;
        int inner_r = outer_r / 2;

        for (int i = 0; i < 10; i++) {
            double angle = (i * 36 - 90) * 3.14159265 / 180.0;
            int r = (i % 2 == 0) ? outer_r : inner_r;
            star[i].x = cx + (int)(r * cos(angle));
            star[i].y = cy + (int)(r * sin(angle));
        }

        display.set_foreground(0, 0, 200);
        display.set_fill_rule(EvenOddRule);
        display.fill_polygon(star, 10, Complex, CoordModeOrigin);
    }

    int tolerance() const override { return 1; }
};
REGISTER_TEST(TestFillRuleEvenOdd)

class TestFillRuleWinding : public TestBase {
public:
    std::string name() const override { return "fill_rule_winding"; }
    std::string description() const override { return "Winding fill rule for polygons"; }

    void render(Display& display) override {
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Same star polygon but with Winding rule
        XPoint star[10];
        int cx = width() / 2;
        int cy = height() / 2;
        int outer_r = std::min(width(), height()) / 2 - 20;
        int inner_r = outer_r / 2;

        for (int i = 0; i < 10; i++) {
            double angle = (i * 36 - 90) * 3.14159265 / 180.0;
            int r = (i % 2 == 0) ? outer_r : inner_r;
            star[i].x = cx + (int)(r * cos(angle));
            star[i].y = cy + (int)(r * sin(angle));
        }

        display.set_foreground(200, 0, 0);
        display.set_fill_rule(WindingRule);
        display.fill_polygon(star, 10, Complex, CoordModeOrigin);
    }

    int tolerance() const override { return 1; }
};
REGISTER_TEST(TestFillRuleWinding)

// =============================================================================
// Arc Drawing Tests
// =============================================================================
class TestArcStyles : public TestBase {
public:
    std::string name() const override { return "arc_styles"; }
    std::string description() const override { return "Arc drawing with various angles"; }

    void render(Display& display) override {
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        display.set_foreground(0, 0, 0);

        // Full circle
        display.draw_arc(20, 20, 60, 60, 0, 360 * 64);

        // Half circle (top)
        display.draw_arc(100, 20, 60, 60, 0, 180 * 64);

        // Quarter circle
        display.draw_arc(180, 20, 60, 60, 0, 90 * 64);

        // Arc from 45 to 135 degrees
        display.draw_arc(20, 100, 60, 60, 45 * 64, 90 * 64);

        // Filled arc (pie slice)
        display.set_foreground(255, 0, 0);
        XFillArc(display.x_display(), display.x_window(), display.gc(),
                 100, 100, 60, 60, 0, 270 * 64);

        // Elliptical arc
        display.set_foreground(0, 128, 0);
        display.draw_arc(20, 180, 100, 50, 0, 360 * 64);
    }
};
REGISTER_TEST(TestArcStyles)

// =============================================================================
// CopyArea Test
// =============================================================================
class TestCopyArea : public TestBase {
public:
    std::string name() const override { return "copy_area"; }
    std::string description() const override { return "XCopyArea operation"; }

    void render(Display& display) override {
        // Draw a pattern in top-left
        display.set_foreground(255, 255, 255);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Draw source pattern
        for (int y = 0; y < 100; y += 10) {
            for (int x = 0; x < 100; x += 10) {
                uint8_t r = (x * 255) / 100;
                uint8_t g = (y * 255) / 100;
                display.set_foreground(r, g, 128);
                display.draw_rectangle(x, y, 10, 10, true);
            }
        }

        // Copy to other locations
        XCopyArea(display.x_display(), display.x_window(), display.x_window(),
                  display.gc(), 0, 0, 100, 100, 120, 0);
        XCopyArea(display.x_display(), display.x_window(), display.x_window(),
                  display.gc(), 0, 0, 100, 100, 0, 120);
        XCopyArea(display.x_display(), display.x_window(), display.x_window(),
                  display.gc(), 0, 0, 100, 100, 120, 120);
    }
};
REGISTER_TEST(TestCopyArea)

// =============================================================================
// Points Drawing Test
// =============================================================================
class TestDrawPoints : public TestBase {
public:
    std::string name() const override { return "draw_points"; }
    std::string description() const override { return "Individual point drawing"; }

    void render(Display& display) override {
        display.set_foreground(0, 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Draw points in various patterns
        for (uint32_t y = 0; y < height(); y += 4) {
            for (uint32_t x = 0; x < width(); x += 4) {
                uint8_t r = (x * 255) / width();
                uint8_t g = (y * 255) / height();
                uint8_t b = 255 - (r + g) / 2;
                display.set_foreground(r, g, b);
                XDrawPoint(display.x_display(), display.x_window(),
                           display.gc(), x, y);
            }
        }
    }
};
REGISTER_TEST(TestDrawPoints)

// =============================================================================
// Subwindow Mode Test
// =============================================================================
class TestSubwindowMode : public TestBase {
public:
    std::string name() const override { return "subwindow_mode"; }
    std::string description() const override { return "Subwindow drawing mode"; }

    void render(Display& display) override {
        // This test verifies subwindow mode setting works
        // Without actual subwindows, we just verify the mode can be set

        display.set_foreground(200, 200, 200);
        display.draw_rectangle(0, 0, width(), height(), true);

        // ClipByChildren mode (default)
        display.set_subwindow_mode(ClipByChildren);
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(20, 20, 100, 100, true);

        // IncludeInferiors mode
        display.set_subwindow_mode(IncludeInferiors);
        display.set_foreground(0, 0, 255);
        display.draw_rectangle(140, 20, 100, 100, true);

        // Reset
        display.set_subwindow_mode(ClipByChildren);
    }
};
REGISTER_TEST(TestSubwindowMode)

// =============================================================================
// GXnoop and other rare functions
// =============================================================================
class TestGcNoop : public TestBase {
public:
    std::string name() const override { return "gc_noop"; }
    std::string description() const override { return "GXnoop - drawing with no effect"; }

    void render(Display& display) override {
        // Draw background pattern
        display.set_foreground(0, 100, 200);
        display.draw_rectangle(0, 0, width(), height(), true);

        // Draw visible rectangle
        display.set_foreground(255, 255, 0);
        display.draw_rectangle(50, 50, 100, 100, true);

        // Try to draw in GXnoop mode - should have no effect
        display.set_function(GXnoop);
        display.set_foreground(255, 0, 0);
        display.draw_rectangle(80, 80, 100, 100, true);

        // Draw another visible rectangle to show noop worked
        display.set_function(GXcopy);
        display.set_foreground(0, 255, 0);
        display.draw_rectangle(width() - 100, height() - 100, 80, 80, true);
    }
};
REGISTER_TEST(TestGcNoop)

class TestGcSet : public TestBase {
public:
    std::string name() const override { return "gc_set"; }
    std::string description() const override { return "GXset - sets all pixels to 1"; }

    void render(Display& display) override {
        // Dark background
        display.set_foreground(50, 50, 100);
        display.draw_rectangle(0, 0, width(), height(), true);

        // GXset should set pixels to all 1s (white on most displays)
        display.set_function(GXset);
        display.draw_rectangle(50, 50, width() - 100, height() - 100, true);

        display.set_function(GXcopy);
    }
};
REGISTER_TEST(TestGcSet)

class TestGcClear : public TestBase {
public:
    std::string name() const override { return "gc_clear"; }
    std::string description() const override { return "GXclear - sets all pixels to 0"; }

    void render(Display& display) override {
        // Light background
        display.set_foreground(200, 200, 200);
        display.draw_rectangle(0, 0, width(), height(), true);

        // GXclear should set pixels to all 0s (black on most displays)
        display.set_function(GXclear);
        display.draw_rectangle(50, 50, width() - 100, height() - 100, true);

        display.set_function(GXcopy);
    }
};
REGISTER_TEST(TestGcClear)

} // namespace x11bench
