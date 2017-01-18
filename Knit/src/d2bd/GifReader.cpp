/*
 *  GIFAnimConverter.cpp
 *  videopicmatic
 *
 *  Created by Daniel Riley on 3/16/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include <iostream>
#include <sstream>
#include <string>
#include "GifReader.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
//#include "gd.h"
using namespace d2bd;

#define VERBOSE         0
#define MAXCOLORMAPSIZE 256
#define MAX_COLOR_INDEX 255

#define CM_RED          0
#define CM_GREEN        1
#define CM_BLUE         2

#define INTERLACE       0x40
#define LOCALCOLORMAP   0x80
#define BitSet(byte, bit)  (((byte) & (bit)) == (bit))
#define ReadOK(file,buffer,len) (file->sgetn((char *)buffer, len) > 0)
#define LM_to_uint(a,b)  (((b)<<8)|(a))

#define GIF_FRAME_DEF_DURATION_S 0.1




GifReader::GifReader()
{}

GifReader::~GifReader()
{}


std::unique_ptr<IndexedImage> GifReader::ReadIndexedImage(std::streambuf* inputBufPtr)
{
  std::unique_ptr<IndexedImage> image;
  Gif89ExtInfo  extInfo;
  // Allocate space for maximum buffer we need to read chunks in the
  // header
  unsigned char   buf[16];
  unsigned char   c;
  unsigned char   ColorMap[3][MAXCOLORMAPSIZE];
  unsigned char   localColorMap[3][MAXCOLORMAPSIZE];
  int             screen_width, screen_height;
  int             gif87a;
  int ZeroDataBlock = 0;
  int globalColorMapSize;
  int localColorMapSize;
  int useGlobalColorMap;
  int hasGlobalColorMap;
  int numLoops = -2; // invalid

  // Read Gif Header
  if (!ReadOK(inputBufPtr, buf, 6)) return image;//-2;
  // Check the header tag
  if (strncmp((char *)buf, "GIF", 3) != 0) return image;//-3;
  
  if (memcmp((char *)buf + 3, "87a", 3) == 0) {
    gif87a = 1;
  } else if (memcmp((char *)buf+3, "89a", 3) == 0) {
    gif87a = 0;
  } else  {
    return image;//-3;
  }
  
  // Read Logical Screen Descriptor
  if (!ReadOK(inputBufPtr, buf, 7)) return image;// -5;

  screen_width = LM_to_uint(buf[0], buf[1]);
  screen_height = LM_to_uint(buf[2], buf[3]);
  //image.width = screen_width;
  //image.height = screen_height;
  
  // Just a safety
  if (screen_width > 1280 || screen_height > 1024) return image;// -6;
  
  hasGlobalColorMap = (buf[4] & 0x80) == 0x80;
  globalColorMapSize = 2 << (buf[4] & 0x07);

  // Read Global Color Map if possible
  if (hasGlobalColorMap && gifInReadColorMap(inputBufPtr, globalColorMapSize, ColorMap) < 0) {
    return image;//-7;
  }
  
  image.reset(new IndexedImage(screen_width, screen_height,
    hasGlobalColorMap ? globalColorMapSize : 0));
  
  if (hasGlobalColorMap) {
    unsigned char* pallette = new unsigned char[globalColorMapSize * 3];
    unsigned char* pal = pallette;
    for (int i = 0; i < globalColorMapSize; ++i, pal += 3) {
      pal[0] = ColorMap[CM_RED][i];
      pal[1] = ColorMap[CM_GREEN][i];
      pal[2] = ColorMap[CM_BLUE][i];
    }
    
    image->SetPallette(pallette);
    delete [] pallette;
  }
  
  // Check if looping tag is there
  int idx = inputBufPtr->pubseekoff(0, std::ios_base::cur);
  
  if (!ReadOK(inputBufPtr, buf, 3)) return image;//-8;

  if (buf[0] == 0x21 && buf[1] == 0xFF && buf[2] == 0x0B) {
    if (!ReadOK(inputBufPtr, buf, 11)) return image;//-9;
    if (memcmp((char *)buf, "NETSCAPE2.0", 11) == 0) {
      if (!ReadOK(inputBufPtr, buf, 5)) return image;//-10;
      if (buf[0] == 0x03 && buf[1] == 0x01 && buf[4] == 0) {
        numLoops = LM_to_uint(buf[2], buf[3]) - 1;
      }
    }
  }
          
  // couldn't find loop tag
  if (numLoops < -1)
  {
    numLoops = -1;
    inputBufPtr->pubseekpos(idx);
  }
  
  bool firstFrameOnly = true;
  if (firstFrameOnly) numLoops = 0;

  // Create an image for compositing
  //gdImagePtr workingImage = gdImageCreateTrueColor(screen_width, screen_height);

  // Create the image
  unsigned char* image_data = new unsigned char[screen_width * screen_height];

  if (image_data == nullptr) {
    return image;//-12;
  }
  

  int error = 0;
  int frameCount = 0;
  bool begun = false;
  bool cancel = false;

  Gif89ExtInfo lastInfo;
  int lastLeft = 0;
  int lastTop = 0;

  while (!error) {
    int top, left;
    int width, height;
    
    // Try to read next block
    if (!ReadOK(inputBufPtr, &c, 1)) 
    {
      error = -1;
      break;
    }
    
    if (c == ';') // Gif Terminator
      break;
    
    if (c == '!') // Extension
    {        
      if (!ReadOK(inputBufPtr, &c, 1))
      {
        error = -2;
        break;
      }
      
      gifInDoExtension(inputBufPtr, c, extInfo, &ZeroDataBlock);
      
      continue;
    }
    
    if (c != ',') // Not valid start character
      continue;
    
    // Try to read image
    if (!ReadOK(inputBufPtr, buf, 9)) 
    {
      error = -1;
      break;
    }
    
    useGlobalColorMap = (buf[8] & 0x80) == 0;
    localColorMapSize = 1 << ((buf[8] & 0x07) + 1);
    
    left = LM_to_uint(buf[0], buf[1]);
    top = LM_to_uint(buf[2], buf[3]);
    width = LM_to_uint(buf[4], buf[5]);
    height = LM_to_uint(buf[6], buf[7]);
    
    //LOG(LOG_DEBUG) << "[gifInReadPtr read frame " << frameCount << " [" << left << ", " << top << ", " << width << ", " << height << "]";
    
    if (left + width > screen_width || top + height > screen_height) 
    {
      //LOG(LOG_WARNING) << "[gifInReadPtr Frame is not confined to screen dimension]";
      error = -3;
      break;
    }

    int interlace = (buf[8] & 0x40) != 0;

    if (!useGlobalColorMap) 
    {
      if (gifInReadColorMap(inputBufPtr, localColorMapSize, localColorMap)) 
      { 
        //gdImageDestroy(im);
        error = -5;
        break;
      }
      
      gifInReadImage(inputBufPtr, image_data, screen_width,
        left, top, width, height, localColorMap, extInfo.transparent,
        interlace, &ZeroDataBlock);
      
    } 
    else 
    {
      if (!hasGlobalColorMap) 
      {
        //gdImageDestroy(im);
        error = -6;
        break;
      }
      
      gifInReadImage(inputBufPtr, image_data, screen_width,
        left, top, width, height, ColorMap, extInfo.transparent,
        interlace, &ZeroDataBlock);
    }
    
    break;
  }
  //fd->gd_free(fd);
  
  image->SetData(image_data);
  delete [] image_data;

  if (error < 0)
  {
    //return -14;
  }

  return image;
}

void GifReader::gifInReadImage(std::streambuf* inputBufPtr,
  unsigned char* image_data, size_t image_width,
  int left, int top, int width, int height, unsigned char (*cmap)[256],
  int transparent, int interlace, int *ZeroDataBlockP)
{
  unsigned char   c;      
  int             v;
  int             xpos = left, ypos = top, pass = 0;
  int len = width + left;
  int ht = height + top;
  LzwStaticData sd;
  
  //  Initialize the Compression routines
  if (!ReadOK(inputBufPtr, &c, 1)) return;
  if (c > MAX_LWZ_BITS) return;


  unsigned char* dest = &image_data[ypos * image_width + xpos];

  if (gifInLWZReadByte(inputBufPtr, &sd, 1, c, ZeroDataBlockP) < 0) return;

  while ((v = gifInLWZReadByte(inputBufPtr, &sd, 0, c, ZeroDataBlockP)) >= 0) 
  {
    if (v > MAX_COLOR_INDEX) v = 0;
    *dest = v;

    if (++xpos < len)
    {
      dest ++;
    }
    else
    {
      xpos = left;
      
      if (!interlace)
      {
        ++ypos;
      }
      else
      {
        switch (pass) 
        {
          case 0:
          case 1: ypos += 8; break;
          case 2: ypos += 4; break;
          case 3: ypos += 2; break;
        }
        
        if (ypos >= ht) 
        {
          switch (++pass) {
            case 1: ypos = top + 4; break;
            case 2: ypos = top + 2; break;
            case 3: ypos = top + 1; break;
            default: break;
          }
        }
      } 
      
      dest = &image_data[ypos * image_width + xpos];
    }
    
    if (ypos >= ht) break;
  }
  
fini:
  if (gifInLWZReadByte(inputBufPtr, &sd, 0, c, ZeroDataBlockP) >=0) {
    /* Ignore extra */
  }

}

int GifReader::gifInReadColorMap(std::streambuf* inputBufPtr, int number, 
                                        unsigned char (*buffer)[256])
{
  int             i;
  unsigned char   rgb[3];

  for (i = 0; i < number; ++i) 
  {
    if (!ReadOK(inputBufPtr, rgb, sizeof(rgb))) 
    {
      return -1;
    }
    
    buffer[CM_RED][i] = rgb[0] ;
    buffer[CM_GREEN][i] = rgb[1] ;
    buffer[CM_BLUE][i] = rgb[2] ;
  }

  return 0;
}

int GifReader::gifInDoExtension(std::streambuf* inputBufPtr, int label, Gif89ExtInfo& info,
                                       int *ZeroDataBlockP)
{
  unsigned char buf[256];
  
  if (label == 0xf9)
  {
    memset(buf, 0, 4);    // initialize a few bytes in the case the next function fails
    gifInGetDataBlock(inputBufPtr, buf, ZeroDataBlockP);

    info.transparent = -1;
    info.disposal    = (buf[0] >> 2) & 0x7;
    info.inputFlag   = (buf[0] >> 1) & 0x1;
    info.delayTime   = LM_to_uint(buf[1],buf[2]);

    if ((buf[0] & 0x1) != 0) info.transparent = buf[3];
  }
  
  while (gifInGetDataBlock(inputBufPtr, buf, ZeroDataBlockP) > 0)
    ;
  
  return 0;
}


int GifReader::gifInGetDataBlock(std::streambuf* inputBufPtr, unsigned char *buf, 
                                        int *ZeroDataBlockP)
{
  unsigned char   count;
  
  if (!ReadOK(inputBufPtr, &count, 1)) return -1;
  
  *ZeroDataBlockP = count == 0;
  
  if ((count != 0) && (!ReadOK(inputBufPtr, buf, count))) return -1;
  
  //LOG(LOG_DEBUG3) << "[gifInGetDataBlock returning " << count;
  
  return count;
}


int GifReader::gifInGetCode(std::streambuf* inputBufPtr, CodeStaticData *scd, 
                                   int code_size, int flag, int *ZeroDataBlockP)
{
  int           i, j, ret;
  unsigned char count;
  
  if (flag) 
  {
    scd->curbit = 0;
    scd->lastbit = 0;
    scd->last_byte = 0;
    scd->done = 0;
    return 0;
  }
  
  if ((scd->curbit + code_size) >= scd->lastbit) 
  {
    if (scd->done) 
    {
      if (scd->curbit >= scd->lastbit) 
      {
        /* Oh well */
      } 
      
      return -1;
    }
    
    scd->buf[0] = scd->buf[scd->last_byte - 2];
    scd->buf[1] = scd->buf[scd->last_byte - 1];
    
    if ((count = gifInGetDataBlock(inputBufPtr, &scd->buf[2], ZeroDataBlockP)) <= 0)
      scd->done = 1;
    
    scd->last_byte = 2 + count;
    scd->curbit = (scd->curbit - scd->lastbit) + 16;
    scd->lastbit = (2 + count) * 8;
  }
  
  ret = 0;
  for (i = scd->curbit, j = 0; j < code_size; ++i, ++j)
    ret |= ((scd->buf[ i / 8 ] & (1 << (i % 8))) != 0) << j;
  
  scd->curbit += code_size;

  //LOG(LOG_DEBUG3) << "[gifInGetCode(," << code_size << "," << flag << ") returning " << ret << "]";
  
  return ret;
}

int GifReader::gifInLWZReadByte(std::streambuf* inputBufPtr, LzwStaticData *sd, 
                            char flag, int input_code_size, int *ZeroDataBlockP)
{
  int code, incode, i;
  
  if (flag) 
  {
    sd->set_code_size = input_code_size;
    sd->code_size = sd->set_code_size + 1;
    sd->clear_code = 1 << sd->set_code_size ;
    sd->end_code = sd->clear_code + 1;
    sd->max_code_size = 2 * sd->clear_code;
    sd->max_code = sd->clear_code+2;
    
    gifInGetCode(inputBufPtr, &sd->scd, 0, 1, ZeroDataBlockP);
    
    sd->fresh = 1;
    
    for (i = 0; i < sd->clear_code; ++i) 
    {
      sd->table[0][i] = 0;
      sd->table[1][i] = i;
    }
    
    sd->table[1][0] = 0;
    memset(&sd->table[0][i], 0, (1 << MAX_LWZ_BITS) - i);
    /*
    for (; i < (1 << MAX_LWZ_BITS); ++i)
      sd->table[0][i] = sd->table[1][0] = 0;
    */
    sd->sp = sd->stack;
    
    return 0;
  } 
  else if (sd->fresh) 
  {
    sd->fresh = 0;
    
    do 
    {
      sd->firstcode = sd->oldcode =
      gifInGetCode(inputBufPtr, &sd->scd, sd->code_size, 0, ZeroDataBlockP);
    } 
    while (sd->firstcode == sd->clear_code);
    
    return sd->firstcode;
  }
  
  if (sd->sp > sd->stack)
    return *--sd->sp;
  
  while ((code = gifInGetCode(inputBufPtr, &sd->scd, sd->code_size, 0, ZeroDataBlockP)) >= 0) 
  {
    if (code == sd->clear_code) 
    {
      for (i = 0; i < sd->clear_code; ++i) 
      {
        sd->table[0][i] = 0;
        sd->table[1][i] = i;
      }
      
      // Is this correct?
      memset(&sd->table[0][i], 0, (1 << MAX_LWZ_BITS) - i);
      memset(&sd->table[1][i], 0, (1 << MAX_LWZ_BITS) - i);
      /*
      for (; i < (1<<MAX_LWZ_BITS); ++i)
        sd->table[0][i] = sd->table[1][i] = 0;
      */
      
      sd->code_size = sd->set_code_size + 1;
      sd->max_code_size = 2 * sd->clear_code;
      sd->max_code = sd->clear_code + 2;
      sd->sp = sd->stack;
      sd->firstcode = sd->oldcode =
      gifInGetCode(inputBufPtr, &sd->scd, sd->code_size, 0, ZeroDataBlockP);
      
      return sd->firstcode;
    } 
    else if (code == sd->end_code) 
    {
      int             count;
      unsigned char   buf[260];
      
      if (*ZeroDataBlockP)
        return -2;
      
      while ((count = gifInGetDataBlock(inputBufPtr, buf, ZeroDataBlockP)) > 0)
        ;
      
      if (count != 0)
        return -2;
    }
    
    incode = code;
    
    if (sd->sp == (sd->stack + STACK_SIZE)) 
    {
      // Bad compressed data stream
      return -1;
    }
    
    if (code >= sd->max_code) 
    {
      *sd->sp++ = sd->firstcode;
      code = sd->oldcode;
    }
    
    while (code >= sd->clear_code) 
    {
      if (sd->sp == (sd->stack + STACK_SIZE)) 
      {
        // Bad compressed data stream
        return -1;
      }
      *sd->sp++ = sd->table[1][code];
      
      if (code == sd->table[0][code]) 
      {
        // Oh well
      }
      
      code = sd->table[0][code];
    }
    
    *sd->sp++ = sd->firstcode = sd->table[1][code];
    
    if ((code = sd->max_code) < (1 << MAX_LWZ_BITS)) 
    {
      sd->table[0][code] = sd->oldcode;
      sd->table[1][code] = sd->firstcode;
      ++sd->max_code;
      
      if ((sd->max_code >= sd->max_code_size) &&
          (sd->max_code_size < (1 << MAX_LWZ_BITS))) 
      {
        sd->max_code_size *= 2;
        ++sd->code_size;
      }
    }
    
    sd->oldcode = incode;
    
    if (sd->sp > sd->stack)
      return *--sd->sp;
  }
  
  //LOG(LOG_DEBUG3) << "[gifInLWZReadByte(," << flag << "," << input_code_size << ") returning " << code << "]";
  
  return code;
}
  


