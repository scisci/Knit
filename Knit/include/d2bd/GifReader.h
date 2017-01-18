/*
 *  GIFAnimConverter.h
 *  videopicmatic
 *
 *  Created by Daniel Riley on 3/16/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef GIF_ANIM_CONVERTER_H
#define GIF_ANIM_CONVERTER_H


#include <string>

namespace d2bd {


class IndexedImage {
public:
  IndexedImage(size_t width, size_t height, size_t pallette_size)
  :width_(width),
   height_(height),
   colors_(pallette_size)
  {
    data_ = new unsigned char[width_ * height_];
    pallette_ = new unsigned char[colors_ * 3];
  }
  
  IndexedImage(const IndexedImage& rhs)
  :width_(rhs.width_),
   height_(rhs.height_),
   colors_(rhs.colors_)
  {
    data_ = new unsigned char[width_ * height_];
    pallette_ = new unsigned char[colors_ * 3];
    memcpy(data_, rhs.data_, width_ * height_);
    memcpy(pallette_, rhs.pallette_, colors_ * 3);
  }
  
  ~IndexedImage()
  {
    delete [] data_;
    delete [] pallette_;
  }
  
  std::unique_ptr<d2bd::IndexedImage> Clone() const
  {
    return std::unique_ptr<d2bd::IndexedImage>(new IndexedImage(*this));
  }
  
  size_t GetWidth() const { return width_; }
  size_t GetHeight() const { return height_; }
  size_t GetPalletteSize() const { return colors_; }
  const unsigned char* GetData() const { return data_; }
  const unsigned char* GetPallette() const { return pallette_; }
  void SetData(const unsigned char* data) { memcpy(data_, data, width_ * height_); }
  void SetPallette(const unsigned char* pallette) { memcpy(pallette_, pallette, colors_ * 3); }
  void FillData(unsigned char color)
  {
    for (int i = 0; i < width_ * height_; ++i) {
      data_[i] = color;
    }
  }
  void SwapColors(int index_1, int index_2)
  {
    unsigned char* modified_data = new unsigned char[width_ * height_];
    
    for (int i = 0; i < width_ * height_; ++i) {
      if (data_[i] == index_1) {
        modified_data[i] = index_2;
      } else if (data_[i] == index_2) {
        modified_data[i] = index_1;
      } else {
        modified_data[i] = data_[i];
      }
    }
    
    delete [] data_;
    data_ = modified_data;
  }
  
  bool ContainsColor(int index)
  {
    for (int i = 0; i < width_ * height_; ++i) {
      if (data_[i] == index) {
        return true;
      }
    }
    
    return false;
  }
  
  void RemoveColor(int index, int replacement_index)
  {
    if (index >= 0 && index < colors_) {
      unsigned char* modified_data = new unsigned char[width_ * height_];
      unsigned char* modified_pallette = new unsigned char[(colors_ - 1) * 3];
      // Go through the entire data array and replace any instances of the color
      for (int i = 0; i < width_ * height_; ++i) {
        if (data_[i] == index) {
          modified_data[i] = replacement_index;
        } else {
          modified_data[i] = data_[i];
        }
      }
      
      // Go through and decrement any indexes that were above index
      for (int i = 0; i < width_ * height_; ++i) {
        if (modified_data[i] > index) {
          modified_data[i] = modified_data[i] - 1;
        }
      }
      
      // Modify pallette data
      unsigned char* src = &pallette_[0];
      unsigned char* dst = &modified_pallette[0];
      for (int i = 0; i < colors_; ++i, src += 3) {
        if (i != index) {
          dst[0] = src[0];
          dst[1] = src[1];
          dst[2] = src[2];
          dst += 3;
        }
      }
      
      colors_ = colors_ - 1;
      
      delete [] data_;
      delete [] pallette_;
      
      data_ = modified_data;
      pallette_ = modified_pallette;
    } else {
     // assert(0);
    }
    
  }
private:
  size_t width_;
  size_t height_;
  unsigned char* data_;
  
  size_t colors_;
  unsigned char* pallette_;
};



#define MAX_LWZ_BITS    12
#define STACK_SIZE  ((1<<(MAX_LWZ_BITS))*2)
struct CodeStaticData
{
	unsigned char    buf[280];
	int              curbit, lastbit, done, last_byte;
};

struct LzwStaticData
{
	int fresh;
	int code_size, set_code_size;
	int max_code, max_code_size;
	int firstcode, oldcode;
	int clear_code, end_code;
	int table[2][(1<< MAX_LWZ_BITS)];
	int stack[STACK_SIZE], *sp;
	CodeStaticData scd;
};

struct Gif89ExtInfo
{
  Gif89ExtInfo()
  :transparent(-1),
   delayTime(0),
   inputFlag(0),
   disposal(0)
  {}
  
  void reset()
  {
    transparent = -1;
    delayTime = 0;
    inputFlag = 0;
    disposal = 0;
  }
  
  int transparent;
  int delayTime;
  int inputFlag;
  int disposal;
};




class GifReader {
public:
  
  virtual ~GifReader();
  // Reads a stream into a gif animation object
  static std::unique_ptr<IndexedImage> ReadIndexedImage(std::streambuf* buf);
private:
  GifReader();
  static int gifInReadColorMap(std::streambuf* inputBufPtr, int number, unsigned char (*buffer)[256]);
  static int gifInDoExtension(std::streambuf* inputBufPtr, int label, Gif89ExtInfo& info, int *ZeroDataBlockP);
  static int gifInGetDataBlock(std::streambuf* inputBufPtr, unsigned char *buf, int *ZeroDataBlockP);
  static void gifInReadImage(std::streambuf* inputBufPtr, unsigned char* image_data, size_t image_width, int left, int top, int width, int height, unsigned char (*cmap)[256], int transparent, int interlace, int *ZeroDataBlockP);
  static int gifInLWZReadByte(std::streambuf* inputBufPtr, LzwStaticData *sd, char flag, int input_code_size, int *ZeroDataBlockP);
  static int gifInGetCode(std::streambuf* inputBufPtr, CodeStaticData *scd, int code_size, int flag, int *ZeroDataBlockP);
};

}

#endif