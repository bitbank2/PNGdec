//
// Copyright 2021 BitBank Software, Inc. All Rights Reserved.
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
#ifndef __PNGDEC__
#define __PNGDEC__
#if defined( __MACH__ ) || defined( __LINUX__ ) || defined( __MCUXPRESSO )
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#define memcpy_P memcpy
#define PROGMEM
#else
#include <Arduino.h>
#endif
//
// PNG Decoder
// Written by Larry Bank
// Copyright (c) 2021 BitBank Software, Inc.
// 
// Designed to decode most PNG images (1-32 bpp)
// using less than 40K of RAM
//
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif
/* Defines and variables */
#define FILE_HIGHWATER 1536
#define PNG_FILE_BUF_SIZE 2048
// max width (you can change it) of 320 x RGBA pixels
#define MAX_BUFFERED_PIXELS (320*4)
// source pixel type
enum {
  PNG_PIXEL_GRAYSCALE=0,
    PNG_PIXEL_TRUECOLOR=2,
    PNG_PIXEL_INDEXED=3,
    PNG_PIXEL_GRAY_ALPHA=4,
    PNG_PIXEL_TRUECOLOR_ALPHA=6
};
// Pixel types (defaults to little endian RGB565)
enum {
    NATIVE_PIXELS = 0, // whatever is encoded in the file
    RGB565_LITTLE_ENDIAN, // for sending to an LCD display
    RGB565_BIG_ENDIAN,
    EIGHT_BIT_PIXELS, // grayscale or palette colors
    INVALID_PIXEL_TYPE
};

enum {
    PNG_MEM_RAM=0,
    PNG_MEM_FLASH
};

// Error codes returned by getLastError()
enum {
    PNG_SUCCESS = 0,
    PNG_INVALID_PARAMETER,
    PNG_DECODE_ERROR,
    PNG_UNSUPPORTED_FEATURE,
    PNG_INVALID_FILE
};

typedef struct buffered_bits
{
unsigned char *pBuf; // buffer pointer
uint32_t ulBits; // buffered bits
uint32_t ulBitOff; // current bit offset
} BUFFERED_BITS;

typedef struct png_file_tag
{
  int32_t iPos; // current file position
  int32_t iSize; // file size
  uint8_t *pData; // memory file pointer
  void * fHandle; // class pointer to File/SdFat or whatever you want
} PNGFILE;

typedef struct png_draw_tag
{
    int x, y; // upper left corner of current MCU
    int iWidth, iHeight; // size of this MCU
    int iBpp; // bit depth of the pixels
    uint16_t *pPixels; // 16-bit pixels
} PNGDRAW;

// Callback function prototypes
typedef int32_t (PNG_READ_CALLBACK)(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen);
typedef int32_t (PNG_SEEK_CALLBACK)(PNGFILE *pFile, int32_t iPosition);
typedef void (PNG_DRAW_CALLBACK)(PNGDRAW *pDraw);
typedef void * (PNG_OPEN_CALLBACK)(const char *szFilename, int32_t *pFileSize);
typedef void (PNG_CLOSE_CALLBACK)(void *pHandle);

//
// our private structure to hold a JPEG image decode state
//
typedef struct png_image_tag
{
    int iWidth, iHeight; // image size
    int iXOffset, iYOffset; // placement on the display
    uint8_t ucBpp, ucSrcPixelType;
    uint8_t ucMemType, ucPixelType;
    uint32_t iTransparent; // transparent color index/value
    int iError;
    int iOptions;
    int iVLCOff; // current VLC data offset
    int iVLCSize; // current quantity of data in the VLC buffer
    PNG_READ_CALLBACK *pfnRead;
    PNG_SEEK_CALLBACK *pfnSeek;
    PNG_DRAW_CALLBACK *pfnDraw;
    PNG_OPEN_CALLBACK *pfnOpen;
    PNG_CLOSE_CALLBACK *pfnClose;
    PNGFILE PNGFile;
    BUFFERED_BITS bb;
    uint8_t ucPalette[1024];
    uint8_t ucPixels[MAX_BUFFERED_PIXELS * 2]; // current and previous lines
    uint8_t ucFileBuf[PNG_FILE_BUF_SIZE]; // holds temp data and pixel stack
} PNGIMAGE;

#ifdef __cplusplus
#define PNG_STATIC static
//
// The PNG class wraps portable C code which does the actual work
//
class PNG
{
  public:
    int openRAM(uint8_t *pData, int iDataSize, PNG_DRAW_CALLBACK *pfnDraw);
    int openFLASH(uint8_t *pData, int iDataSize, PNG_DRAW_CALLBACK *pfnDraw);
    int open(const char *szFilename, PNG_OPEN_CALLBACK *pfnOpen, PNG_CLOSE_CALLBACK *pfnClose, PNG_READ_CALLBACK *pfnRead, PNG_SEEK_CALLBACK *pfnSeek, PNG_DRAW_CALLBACK *pfnDraw);
    void close();
    int decode(int x, int y, int iOptions);
    int getWidth();
    int getHeight();
    int getBpp();
    int getLastError();
    void setPixelType(int iType); // defaults to little endian

  private:
    PNGIMAGE _png;
};
#else
#define PNG_STATIC
int PNG_openRAM(PNGIMAGE *pPNG, uint8_t *pData, int iDataSize, PNG_DRAW_CALLBACK *pfnDraw);
int PNG_openFile(PNGIMAGE *pPNG, const char *szFilename, PNG_DRAW_CALLBACK *pfnDraw);
int PNG_getWidth(PNGIMAGE *pPNG);
int PNG_getHeight(PNGIMAGE *pPNG);
int PNG_decode(PNGIMAGE *pPNG, int x, int y, int iOptions);
void PNG_close(PNGIMAGE *pPNG);
int PNG_getLastError(PNGIMAGE *pPNG);
int PNG_getBpp(PNGIMAGE *pPNG);
int PNG_getLastError(PNGIMAGE *pPNG);
void PNG_setPixelType(PNGIMAGE *pPNG, int iType); // defaults to little endian
#endif // __cplusplus

// Due to unaligned memory causing an exception, we have to do these macros the slow way
#define INTELSHORT(p) ((*p) + (*(p+1)<<8))
#define INTELLONG(p) ((*p) + (*(p+1)<<8) + (*(p+2)<<16) + (*(p+3)<<24))
#define MOTOSHORT(p) (((*(p))<<8) + (*(p+1)))
#define MOTOLONG(p) (((*p)<<24) + ((*(p+1))<<16) + ((*(p+2))<<8) + (*(p+3)))

// Must be a 32-bit target processor
#define REGISTER_WIDTH 32

#endif // __PNGDEC__
