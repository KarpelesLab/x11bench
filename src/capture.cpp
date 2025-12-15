#include "capture.hpp"
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

Image Capture::ximage_to_image(XImage* ximg) {
    if (!ximg) {
        return Image();
    }

    uint32_t width = ximg->width;
    uint32_t height = ximg->height;
    Image img(width, height);

    // Handle different pixel formats
    int bits_per_pixel = ximg->bits_per_pixel;
    int byte_order = ximg->byte_order;

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(ximg, x, y);

            uint8_t r, g, b, a;

            if (bits_per_pixel == 32) {
                // Common 32-bit format (ARGB or BGRA depending on byte order)
                if (byte_order == LSBFirst) {
                    // BGRA format (common on x86)
                    b = (pixel >> 0) & 0xFF;
                    g = (pixel >> 8) & 0xFF;
                    r = (pixel >> 16) & 0xFF;
                    a = (pixel >> 24) & 0xFF;
                } else {
                    // ARGB format
                    a = (pixel >> 24) & 0xFF;
                    r = (pixel >> 16) & 0xFF;
                    g = (pixel >> 8) & 0xFF;
                    b = (pixel >> 0) & 0xFF;
                }
                // X11 windows without alpha typically have a = 0, treat as opaque
                if (a == 0) a = 255;
            } else if (bits_per_pixel == 24) {
                // 24-bit RGB
                r = (pixel >> 16) & 0xFF;
                g = (pixel >> 8) & 0xFF;
                b = (pixel >> 0) & 0xFF;
                a = 255;
            } else if (bits_per_pixel == 16) {
                // 16-bit (565 or 555)
                // Assuming 565 format
                r = ((pixel >> 11) & 0x1F) * 255 / 31;
                g = ((pixel >> 5) & 0x3F) * 255 / 63;
                b = (pixel & 0x1F) * 255 / 31;
                a = 255;
            } else {
                // Fallback: grayscale or unknown
                r = g = b = (pixel & 0xFF);
                a = 255;
            }

            img.set_pixel(x, y, r, g, b, a);
        }
    }

    return img;
}

} // namespace x11bench
