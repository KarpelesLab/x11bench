#include "compare.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace x11bench {

int Compare::channel_diff(uint8_t a, uint8_t b) {
    return std::abs(static_cast<int>(a) - static_cast<int>(b));
}

CompareResult Compare::exact(const Image& img1, const Image& img2) {
    return fuzzy(img1, img2, 0);
}

CompareResult Compare::fuzzy(const Image& img1, const Image& img2, int tolerance) {
    CompareResult result;

    // Check dimensions
    if (img1.width() != img2.width() || img1.height() != img2.height()) {
        result.match = false;
        std::ostringstream oss;
        oss << "Dimension mismatch: " << img1.width() << "x" << img1.height()
            << " vs " << img2.width() << "x" << img2.height();
        result.message = oss.str();
        return result;
    }

    if (img1.empty() || img2.empty()) {
        result.match = img1.empty() && img2.empty();
        result.message = img1.empty() && img2.empty() ? "Both images empty" : "One image empty";
        return result;
    }

    result.total_pixels = img1.width() * img1.height();
    result.different_pixels = 0;
    result.max_channel_diff = 0.0;
    double total_diff = 0.0;
    uint64_t channel_count = 0;

    for (uint32_t y = 0; y < img1.height(); y++) {
        for (uint32_t x = 0; x < img1.width(); x++) {
            Pixel p1 = img1.get_pixel(x, y);
            Pixel p2 = img2.get_pixel(x, y);

            int dr = channel_diff(p1.r, p2.r);
            int dg = channel_diff(p1.g, p2.g);
            int db = channel_diff(p1.b, p2.b);
            int da = channel_diff(p1.a, p2.a);

            int max_diff = std::max({dr, dg, db, da});
            result.max_channel_diff = std::max(result.max_channel_diff, static_cast<double>(max_diff));

            total_diff += dr + dg + db + da;
            channel_count += 4;

            if (max_diff > tolerance) {
                result.different_pixels++;
            }
        }
    }

    result.avg_channel_diff = channel_count > 0 ? total_diff / channel_count : 0.0;
    result.difference_percent = result.total_pixels > 0 ?
        (100.0 * result.different_pixels / result.total_pixels) : 0.0;
    result.match = (result.different_pixels == 0);

    if (result.match) {
        result.message = "Images match";
        if (tolerance > 0) {
            std::ostringstream oss;
            oss << " (within tolerance " << tolerance << ")";
            result.message += oss.str();
        }
    } else {
        std::ostringstream oss;
        oss << result.different_pixels << " pixels differ ("
            << std::fixed << result.difference_percent << "%), "
            << "max channel diff: " << result.max_channel_diff;
        result.message = oss.str();
    }

    return result;
}

CompareResult Compare::fuzzy_percent(const Image& img1, const Image& img2,
                                      double max_diff_percent) {
    CompareResult result = fuzzy(img1, img2, 255);  // Get full stats

    // Override match based on percentage threshold
    result.match = (result.difference_percent <= max_diff_percent);

    if (result.match && result.different_pixels > 0) {
        std::ostringstream oss;
        oss << result.different_pixels << " pixels differ ("
            << std::fixed << result.difference_percent
            << "%) - within " << max_diff_percent << "% threshold";
        result.message = oss.str();
    }

    return result;
}

Image Compare::generate_diff(const Image& img1, const Image& img2, int tolerance) {
    uint32_t width = std::max(img1.width(), img2.width());
    uint32_t height = std::max(img1.height(), img2.height());

    if (width == 0 || height == 0) {
        return Image();
    }

    Image diff(width, height);

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            // Handle images of different sizes
            bool in_img1 = (x < img1.width() && y < img1.height());
            bool in_img2 = (x < img2.width() && y < img2.height());

            if (!in_img1 && !in_img2) {
                diff.set_pixel(x, y, 0, 0, 0, 255);  // Black
            } else if (!in_img1) {
                diff.set_pixel(x, y, 0, 255, 0, 255);  // Green - only in img2
            } else if (!in_img2) {
                diff.set_pixel(x, y, 0, 0, 255, 255);  // Blue - only in img1
            } else {
                Pixel p1 = img1.get_pixel(x, y);
                Pixel p2 = img2.get_pixel(x, y);

                int dr = channel_diff(p1.r, p2.r);
                int dg = channel_diff(p1.g, p2.g);
                int db = channel_diff(p1.b, p2.b);
                int da = channel_diff(p1.a, p2.a);

                int max_diff = std::max({dr, dg, db, da});

                if (max_diff > tolerance) {
                    // Highlight differences: red intensity proportional to difference
                    uint8_t intensity = static_cast<uint8_t>(std::min(255, max_diff * 2));
                    diff.set_pixel(x, y, 255, 255 - intensity, 255 - intensity, 255);
                } else {
                    // Matching pixels: show original (darkened)
                    diff.set_pixel(x, y, p1.r / 2, p1.g / 2, p1.b / 2, 255);
                }
            }
        }
    }

    return diff;
}

} // namespace x11bench
