#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace x11bench {

struct Pixel {
    uint8_t r, g, b, a;

    bool operator==(const Pixel& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!=(const Pixel& other) const {
        return !(*this == other);
    }
};

class Image {
public:
    Image() = default;
    Image(uint32_t width, uint32_t height);
    Image(const Image& other);
    Image(Image&& other) noexcept;
    Image& operator=(const Image& other);
    Image& operator=(Image&& other) noexcept;
    ~Image() = default;

    // Accessors
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    bool empty() const { return data_.empty(); }

    // Pixel access
    Pixel get_pixel(uint32_t x, uint32_t y) const;
    void set_pixel(uint32_t x, uint32_t y, const Pixel& pixel);
    void set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Fill operations
    void fill(const Pixel& pixel);
    void fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // Raw data access (for X11 interop)
    uint8_t* data() { return data_.data(); }
    const uint8_t* data() const { return data_.data(); }
    size_t stride() const { return width_ * 4; }
    size_t size() const { return data_.size(); }

    // PNG I/O
    bool save_png(const std::string& filename) const;
    bool load_png(const std::string& filename);

    // Create from raw BGRA data (common X11 format)
    static Image from_bgra(const uint8_t* data, uint32_t width, uint32_t height);

    // Create from raw RGB data
    static Image from_rgb(const uint8_t* data, uint32_t width, uint32_t height);

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::vector<uint8_t> data_;  // RGBA format, 4 bytes per pixel
};

} // namespace x11bench
