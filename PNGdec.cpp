//
// PNG Decoder
//
// written by Larry Bank
// bitbank@pobox.com
// Arduino port started 8/2/2020
// Original PNG code written 26+ years ago :)
// The goal of this code is to decode baseline PNG images
// using no more than 18K of RAM (if sent directly to an LCD display)
//
// Copyright 2020 BitBank Software, Inc. All Rights Reserved.
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
#include "PNGdec.h"

// forward references
PNG_STATIC int PNGInit(PNGIMAGE *pPNG);
PNG_STATIC int PNGParseInfo(PNGIMAGE *pPage, int bExtractThumb);
PNG_STATIC void PNGGetMoreData(PNGIMAGE *pPage);
PNG_STATIC int DecodePNG(PNGIMAGE *pImage);

// Include the C code which does the actual work
#include "png.inl"

void PNG::setPixelType(int iType)
{
    if (iType >= 0 && iType < INVALID_PIXEL_TYPE)
        _png.ucPixelType = (uint8_t)iType;
    else
        _png.iError = PNG_INVALID_PARAMETER;
} /* setPixelType() */

//
// Memory initialization
//
int PNG::openRAM(uint8_t *pData, int iDataSize, PNG_DRAW_CALLBACK *pfnDraw)
{
    memset(&_png, 0, sizeof(PNGIMAGE));
    _png.ucMemType = PNG_MEM_RAM;
    _png.pfnRead = readRAM;
    _png.pfnSeek = seekMem;
    _png.pfnDraw = pfnDraw;
    _png.pfnOpen = NULL;
    _png.pfnClose = NULL;
    _png.PNGFile.iSize = iDataSize;
    _png.PNGFile.pData = pData;
    return PNGInit(&_png);
} /* openRAM() */

int PNG::openFLASH(uint8_t *pData, int iDataSize, PNG_DRAW_CALLBACK *pfnDraw)
{
    memset(&_png, 0, sizeof(PNGIMAGE));
    _png.ucMemType = PNG_MEM_FLASH;
    _png.pfnRead = readFLASH;
    _png.pfnSeek = seekMem;
    _png.pfnDraw = pfnDraw;
    _png.pfnOpen = NULL;
    _png.pfnClose = NULL;
    _png.PNGFile.iSize = iDataSize;
    _png.PNGFile.pData = pData;
    return PNGInit(&_png);
} /* openRAM() */

int PNG::getLastError()
{
    return _png.iError;
} /* getLastError() */

int PNG::getWidth()
{
    return _png.iWidth;
} /* getWidth() */

int PNG::getHeight()
{
    return _png.iHeight;
} /* getHeight() */

int PNG::getBpp()
{
    return (int)_png.ucBpp;
} /* getBpp() */

//
// File (SD/MMC) based initialization
//
int PNG::open(const char *szFilename, PNG_OPEN_CALLBACK *pfnOpen, PNG_CLOSE_CALLBACK *pfnClose, PNG_READ_CALLBACK *pfnRead, PNG_SEEK_CALLBACK *pfnSeek, PNG_DRAW_CALLBACK *pfnDraw)
{
    memset(&_png, 0, sizeof(PNGIMAGE));
    _png.pfnRead = pfnRead;
    _png.pfnSeek = pfnSeek;
    _png.pfnDraw = pfnDraw;
    _png.pfnOpen = pfnOpen;
    _png.pfnClose = pfnClose;
    _png.PNGFile.fHandle = (*pfnOpen)(szFilename, &_png.PNGFile.iSize);
    if (_png.PNGFile.fHandle == NULL)
       return 0;
    return PNGInit(&_png);

} /* open() */

void PNG::close()
{
    if (_png.pfnClose)
        (*_png.pfnClose)(_png.PNGFile.fHandle);
} /* close() */

//
// Decode the image
// returns:
// 1 = good result
// 0 = error
//
int PNG::decode(int x, int y, int iOptions)
{
    _png.iXOffset = x;
    _png.iYOffset = y;
    _png.iOptions = iOptions;
    return DecodePNG(&_png);
} /* decode() */
