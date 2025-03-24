#include "PNGDisplay.h"
#include <SD.h>

static void PNGDraw(PNGDRAW *pDraw)
{
int *pInfo = (int *)pDraw->pUser;
uint16_t *p16;
BB_SPI_LCD *pLCD;
PNG *pPNG;
    
    pLCD = (BB_SPI_LCD *)pInfo[0]; // pointer to LCD class pointer
    pPNG = (PNG *)pInfo[1]; // pointer to PNG class pointe
    p16 = (uint16_t *)&pInfo[3]; // start of temporary pixel space
    pPNG->getLineAsRGB565(pDraw, p16, PNG_RGB565_BIG_ENDIAN, (uint32_t)pInfo[2]);
    pLCD->pushPixels(p16, pDraw->iWidth);
} /* PNGDraw() */

// Functions to access a file on the SD card
static File myfile;

static void * myOpen(const char *filename, int32_t *size) {
  myfile = SD.open(filename);
  *size = myfile.size();
  return &myfile;
}
static void myClose(void *handle) {
  if (myfile) myfile.close();
}
static int32_t myRead(PNGFILE *handle, uint8_t *buffer, int32_t length) {
  if (!myfile) return 0;
  return myfile.read(buffer, length);
}
static int32_t mySeek(PNGFILE *handle, int32_t position) {
  if (!myfile) return 0;
  return myfile.seek(position);
}

int PNGDisplay::loadPNG(BB_SPI_LCD *pLCD, int x, int y, const void *pData, int iDataSize, uint32_t bgColor)
{
PNG *png;
int w, h, rc;
uint32_t *png_info;

    png = (PNG *)malloc(sizeof(PNG));
    if (!png) return 0;
    rc = png->openRAM((uint8_t *)pData, iDataSize, PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png->getWidth();
        h = png->getHeight();
        if (x < 0 || w + x > pLCD->width() || y < 0 || y + h > pLCD->height()) {
        // clipping is not supported
            return 0;
        }
        png_info = (uint32_t *)malloc((w * sizeof(uint16_t)) + 3 * sizeof(int)); // enough for pixels and 3 32-bit values
        png_info[0] = (uint32_t)pLCD;
        png_info[1] = (uint32_t)png;
        png_info[2] = bgColor;
        pLCD->setAddrWindow(x, y, w, h);
        png->decode((void *)png_info, 0); // simple decode, no options
        png->close();
        free(png_info);
        free(png);
        return 1;
   }
   free(png);
   return 0;
} /* loadPNG() */

int PNGDisplay::loadPNG(BB_SPI_LCD *pLCD, int x, int y, const char *fname, uint32_t bgColor)
{
    PNG *png;
    int w, h, rc;
    uint32_t *png_info;

    png = (PNG *)malloc(sizeof(PNG));
    if (!png) return 0;
    rc = png->open(fname, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png->getWidth();
        h = png->getHeight();
        if (x < 0 || w + x > pLCD->width() || y < 0 || y + h > pLCD->height()) {
           // clipping is not supported
           return 0;
        }
        png_info = (uint32_t *)malloc((w * sizeof(uint16_t)) + 3 * sizeof(int)); // enough for pixels and 3 32-bit values
        png_info[0] = (uint32_t)pLCD;
        png_info[1] = (uint32_t)png;
        png_info[2] = bgColor;
        pLCD->setAddrWindow(x, y, w, h);
        png->decode((void *)png_info, 0); // simple decode, no options
        png->close();
        free(png_info);
        free(png);
        return 1;
    }
    free(png);
    return 0;
} /* loadPNG() */

int PNGDisplay::getPNGInfo(int *width, int *height, int *bpp, const void *pData, int iDataSize)
{
    PNG *png;
    int rc;

    if (!width || !height || !bpp || !pData || iDataSize < 32) return 0;
    
    png = (PNG *)malloc(sizeof(PNG));
    if (!png) return 0;
    rc = png->openRAM((uint8_t *)pData, iDataSize, PNGDraw);
    if (rc == PNG_SUCCESS) {
        *width = png->getWidth();
        *height = png->getHeight();
        *bpp = png->getBpp();
        free(png);
        return 1;
    }
    free(png);
    return 0;
} /* getPNGInfo() */

int PNGDisplay::getPNGInfo(int *width, int *height, int *bpp, const char *fname)
{
    PNG *png;
    int rc;

    if (!width || !height || !bpp || !fname) return 0;
    png = (PNG *)malloc(sizeof(PNG));
    if (!png) return 0;
    rc = png->open(fname, myOpen, myClose, myRead, mySeek, PNGDraw);
    if (rc == PNG_SUCCESS) {
        *width = png->getWidth();
        *height = png->getHeight();
        *bpp = png->getBpp();
        png->close();
        free(png);
        return 1;
    }
    free(png);
    return 0;
} /* getPNGInfo() */
