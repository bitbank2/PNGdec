//
// PNG Display helper class
//
// written by Larry Bank
// bitbank@pobox.com
//
// Copyright 2025 BitBank Software, Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//    http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//===========================================================================
//
#ifndef __PNGDISPLAY__
#define __PNGDISPLAY__
#include <PNGdec.h>
#include <bb_spi_lcd.h>
#include <SD.h>
#include "FS.h"
#include <LittleFS.h>

// To center one or both coordinates for the drawing position
//  use this constant value
#define PNGDISPLAY_CENTER -2

class PNGDisplay
{
  public:
    int loadPNG(BB_SPI_LCD *pLCD, int x, int y, const void *pData, int iDataSize, uint32_t bgColor = 0xffffffff);
    int loadPNG(BB_SPI_LCD *pLCD, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff);
    int loadPNG_LFS(BB_SPI_LCD *pLCD, int x, int y, const char *fname, uint32_t bgColor = 0xffffffff);
    int getPNGInfo(int *width, int *height, int *bpp, const void *pData, int iDataSize);
    int getPNGInfo(int *width, int *height, int *bpp, const char *fname);
    int getPNGInfo_LFS(int *width, int *height, int *bpp, const char *fname);
    int getLastError() {return _iLastError;}
  private:
    int _iLastError;
};

#endif // __PNGDISPLAY__
