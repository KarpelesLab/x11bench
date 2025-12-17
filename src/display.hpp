#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xft/Xft.h>
#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace x11bench {

class Display {
public:
    Display();
    ~Display();

    // Non-copyable
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    // Move semantics
    Display(Display&& other) noexcept;
    Display& operator=(Display&& other) noexcept;

    // Connection
    bool connect(const std::string& display_name = "");
    void disconnect();
    bool is_connected() const { return display_ != nullptr; }

    // Window management
    bool create_window(uint32_t width, uint32_t height,
                       const std::string& title = "x11bench");
    void destroy_window();
    void show_window();
    void hide_window();
    bool has_window() const { return window_ != 0; }

    // Window management operations
    void raise_window();
    void lower_window();
    void move_window(int x, int y);
    void resize_window(uint32_t width, uint32_t height);
    void set_window_position(int x, int y, uint32_t width, uint32_t height);

    // Multi-window support - create additional windows
    ::Window create_child_window(uint32_t width, uint32_t height,
                                  int x = 0, int y = 0,
                                  const std::string& title = "x11bench child");
    void destroy_child_window(::Window win);
    void show_child_window(::Window win);
    void hide_child_window(::Window win);
    void raise_child_window(::Window win);
    void lower_child_window(::Window win);
    void move_child_window(::Window win, int x, int y);
    GC create_gc_for_window(::Window win);

    // Draw to specific window
    void draw_rectangle_on(::Window win, GC gc, int x, int y,
                           int width, int height, bool filled,
                           uint8_t r, uint8_t g, uint8_t b);
    void fill_window(::Window win, GC gc, uint8_t r, uint8_t g, uint8_t b);

    // Capture specific window
    XImage* capture_window_ximage(::Window win);

    // Capture region of root window (screen capture)
    XImage* capture_root_region(int x, int y, uint32_t width, uint32_t height);

    // Root window access for absolute positioning
    ::Window root_window() const;
    uint32_t screen_width() const;
    uint32_t screen_height() const;

    // Get window dimensions
    uint32_t window_width() const { return width_; }
    uint32_t window_height() const { return height_; }

    // Graphics context for basic drawing
    GC gc() const { return gc_; }

    // Direct X11 access (for advanced operations)
    ::Display* x_display() const { return display_; }
    ::Window x_window() const { return window_; }
    int screen() const { return screen_; }
    Visual* visual() const { return visual_; }
    Colormap colormap() const { return colormap_; }
    int depth() const { return depth_; }

    // XRender support
    bool has_xrender() const { return has_xrender_; }
    Picture picture() const { return picture_; }
    XRenderPictFormat* pict_format() const { return pict_format_; }

    // Xft font support
    XftDraw* xft_draw() const { return xft_draw_; }
    XftFont* load_font(const std::string& font_name, int size);
    void free_font(XftFont* font);

    // Basic drawing operations (for convenience)
    void set_foreground(uint8_t r, uint8_t g, uint8_t b);
    void set_foreground(unsigned long pixel);
    void draw_rectangle(int x, int y, int width, int height, bool filled = true);
    void draw_line(int x1, int y1, int x2, int y2);
    void draw_arc(int x, int y, int width, int height,
                  int angle1 = 0, int angle2 = 360 * 64);
    void draw_text(XftFont* font, int x, int y, const std::string& text,
                   uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // XRender drawing
    void render_fill_rectangle(int x, int y, int width, int height,
                               uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    // Advanced GC operations
    void set_function(int function);  // GXcopy, GXxor, GXand, GXor, etc.
    void set_background(uint8_t r, uint8_t g, uint8_t b);
    void set_background(unsigned long pixel);
    void set_line_attributes(unsigned int width, int line_style,
                             int cap_style, int join_style);
    void set_dashes(int dash_offset, const char* dash_list, int n);
    void set_fill_style(int fill_style);  // FillSolid, FillTiled, FillStippled, etc.
    void set_fill_rule(int fill_rule);    // EvenOddRule, WindingRule
    void set_stipple(Pixmap stipple);
    void set_tile(Pixmap tile);
    void set_clip_mask(Pixmap mask);
    void set_clip_rectangles(int x, int y, XRectangle* rects, int n, int ordering);
    void set_plane_mask(unsigned long mask);
    void set_subwindow_mode(int mode);  // ClipByChildren, IncludeInferiors

    // Pixmap operations
    Pixmap create_pixmap(unsigned int width, unsigned int height, unsigned int depth);
    Pixmap create_bitmap(unsigned int width, unsigned int height);
    void free_pixmap(Pixmap pixmap);
    GC create_gc_for_pixmap(Pixmap pixmap);
    void free_gc(GC gc);

    // Draw to pixmap
    void draw_to_pixmap(Pixmap pixmap, GC gc,
                        std::function<void(::Display*, Drawable, GC)> draw_func);

    // Filled polygon
    void fill_polygon(XPoint* points, int npoints, int shape, int mode);

    // Event handling
    void flush();
    void sync(bool discard = false);
    bool wait_for_expose(int timeout_ms = 1000);
    void process_pending_events();
    void clear_window();

    // Color allocation
    unsigned long alloc_color(uint8_t r, uint8_t g, uint8_t b);

private:
    ::Display* display_ = nullptr;
    ::Window window_ = 0;
    int screen_ = 0;
    Visual* visual_ = nullptr;
    Colormap colormap_ = 0;
    int depth_ = 0;
    GC gc_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;

    // XRender
    bool has_xrender_ = false;
    Picture picture_ = 0;
    XRenderPictFormat* pict_format_ = nullptr;

    // Xft
    XftDraw* xft_draw_ = nullptr;

    void cleanup();
    bool init_xrender();
    bool init_xft();
};

} // namespace x11bench
