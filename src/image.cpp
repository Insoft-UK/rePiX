/*
 The MIT License (MIT)
 
 Copyright (c) 2024 Insoft. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

#include "image.hpp"

#include <fstream>
#include <cstring>
#include <png.h>


/* Windows 3.x bitmap file header */
typedef struct __attribute__((__packed__)) {
    char      bfType[2];   /* magic - always 'B' 'M' */
    uint32_t  bfSize;
    uint16_t  bfReserved1;
    uint16_t  bfReserved2;
    uint32_t  bfOffBits;    /* offset in bytes to actual bitmap data */
} BMPHeader;

/* Windows 3.x bitmap full header, including file header */

typedef struct __attribute__((__packed__)) {
    BMPHeader fileHeader;
    uint32_t  biSize;
    int32_t   biWidth;
    int32_t   biHeight;
    int16_t   biPlanes;           // Number of colour planes, set to 1
    int16_t   biBitCount;         // Colour bits per pixel. 1 4 8 24 or 32
    uint32_t  biCompression;      // *Code for the compression scheme
    uint32_t  biSizeImage;        // *Size of the bitmap bits in bytes
    int32_t   biXPelsPerMeter;    // *Horizontal resolution in pixels per meter
    int32_t   biYPelsPerMeter;    // *Vertical resolution in pixels per meter
    uint32_t  biClrUsed;          // *Number of colours defined in the palette
    uint32_t  biClImportant;      // *Number of important colours in the image
} BIPHeader;

static void flipImageVertically(const TImage *image)
{
    uint8_t *byte = (uint8_t *)image->data;
    int w = (int)image->width / (8 / image->bitWidth);
    int h = (int)image->height;
    
    for (int row = 0; row < h / 2; ++row)
        for (int col = 0; col < w; ++col)
            std::swap(byte[col + row * w], byte[col + (h - 1 - row) * w]);
    
}

TImage *loadPNGGraphicFile(const std::string& filename) {
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    // Open the file using an ifstream
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // Read the PNG signature (first 8 bytes)
    png_byte header[8];
    file.read(reinterpret_cast<char*>(header), sizeof(header));
    if (file.gcount() != sizeof(header) || png_sig_cmp(header, 0, 8)) {
        throw std::runtime_error("File is not a valid PNG: " + filename);
    }

    // Initialize PNG structs
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) throw std::runtime_error("Failed to create PNG read struct");

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        throw std::runtime_error("Failed to create PNG info struct");
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        throw std::runtime_error("Error during PNG read");
    }

    // Set the custom read function to use ifstream
    png_set_read_fn(png, &file, [](png_structp png, png_bytep data, png_size_t length) {
        std::ifstream* file = static_cast<std::ifstream*>(png_get_io_ptr(png));
        file->read(reinterpret_cast<char*>(data), length);
    });

    // Inform libpng we've already read the first 8 bytes
    png_set_sig_bytes(png, 8);

    // Read the image info
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Convert the PNG to 8-bit RGBA format
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY) png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    png_read_update_info(png, info);

    // Create the TImage structure
    image->width = static_cast<uint16_t>(width);
    image->height = static_cast<uint16_t>(height);
    image->bitWidth = 32;  // Assume we're loading in RGBA format (32 bits per pixel)

    // Allocate memory for the pixel data
    size_t dataSize = width * height * 4; // 4 bytes per pixel (RGBA)
    image->data = new uint8_t[dataSize];

    // Read the image data row by row
    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; ++y) {
        row_pointers[y] = image->data + y * width * 4;
    }

    png_read_image(png, row_pointers.data());

    // Clean up
    png_destroy_read_struct(&png, &info, nullptr);

    return image;
}

TImage *loadBMPGraphicFile(const std::string& filename) {
    BIPHeader bip_header;
    
    std::ifstream infile;
    
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    infile.open(filename, std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
        free(image);
        return nullptr;
    }
    
    infile.read((char *)&bip_header, sizeof(BIPHeader));
    
    if (strncmp(bip_header.fileHeader.bfType, "BM", 2) != 0) {
        infile.close();
        free(image);
        return nullptr;
    }
    
    image->bitWidth = bip_header.biBitCount;
    image->data = (unsigned char *)malloc(bip_header.biSizeImage);
    if (!image->data) {
        free(image);
        infile.close();
        return nullptr;
    }
    
    image->width = abs(bip_header.biWidth);
    image->height = abs(bip_header.biHeight);
    size_t length = image->width / (8 / bip_header.biBitCount);
    
    infile.seekg(bip_header.fileHeader.bfOffBits, std::ios_base::beg);
    for (int r = 0; r < image->height; ++r) {
        infile.read((char *)&image->data[length * r], length);
        if (infile.gcount() != length) {
            std::cout << filename << " Read failed!\n";
            break;
        }
        
        /*
         Each scan line is zero padded to the nearest 4-byte boundary.
         
         If the image has a width that is not divisible by four, say, 21 bytes, there
         would be 3 bytes of padding at the end of every scan line.
         */
        if (length % 4)
            infile.seekg(4 - length % 4, std::ios_base::cur);
    }
    
    infile.close();
    
    if (bip_header.biHeight > 0)
        flipImageVertically(image);
    
    return image;
}

TImage *loadPBMGraphicFile(const std::string &filename)
{
    std::ifstream infile;
    
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    infile.open(filename, std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
        free(image);
        return nullptr;
    }
    
    std::string s;
    
    getline(infile, s);
    if (s != "P4") {
        infile.close();
        return image;
    }
    
    image->bitWidth = 1;
    
    getline(infile, s);
    image->width = atoi(s.c_str());
    
    getline(infile, s);
    image->height = atoi(s.c_str());
    
    size_t length = ((image->width + 7) >> 3) * image->height;
    image->data = (unsigned char *)malloc(length);
    
    if (!image->data) {
        free(image);
        infile.close();
        return nullptr;
    }
    infile.read((char *)image->data, length);
    
    infile.close();
    return image;
}

bool saveImageAsPNGFile(TImage* image, const std::string& filename) {

    // Open file
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        std::cerr << "Error: Unable to open file for writing: " << filename << std::endl;
        return false;
    }

    // Create PNG write struct
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) {
        std::cerr << "Error: Unable to create PNG write struct." << std::endl;
        fclose(fp);
        return false;
    }

    // Create PNG info struct
    png_infop info = png_create_info_struct(png);
    if (!info) {
        std::cerr << "Error: Unable to create PNG info struct." << std::endl;
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        return false;
    }

    // Setup error handling
    if (setjmp(png_jmpbuf(png))) {
        std::cerr << "Error: Exception during PNG creation." << std::endl;
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return false;
    }

    // Initialize file I/O
    png_init_io(png, fp);

    // Set image header info
    int color_type;
    switch (image->bitWidth) {
        case 8:
            color_type = PNG_COLOR_TYPE_GRAY;
            break;
        case 24:
            color_type = PNG_COLOR_TYPE_RGB;
            break;
        case 32:
            color_type = PNG_COLOR_TYPE_RGBA;
            break;
        default:
            std::cerr << "Error: Unsupported bit width." << std::endl;
            png_destroy_write_struct(&png, &info);
            fclose(fp);
            return false;
    }

    png_set_IHDR(png, info, image->width, image->height, 8, color_type,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    // Write image data row by row
    const int bytes_per_pixel = image->bitWidth / 8;
    std::vector<png_bytep> row_pointers(image->height);
    for (size_t y = 0; y < image->height; ++y) {
        row_pointers[y] = (png_bytep)(&image->data[y * image->width * bytes_per_pixel]);
    }
    png_write_image(png, row_pointers.data());

    // End write
    png_write_end(png, nullptr);

    // Cleanup
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    std::cout << "PNG file saved successfully: " << filename << std::endl;
    return true;
}

TImage *createBitmap(int w, int h)
{
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    w = (w + 7) & ~7;
    image->data = (uint8_t *)calloc(sizeof(char), w * h / 8);
    if (!image->data) {
        free(image);
        return nullptr;
    }
    
    image->bitWidth = 1;
    image->width = w;
    image->height = h;
    
    return image;
}

TImage *createPixmap(int w, int h, int bitWidth)
{
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image) {
        return nullptr;
    }
    
    image->data = (uint8_t *)calloc(sizeof(char), w * h * (bitWidth / 8));
    if (!image->data) {
        free(image);
        return nullptr;
    }
    
    image->bitWidth = bitWidth;
    image->width = w;
    image->height = h;
    
    return image;
}


void copyPixmap(const TImage *dst, int dx, int dy, const TImage *src, int x, int y, uint16_t w, uint16_t h)
{
    if (!dst || !src)
        return;
    
    if (!dst->data || !src->data)
        return;
    
    uint8_t *d = (uint8_t *)dst->data;
    uint8_t *s = (uint8_t *)src->data;
    
    d += dx + dy * (int)dst->width;
    s += x + y * (int)src->width;
    
    for (int j=0; j<h; j++) {
        for (int i=0; i<w; i++) {
            dst->data[i + dx + (j + dy) * dst->width] = src->data[i + x + (j + y) * src->width];
        }
        d += dst->width;
        s += src->width;
    }
}

TImage *convertMonochromeBitmapToPixmap(const TImage *monochrome)
{
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image)
        return nullptr;
    
    uint8_t *src = (uint8_t *)monochrome->data;
    uint8_t bitPosition = 1 << 7;
    
    image->bitWidth = 8;
    image->width = monochrome->width;
    image->height = monochrome->height;
    image->data = (uint8_t *)malloc(image->width * image->height);
    if (!image->data) return image;
    
    memset(image->data, 0, image->width * image->height);
    
    uint8_t *dest = (uint8_t *)image->data;
    
    int x, y;
    for (y=0; y<monochrome->height; y++) {
        bitPosition = 1 << 7;
        for (x=0; x<monochrome->width; x++) {
            *dest++ = (*src & bitPosition ? 1 : 0);
            if (bitPosition == 1) {
                src++;
                bitPosition = 1 << 7;
            } else {
                bitPosition >>= 1;
            }
        }
        if (x & 7) src++;
    }
    
    return image;
}

TImage *convertPixmapTo8BitPixmap(const TImage *pixmap)
{
    if (pixmap->bitWidth != 4)
        return nullptr;
    
    TImage *image = (TImage *)malloc(sizeof(TImage ));
    if (!image)
        return nullptr;
    
    uint8_t *src = (uint8_t *)pixmap->data;
    
    image->bitWidth = 8;
    image->width = pixmap->width;
    image->height = pixmap->height;
    image->data = (uint8_t *)malloc(image->width * image->height);
    if (!image->data) return image;
    
    memset(image->data, 0, image->width * image->height);
    
    uint8_t *dest = (uint8_t *)image->data;
    
    int length = pixmap->width * pixmap->height;
    
    while (length--) {
        uint8_t byte = *src++;
        if (pixmap->bitWidth == 4) {
            *dest++ = byte >> 4;
            *dest++ = byte & 15;
        }
        if (pixmap->bitWidth == 2) {
            *dest++ = byte >> 6;
            *dest++ = byte >> 4 & 3;
            *dest++ = byte >> 2 & 3;
            *dest++ = byte & 3;
        }
    }
    
    return image;
}

void convertPixmapTo8BitPixmapNoCopy(TImage *pixmap)
{
    if (pixmap->bitWidth != 4 && pixmap->bitWidth != 2)
        return;
    
    uint8_t* new_data = (uint8_t *)malloc(pixmap->width * pixmap->height);
    if (new_data == nullptr)
        return;
    
    uint8_t* dest = new_data;
    
    uint8_t *src = (uint8_t *)pixmap->data;
    
    int length = pixmap->width * pixmap->height;
    
    while (length--) {
        uint8_t byte = *src++;
        if (pixmap->bitWidth == 4) {
            *dest++ = byte >> 4;
            *dest++ = byte & 15;
        }
        if (pixmap->bitWidth == 2) {
            *dest++ = byte >> 6;
            *dest++ = byte >> 4 & 3;
            *dest++ = byte >> 2 & 3;
            *dest++ = byte & 3;
        }
    }
    
    free(pixmap->data);
    pixmap->data = new_data;
    pixmap->bitWidth = 8;
}

void reset(TImage *&image)
{
    if (image) {
        if (image->data) free(image->data);
        free(image);
        image = nullptr;
    }
}

bool containsImage(const TImage *image, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (!image || !image->data) return false;
    if (x + w > image->width || y + h > image->height) return false;
    uint8_t *p = (uint8_t *)image->data;
    
    p += x + y * image->width;
    while (h--) {
        for (int i=0; i<w; i++) {
            if (p[i]) return true;
        }
        p+=image->width;
    }
    return false;
}

TImage *extractImageSection(TImage *image)
{
    TImage *extractedImage = nullptr;
    extractedImage = extractImageSectionMasked(image, 0);
    return extractedImage;
}

TImage *extractImageSectionMasked(TImage *image, uint8_t maskColor)
{
    TImage *extractedImage = nullptr;
    
    int minX, maxX, minY, maxY;
    
    if (!image || !image->data) return nullptr;
    
    uint8_t *p = (uint8_t *)image->data;
    
    maxX = 0;
    maxY = 0;
    minX = image->width - 1;
    minY = image->height - 1;
    
    for (int y=0; y<image->height; y++) {
        for (int x=0; x<image->width; x++) {
            if (p[x + y * image->width] == maskColor) continue;
            if (minX > x) minX = x;
            if (maxX < x) maxX = x;
            if (minY > y) minY = y;
            if (maxY < y) maxY = y;
        }
    }
    
    if (maxX < minX || maxY < minY)
        return nullptr;
    
    
    int width = maxX - minX + 1;
    int height = maxY - minY + 1;
    
    extractedImage = createPixmap(width, height, image->bitWidth);
    if (!extractedImage) return nullptr;
    copyPixmap(extractedImage, 0, 0, image, minX, minY, width, height);
    
    return extractedImage;
}

TImage* scaleImage(const TImage *image, int scale) {
    if (image == nullptr || image->data == nullptr)
        return nullptr;
    
    if (image->bitWidth != 32)
        return nullptr;
    
    TImage *scaledImage = createPixmap(image->width * scale, image->height * scale, 32);
    if (scaledImage == nullptr)
        return nullptr;
    
    uint32_t* src = (uint32_t*)image->data;
    uint32_t* dest = (uint32_t*)scaledImage->data;
    
    float scale_x = (float)scale;
    float scale_y = (float)scale;
    
                     
    // Iterate through the bitmap and draw it scaled
    for (int y = 0; y < image->height; y++) {
        for (int x = 0; x < image->width; x++) {
            // Access pixel color data from the bitmap array
            uint32_t color = src[y * image->width + x];
            
            // Draw the pixel at the scaled position
            for (float sy = 0; sy < scale_y; sy += 1.0f) {
                for (float sx = 0; sx < scale_x; sx += 1.0f) {
                    // Calculate the scaled position
                    int posX = (int)(x * scale_x + sx);
                    int posY = (int)(y * scale_y + sy);
                    dest[posX + posY * scaledImage->width] = color;
                }
            }
        }
    }
    return scaledImage;
}

