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

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <fstream>
#include <array>

#include "image.hpp"

#include "build.h"

bool verbose = false;

enum class MessageType {
    Warning,
    Error,
    Verbose
};

std::ostream& operator<<(std::ostream& os, MessageType type) {
    switch (type) {
        case MessageType::Error:
            os << R"(\e[1;91mError\e[0m: )";
            break;

        case MessageType::Warning:
            os << R"(\e[1;93mWarning\e[0m: )";
            break;
            
        case MessageType::Verbose:
            os << ": ";
            break;

        default:
            os << ": ";
            break;
    }

    return os;
}

/*
 The decimalToBase24 function converts a given 
 base 10 integer into its base 24 representation using a
 specific set of characters. The character set is
 comprised of the following 24 symbols:

     •    Numbers: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
     •    Letters: C, D, F, H, J, K, M, N, R, U, V, W, X, Y
     
 Character Selection:
 The choice of characters was made to avoid confusion
 with common alphanumeric representations, ensuring
 that each character is visually distinct and easily
 recognizable. This set excludes characters that closely
 resemble each other or numerical digits, promoting
 clarity in representation.
 */
static std::string decimalToBase24(int num) {
    if (num == 0) {
        return "C";
    }

    const std::string base24Chars = "0123456789CDFHJKMNRUVWXY";
    std::string base24;

    while (num > 0) {
        int remainder = num % 24;
        base24 = base24Chars[remainder] + base24; // Prepend character
        num /= 24; // Integer division
    }

    return base24;
}

static std::string getBuildCode(void) {
    std::string str;
    int majorVersionNumber = BUILD_NUMBER / 100000;
    str = std::to_string(majorVersionNumber) + decimalToBase24(BUILD_NUMBER - majorVersionNumber * 100000);
    return str;
}

void help(void)
{
    int rev = BUILD_NUMBER / 1000 % 10;
    
    std::cout << "Copyright (C) 2024 Insoft. All rights reserved.\n";
    std::cout << "Insoft rePix version, " << BUILD_NUMBER / 100000 << "." << BUILD_NUMBER / 10000 % 10 << (rev ? "." + std::to_string(rev) : "")
    << " (BUILD " << getBuildCode() << "-" << decimalToBase24(BUILD_DATE) << ")\n\n";
    std::cout << "Usage: repix <input-file> [-o <output-file>] [-b <size>] [-s <scale>] [-p <levels>]\n\n";
    std::cout << "Options:\n";
    std::cout << "    -o  <output-file>        Specify the filename for repixilated image.\n";
    std::cout << "    -b  <size>               Specify the block size.\n";
    std::cout << "    -s  <scale>              Specify the scale factor for output image.\n";
    std::cout << "    -p  <levels>             Posterize.\n";
    std::cout << "\n";
    std::cout << "Additional Commands:\n";
    std::cout << "  repix {-version | -help}\n";
    std::cout << "    -version                 Display the version information.\n";
    std::cout << "    -help                    Show this help message.\n";
}

void version(void) {
    std::cout << "Copyright (C) 2024 Insoft. All rights reserved.\n";
    std::cout << "Insoft rePix version, " << BUILD_NUMBER / 100000 << "." << BUILD_NUMBER / 10000 % 10 << "." << BUILD_NUMBER / 1000 % 10
    << " (BUILD " << getBuildCode() << ")\n";
    std::cout << "Built on: " << CURRENT_DATE << "\n";
    std::cout << "Licence: MIT License\n\n";
    std::cout << "For more information, visit: http://www.insoft.uk\n";
}

void error(void)
{
    std::cout << "repix: try 'repix -help' for more information\n";
    exit(0);
}

void info(void) {
    std::cout << "Copyright (c) 2024 Insoft. All rights reserved.\n";
    int rev = BUILD_NUMBER / 1000 % 10;
    std::cout << "rePix version, " << BUILD_NUMBER / 100000 << "." << BUILD_NUMBER / 10000 % 10 << (rev ? "." + std::to_string(rev) : "")
    << " (BUILD " << getBuildCode() << "-" << decimalToBase24(BUILD_DATE) << ")\n\n";
}

bool fileExists(const std::string& filename) {
    std::ofstream outfile;
    outfile.open(filename, std::ios::in | std::ios::binary);
    if(!outfile.is_open()) {
        return false;
    }
    outfile.close();
    return true;
}

std::string removeExtension(const std::string& filename) {
    // Find the last dot in the string
    size_t lastDotPosition = filename.find_last_of('.');

    // If there is no dot, return the original string
    if (lastDotPosition == std::string::npos) {
        return filename;
    }

    // Return the substring from the beginning up to the last dot
    return filename.substr(0, lastDotPosition);
}

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

HSV rgbToHsv(RGB rgb) {
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

RGB hsvToRgb(HSV hsv) {
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

RGB argbToRgb(unsigned int argb) {
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

unsigned int rgbToArgb(RGB rgb, unsigned char alpha) {
    // Convert the RGB values (0-1 range) back to 0-255 range
    unsigned char r = (unsigned char)(rgb.r * 255.0f);
    unsigned char g = (unsigned char)(rgb.g * 255.0f);
    unsigned char b = (unsigned char)(rgb.b * 255.0f);

    // Combine ARGB into a 32-bit integer
    unsigned int argb = (alpha << 24) | (r << 16) | (g << 8) | b;
    return argb;
}

// Function to reduce resolution of a single color channel (0-1 float)
float postorize(float value, int levels) {
    float step = 1.0f / (levels - 1);  // Calculate step size for quantization
    return round(value / step) * step; // Quantize by reducing the range
}

// Function to reduce the resolution of RGB channels (0-1 range)
RGB posterizeRgb(RGB rgb, int levels) {
    RGB reducedRgb;
    reducedRgb.r = postorize(rgb.r, levels);
    reducedRgb.g = postorize(rgb.g, levels);
    reducedRgb.b = postorize(rgb.b, levels);
    return reducedRgb;
}


void setImagePixel(const TImage* image, unsigned short x, unsigned short y, uint32_t color) {
    if (x >= image->width || y >= image->height) return;
    
    uint32_t* pixelData = (uint32_t*)image->data;
    pixelData[x + y * image->width] = color;
}

uint32_t getImagePixel(const TImage* image, unsigned short x, unsigned short y) {
    if (x >= image->width || y >= image->height) return 0;
    
    uint32_t* pixelData = (uint32_t*)image->data;
    return pixelData[x + y * image->width];
}

void posterizeImage(const TImage* image, int levels) {
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            RGB rgb = argbToRgb(getImagePixel(image, x, y));
            rgb = posterizeRgb(rgb, levels);
            setImagePixel(image, x, y, rgbToArgb(rgb, 255));
        }
    }
}

uint32_t blockColor(const TImage* image, int blockSize, int x, int y) {
    typedef struct {
        union {
            uint32_t ARGB;
            struct {
                uint32_t R:8;
                uint32_t G:8;
                uint32_t B:8;
                uint32_t A:8;
            } channel;
        };
    } TColor;
    
    
    TColor color;
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

TImage* restorePixelatedImage(const TImage* image, float blockSize) {
    uint32_t color;
    
    TImage* restored;
    restored = createPixmap(floor(image->width / blockSize), floor(image->height / blockSize), 32);
    if (restored == nullptr) return nullptr;
    
    for (float y = 0; y < image->height; y += blockSize) {
        for (float x = 0; x < image->width; x += blockSize) {
            if (blockSize > 3.0) {
                color = blockColor(image, 2, x + blockSize / 2 - 1, y + blockSize / 2 - 1);
            }
            else {
                color = getImagePixel(image, x + blockSize / 2, y + blockSize / 2);
            }
            
            setImagePixel(restored, floor(x / blockSize), floor(y / blockSize), color);
        }
    }
    
    return restored;
}

int main(int argc, const char * argv[])
{
    if ( argc == 1 ) {
        error();
        return 0;
    }
    
    std::string out_filename, in_filename;
    float blockSize = 1;
    int levels = 255;
    int scale = 1;
    
    for( int n = 1; n < argc; n++ ) {
        if (*argv[n] == '-') {
            std::string args(argv[n]);
            
            if (args == "-o") {
                if (++n > argc) error();
                out_filename = argv[n];
                continue;
            }
            
            if (args == "-b") {
                if (++n > argc) error();
                blockSize = atof(argv[n]);
                continue;
            }
            
            if (args == "-p") {
                if (++n > argc) error();
                levels = atoi(argv[n]);
                continue;
            }
            
            if (args == "-s") {
                if (++n > argc) error();
                scale = atoi(argv[n]);
                continue;
            }
            
            if (args == "-help") {
                help();
                return 0;
            }
            
            if (args == "-version") {
                version();
                return 0;
            }
            
            error();
            return 0;
        }
        in_filename = argv[n];
    }
    
    info();
    
    if (!fileExists(in_filename)) {
        std::cout << MessageType::Error << "File '" << in_filename << "' not found.\n";
        return -1;
    }
    
    if (out_filename.empty() || out_filename == in_filename) {
        out_filename = removeExtension(in_filename) + "@" + std::to_string(scale) + "x.png";
    }
    

    
    TImage* image;
    image = loadPNGGraphicFile(in_filename);
    if (image == nullptr) {
        std::cout << MessageType::Error << "File '" << in_filename << "' failed to load.\n";
        return -1;
    }
    
    TImage* restoredImage = restorePixelatedImage(image, blockSize);
    
    if (restoredImage != nullptr) {
        posterizeImage(restoredImage, levels);
        if (scale > 1) {
            TImage* scaledImage = scaleImage(restoredImage, scale);
            if (scaledImage != nullptr) {
                saveImageAsPNGFile(scaledImage, out_filename);
                reset(scaledImage);
            }
        } else {
            saveImageAsPNGFile(restoredImage, out_filename);
        }
        
        reset(restoredImage);
    }
    
    reset(image);
    
    return 0;
}

