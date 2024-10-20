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

#include "rePiX.hpp"

#include <string>

//MARK: - ColorSpace Type/s

typedef struct {
    float r; // Red component (0-1)
    float g; // Green component (0-1)
    float b; // Blue component (0-1)
} RGB;

typedef struct {
    float h; // Hue (0-360 degrees)
    float s; // Saturation (0-1)
    float v; // Value (0-1)
} HSV;

//MARK: - ColorSpace Function/s

static HSV rgbToHsv(RGB rgb) {
    HSV hsv;
    float r = rgb.r, g = rgb.g, b = rgb.b;
    float max = (r > g) ? (r > b ? r : b) : (g > b ? g : b); // Max RGB value
    float min = (r < g) ? (r < b ? r : b) : (g < b ? g : b); // Min RGB value
    float delta = max - min;

    // Compute Hue
    if (delta < 0.00001f) {
        hsv.h = 0; // Undefined hue
    } else if (max == r) {
        hsv.h = 60.0f * (fmod(((g - b) / delta), 6.0f));
    } else if (max == g) {
        hsv.h = 60.0f * (((b - r) / delta) + 2.0f);
    } else {
        hsv.h = 60.0f * (((r - g) / delta) + 4.0f);
    }

    if (hsv.h < 0) {
        hsv.h += 360.0f;
    }

    // Compute Saturation
    hsv.s = (max == 0) ? 0 : (delta / max);

    // Compute Value
    hsv.v = max;

    return hsv;
}

static RGB hsvToRgb(HSV hsv) {
    RGB rgb;
    float h = hsv.h, s = hsv.s, v = hsv.v;
    float c = v * s;   // Chroma
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1)); // Intermediate value
    float m = v - c;

    float r_prime, g_prime, b_prime;

    if (h >= 0 && h < 60) {
        r_prime = c;
        g_prime = x;
        b_prime = 0;
    } else if (h >= 60 && h < 120) {
        r_prime = x;
        g_prime = c;
        b_prime = 0;
    } else if (h >= 120 && h < 180) {
        r_prime = 0;
        g_prime = c;
        b_prime = x;
    } else if (h >= 180 && h < 240) {
        r_prime = 0;
        g_prime = x;
        b_prime = c;
    } else if (h >= 240 && h < 300) {
        r_prime = x;
        g_prime = 0;
        b_prime = c;
    } else {
        r_prime = c;
        g_prime = 0;
        b_prime = x;
    }

    // Adjust based on the lightness (add m)
    rgb.r = r_prime + m;
    rgb.g = g_prime + m;
    rgb.b = b_prime + m;

    return rgb;
}

static RGB argbToRgb(unsigned int argb) {
    RGB rgb;

    // Extract the RGB components from the ARGB value
    unsigned char r = (argb >> 16) & 0xFF; // Red (bits 16-23)
    unsigned char g = (argb >> 8) & 0xFF;  // Green (bits 8-15)
    unsigned char b = argb & 0xFF;         // Blue (bits 0-7)

    // Normalize the values to the range 0-1
    rgb.r = r / 255.0f;
    rgb.g = g / 255.0f;
    rgb.b = b / 255.0f;

    return rgb;
}

static unsigned int rgbToArgb(RGB rgb, unsigned char alpha) {
    // Convert the RGB values (0-1 range) back to 0-255 range
    unsigned char r = (unsigned char)(rgb.r * 255.0f);
    unsigned char g = (unsigned char)(rgb.g * 255.0f);
    unsigned char b = (unsigned char)(rgb.b * 255.0f);

    // Combine ARGB into a 32-bit integer
    unsigned int argb = (alpha << 24) | (r << 16) | (g << 8) | b;
    return argb;
}

//MARK: - Image Function/s

static void setImagePixel(const TImage* image, unsigned short x, unsigned short y, uint32_t color) {
    if (x >= image->width || y >= image->height) return;
    
    uint32_t* pixelData = (uint32_t*)image->data;
    pixelData[x + y * image->width] = color;
}

static uint32_t getImagePixel(const TImage* image, unsigned short x, unsigned short y) {
    if (x >= image->width || y >= image->height) return 0;
    
    uint32_t* pixelData = (uint32_t*)image->data;
    return pixelData[x + y * image->width];
}

// Function to reduce resolution of a single color channel (0-1 float)
static float postorize(float value, int levels) {
    float step = 1.0f / (levels - 1);  // Calculate step size for quantization
    return round(value / step) * step; // Quantize by reducing the range
}

// Function to reduce the resolution of RGB channels (0-1 range)
static RGB posterizeRgb(RGB rgb, int levels) {
    RGB reducedRgb;
    reducedRgb.r = postorize(rgb.r, levels);
    reducedRgb.g = postorize(rgb.g, levels);
    reducedRgb.b = postorize(rgb.b, levels);
    return reducedRgb;
}

static uint32_t blockColor(const TImage* image, int blockSize, int x, int y) {
    struct {
        union {
            uint32_t ARGB;
            struct {
                uint32_t R:8;
                uint32_t G:8;
                uint32_t B:8;
                uint32_t A:8;
            } channel;
        };
    } color;
    

    long r,g,b,a;
    
    r = g = b = a = 0;
    
    for (int i = 0; i < blockSize; ++i) {
        for (int j = 0; j < blockSize; ++j) {
            color.ARGB = getImagePixel(image, x + j, y + i);
            r += color.channel.R;
            g += color.channel.G;
            b += color.channel.B;
            a += color.channel.A;
        }
    }
    
    long pixelCount = blockSize * blockSize;
    color.channel.R = (uint32_t)(r /= pixelCount);
    color.channel.G = (uint32_t)(g /= pixelCount);
    color.channel.B = (uint32_t)(b /= pixelCount);
    color.channel.A = (uint32_t)(a /= pixelCount);
    
   
    return color.ARGB;
    
    
}

//MARK: - Method/s Implimentatin

void rePiX::setBlockSize(float value) {
    _blockSize = value < 1 ? 1 : value;
}

void rePiX::setScale(unsigned int scale) {
    _scale = scale < 1 ? 1 : scale;
}

void rePiX::restorePixelatedImage(void) {
    uint32_t color;
    
    _newImage = createPixmap(floor(_originalImage->width / _blockSize), floor(_originalImage->height / _blockSize), 32);
    if (_originalImage == nullptr || _originalImage->data == nullptr) return;
    
    for (float y = 0; y < _originalImage->height; y += _blockSize) {
        for (float x = 0; x < _originalImage->width; x += _blockSize) {
            if (_blockSize > 3.0) {
                color = blockColor(_originalImage, 2, x + _blockSize / 2 - 1, y + _blockSize / 2 - 1);
            }
            else {
                color = getImagePixel(_originalImage, x + _blockSize / 2, y + _blockSize / 2);
            }
            
            setImagePixel(_newImage, floor(x / _blockSize), floor(y / _blockSize), color);
        }
    }
    
    TImage* scaledImage = scaleImage(_newImage, _scale);
    reset(_newImage);
    _newImage = scaledImage;
}

void rePiX::postorize(unsigned int levels) {
    if (_newImage == nullptr || _newImage->data == nullptr) return;
    
    for (int y = 0; y < _newImage->height; ++y) {
        for (int x = 0; x < _newImage->width; ++x) {
            RGB rgb = argbToRgb(getImagePixel(_newImage, x, y));
            rgb = posterizeRgb(rgb, levels);
            setImagePixel(_newImage, x, y, rgbToArgb(rgb, 255));
        }
    }
}

void rePiX::saveAs(std::string& filename) {
    saveImageAsPNGFile(_newImage, filename);
}
