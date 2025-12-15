#include "test_base.hpp"
#include "../capture.hpp"
#include <thread>
#include <chrono>
#include <stdexcept>

namespace x11bench {

// =============================================================================
// Window Pattern System
// =============================================================================
// Each window gets a unique 4x4 pixel marker in its top-left corner.
// The marker consists of a specific RGB color that identifies the window.
// By scanning a screen capture for these markers, we can determine which
// windows are visible and their approximate positions.

struct WindowMarker {
    uint8_t r, g, b;
    const char* name;
};

// Predefined markers - distinct colors that are easy to detect
static const WindowMarker MARKERS[] = {
    {255, 0, 0, "RED"},       // Window 0 - Pure Red
    {0, 255, 0, "GREEN"},     // Window 1 - Pure Green
    {0, 0, 255, "BLUE"},      // Window 2 - Pure Blue
    {255, 255, 0, "YELLOW"},  // Window 3 - Yellow
    {255, 0, 255, "MAGENTA"}, // Window 4 - Magenta
    {0, 255, 255, "CYAN"},    // Window 5 - Cyan
    {255, 128, 0, "ORANGE"},  // Window 6 - Orange
    {128, 0, 255, "PURPLE"},  // Window 7 - Purple
};

static const int MARKER_SIZE = 16;  // Size of the identification marker
static const int MARKER_BORDER = 2; // White border around marker for detection

// Corner positions for markers
enum MarkerCorner { TOP_LEFT = 0, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };

// Draw a window's identifying pattern at a specific corner
static void draw_window_pattern(Display& display, ::Window win, GC gc,
                                 int marker_id, int win_width, int win_height,
                                 MarkerCorner corner = TOP_LEFT) {
    const WindowMarker& marker = MARKERS[marker_id % 8];

    // Fill window with a lighter version of the marker color as background
    display.draw_rectangle_on(win, gc, 0, 0, win_width, win_height, true,
                              marker.r / 2 + 64, marker.g / 2 + 64, marker.b / 2 + 64);

    // Calculate marker position based on corner
    int marker_x = 0, marker_y = 0;
    int total_marker_size = MARKER_SIZE + MARKER_BORDER * 2;

    switch (corner) {
        case TOP_LEFT:
            marker_x = 0;
            marker_y = 0;
            break;
        case TOP_RIGHT:
            marker_x = win_width - total_marker_size;
            marker_y = 0;
            break;
        case BOTTOM_LEFT:
            marker_x = 0;
            marker_y = win_height - total_marker_size;
            break;
        case BOTTOM_RIGHT:
            marker_x = win_width - total_marker_size;
            marker_y = win_height - total_marker_size;
            break;
    }

    // Draw white border for marker detection
    display.draw_rectangle_on(win, gc, marker_x, marker_y,
                              total_marker_size, total_marker_size,
                              true, 255, 255, 255);

    // Draw the colored marker
    display.draw_rectangle_on(win, gc, marker_x + MARKER_BORDER, marker_y + MARKER_BORDER,
                              MARKER_SIZE, MARKER_SIZE, true,
                              marker.r, marker.g, marker.b);
}

// Structure to track window patterns for redraw
struct WindowPattern {
    ::Window win;
    GC gc;
    int marker_id;
    int width;
    int height;
    MarkerCorner corner = TOP_LEFT;
};

// Helper to wait for window operations and handle Expose events
static void settle_and_redraw(Display& display,
                               const std::vector<WindowPattern>& windows,
                               int ms = 150) {
    display.flush();
    display.sync(false);

    auto start = std::chrono::steady_clock::now();

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= ms) break;

        // Check for pending events
        while (XPending(display.x_display()) > 0) {
            XEvent event;
            XNextEvent(display.x_display(), &event);

            if (event.type == Expose && event.xexpose.count == 0) {
                // Find the window that needs redraw
                for (const auto& wp : windows) {
                    if (event.xexpose.window == wp.win) {
                        draw_window_pattern(display, wp.win, wp.gc,
                                          wp.marker_id, wp.width, wp.height, wp.corner);
                        display.flush();
                        break;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    display.sync(false);
}

// Simple settle without redraw handling
static void settle(Display& display, int ms = 150) {
    display.flush();
    display.sync(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    display.sync(false);
    display.process_pending_events();
}

// Convert XImage to Image
static Image ximage_to_image(XImage* ximg) {
    if (!ximg) return Image();

    uint32_t width = ximg->width;
    uint32_t height = ximg->height;
    Image img(width, height);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(ximg, x, y);
            uint8_t r, g, b;

            if (ximg->bits_per_pixel == 32) {
                if (ximg->byte_order == LSBFirst) {
                    b = (pixel >> 0) & 0xFF;
                    g = (pixel >> 8) & 0xFF;
                    r = (pixel >> 16) & 0xFF;
                } else {
                    r = (pixel >> 16) & 0xFF;
                    g = (pixel >> 8) & 0xFF;
                    b = (pixel >> 0) & 0xFF;
                }
            } else {
                r = g = b = pixel & 0xFF;
            }
            img.set_pixel(x, y, r, g, b, 255);
        }
    }
    return img;
}

// Scan image to find if a marker is visible anywhere
static bool find_marker_in_image(const Image& img, int marker_id, int* out_x = nullptr, int* out_y = nullptr) {
    const WindowMarker& marker = MARKERS[marker_id % 8];
    int tolerance = 40;  // Increased tolerance

    // Scan the image looking for the marker color directly
    // Instead of looking for white border first, scan for the marker color itself
    for (uint32_t y = MARKER_BORDER; y < img.height() - MARKER_SIZE - MARKER_BORDER; y += 2) {
        for (uint32_t x = MARKER_BORDER; x < img.width() - MARKER_SIZE - MARKER_BORDER; x += 2) {
            // Check center of potential marker area directly
            Pixel p = img.get_pixel(x + MARKER_SIZE/2, y + MARKER_SIZE/2);

            // Quick check if this could be our marker color
            if (std::abs(p.r - marker.r) <= tolerance &&
                std::abs(p.g - marker.g) <= tolerance &&
                std::abs(p.b - marker.b) <= tolerance) {

                // Verify with multi-pixel sampling
                int matches = 0;
                int samples = 0;
                for (int dy = 0; dy < MARKER_SIZE; dy += 4) {
                    for (int dx = 0; dx < MARKER_SIZE; dx += 4) {
                        Pixel sp = img.get_pixel(x + dx, y + dy);
                        if (std::abs(sp.r - marker.r) <= tolerance &&
                            std::abs(sp.g - marker.g) <= tolerance &&
                            std::abs(sp.b - marker.b) <= tolerance) {
                            matches++;
                        }
                        samples++;
                    }
                }

                // Require most samples to match
                if (samples > 0 && matches >= samples * 2 / 3) {
                    if (out_x) *out_x = x;
                    if (out_y) *out_y = y;
                    return true;
                }
            }
        }
    }
    return false;
}

// =============================================================================
// Self-Verifying Window Test Base
// =============================================================================
class WindowTestBase : public TestBase {
public:
    // Window tests don't use reference images - they self-verify
    bool captures_screen() const override { return true; }

    // Window tests verify programmatically, not via image comparison
    // Return a special tolerance that signals self-verification
    int tolerance() const override { return -1; }  // Special value

    // The render function should set test_passed_ based on verification
    bool test_passed() const override { return test_passed_; }
    std::string failure_reason() const override { return failure_reason_; }

protected:
    mutable bool test_passed_ = false;
    mutable std::string failure_reason_;

    // Capture the full screen and return as Image
    Image capture_screen(Display& display) const {
        XImage* ximg = display.capture_root_region(0, 0,
            display.screen_width(), display.screen_height());
        if (!ximg) {
            return Image();
        }
        Image img = ximage_to_image(ximg);
        XDestroyImage(ximg);
        return img;
    }

    // Verify that a marker IS visible
    bool verify_visible(const Image& screen, int marker_id, const char* context) const {
        if (!find_marker_in_image(screen, marker_id)) {
            failure_reason_ = std::string(context) + ": " +
                MARKERS[marker_id % 8].name + " window should be visible but wasn't found";
            return false;
        }
        return true;
    }

    // Verify that a marker is NOT visible
    bool verify_hidden(const Image& screen, int marker_id, const char* context) const {
        if (find_marker_in_image(screen, marker_id)) {
            failure_reason_ = std::string(context) + ": " +
                MARKERS[marker_id % 8].name + " window should be hidden but was found";
            return false;
        }
        return true;
    }
};

// =============================================================================
// Window Stacking Tests
// =============================================================================

class TestWinStackBasic : public WindowTestBase {
public:
    std::string name() const override { return "win_stack_basic"; }
    std::string description() const override { return "Basic window stacking - later window on top"; }

    void render(Display& display) override {
        test_passed_ = false;

        // Create two overlapping windows
        ::Window win1 = display.create_child_window(200, 200, 100, 100, "Win1-Red");
        ::Window win2 = display.create_child_window(200, 200, 150, 150, "Win2-Green");

        GC gc1 = display.create_gc_for_window(win1);
        GC gc2 = display.create_gc_for_window(win2);

        std::vector<WindowPattern> windows = {
            {win1, gc1, 0, 200, 200},
            {win2, gc2, 1, 200, 200}
        };

        // Map win1 first, then win2 - win2 should be on top
        display.show_child_window(win1);
        settle_and_redraw(display, windows);
        draw_window_pattern(display, win1, gc1, 0, 200, 200);  // RED

        display.show_child_window(win2);
        settle_and_redraw(display, windows);
        draw_window_pattern(display, win2, gc2, 1, 200, 200);  // GREEN

        settle_and_redraw(display, windows, 200);

        // Capture screen
        Image screen = capture_screen(display);

        // Verify: GREEN (win2) should be visible
        bool green_visible = find_marker_in_image(screen, 1);  // GREEN

        if (!green_visible) {
            failure_reason_ = "GREEN window (win2) not visible - should be on top";
        } else {
            test_passed_ = true;
        }

        // Cleanup
        display.free_gc(gc1);
        display.free_gc(gc2);
        display.destroy_child_window(win1);
        display.destroy_child_window(win2);

        // Draw result indicator in main window
        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinStackBasic)

class TestWinRaise : public WindowTestBase {
public:
    std::string name() const override { return "win_raise"; }
    std::string description() const override { return "XRaiseWindow brings window to front"; }

    void render(Display& display) override {
        test_passed_ = false;

        // Create two overlapping windows at same position
        ::Window win1 = display.create_child_window(200, 200, 100, 100, "Win1-Red");
        ::Window win2 = display.create_child_window(200, 200, 100, 100, "Win2-Green");

        GC gc1 = display.create_gc_for_window(win1);
        GC gc2 = display.create_gc_for_window(win2);

        // Track windows for Expose event handling
        std::vector<WindowPattern> windows = {
            {win1, gc1, 0, 200, 200},  // RED
            {win2, gc2, 1, 200, 200}   // GREEN
        };

        // Show both - win2 on top
        display.show_child_window(win1);
        display.show_child_window(win2);
        settle_and_redraw(display, windows);

        draw_window_pattern(display, win1, gc1, 0, 200, 200);  // RED
        draw_window_pattern(display, win2, gc2, 1, 200, 200);  // GREEN
        settle_and_redraw(display, windows, 200);

        // Verify GREEN is visible (on top)
        Image screen1 = capture_screen(display);
        if (!find_marker_in_image(screen1, 1)) {
            failure_reason_ = "Initial: GREEN should be visible";
            goto cleanup;
        }

        // Raise win1 (RED) to top
        display.raise_child_window(win1);
        settle_and_redraw(display, windows, 250);

        // Verify RED is now visible
        {
            Image screen2 = capture_screen(display);
            if (!find_marker_in_image(screen2, 0)) {
                failure_reason_ = "After raise: RED should be visible";
                goto cleanup;
            }
            test_passed_ = true;
        }

    cleanup:
        display.free_gc(gc1);
        display.free_gc(gc2);
        display.destroy_child_window(win1);
        display.destroy_child_window(win2);

        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinRaise)

class TestWinLower : public WindowTestBase {
public:
    std::string name() const override { return "win_lower"; }
    std::string description() const override { return "XLowerWindow sends window to back"; }

    void render(Display& display) override {
        test_passed_ = false;

        ::Window win1 = display.create_child_window(200, 200, 100, 100, "Win1-Red");
        ::Window win2 = display.create_child_window(200, 200, 100, 100, "Win2-Green");

        GC gc1 = display.create_gc_for_window(win1);
        GC gc2 = display.create_gc_for_window(win2);

        std::vector<WindowPattern> windows = {
            {win1, gc1, 0, 200, 200},
            {win2, gc2, 1, 200, 200}
        };

        display.show_child_window(win1);
        display.show_child_window(win2);
        settle_and_redraw(display, windows);

        draw_window_pattern(display, win1, gc1, 0, 200, 200);
        draw_window_pattern(display, win2, gc2, 1, 200, 200);
        settle_and_redraw(display, windows, 200);

        // Lower win2 (GREEN) - RED should become visible
        display.lower_child_window(win2);
        settle_and_redraw(display, windows, 250);

        Image screen = capture_screen(display);
        if (find_marker_in_image(screen, 0)) {
            test_passed_ = true;
        } else {
            failure_reason_ = "After lower: RED window should be visible";
        }

        display.free_gc(gc1);
        display.free_gc(gc2);
        display.destroy_child_window(win1);
        display.destroy_child_window(win2);

        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinLower)

class TestWinHide : public WindowTestBase {
public:
    std::string name() const override { return "win_hide"; }
    std::string description() const override { return "XUnmapWindow hides window"; }

    void render(Display& display) override {
        test_passed_ = false;

        ::Window win1 = display.create_child_window(200, 200, 100, 100, "Win1-Red");
        ::Window win2 = display.create_child_window(200, 200, 100, 100, "Win2-Green");

        GC gc1 = display.create_gc_for_window(win1);
        GC gc2 = display.create_gc_for_window(win2);

        std::vector<WindowPattern> windows = {
            {win1, gc1, 0, 200, 200},
            {win2, gc2, 1, 200, 200}
        };

        display.show_child_window(win1);
        display.show_child_window(win2);
        settle_and_redraw(display, windows);

        draw_window_pattern(display, win1, gc1, 0, 200, 200);
        draw_window_pattern(display, win2, gc2, 1, 200, 200);
        settle_and_redraw(display, windows, 200);

        // Hide win2 - win1 should become visible
        display.hide_child_window(win2);
        settle_and_redraw(display, windows, 250);

        Image screen = capture_screen(display);
        bool red_visible = find_marker_in_image(screen, 0);
        bool green_visible = find_marker_in_image(screen, 1);

        if (red_visible && !green_visible) {
            test_passed_ = true;
        } else {
            failure_reason_ = "After hide: RED should be visible, GREEN should not";
        }

        display.free_gc(gc1);
        display.free_gc(gc2);
        display.destroy_child_window(win1);
        display.destroy_child_window(win2);

        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinHide)

class TestWinShowAfterHide : public WindowTestBase {
public:
    std::string name() const override { return "win_show_after_hide"; }
    std::string description() const override { return "XMapWindow shows hidden window"; }

    void render(Display& display) override {
        test_passed_ = false;

        ::Window win = display.create_child_window(200, 200, 100, 100, "Win-Red");
        GC gc = display.create_gc_for_window(win);

        std::vector<WindowPattern> windows = {{win, gc, 0, 200, 200}};

        display.show_child_window(win);
        settle_and_redraw(display, windows);
        draw_window_pattern(display, win, gc, 0, 200, 200);
        settle_and_redraw(display, windows, 200);

        // Verify visible
        Image screen1 = capture_screen(display);
        if (!find_marker_in_image(screen1, 0)) {
            failure_reason_ = "Initial: window should be visible";
            goto cleanup;
        }

        // Hide
        display.hide_child_window(win);
        settle(display, 200);

        // Verify hidden
        {
            Image screen2 = capture_screen(display);
            if (find_marker_in_image(screen2, 0)) {
                failure_reason_ = "After hide: window should not be visible";
                goto cleanup;
            }
        }

        // Show again
        display.show_child_window(win);
        settle_and_redraw(display, windows, 250);

        // Verify visible again
        {
            Image screen3 = capture_screen(display);
            if (find_marker_in_image(screen3, 0)) {
                test_passed_ = true;
            } else {
                failure_reason_ = "After show: window should be visible again";
            }
        }

    cleanup:
        display.free_gc(gc);
        display.destroy_child_window(win);

        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinShowAfterHide)

class TestWinDestroy : public WindowTestBase {
public:
    std::string name() const override { return "win_destroy"; }
    std::string description() const override { return "XDestroyWindow removes window"; }

    void render(Display& display) override {
        test_passed_ = false;

        ::Window win1 = display.create_child_window(200, 200, 100, 100, "Win1-Red");
        ::Window win2 = display.create_child_window(200, 200, 100, 100, "Win2-Green");

        GC gc1 = display.create_gc_for_window(win1);
        GC gc2 = display.create_gc_for_window(win2);

        std::vector<WindowPattern> windows = {
            {win1, gc1, 0, 200, 200},
            {win2, gc2, 1, 200, 200}
        };

        display.show_child_window(win1);
        display.show_child_window(win2);
        settle_and_redraw(display, windows);

        draw_window_pattern(display, win1, gc1, 0, 200, 200);
        draw_window_pattern(display, win2, gc2, 1, 200, 200);
        settle_and_redraw(display, windows, 200);

        // Destroy win2 - win1 should become visible
        display.free_gc(gc2);
        display.destroy_child_window(win2);
        // Update windows list - only win1 remains
        windows = {{win1, gc1, 0, 200, 200}};
        settle_and_redraw(display, windows, 250);

        Image screen = capture_screen(display);
        bool red_visible = find_marker_in_image(screen, 0);
        bool green_visible = find_marker_in_image(screen, 1);

        if (red_visible && !green_visible) {
            test_passed_ = true;
        } else {
            failure_reason_ = "After destroy: RED visible, GREEN gone";
        }

        display.free_gc(gc1);
        display.destroy_child_window(win1);

        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinDestroy)

class TestWinThreeStack : public WindowTestBase {
public:
    std::string name() const override { return "win_three_stack"; }
    std::string description() const override { return "Three stacked windows with all markers visible"; }

    void render(Display& display) override {
        test_passed_ = false;

        // Three windows at same position but very different sizes
        // Largest at bottom, smallest on top - all markers should be visible
        ::Window win1 = display.create_child_window(300, 300, 50, 50, "Win1");   // Largest (bottom)
        ::Window win2 = display.create_child_window(180, 180, 50, 50, "Win2");   // Medium
        ::Window win3 = display.create_child_window(60, 60, 50, 50, "Win3");     // Smallest (top)

        GC gc1 = display.create_gc_for_window(win1);
        GC gc2 = display.create_gc_for_window(win2);
        GC gc3 = display.create_gc_for_window(win3);

        // Draw markers at different corners so they're all visible
        // RED (largest): bottom-right, GREEN (medium): bottom-left, BLUE (smallest): top-left
        std::vector<WindowPattern> windows = {
            {win1, gc1, 0, 300, 300, BOTTOM_RIGHT},
            {win2, gc2, 1, 180, 180, BOTTOM_LEFT},
            {win3, gc3, 2, 60, 60, TOP_LEFT}
        };

        display.show_child_window(win1);
        display.show_child_window(win2);
        display.show_child_window(win3);
        settle_and_redraw(display, windows);

        draw_window_pattern(display, win1, gc1, 0, 300, 300, BOTTOM_RIGHT);  // RED (largest)
        draw_window_pattern(display, win2, gc2, 1, 180, 180, BOTTOM_LEFT);   // GREEN (medium)
        draw_window_pattern(display, win3, gc3, 2, 60, 60, TOP_LEFT);        // BLUE (smallest)
        settle_and_redraw(display, windows, 250);

        // All three markers should be visible due to size differences
        Image screen = capture_screen(display);
        bool red = find_marker_in_image(screen, 0);
        bool green = find_marker_in_image(screen, 1);
        bool blue = find_marker_in_image(screen, 2);

        if (red && green && blue) {
            test_passed_ = true;
        } else {
            failure_reason_ = "Missing markers:";
            if (!red) failure_reason_ += " RED";
            if (!green) failure_reason_ += " GREEN";
            if (!blue) failure_reason_ += " BLUE";
        }

        display.free_gc(gc1);
        display.free_gc(gc2);
        display.free_gc(gc3);
        display.destroy_child_window(win1);
        display.destroy_child_window(win2);
        display.destroy_child_window(win3);

        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinThreeStack)

class TestWinRestackMiddle : public WindowTestBase {
public:
    std::string name() const override { return "win_restack_middle"; }
    std::string description() const override { return "Raise middle window to top of three"; }

    void render(Display& display) override {
        test_passed_ = false;

        // Three windows at same position - only top one's marker visible
        ::Window win1 = display.create_child_window(200, 200, 100, 100, "Win1");
        ::Window win2 = display.create_child_window(200, 200, 100, 100, "Win2");
        ::Window win3 = display.create_child_window(200, 200, 100, 100, "Win3");

        GC gc1 = display.create_gc_for_window(win1);
        GC gc2 = display.create_gc_for_window(win2);
        GC gc3 = display.create_gc_for_window(win3);

        std::vector<WindowPattern> windows = {
            {win1, gc1, 0, 200, 200},
            {win2, gc2, 1, 200, 200},
            {win3, gc3, 2, 200, 200}
        };

        display.show_child_window(win1);
        display.show_child_window(win2);
        display.show_child_window(win3);
        settle_and_redraw(display, windows);

        draw_window_pattern(display, win1, gc1, 0, 200, 200);  // RED
        draw_window_pattern(display, win2, gc2, 1, 200, 200);  // GREEN
        draw_window_pattern(display, win3, gc3, 2, 200, 200);  // BLUE
        settle_and_redraw(display, windows, 200);

        // Initially BLUE should be on top
        Image screen1 = capture_screen(display);
        if (!find_marker_in_image(screen1, 2)) {
            failure_reason_ = "Initial: BLUE should be visible";
            goto cleanup;
        }

        // Raise RED to top
        display.raise_child_window(win1);
        settle_and_redraw(display, windows, 250);

        {
            Image screen2 = capture_screen(display);
            if (find_marker_in_image(screen2, 0)) {
                test_passed_ = true;
            } else {
                failure_reason_ = "After raise: RED should be visible";
            }
        }

    cleanup:
        display.free_gc(gc1);
        display.free_gc(gc2);
        display.free_gc(gc3);
        display.destroy_child_window(win1);
        display.destroy_child_window(win2);
        display.destroy_child_window(win3);

        display.set_foreground(test_passed_ ? 0 : 255, test_passed_ ? 255 : 0, 0);
        display.draw_rectangle(0, 0, width(), height(), true);
    }
};
REGISTER_TEST(TestWinRestackMiddle)

} // namespace x11bench
