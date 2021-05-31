#include <PNGdec.h>
#include <bb_spi_lcd.h>
#include <Wire.h>

//#include "snaponair.h"
#include "m5logosmall.h"

PNG png;
SPILCD lcd;
static uint8_t ucTXBuf[1024];
#define TFT_CS 5
#define TFT_RST 18
#define TFT_DC 23
#define TFT_CLK 13
#define TFT_MOSI 15
#define BUTTON_A 37
#define BUTTON_B 39

void Write1Byte( uint8_t Addr ,  uint8_t Data )
{   
    Wire1.beginTransmission(0x34);
    Wire1.write(Addr);
    Wire1.write(Data);
    Wire1.endTransmission();
}
uint8_t Read8bit( uint8_t Addr )
{
    Wire1.beginTransmission(0x34);
    Wire1.write(Addr);
    Wire1.endTransmission();
    Wire1.requestFrom(0x34, 1);
    return Wire1.read();
}
void AxpBrightness(uint8_t brightness)
{
    if (brightness > 12)
    {
        brightness = 12;
    }
    uint8_t buf = Read8bit( 0x28 );
    Write1Byte( 0x28 , ((buf & 0x0f) | (brightness << 4)) );
}
void AxpPowerUp()
{
    Wire1.begin(21, 22);
    Wire1.setClock(400000);
    // Set LDO2 & LDO3(TFT_LED & TFT) 3.0V
    Write1Byte(0x28, 0xcc);

    // Set ADC sample rate to 200hz
    Write1Byte(0x84, 0b11110010);

    // Set ADC to All Enable
    Write1Byte(0x82, 0xff);

    // Bat charge voltage to 4.2, Current 100MA
    Write1Byte(0x33, 0xc0);

    // Depending on configuration enable LDO2, LDO3, DCDC1, DCDC3.
    byte buf = (Read8bit(0x12) & 0xef) | 0x4D;
//    if(disableLDO3) buf &= ~(1<<3);
//    if(disableLDO2) buf &= ~(1<<2);
//    if(disableDCDC3) buf &= ~(1<<1);
//    if(disableDCDC1) buf &= ~(1<<0);
    Write1Byte(0x12, buf);
     // 128ms power on, 4s power off
    Write1Byte(0x36, 0x0C);

    if (1) //if(!disableRTC)
    {
        // Set RTC voltage to 3.3V
        Write1Byte(0x91, 0xF0);

        // Set GPIO0 to LDO
        Write1Byte(0x90, 0x02);
    }

    // Disable vbus hold limit
    Write1Byte(0x30, 0x80);

    // Set temperature protection
    Write1Byte(0x39, 0xfc);

    // Enable RTC BAT charge
//    Write1Byte(0x35, 0xa2 & (disableRTC ? 0x7F : 0xFF));
    Write1Byte(0x35, 0xa2);
     // Enable bat detection
    Write1Byte(0x32, 0x46);

    // Set Power off voltage 3.0v
    Write1Byte(0x31 , (Read8bit(0x31) & 0xf8) | (1 << 2));

} /* AxpPowerUp() */

void PNGDraw(PNGDRAW *pDraw)
{
uint16_t usPixels[320];

  png.getLineAsRGB565(pDraw, usPixels, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  spilcdSetPosition(&lcd, 0, pDraw->y, pDraw->iWidth, 1, DRAW_TO_LCD);
  spilcdWriteDataBlock(&lcd, (uint8_t *)usPixels, pDraw->iWidth*2, DRAW_TO_LCD | DRAW_WITH_DMA);
} /* PNGDraw() */

void setup() {
  AxpPowerUp();
  AxpBrightness(7); // turn on backlight (0-12)
  spilcdSetTXBuffer(ucTXBuf, sizeof(ucTXBuf));
  spilcdInit(&lcd, LCD_ST7789_135, FLAGS_NONE, 40000000, TFT_CS, TFT_DC, TFT_RST, -1, -1, TFT_MOSI, TFT_CLK); // M5Stick-C plus pin numbering, 40Mhz
  spilcdSetOrientation(&lcd, LCD_ORIENTATION_90);
  spilcdFill(&lcd, 0, DRAW_TO_LCD);
  spilcdWriteString(&lcd, 0, 4, (char *)"PNG Test", 0xffff, 0, FONT_12x16, DRAW_TO_LCD);
  Serial.begin(115200);
  delay(2000);
}

void loop() {
  int rc;
  ulong ulTime;
    spilcdFill(&lcd, 0, DRAW_TO_LCD);
    rc = png.openRAM((uint8_t *)m5logosmall, sizeof(m5logosmall), PNGDraw);
//    rc = png.openRAM((uint8_t *)snaponair, sizeof(snaponair), PNGDraw);
    if (rc == PNG_SUCCESS) {
        Serial.printf("image specs: (%d x %d), %d bpp, pixel type: %d\n", png.getWidth(), png.getHeight(), png.getBpp(), png.getPixelType());
        ulTime = millis();
        rc = png.decode(NULL, 0);
        ulTime = millis() - ulTime;
        png.close();
        Serial.printf("Decode time = %dms\n", (int)ulTime);
    }
    delay(2000);
} /* loop() */
