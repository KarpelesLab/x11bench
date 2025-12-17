#include "capture.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace x11bench {

Image Capture::capture_window(Display& display) {
    if (!display.is_connected() || !display.has_window()) {
        throw std::runtime_error("Display not connected or no window");
    }

    return capture_region(display, 0, 0,
                          display.window_width(), display.window_height());
}

Image Capture::capture_region(Display& display, int x, int y,
                               uint32_t width, uint32_t height) {
    if (!display.is_connected() || !display.has_window()) {
        throw std::runtime_error("Display not connected or no window");
    }

    // Ensure we sync before capture
    display.sync(false);

    XImage* ximg = XGetImage(
        display.x_display(),
        display.x_window(),
        x, y,
        width, height,
        AllPlanes,
        ZPixmap
    );

    if (!ximg) {
        throw std::runtime_error("XGetImage failed");
    }

    Image result = ximage_to_image(ximg);
    XDestroyImage(ximg);

    return result;
}

namespace {
uint8_t extract_channel(unsigned long pixel, unsigned long mask) {
    if (mask == 0) {
        return 0;
    }

    // Find shift and max value from mask
    int shift = 0;
    while (((mask >> shift) & 1UL) == 0UL && shift < static_cast<int>(sizeof(unsigned long) * 8)) {
        shift++;
    }

    unsigned long shifted_mask = mask >> shift;
    int bits = 0;
    unsigned long tmp = shifted_mask;
    while (tmp) {
        bits += (tmp & 1UL);
        tmp >>= 1;
    }

    if (bits == 0) {
        return 0;
    }

    unsigned long value = (pixel & mask) >> shift;
    double max_val = static_cast<double>(shifted_mask);
    double normalized = (max_val > 0.0) ? (static_cast<double>(value) * 255.0 / max_val) : 0.0;
    return static_cast<uint8_t>(std::lround(std::min(255.0, normalized)));
}
} // namespace

Image Capture::ximage_to_image(XImage* ximg) {
    if (!ximg) {
        return Image();
    }

    uint32_t width = ximg->width;
    uint32_t height = ximg->height;
    Image img(width, height);

    unsigned long alpha_mask = 0;

    // Only consider alpha when the drawable depth suggests it exists (e.g. ARGB visuals).
    if (ximg->depth > 24) {
        unsigned long rgb_mask = ximg->red_mask | ximg->green_mask | ximg->blue_mask;

        // Compute mask for all bits within the pixel format
        int bpp = ximg->bits_per_pixel;
        unsigned long pixel_mask;
        if (bpp >= static_cast<int>(sizeof(unsigned long) * 8)) {
            pixel_mask = ~0UL;
        } else {
            pixel_mask = (1UL << bpp) - 1;
        }

        // Alpha occupies the non-RGB bits within the pixel
        alpha_mask = pixel_mask & ~rgb_mask;
    }

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(ximg, x, y);

            uint8_t r = extract_channel(pixel, ximg->red_mask);
            uint8_t g = extract_channel(pixel, ximg->green_mask);
            uint8_t b = extract_channel(pixel, ximg->blue_mask);
            uint8_t a = alpha_mask ? extract_channel(pixel, alpha_mask) : 255;
            if (alpha_mask && a == 0) {
                // Many ARGB visuals leave alpha at 0 for opaque drawables; treat as fully opaque.
                a = 255;
            }

            img.set_pixel(x, y, r, g, b, a);
        }
    }

    return img;
}

} // namespace x11bench
