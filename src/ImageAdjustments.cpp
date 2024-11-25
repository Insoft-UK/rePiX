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

#include "ImageAdjustments.hpp"
#include <string>

typedef uint32_t Color;

typedef struct {
    float r;
    float g;
    float b;
} ColorRGB;

// Function to extract color components from ARGB value
static void getColorComponents(Color color, int* r, int* g, int* b) {
    *r = (color >> 16) & 0xFF;  // Red component
    *g = (color >> 8) & 0xFF;   // Green component
    *b = color & 0xFF;          // Blue component
}

// Function to calculate Euclidean distance between two ARGB colors
static unsigned colorDistance(Color color1, Color color2) {
    int r1, g1, b1;
    int r2, g2, b2;
    
    getColorComponents(color1, &r1, &g1, &b1);
    getColorComponents(color2, &r2, &g2, &b2);

    // Euclidean distance ignoring the alpha channel
    return sqrt((r1 - r2) * (r1 - r2) +
                (g1 - g2) * (g1 - g2) +
                (b1 - b2) * (b1 - b2));
}

static ColorRGB convertFromPackedRGB(const Color color) {
    ColorRGB colorRGB;
    
    // Extract the RGB components from the ARGB value
    unsigned char r = (color >> 16) & 0xFF; // Red (bits 16-23)
    unsigned char g = (color >> 8) & 0xFF;  // Green (bits 8-15)
    unsigned char b = color & 0xFF;         // Blue (bits 0-7)
    
    // Normalize the values to the range 0-1
    colorRGB.r = r / 255.0f;
    colorRGB.g = g / 255.0f;
    colorRGB.b = b / 255.0f;
    
    return colorRGB;
}

static Color convertToPackedRGB(const ColorRGB colorRGB, float alpha) {
    // Convert the RGB values (0-1 range) back to 0-255 range
    Color r = (Color)(colorRGB.r * 255.0f);
    Color g = (Color)(colorRGB.g * 255.0f);
    Color b = (Color)(colorRGB.b * 255.0f);
    Color a = (Color)(alpha * 255.0f);
    
    // Combine ARGB into a 32-bit integer
    return a << 24 | r << 16 | g << 8 | b;
}

// Function to reduce resolution of a single color channel (0-1 float)
static float postorizeChannel(float value, unsigned levels) {
    float step = 1.0f / (float)(levels - 1);
    return round(value / step) * step;
}

// Function to reduce the resolution of RGB channels (0-1 range)
static ColorRGB posterizeRGB(ColorRGB colorRGB, unsigned levels) {
    colorRGB.r = postorizeChannel(colorRGB.r, levels);
    colorRGB.g = postorizeChannel(colorRGB.g, levels);
    colorRGB.b = postorizeChannel(colorRGB.b, levels);
    return colorRGB;
}


void ImageAdjustments::postorize(const void* pixels, long length, unsigned levels) {
    uint32_t* color = (uint32_t*)pixels;
    
    for (long i = 0; i < length; ++i) {
        ColorRGB colorRGB = convertFromPackedRGB(*color);
        colorRGB = posterizeRGB(colorRGB, levels);
        *color = convertToPackedRGB(colorRGB, 1.0);
        color++;
    }
}

void ImageAdjustments::normalizeColors(const void* pixels, int w, int h, unsigned threshold) {
    typedef struct {
        Color baseColor;
        int count;
    } Bucket;
    
    Bucket* buckets = (Bucket *)calloc(256 * 256 * 256, sizeof(Bucket)); // Max size for all RGB combinations
    Color* colors = (Color *)pixels;
    
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = x + y * w;
            Color baseColor = colors[i];
            uint32_t key = baseColor & 0xFFFFFF;
            if (buckets[key].count >= 1) continue;
            
            buckets[key].count++;
            buckets[key].baseColor = baseColor;
            
            for (int j = 0; j < w * h; j++) {
                if (colorDistance(baseColor, colors[j]) < threshold && j != i) {
                    colors[j] = baseColor;
                    buckets[key].count++;
                }
            }
        }
    }

    free(buckets);
}

void ImageAdjustments::mapColorsToNearestPalette(const void* pixels, int w, int h, const uint32_t* palt, int paletteSize, int transparencyIndex) {
    Color* colors = (Color *)pixels;
    
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int i = x + y * w;
            Color baseColor = colors[i];
            Color matchedColor = baseColor;
            
            int distance = 256;
            for (int n = 0; n < paletteSize; ++n) {
                if (colorDistance(baseColor, palt[n]) < distance) {
                    distance = colorDistance(baseColor, palt[n]);
                    matchedColor = palt[n];
                }
            }
            if (transparencyIndex >= 0) {
                if (matchedColor == palt[transparencyIndex]) matchedColor = 0;
            }
            colors[i] = matchedColor;
        }
    }
}

void ImageAdjustments::applyOutline(const void* pixels, int w, int h) {
    Color color;
    Color* colors = (Color *)pixels;
    
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            color = colors[x + y * w];
            if (!color) {
                continue;
            }
            
            if (color == 0xFF000000) {
                continue;
            }
            
            if (x > 0) {
                if (colors[x + y * w - 1] == 0) {
                    colors[x + y * w - 1] = 0xFF000000;
                }
            }
            
            if (x < w - 1) {
                if (colors[x + y * w + 1] == 0) {
                    colors[x + y * w + 1] = 0xFF000000;
                }
            }
            
            if (y > 0) {
                if (colors[x + y * w - w] == 0) {
                    colors[x + y * w - w] = 0xFF000000;
                }
            }
            
            if (y < h - 1) {
                if (colors[x + y * w + w] == 0) {
                    colors[x + y * w + w] = 0xFF000000;
                }
            }
        }
    }
}

