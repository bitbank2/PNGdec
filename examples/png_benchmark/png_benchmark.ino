//
// PNG decoder benchmark
//
// Runs images through the PNG decoder without displaying the pixels
// to measure the execution time on various Arduino boards
//
#include <PNGdec.h>
#include "octocat_4bpp.h"
#include "octocat_8bpp.h"
#include "octocat_32bpp.h"

PNG png; // statically allocate the PNG structure (about 50K of RAM)

// simple private structure to pass a boolean value to the PNGDRAW callback
typedef struct myprivate
{
  bool bConvert;
} PRIVATE;

void PNGDraw(PNGDRAW *pDraw)
{
PRIVATE *pPriv = (PRIVATE *)pDraw->pUser;
uint16_t usPixels[240];

  if (pPriv->bConvert)
     png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_LITTLE_ENDIAN, 0xffffffff); // don't do alpha color blending
} /* PNGDraw() */

void setup() {
  Serial.begin(115200);
} /* setup() */

void loop() {
long lTime;
int rc, w, h;
PRIVATE priv;

// 32-bpp image
    rc = png.openFLASH((uint8_t *)octocat_32bpp, sizeof(octocat_32bpp), PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png.getWidth();
        h = png.getHeight();
        Serial.printf("Successfully opened octocat_32bpp.png, size = (%d x %d), %d bpp, pixel type: %d\n", w, h, png.getBpp(), png.getPixelType());
        priv.bConvert = false;
        lTime = micros();
        rc = png.decode((void *)&priv, 0);
        lTime = micros() - lTime;
        Serial.printf("Decode time for native pixels = %d us\n", (int)lTime); 
        priv.bConvert = true;
        lTime = micros();
        rc = png.decode((void *)&priv, 0);
        lTime = micros() - lTime;
        Serial.printf("Decode time for RGB565 pixels = %d us\n", (int)lTime); 
        png.close(); // not needed for memory->memory decode
    }

// 8-bpp image
    rc = png.openFLASH((uint8_t *)octocat_8bpp, sizeof(octocat_8bpp), PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png.getWidth();
        h = png.getHeight();
        Serial.printf("Successfully opened octocat_8bpp.png, size = (%d x %d), %d bpp, pixel type: %d\n", w, h, png.getBpp(), png.getPixelType());
        priv.bConvert = false;
        lTime = micros();
        rc = png.decode((void *)&priv, 0);
        lTime = micros() - lTime;
        Serial.printf("Decode time for native pixels = %d us\n", (int)lTime); 
        priv.bConvert = true;
        lTime = micros();
        rc = png.decode((void *)&priv, 0);
        lTime = micros() - lTime;
        Serial.printf("Decode time for RGB565 pixels = %d us\n", (int)lTime); 
        png.close(); // not needed for memory->memory decode
    }
// 4-bpp image
    rc = png.openFLASH((uint8_t *)octocat_4bpp, sizeof(octocat_4bpp), PNGDraw);
    if (rc == PNG_SUCCESS) {
        w = png.getWidth();
        h = png.getHeight();
        Serial.printf("Successfully opened octocat_4bpp.png, size = (%d x %d), %d bpp, pixel type: %d\n", w, h, png.getBpp(), png.getPixelType());
        priv.bConvert = false;
        lTime = micros();
        rc = png.decode((void *)&priv, 0);
        lTime = micros() - lTime;
        Serial.printf("Decode time for native pixels = %d us\n", (int)lTime); 
        priv.bConvert = true;
        lTime = micros();
        rc = png.decode((void *)&priv, 0);
        lTime = micros() - lTime;
        Serial.printf("Decode time for RGB565 pixels = %d us\n", (int)lTime); 
        png.close(); // not needed for memory->memory decode
    }
  delay(5000);
} /* loop() */
