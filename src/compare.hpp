#pragma once

#include "image.hpp"
#include <string>

namespace x11bench {

struct CompareResult {
    bool match = false;
    uint32_t different_pixels = 0;
    uint32_t total_pixels = 0;
    double difference_percent = 0.0;
    double max_channel_diff = 0.0;  // Maximum difference in any channel
    double avg_channel_diff = 0.0;  // Average difference across channels
    std::string message;
};

class Compare {
public:
    // Exact pixel comparison
    static CompareResult exact(const Image& img1, const Image& img2);

    // Fuzzy comparison with tolerance (0-255 per channel)
    static CompareResult fuzzy(const Image& img1, const Image& img2, int tolerance);

    // Fuzzy comparison with percentage tolerance (0.0 - 1.0)
    static CompareResult fuzzy_percent(const Image& img1, const Image& img2,
                                        double max_diff_percent);

    // Generate a diff image (highlights differences in red)
    static Image generate_diff(const Image& img1, const Image& img2, int tolerance = 0);

private:
    static int channel_diff(uint8_t a, uint8_t b);
};

} // namespace x11bench
