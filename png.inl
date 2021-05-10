//
// PNGdec implementation
//
#include "miniz.h"

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
        pPage->ucSrcPixelType = s[25]; // pixel type
        if (s[28] || pPage->ucBpp > 8) // interlace flag & 16-bit pixels are not supported (yet)
            pPage->iError = PNG_UNSUPPORTED_FEATURE;
    }
    return 0;
} /* PNGParseInfo() */

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
    int err, y, iLen, iBpp, iPitch;
    int bDone, iOffset, iFileOffset, iBytesRead;
    int iMarker;
    uint8_t *tmp, *pCurr, *pPrev;
    mz_stream d_stream; /* decompression stream */
    uint8_t *s = pPage->ucFileBuf;
    
    // calculate the number of bytes per line of pixels
    switch (pPage->ucSrcPixelType) {
        case PNG_PIXEL_GRAYSCALE: // grayscale
        case PNG_PIXEL_INDEXED: // indexed
            iBpp = 1; // bytes per pixel
            break;
        case PNG_PIXEL_TRUECOLOR: // truecolor
            iBpp = 3;
            break;
        case PNG_PIXEL_GRAY_ALPHA: // grayscale + alpha
            iBpp = 2;
            break;
        case PNG_PIXEL_TRUECOLOR_ALPHA: // truecolor + alpha
            iBpp = 4;
    } // switch
    iPitch = iBpp * pPage->iWidth; // now we have bytes per line
    // Start decoding the image
    bDone = FALSE;
    // inflate the image data
    d_stream.zalloc = (alloc_func)0;
    d_stream.zfree = (free_func)0;
    d_stream.opaque = (voidpf)0;
//    d_stream.next_out = cOutput;
//    d_stream.avail_out = iUncompressedSize;
    err = mz_inflateInit(&d_stream);
//    if (inpage->cCompression == PIL_COMP_IPHONE_FLATE)
//        err = mz_inflateInit2(&d_stream, -15); // undocumented option which ignores header and crcs
//    else
        err = mz_inflateInit2(&d_stream, 15);

    iFileOffset = 8;
    iOffset = 0;
    memset(&pPage->ucPixels, 0, MAX_BUFFERED_PIXELS); // previous line set to 0
    pPrev = pPage->ucPixels;
    pCurr = &pPage->ucPixels[MAX_BUFFERED_PIXELS];
    // Read some data to start
    (*pPage->pfnSeek)(&pPage->PNGFile, iFileOffset);
    iBytesRead = (*pPage->pfnRead)(&pPage->PNGFile, s, PNG_FILE_BUF_SIZE);
    y = 0;
    while (y < pPage->iHeight) { // continue until fully decoded
        // parse the markers until the next data block
    while (!bDone && iFileOffset < pPage->PNGFile.iSize-8)
    {
        iLen = MOTOLONG(&s[iOffset]); // chunk length
        if (iLen < 0 || iLen + iFileOffset > pPage->PNGFile.iSize) // invalid data
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
                if (pPage->ucSrcPixelType == PNG_PIXEL_INDEXED) // if palette exists
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
                bDone = TRUE;
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
        iOffset += iLen + 4; // skip data + CRC
    } // while !bDone
        if (iMarker != 0x49444154) { // something went wrong
            pPage->iError = PNG_DECODE_ERROR;
            return 1;
        }
        // decode this data block
        d_stream.next_out = pCurr;
        d_stream.avail_out = iPitch;
        d_stream.next_in  = &s[iOffset];
        d_stream.avail_in = iLen;
//        if (iMarker == 0x66644154) // data starts at offset 4 in APNG frame data block
//        {
//            d_stream.next_in += 4;
//            d_stream.avail_in -= 4;
//        }
        err = mz_inflate(&d_stream, Z_NO_FLUSH);
        if (err == Z_STREAM_END || err == Z_DATA_ERROR || err == Z_STREAM_ERROR)
            y = pPage->iHeight; // force loop to exit

    } // while y < height
    return 0;
} /* DecodePNG() */
