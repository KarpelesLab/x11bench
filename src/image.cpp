#include "image.hpp"
#include <png.h>
#include <cstring>
#include <stdexcept>

namespace x11bench {

Image::Image(uint32_t width, uint32_t height)
    : width_(width), height_(height), data_(width * height * 4, 0) {
}

Image::Image(const Image& other)
    : width_(other.width_), height_(other.height_), data_(other.data_) {
}

Image::Image(Image&& other) noexcept
    : width_(other.width_), height_(other.height_), data_(std::move(other.data_)) {
    other.width_ = 0;
    other.height_ = 0;
}

Image& Image::operator=(const Image& other) {
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        data_ = other.data_;
    }
    return *this;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        data_ = std::move(other.data_);
        other.width_ = 0;
        other.height_ = 0;
    }
    return *this;
}

Pixel Image::get_pixel(uint32_t x, uint32_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("Pixel coordinates out of range");
    }
    size_t offset = (y * width_ + x) * 4;
    return Pixel{data_[offset], data_[offset + 1], data_[offset + 2], data_[offset + 3]};
}

void Image::set_pixel(uint32_t x, uint32_t y, const Pixel& pixel) {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("Pixel coordinates out of range");
    }
    size_t offset = (y * width_ + x) * 4;
    data_[offset] = pixel.r;
    data_[offset + 1] = pixel.g;
    data_[offset + 2] = pixel.b;
    data_[offset + 3] = pixel.a;
}

void Image::set_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    set_pixel(x, y, Pixel{r, g, b, a});
}

void Image::fill(const Pixel& pixel) {
    for (size_t i = 0; i < data_.size(); i += 4) {
        data_[i] = pixel.r;
        data_[i + 1] = pixel.g;
        data_[i + 2] = pixel.b;
        data_[i + 3] = pixel.a;
    }
}

void Image::fill(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    fill(Pixel{r, g, b, a});
}

bool Image::save_png(const std::string& filename) const {
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        return false;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);

    png_set_IHDR(png, info, width_, height_, 8,
                 PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    std::vector<png_bytep> row_pointers(height_);
    for (uint32_t y = 0; y < height_; y++) {
        row_pointers[y] = const_cast<png_bytep>(data_.data() + y * width_ * 4);
    }

    png_write_image(png, row_pointers.data());
    png_write_end(png, nullptr);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return true;
}

bool Image::load_png(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        return false;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return false;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    width_ = png_get_image_width(png, info);
    height_ = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Convert to RGBA
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }
    if (png_get_valid(png, info, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png);
    }
    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    png_read_update_info(png, info);

    data_.resize(width_ * height_ * 4);
    std::vector<png_bytep> row_pointers(height_);
    for (uint32_t y = 0; y < height_; y++) {
        row_pointers[y] = data_.data() + y * width_ * 4;
    }

    png_read_image(png, row_pointers.data());

    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
    return true;
}

Image Image::from_bgra(const uint8_t* data, uint32_t width, uint32_t height) {
    Image img(width, height);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            size_t src_offset = (y * width + x) * 4;
            // BGRA to RGBA
            img.set_pixel(x, y, data[src_offset + 2], data[src_offset + 1],
                          data[src_offset], data[src_offset + 3]);
        }
    }
    return img;
}

Image Image::from_rgb(const uint8_t* data, uint32_t width, uint32_t height) {
    Image img(width, height);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            size_t src_offset = (y * width + x) * 3;
            img.set_pixel(x, y, data[src_offset], data[src_offset + 1],
                          data[src_offset + 2], 255);
        }
    }
    return img;
}

} // namespace x11bench
