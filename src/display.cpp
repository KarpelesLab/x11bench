#include "display.hpp"
#include <cstring>
#include <stdexcept>
#include <chrono>

namespace x11bench {

Display::Display() = default;

Display::~Display() {
    cleanup();
}

Display::Display(Display&& other) noexcept
    : display_(other.display_), window_(other.window_), screen_(other.screen_),
      visual_(other.visual_), colormap_(other.colormap_), depth_(other.depth_),
      gc_(other.gc_), width_(other.width_), height_(other.height_),
      has_xrender_(other.has_xrender_), picture_(other.picture_),
      pict_format_(other.pict_format_), xft_draw_(other.xft_draw_) {
    other.display_ = nullptr;
    other.window_ = 0;
    other.gc_ = nullptr;
    other.picture_ = 0;
    other.xft_draw_ = nullptr;
}

Display& Display::operator=(Display&& other) noexcept {
    if (this != &other) {
        cleanup();
        display_ = other.display_;
        window_ = other.window_;
        screen_ = other.screen_;
        visual_ = other.visual_;
        colormap_ = other.colormap_;
        depth_ = other.depth_;
        gc_ = other.gc_;
        width_ = other.width_;
        height_ = other.height_;
        has_xrender_ = other.has_xrender_;
        picture_ = other.picture_;
        pict_format_ = other.pict_format_;
        xft_draw_ = other.xft_draw_;

        other.display_ = nullptr;
        other.window_ = 0;
        other.gc_ = nullptr;
        other.picture_ = 0;
        other.xft_draw_ = nullptr;
    }
    return *this;
}

void Display::cleanup() {
    if (xft_draw_) {
        XftDrawDestroy(xft_draw_);
        xft_draw_ = nullptr;
    }
    if (picture_ && display_) {
        XRenderFreePicture(display_, picture_);
        picture_ = 0;
    }
    destroy_window();
    disconnect();
}

bool Display::connect(const std::string& display_name) {
    if (display_) {
        return true;  // Already connected
    }

    const char* name = display_name.empty() ? nullptr : display_name.c_str();
    display_ = XOpenDisplay(name);
    if (!display_) {
        return false;
    }

    screen_ = DefaultScreen(display_);
    visual_ = DefaultVisual(display_, screen_);
    depth_ = DefaultDepth(display_, screen_);
    colormap_ = DefaultColormap(display_, screen_);

    // Check for XRender
    int event_base, error_base;
    has_xrender_ = XRenderQueryExtension(display_, &event_base, &error_base);

    return true;
}

void Display::disconnect() {
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
    }
}

bool Display::create_window(uint32_t width, uint32_t height, const std::string& title) {
    if (!display_) {
        return false;
    }
    if (window_) {
        return true;  // Already have a window
    }

    width_ = width;
    height_ = height;

    // Create window with simple attributes
    XSetWindowAttributes attrs;
    attrs.background_pixel = WhitePixel(display_, screen_);
    attrs.border_pixel = BlackPixel(display_, screen_);
    attrs.event_mask = ExposureMask | StructureNotifyMask;
    attrs.colormap = colormap_;

    window_ = XCreateWindow(
        display_,
        RootWindow(display_, screen_),
        0, 0,
        width, height,
        0,
        depth_,
        InputOutput,
        visual_,
        CWBackPixel | CWBorderPixel | CWEventMask | CWColormap,
        &attrs
    );

    if (!window_) {
        return false;
    }

    // Set window title
    XStoreName(display_, window_, title.c_str());

    // Create graphics context
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    if (!gc_) {
        XDestroyWindow(display_, window_);
        window_ = 0;
        return false;
    }

    // Initialize GC with known defaults to avoid server-defined surprises
    XSetForeground(display_, gc_, BlackPixel(display_, screen_));
    XSetBackground(display_, gc_, WhitePixel(display_, screen_));
    XSetFunction(display_, gc_, GXcopy);
    XSetPlaneMask(display_, gc_, AllPlanes);

    // Initialize XRender for this window
    if (has_xrender_) {
        init_xrender();
    }

    // Initialize Xft
    init_xft();

    return true;
}

bool Display::init_xrender() {
    if (!has_xrender_ || !window_) {
        return false;
    }

    pict_format_ = XRenderFindVisualFormat(display_, visual_);
    if (!pict_format_) {
        return false;
    }

    XRenderPictureAttributes pa;
    pa.subwindow_mode = IncludeInferiors;
    picture_ = XRenderCreatePicture(display_, window_, pict_format_,
                                     CPSubwindowMode, &pa);
    return picture_ != 0;
}

bool Display::init_xft() {
    if (!window_) {
        return false;
    }

    xft_draw_ = XftDrawCreate(display_, window_, visual_, colormap_);
    return xft_draw_ != nullptr;
}

void Display::destroy_window() {
    if (xft_draw_) {
        XftDrawDestroy(xft_draw_);
        xft_draw_ = nullptr;
    }
    if (picture_ && display_) {
        XRenderFreePicture(display_, picture_);
        picture_ = 0;
    }
    if (gc_ && display_) {
        XFreeGC(display_, gc_);
        gc_ = nullptr;
    }
    if (window_ && display_) {
        XDestroyWindow(display_, window_);
        window_ = 0;
    }
    width_ = 0;
    height_ = 0;
}

void Display::show_window() {
    if (display_ && window_) {
        XMapWindow(display_, window_);
    }
}

void Display::hide_window() {
    if (display_ && window_) {
        XUnmapWindow(display_, window_);
    }
}

void Display::raise_window() {
    if (display_ && window_) {
        XRaiseWindow(display_, window_);
    }
}

void Display::lower_window() {
    if (display_ && window_) {
        XLowerWindow(display_, window_);
    }
}

void Display::move_window(int x, int y) {
    if (display_ && window_) {
        XMoveWindow(display_, window_, x, y);
    }
}

void Display::resize_window(uint32_t width, uint32_t height) {
    if (display_ && window_) {
        XResizeWindow(display_, window_, width, height);
        width_ = width;
        height_ = height;
    }
}

void Display::set_window_position(int x, int y, uint32_t width, uint32_t height) {
    if (display_ && window_) {
        XMoveResizeWindow(display_, window_, x, y, width, height);
        width_ = width;
        height_ = height;
    }
}

::Window Display::create_child_window(uint32_t width, uint32_t height,
                                       int x, int y,
                                       const std::string& title) {
    if (!display_) return 0;

    XSetWindowAttributes attrs;
    attrs.background_pixel = WhitePixel(display_, screen_);
    attrs.border_pixel = BlackPixel(display_, screen_);
    attrs.event_mask = ExposureMask | StructureNotifyMask;
    attrs.colormap = colormap_;
    attrs.override_redirect = False;

    ::Window win = XCreateWindow(
        display_,
        RootWindow(display_, screen_),
        x, y,
        width, height,
        0,
        depth_,
        InputOutput,
        visual_,
        CWBackPixel | CWBorderPixel | CWEventMask | CWColormap | CWOverrideRedirect,
        &attrs
    );

    if (win) {
        XStoreName(display_, win, title.c_str());
    }

    return win;
}

void Display::destroy_child_window(::Window win) {
    if (display_ && win) {
        XDestroyWindow(display_, win);
    }
}

void Display::show_child_window(::Window win) {
    if (display_ && win) {
        XMapWindow(display_, win);
    }
}

void Display::hide_child_window(::Window win) {
    if (display_ && win) {
        XUnmapWindow(display_, win);
    }
}

void Display::raise_child_window(::Window win) {
    if (display_ && win) {
        XRaiseWindow(display_, win);
    }
}

void Display::lower_child_window(::Window win) {
    if (display_ && win) {
        XLowerWindow(display_, win);
    }
}

void Display::move_child_window(::Window win, int x, int y) {
    if (display_ && win) {
        XMoveWindow(display_, win, x, y);
    }
}

GC Display::create_gc_for_window(::Window win) {
    if (!display_ || !win) return nullptr;
    return XCreateGC(display_, win, 0, nullptr);
}

void Display::draw_rectangle_on(::Window win, GC gc, int x, int y,
                                 int w, int h, bool filled,
                                 uint8_t r, uint8_t g, uint8_t b) {
    if (!display_ || !win || !gc) return;
    unsigned long pixel = alloc_color(r, g, b);
    XSetForeground(display_, gc, pixel);
    if (filled) {
        XFillRectangle(display_, win, gc, x, y, w, h);
    } else {
        XDrawRectangle(display_, win, gc, x, y, w - 1, h - 1);
    }
}

void Display::fill_window(::Window win, GC gc, uint8_t r, uint8_t g, uint8_t b) {
    if (!display_ || !win || !gc) return;

    XWindowAttributes attrs;
    if (XGetWindowAttributes(display_, win, &attrs)) {
        draw_rectangle_on(win, gc, 0, 0, attrs.width, attrs.height, true, r, g, b);
    }
}

XImage* Display::capture_window_ximage(::Window win) {
    if (!display_ || !win) return nullptr;

    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display_, win, &attrs)) {
        return nullptr;
    }

    return XGetImage(display_, win, 0, 0, attrs.width, attrs.height, AllPlanes, ZPixmap);
}

::Window Display::root_window() const {
    if (!display_) return 0;
    return RootWindow(display_, screen_);
}

uint32_t Display::screen_width() const {
    if (!display_) return 0;
    return DisplayWidth(display_, screen_);
}

uint32_t Display::screen_height() const {
    if (!display_) return 0;
    return DisplayHeight(display_, screen_);
}

XImage* Display::capture_root_region(int x, int y, uint32_t w, uint32_t h) {
    if (!display_) return nullptr;
    ::Window root = RootWindow(display_, screen_);
    return XGetImage(display_, root, x, y, w, h, AllPlanes, ZPixmap);
}

void Display::set_foreground(uint8_t r, uint8_t g, uint8_t b) {
    if (!display_ || !gc_) return;
    unsigned long pixel = alloc_color(r, g, b);
    XSetForeground(display_, gc_, pixel);
}

void Display::set_foreground(unsigned long pixel) {
    if (!display_ || !gc_) return;
    XSetForeground(display_, gc_, pixel);
}

void Display::draw_rectangle(int x, int y, int width, int height, bool filled) {
    if (!display_ || !gc_ || !window_) return;
    if (filled) {
        XFillRectangle(display_, window_, gc_, x, y, width, height);
    } else {
        XDrawRectangle(display_, window_, gc_, x, y, width - 1, height - 1);
    }
}

void Display::draw_line(int x1, int y1, int x2, int y2) {
    if (!display_ || !gc_ || !window_) return;
    XDrawLine(display_, window_, gc_, x1, y1, x2, y2);
}

void Display::draw_arc(int x, int y, int width, int height, int angle1, int angle2) {
    if (!display_ || !gc_ || !window_) return;
    XDrawArc(display_, window_, gc_, x, y, width, height, angle1, angle2);
}

void Display::draw_text(XftFont* font, int x, int y, const std::string& text,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!xft_draw_ || !font) return;

    XftColor color;
    XRenderColor render_color;
    render_color.red = r * 257;    // Scale 0-255 to 0-65535
    render_color.green = g * 257;
    render_color.blue = b * 257;
    render_color.alpha = a * 257;

    XftColorAllocValue(display_, visual_, colormap_, &render_color, &color);
    XftDrawStringUtf8(xft_draw_, &color, font, x, y,
                      reinterpret_cast<const FcChar8*>(text.c_str()), text.length());
    XftColorFree(display_, visual_, colormap_, &color);
}

void Display::render_fill_rectangle(int x, int y, int width, int height,
                                    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!has_xrender_ || !picture_) return;

    XRenderColor color;
    color.red = r * 257;
    color.green = g * 257;
    color.blue = b * 257;
    color.alpha = a * 257;

    XRenderFillRectangle(display_, PictOpOver, picture_, &color, x, y, width, height);
}

XftFont* Display::load_font(const std::string& font_name, int size) {
    if (!display_) return nullptr;

    std::string pattern = font_name + ":size=" + std::to_string(size);
    return XftFontOpenName(display_, screen_, pattern.c_str());
}

void Display::free_font(XftFont* font) {
    if (display_ && font) {
        XftFontClose(display_, font);
    }
}

void Display::flush() {
    if (display_) {
        XFlush(display_);
    }
}

void Display::sync(bool discard) {
    if (display_) {
        XSync(display_, discard ? True : False);
    }
}

void Display::clear_window() {
    if (display_ && window_) {
        XClearWindow(display_, window_);
        XSync(display_, False);
    }
}

bool Display::wait_for_expose(int timeout_ms) {
    if (!display_ || !window_) return false;

    auto start = std::chrono::steady_clock::now();
    XEvent event;

    while (true) {
        // Check for timeout
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= timeout_ms) {
            return false;
        }

        // Check if there's an event
        if (XPending(display_) > 0) {
            XNextEvent(display_, &event);
            if (event.type == Expose && event.xexpose.count == 0) {
                return true;
            }
        } else {
            // Small sleep to avoid busy waiting
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 10000000;  // 10ms
            nanosleep(&ts, nullptr);
        }
    }
}

void Display::process_pending_events() {
    if (!display_) return;

    XEvent event;
    while (XPending(display_) > 0) {
        XNextEvent(display_, &event);
        // Just discard events for now
    }
}

unsigned long Display::alloc_color(uint8_t r, uint8_t g, uint8_t b) {
    if (!display_) return 0;

    XColor color;
    color.red = r * 257;    // Scale 0-255 to 0-65535
    color.green = g * 257;
    color.blue = b * 257;
    color.flags = DoRed | DoGreen | DoBlue;

    if (XAllocColor(display_, colormap_, &color)) {
        return color.pixel;
    }

    // Fallback: approximate with default colors
    return BlackPixel(display_, screen_);
}

// Advanced GC operations
void Display::set_function(int function) {
    if (!display_ || !gc_) return;
    // Sync pending operations before changing function - important for modes
    // like GXxor that read from the destination
    XSync(display_, False);
    XSetFunction(display_, gc_, function);
    // Also sync after to ensure the function change is processed
    XSync(display_, False);
}

void Display::set_background(uint8_t r, uint8_t g, uint8_t b) {
    if (!display_ || !gc_) return;
    unsigned long pixel = alloc_color(r, g, b);
    XSetBackground(display_, gc_, pixel);
}

void Display::set_background(unsigned long pixel) {
    if (!display_ || !gc_) return;
    XSetBackground(display_, gc_, pixel);
}

void Display::set_line_attributes(unsigned int width, int line_style,
                                  int cap_style, int join_style) {
    if (!display_ || !gc_) return;
    XSetLineAttributes(display_, gc_, width, line_style, cap_style, join_style);
}

void Display::set_dashes(int dash_offset, const char* dash_list, int n) {
    if (!display_ || !gc_) return;
    XSetDashes(display_, gc_, dash_offset, dash_list, n);
}

void Display::set_fill_style(int fill_style) {
    if (!display_ || !gc_) return;
    XSetFillStyle(display_, gc_, fill_style);
}

void Display::set_fill_rule(int fill_rule) {
    if (!display_ || !gc_) return;
    XSetFillRule(display_, gc_, fill_rule);
}

void Display::set_stipple(Pixmap stipple) {
    if (!display_ || !gc_) return;
    XSetStipple(display_, gc_, stipple);
}

void Display::set_tile(Pixmap tile) {
    if (!display_ || !gc_) return;
    XSetTile(display_, gc_, tile);
}

void Display::set_clip_mask(Pixmap mask) {
    if (!display_ || !gc_) return;
    XSetClipMask(display_, gc_, mask);
}

void Display::set_clip_rectangles(int x, int y, XRectangle* rects, int n, int ordering) {
    if (!display_ || !gc_) return;
    XSetClipRectangles(display_, gc_, x, y, rects, n, ordering);
}

void Display::set_plane_mask(unsigned long mask) {
    if (!display_ || !gc_) return;
    XSetPlaneMask(display_, gc_, mask);
}

void Display::set_subwindow_mode(int mode) {
    if (!display_ || !gc_) return;
    XSetSubwindowMode(display_, gc_, mode);
}

// Pixmap operations
Pixmap Display::create_pixmap(unsigned int width, unsigned int height, unsigned int depth) {
    if (!display_ || !window_) return 0;
    return XCreatePixmap(display_, window_, width, height, depth);
}

Pixmap Display::create_bitmap(unsigned int width, unsigned int height) {
    if (!display_ || !window_) return 0;
    return XCreatePixmap(display_, window_, width, height, 1);
}

void Display::free_pixmap(Pixmap pixmap) {
    if (display_ && pixmap) {
        XFreePixmap(display_, pixmap);
    }
}

GC Display::create_gc_for_pixmap(Pixmap pixmap) {
    if (!display_ || !pixmap) return nullptr;
    return XCreateGC(display_, pixmap, 0, nullptr);
}

void Display::free_gc(GC gc) {
    if (display_ && gc) {
        XFreeGC(display_, gc);
    }
}

void Display::draw_to_pixmap(Pixmap pixmap, GC gc,
                             std::function<void(::Display*, Drawable, GC)> draw_func) {
    if (!display_ || !pixmap || !gc) return;
    draw_func(display_, pixmap, gc);
}

void Display::fill_polygon(XPoint* points, int npoints, int shape, int mode) {
    if (!display_ || !gc_ || !window_) return;
    XFillPolygon(display_, window_, gc_, points, npoints, shape, mode);
}

} // namespace x11bench
