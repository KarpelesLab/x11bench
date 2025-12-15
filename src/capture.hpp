#pragma once

#include "image.hpp"
#include "display.hpp"
#include <memory>

namespace x11bench {

class Capture {
public:
    // Capture entire window content
    static Image capture_window(Display& display);

    // Capture a region of the window
    static Image capture_region(Display& display, int x, int y,
                                uint32_t width, uint32_t height);

private:
    // Convert XImage to our Image format
    static Image ximage_to_image(XImage* ximg);
};

} // namespace x11bench
