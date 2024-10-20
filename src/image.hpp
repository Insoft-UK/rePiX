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


#pragma once

#include <iostream>
#include <sstream>
#include <stdint.h>

typedef struct __attribute__((__packed__)) {
    uint16_t width;
    uint16_t height;
    uint8_t  bitWidth;
    uint8_t *data;
} TImage;

/**
 @brief    Loads a file in the Portable Network Graphic (PNG) format.
 @param    filename The filename of the Portable Network Graphic (PNG) to be loaded.
 @return   A structure containing the image data.
 */
TImage *loadPNGGraphicFile(const std::string& filename);

/**
 @brief    Loads a file in the Bitmap (BMP) format.
 @param    filename The filename of the Bitmap (BMP) to be loaded.
 @return   A structure containing the image data.
 */
TImage *loadBMPGraphicFile(const std::string& filename);

/**
 @brief    Loads a file in the Portable Bitmap (PBM) format.
 @param    filename The filename of the Portable Bitmap (PBM) to be loaded.
 @return   A structure containing the image data.
 */
TImage *loadPBMGraphicFile(const std::string& filename);

/**
 @brief    Saves a file in the Portable Network Graphic (PNG) format.
 @param    image The image.
 @param    filename The filename of the Portable Network Graphic (PNG) to be loaded.
 @return   A true on success.
 */
bool saveImageAsPNGFile(TImage* image, const std::string& filename);

/**
 @brief    Creates a bitmap with the specified dimensions.
 @param    w The width of the bitmap.
 @param    h The height of the bitmap.
 @return   A structure containing the bitmap image data.
 */
TImage *createBitmap(int w, int h);

/**
 @brief    Creates a pixmap with the specified dimensions.
 @param    w The width of the pixmap.
 @param    h The height of the pixmap.
 @param    bitWidth The bit width of the bitmap.
 @return   A structure containing the pixmap image data.
 */
TImage *createPixmap(int w, int h, int bitWidth);

/**
 @brief    Copies a section of a pixmap to another bitmap.
 @param    dst The pixmap to which the section will be copied.
 @param    dx The horizontal position where the copied section will be placed within the destination pixmap.
 @param    dy The vertical axis position within the destination pixmap where the copied section will be placed.
 @param    src The pixmap from which the section will be copied.
 @param    x The x-axis from which the pixmap will be copied.
 @param    y The y-axis source of the pixmap to be copyed from.
 @param    w The width of the pixmap to be copied.
 @param    h The height of the pixmap to be copied.
 */
void copyPixmap(const TImage* dst, int dx, int dy, const TImage* src, int x, int y, uint16_t w, uint16_t h);

/**
 @brief    Converts a monochrome bitmap to a pixmap, where each pixel is represented by a single byte.
 @param    monochrome The monochrome bitmap to be converted to a pixmap bitmap.
 @return   A structure containing the pixmap image data.
 */
TImage *convertMonochromeBitmapToPixmap(const TImage* monochrome);

/**
 @brief    Converts a 4-bit pixmap to a 8-bit pixmap, where each pixel is represented by a single byte.
 @param    pixmap The 4-bit pixmap to be converted to a 8-bit pixmap bitmap.
 @return   A structure containing the new pixmap image data.
 */
TImage *convertPixmapTo8BitPixmap(const TImage* pixmap);

/**
 @brief    Converts a 4-bit pixmap to a 8-bit pixmap, where each pixel is represented by a single byte.
 @param    pixmap The 4-bit pixmap to be converted to a 8-bit pixmap bitmap.
 */
void convertPixmapTo8BitPixmapNoCopy(TImage* pixmap);

/**
 @brief    Frees the memory allocated for the image.
 @param    image The image to be deallocated.
 */
void reset(TImage* &image);

/**
 @brief    Takes an input image and identifies if the image contains an actual image at the specified section.
 @param    image The input image that is to be inspected.
 @param    x The start x-axis position from which the pixmap will be inspected at.
 @param    y The start y-axis position from which the pixmap will be inspected at.
 @param    w The width of the pixmap section to inspect.
 @param    h The height of the pixmap section to inspect.
 */
bool containsImage(const TImage* image, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 @brief    Takes an input image and identifies and extracts a section of the image that contains an actual image.
 @param    image The input image from which a section containing an actual image will be extracted.
 */
TImage *extractImageSection(TImage* image);

/**
 @brief    Takes an input image and identifies and extracts a section of the image that contains an actual image.
 @param    image The input image from which a section containing an actual image will be extracted.
 @param    maskColor Color that should be treated as transparent.
 */
TImage *extractImageSectionMasked(TImage* image, uint8_t maskColor);

/**
 @brief    Takes an input image and identifies and extracts a section of the image that contains an actual image.
 @param    image The input image from which a section containing an actual image will be grabed.
 @param    maskColor Color that should be treated as transparent.
 @param    x The start x-axis position from which the pixmap will grabed at.
 @param    y The start y-axis position from which the pixmap will be grabed at.
 @param    w The width of the pixmap section to grab.
 @param    h The height of the pixmap section to grab.
 */
TImage *grabImageSectionMasked(TImage* image, uint8_t maskColor, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

TImage* scaleImage(const TImage *image, int scale);

