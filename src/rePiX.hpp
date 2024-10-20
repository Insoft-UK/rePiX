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

class rePiX {
public:
    const unsigned int& scale = _scale;
    
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
    void setBlockSize(float value);
    void setScale(unsigned int scale);
    void restorePixelatedImage(void);
    void postorize(unsigned int levels);
    void saveAs(std::string& filename);
    
private:
    TImage* _originalImage;
    TImage* _newImage;
    float _blockSize;
    unsigned int _scale;
};

#endif /* rePiX_hpp */
