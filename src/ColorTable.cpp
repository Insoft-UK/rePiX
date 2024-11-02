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

#include "ColorTable.hpp"

#include <fstream>

typedef struct __attribute__((__packed__)) {
    struct {
        uint8_t r, g, b;
    } colors[256];
    
    int16_t defined;
    int16_t transparency;
} AdobeColorTable;

template <typename T> static T swap_endian(T u) {
    static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        dest.u8[k] = source.u8[sizeof(T) - k - 1];

    return dest.u;
}

void ColorTable::loadAdobeColorTable(const char* filename) {
    AdobeColorTable adobeColorTable;
    std::ifstream infile;
    
    infile.open(filename, std::ios::in | std::ios::binary);
    if (!infile.is_open()) {
        return;
    }
    
    infile.read((char *)&adobeColorTable, sizeof(AdobeColorTable));
    infile.close();
    
    _defined = adobeColorTable.defined;
    _transparency = adobeColorTable.transparency;
#ifdef __LITTLE_ENDIAN__
    _defined = swap_endian(_defined);
    _transparency = swap_endian(_transparency);
#endif
    
    for (int n = 0; n < _defined; n++) {
        uint32_t color = (uint32_t)adobeColorTable.colors[n].r << 24 | (uint32_t)adobeColorTable.colors[n].g << 16 | (uint32_t)adobeColorTable.colors[n].b << 8 | 255;
#ifdef __LITTLE_ENDIAN__
        color = swap_endian(color);
#endif
        _colors[n] = color;
    }
}
