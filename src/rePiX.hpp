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

#ifndef rePiX_hpp
#define rePiX_hpp

#include "image.hpp"
#include "ColorTable.hpp"

class rePiX {
public:
    const unsigned int& scale = _scale;
    unsigned width = 0;
    unsigned height = 0;
    unsigned margin = 0;
    
    ~rePiX() {
        reset(_originalImage);
        reset(_newImage);
    }
    
    bool isPixelatedImageLoaded(void) {
        return (_originalImage != nullptr && _originalImage->data != nullptr);
    }
    
    void loadPixelatedImage(std::string& imagefile) {
        _originalImage = loadPNGGraphicFile(imagefile);
    }
    
    void setBlockSize(const float value);
    void autoAdjustBlockSize(void);
    void setScale(const unsigned int scale);
    void setSamplePointSize(const unsigned size);
    void restorePixelatedImage(void);
    void postorize(const unsigned int levels);
    void normalizeColors(const float threshold);
    void normalizeColorsToColorTable(const ColorTable& colorTable);
    void applyOutline(void);
    void saveAs(std::string& filename);
    void applyScale(void);
    
private:
    TImage* _originalImage = nullptr;
    TImage* _newImage = nullptr;
    float _blockSize = 1.0;
    unsigned _scale = 1.0;
    unsigned _samplePointSize;
};

#endif /* rePiX_hpp */
