//
// PNGdec implementation
//
//#include "miniz.h"
#include "zlib.h"
//
// Convert a line of pixels into RGB565
//
PNG_STATIC void PNGRGB565(PNGDRAW *pDraw, uint16_t *pPixels, int iEndiannes)
{
    int x;
    uint16_t usPixel, *pDest = pPixels;
    uint8_t *s = pDraw->pPixels;
    
    switch (pDraw->iPixelType) {
        case 6: // truecolor + alpha
            for (x=0; x<pDraw->iWidth; x++) {
                usPixel = (s[0] >> 3); // blue
                usPixel |= ((s[1] >> 2) << 5); // green
                usPixel |= ((s[2] >> 3) << 11); // red
                *pDest++ = usPixel;
                s += 4; // skip alpha
            }
            break;
    }
} /* PNGRGB565() */
//
// Helper functions for memory based images
//
PNG_STATIC int32_t seekMem(PNGFILE *pFile, int32_t iPosition)
{
    if (iPosition < 0) iPosition = 0;
    else if (iPosition >= pFile->iSize) iPosition = pFile->iSize-1;
    pFile->iPos = iPosition;
    return iPosition;
} /* seekMem() */

PNG_STATIC int32_t readFLASH(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;

    iBytesRead = iLen;
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos;
    if (iBytesRead <= 0)
       return 0;
    memcpy_P(pBuf, &pFile->pData[pFile->iPos], iBytesRead);
    pFile->iPos += iBytesRead;
    return iBytesRead;
} /* readFLASH() */

PNG_STATIC int32_t readRAM(PNGFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
    int32_t iBytesRead;

    iBytesRead = iLen;
    if ((pFile->iSize - pFile->iPos) < iLen)
       iBytesRead = pFile->iSize - pFile->iPos;
    if (iBytesRead <= 0)
       return 0;
    memcpy(pBuf, &pFile->pData[pFile->iPos], iBytesRead);
    pFile->iPos += iBytesRead;
    return iBytesRead;
} /* readRAM() */

PNG_STATIC int PNGParseInfo(PNGIMAGE *pPage)
{
    uint8_t *s = pPage->ucFileBuf;
    int iBytesRead;
    
    // Read a few bytes to just parse the size/pixel info
    iBytesRead = (*pPage->pfnRead)(&pPage->PNGFile, s, 32);
    if (iBytesRead < 32) { // a PNG file this tiny? probably bad
        pPage->iError = PNG_INVALID_FILE;
        return 0;
    }

    if (MOTOLONG(s) != 0x89504e47) { // check that it's a PNG file
        pPage->iError = PNG_INVALID_FILE;
        return 0;
    }
    if (MOTOLONG(&s[12]) == 0x49484452/*'IHDR'*/) {
        pPage->iWidth = MOTOLONG(&s[16]);
        pPage->iHeight = MOTOLONG(&s[20]);
        pPage->ucBpp = s[24]; // bits per pixel
        pPage->ucPixelType = s[25]; // pixel type
        if (s[28] || pPage->ucBpp > 8) // interlace flag & 16-bit pixels are not supported (yet)
            pPage->iError = PNG_UNSUPPORTED_FEATURE;
        // calculate the number of bytes per line of pixels
        switch (pPage->ucPixelType) {
            case PNG_PIXEL_GRAYSCALE: // grayscale
            case PNG_PIXEL_INDEXED: // indexed
                pPage->iBpp = 1; // bytes per pixel
                break;
            case PNG_PIXEL_TRUECOLOR: // truecolor
                pPage->iBpp = 3;
                break;
            case PNG_PIXEL_GRAY_ALPHA: // grayscale + alpha
                pPage->iBpp = 2;
                break;
            case PNG_PIXEL_TRUECOLOR_ALPHA: // truecolor + alpha
                pPage->iBpp = 4;
        } // switch
        pPage->iPitch = pPage->iBpp * pPage->iWidth; // now we have bytes per line
    }
    return 0;
} /* PNGParseInfo() */
//
// De-filter the current line of pixels
//
PNG_STATIC void DeFilter(uint8_t *pCurr, uint8_t *pPrev, int iWidth, int iBpp)
{
    uint8_t ucFilter = *pCurr++;
    int x;
    iWidth *= iBpp; // bytes per line
    
    pPrev++; // skip filter of previous line
    switch (ucFilter) { // switch on filter type
        case PNG_FILTER_NONE:
            // nothing to do :)
            break;
        case PNG_FILTER_SUB:
            for (x=iBpp; x<iWidth; x++) {
                pCurr[x] += pCurr[x-iBpp];
            }
            break;
        case PNG_FILTER_UP:
            for (x = 0; x < iWidth; x++) {
               pCurr[x] += pPrev[x];
            }
            break;
        case PNG_FILTER_AVG:
            for (x = 0; x < iBpp; x++) {
               pCurr[x] = (pCurr[x] +
                  pPrev[x] / 2 );
            }
            for (x = iBpp; x < iWidth; x++) {
               pCurr[x] = pCurr[x] +
                  (pPrev[x] + pCurr[x-iBpp]) / 2;
            }
            break;
        case PNG_FILTER_PAETH:
            if (iBpp == 1) {
                int a, c;
                uint8_t *pEnd = &pCurr[iWidth];
                /* First pixel/byte */
                c = *pPrev++;
                a = *pCurr + c;
                *pCurr++ = (uint8_t)a;

                /* Remainder */
                while (pCurr < pEnd)
                {
                   int b, pa, pb, pc, p;

                   a &= 0xff; /* From previous iteration or start */
                   b = *pPrev++;
                   p = b - c;
                   pc = a - c;

//             #ifdef PNG_USE_ABS
//                   pa = abs(p);
//                   pb = abs(pc);
//                   pc = abs(p + pc);
//             #else
                   pa = p < 0 ? -p : p;
                   pb = pc < 0 ? -pc : pc;
                   pc = (p + pc) < 0 ? -(p + pc) : p + pc;
//             #endif

                   /* Find the best predictor, the least of pa, pb, pc favoring the earlier
                    * ones in the case of a tie.
                    */
                   if (pb < pa)
                   {
                      pa = pb; a = b;
                   }
                   if (pc < pa) a = c;

                   /* Calculate the current pixel in a, and move the previous row pixel to c
                    * for the next time round the loop
                    */
                   c = b;
                   a += *pCurr;
                   *pCurr++ = (uint8_t)a;
                }
            } else { // multi-byte
                uint8_t *pEnd = &pCurr[iBpp];

                /* Process the first pixel in the row completely (this is the same as 'up'
                 * because there is only one candidate predictor for the first row).
                 */
                while (pCurr < pEnd)
                {
                   int a = *pCurr + *pPrev++;
                   *pCurr++ = (uint8_t)a;
                }
                /* Remainder */
                pEnd = pEnd + (iWidth - iBpp);
                while (pCurr < pEnd)
                {
                   int a, b, c, pa, pb, pc, p;
                   c = pPrev[-iBpp];
                   a = pCurr[-iBpp];
                   b = *pPrev++;
                   p = b - c;
                   pc = a - c;
//             #ifdef PNG_USE_ABS
//                   pa = abs(p);
//                   pb = abs(pc);
//                   pc = abs(p + pc);
//             #else
                   pa = p < 0 ? -p : p;
                   pb = pc < 0 ? -pc : pc;
                   pc = (p + pc) < 0 ? -(p + pc) : p + pc;
//             #endif

                   if (pb < pa)
                   {
                      pa = pb; a = b;
                   }
                   if (pc < pa) a = c;

                   a += *pCurr;
                   *pCurr++ = (uint8_t)a;
                }
            } // multi-byte
            break;
    } // switch on filter type
} /* DeFilter() */
//
// PNGInit
// Parse the PNG file header and confirm that it's a valid file
//
// returns 0 for success, 1 for failure
//
PNG_STATIC int PNGInit(PNGIMAGE *pPNG)
{
    return PNGParseInfo(pPNG); // gather info for image
} /* PNGInit() */

PNG_STATIC int DecodePNG(PNGIMAGE *pPage)
{
    int err, y, iLen=0;
    int bDone, iOffset, iFileOffset, iBytesRead;
    int iMarker=0;
    uint8_t *tmp, *pCurr, *pPrev;
    z_stream d_stream; /* decompression stream */
    uint8_t *s = pPage->ucFileBuf;
    
    // Either the image buffer must be allocated or a draw callback must be set before entering
    if (pPage->pImage == NULL && pPage->pfnDraw == NULL) {
        pPage->iError = PNG_NO_BUFFER;
        return 0;
    }
    pCurr = pPage->ucPixels;
    pPrev = &pPage->ucPixels[MAX_BUFFERED_PIXELS];
    
    // Start decoding the image
    bDone = FALSE;
    // inflate the image data
    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
//    d_stream.next_out = cOutput;
//    d_stream.avail_out = iUncompressedSize;
    err = inflateInit(&d_stream);
//    if (inpage->cCompression == PIL_COMP_IPHONE_FLATE)
//        err = mz_inflateInit2(&d_stream, -15); // undocumented option which ignores header and crcs
//    else
//        err = mz_inflateInit2(&d_stream, 15);

    iFileOffset = 8;
    iOffset = 0;
    // Read some data to start
    (*pPage->pfnSeek)(&pPage->PNGFile, iFileOffset);
    iBytesRead = (*pPage->pfnRead)(&pPage->PNGFile, s, PNG_FILE_BUF_SIZE);
    iFileOffset += iBytesRead;
    y = 0;
    // tell miniz to uncompress the entire bitmap
    d_stream.avail_out = 0; //(pPage->iPitch+1) * pPage->iHeight;
    d_stream.next_out = pPage->pImage;

    while (y < pPage->iHeight) { // continue until fully decoded
        // parse the markers until the next data block
    while (!bDone)
    {
        iLen = MOTOLONG(&s[iOffset]); // chunk length
        if (iLen < 0 || iLen + (iFileOffset - iBytesRead) > pPage->PNGFile.iSize) // invalid data
        {
            pPage->iError = PNG_DECODE_ERROR;
            return 1;
        }
        iMarker = MOTOLONG(&s[iOffset+4]);
        iOffset += 8; // point to the marker data
        switch (iMarker)
        {
            case 0x44474b62: // 'bKGD' DEBUG
                break;
            case 0x67414d41: //'gAMA'
                break;
#ifdef FUTURE
            case 0x6663544C: //'fcTL' frame control block for animated PNG (need to get size of this partial image)
                pPage->iWidth = MOTOLONG(&pPage->pData[iOffset + 4]); // frame width
                pPage->iHeight = MOTOLONG(&pPage->pData[iOffset + 8]); // frame height
                bDone = TRUE;
                break;
#endif
            case 0x504c5445: //'PLTE' palette colors
                memset(&pPage->ucPalette[768], 0xff, 256); // assume all colors are opaque unless specified
                memcpy(pPage->ucPalette, &s[iOffset], iLen);
                break;
            case 0x74524e53: //'tRNS' transparency info
                if (pPage->ucPixelType == PNG_PIXEL_INDEXED) // if palette exists
                {
                    memcpy(&pPage->ucPalette[768], &s[iOffset], iLen);
                }
                else if (iLen == 2) // for grayscale images
                {
                    pPage->iTransparent = s[iOffset + 1]; // lower part of 2-byte value is transparent color index
                }
                else if (iLen == 6) // transparent color for 24-bpp image
                {
                    pPage->iTransparent = s[iOffset + 5]; // lower part of 2-byte value is transparent color value
                    pPage->iTransparent |= (s[iOffset + 3] << 8);
                    pPage->iTransparent |= (s[iOffset + 1] << 16);
                }
                break;
            case 0x49444154: //'IDAT' image data block
                while (iLen) {
                    if (iOffset >= iBytesRead) {
                        // we ran out of data; get some more
//                        (*pPage->pfnSeek)(&pPage->PNGFile, iFileOffset);
                        iBytesRead = (*pPage->pfnRead)(&pPage->PNGFile, pPage->ucFileBuf, (iLen > PNG_FILE_BUF_SIZE) ? PNG_FILE_BUF_SIZE : iLen);
                        iFileOffset += iBytesRead;
                        iOffset = 0;
                    } else {
                        // number of bytes remaining in buffer
                        iBytesRead -= iOffset;
                    }
                    d_stream.next_in  = &pPage->ucFileBuf[iOffset];
                    d_stream.avail_in = iBytesRead;
                    iLen -= iBytesRead;
                    iOffset += iBytesRead;
            //        if (iMarker == 0x66644154) // data starts at offset 4 in APNG frame data block
            //        {
            //            d_stream.next_in += 4;
            //            d_stream.avail_in -= 4;
            //        }
                    err = 0;
                    while (err == Z_OK) {
                        if (d_stream.avail_out == 0) { // reset for next line
                            d_stream.avail_out = pPage->iPitch+1;
                            d_stream.next_out = pCurr;
                        } // otherwise it could be a continuation of an unfinished line
                        err = inflate(&d_stream, Z_NO_FLUSH);
                        if ((err == Z_OK || err == Z_STREAM_END) && d_stream.avail_out == 0) {// successfully decoded line
                            DeFilter(pCurr, pPrev, pPage->iWidth, pPage->iBpp);
                            if (pPage->pImage == NULL) { // no image buffer, send it line by line
                                PNGDRAW pngd;
                                pngd.iBpp = pPage->iBpp;
                                pngd.iWidth = pPage->iWidth;
                                pngd.pPalette = pPage->ucPalette;
                                pngd.pPixels = pCurr+1;
                                pngd.iPixelType = pPage->ucPixelType;
                                pngd.x = 0;
                                pngd.y = y;
                                (*pPage->pfnDraw)(&pngd);
                            } else {
                                // copy to destination bitmap
                                memcpy(&pPage->pImage[y * pPage->iPitch], &pCurr[1], pPage->iPitch);
                            }
                            y++;
                        // swap current and previous lines
                        tmp = pCurr; pCurr = pPrev; pPrev = tmp;
                        } else { // some error
                            tmp = NULL;
                        }
                    }
                    if (err == Z_STREAM_END && d_stream.avail_out == 0) {
                        // successful decode, stop here
                        y = pPage->iHeight;
                        bDone = TRUE;
                    } else  if (err == Z_DATA_ERROR || err == Z_STREAM_ERROR) {
                        iLen = 0; // quit now
                        y = pPage->iHeight;
                        bDone = TRUE; // force loop to exit with error
                    }
                }
                break;
                //               case 0x69545874: //'iTXt'
                //               case 0x7a545874: //'zTXt'
#ifdef FUTURE
            case 0x74455874: //'tEXt'
            {
                char szTemp[256];
                char *pDest = NULL;
                memcpy(szTemp, &s[iOffset], 80); // get the label length (Title, Author, Description, Copyright, Creation Time, Software, Disclaimer, Warning, Source, Comment)
                i = (int)strlen(szTemp) + 1; // start of actual text
                if (strcmp(szTemp, "Comment") == 0 || strcmp(szTemp, "Description") == 0) pDest = &pPage->szComment[0];
                else if (strcmp(szTemp, "Software") == 0) pDest = &pPage->szSoftware[0];
                else if (strcmp(szTemp, "Author") == 0) pDest = &pPage->szArtist[0];
                if (pDest != NULL)
                {
                    if ((iLen - i) < 128)
                    {
                        memcpy(pPage->szComment, &pPage->pData[iOffset + i], iLen - i);
                        pPage->szComment[iLen - i + 1] = 0;
                    }
                    else
                    {
                        memcpy(pPage->szComment, &pPage->pData[iOffset + i], 127);
                        pPage->szComment[127] = '\0';
                    }
                }
            }
                break;
#endif
        } // switch
        iOffset += (iLen + 4); // skip data + CRC
    } // while !bDone
//            DeFilter(pCurr, pPrev, pPage->iWidth, iBpp);
            // pass the pixels to the draw function
//            pngd.iHeight = pPage->iHeight;
//            pngd.iWidth = pPage->iWidth;
//            pngd.pPixels = pCurr;
//            pngd.y = y;
//            (*pPage->pfnDraw)(&pngd);
            // swap current and reference lines
//            tmp = pCurr;
//            pCurr = pPrev;
//            pPrev = tmp;
//            y++;
//        }  while (y < pPage->iHeight && d_stream.avail_out == 0);
    } // while y < height
    err = inflateEnd(&d_stream);
    return 0;
} /* DecodePNG() */
