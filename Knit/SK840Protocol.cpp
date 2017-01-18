//
//  PassapProtocol.cpp
//  Knit
//
//  Created by Daniel Riley on 8/9/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#include "SK840Protocol.h"

#include "d2bd/SerialPort.h"
#include "d2bd/SerialPortManager.h"
#include "d2bd/GifReader.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <assert.h>

#define SINGLE_BED

SK840Protocol::SK840Protocol()
:row_(0),
 color_(0)
{}


void SK840Protocol::SetPatternImage(const d2bd::IndexedImage* image)
{
  // TODO: mutex image data.
  pattern_image_ = image->Clone();
}

int SK840Protocol::GetBaudRate() const
{
  return 115200;
}

d2bd::SerialPortOptions SK840Protocol::GetSerialPortOptions() const
{
  d2bd::SerialPortOptions options;
  options.timeout = 2.0;
  return options;
}

void SK840Protocol::HandleEvent(const KnitEvent* event)
{
  switch (event->GetType()) {
    
    case kKnitEventRowLeft:
#ifdef SINGLE_BED
      next();
#endif
      fillBuffer(kKnitDirectionLeftToRight);
      break;
    
    case kKnitEventRowRight:
      next();
      fillBuffer(kKnitDirectionRightToLeft);
      break;
  }
}

void SK840Protocol::SetPosition(int row, int color)
{
  QueuePosition(row, color);
  next();
  fillBuffer(kKnitDirectionRightToLeft);
}

void SK840Protocol::QueuePosition(int row, int color)
{
  // Same as reset but reset to a particular row/color
  
  // The row will be incremented in the next() method so we set it to
  // one before the row we want to start on, or in the case we are
  // doing multiple colors per row we start one color before the start
  // color.

#ifdef SINGLE_BED
  row_ = row - 1;
  color_ = 0;
#else
  row_ = row;
  color_ = color - 1;
#endif

}


void SK840Protocol::next()
{
#ifdef SINGLE_BED
  row_++;
#else
  if (++color_ == image_.colors) {
    color_ = 0;
    row_++;
  }
#endif
  int row = row_;
  int color = color_;
  Update(row, color);
}

void SK840Protocol::fillBuffer(KnitDirection direction)
{
  bool singleBed = true; // Single bed knits the same back and forth whereas double bed knits inverse
  
#ifndef SINGLE_BED
  singleBed = false;
#endif


  int color = color_;
  int row = row_;

  unsigned char dataOutBuffer_[1024];

  bool imageValid = pattern_image_ != nullptr && pattern_image_->GetData() != 0L && row >= 0 && row < pattern_image_->GetHeight();
  
  if (imageValid) {
    int checksum = 0;
    int dataOutPos = 0;
    unsigned char byte = 0;
    

    const unsigned char* data = &pattern_image_->GetData()[row * pattern_image_->GetWidth()];
    
    dataOutBuffer_[0] = 'K';
    dataOutBuffer_[1] = 'D';
    dataOutBuffer_[2] = 'B';
    dataOutBuffer_[3] = (pattern_image_->GetWidth() >> 8) & 0xFF;
    dataOutBuffer_[4] = pattern_image_->GetWidth() & 0xFF;
    
    
    for (;dataOutPos < 5; ++dataOutPos) {                 // Start checksum for header
      checksum = (checksum + dataOutBuffer_[dataOutPos]) & 0xFF;
    }

    if (direction == kKnitDirectionLeftToRight || singleBed) {          // Knitting left to right knits the pattern
      for (int i = 0; i < pattern_image_->GetWidth(); ++i) {
        if (data[i] == color) {
          byte |= (1 << (7 - (i & 7)));
        }
        
        // If the next byte complet
        if ((((i + 1) & 7) == 0) || (i == pattern_image_->GetWidth() - 1)) {
          dataOutBuffer_[dataOutPos++] = byte;
          checksum = (checksum + byte) & 0xFF;
          byte = 0;
        }
      }
    } else {                                              // Knitting right to left knits inverse
      for (int i = 0; i < pattern_image_->GetWidth(); ++i) {
        if (data[i] != color) {
          byte |= (1 << (7 - (i & 7)));
        }
        
        if ((((i + 1) & 7) == 0) || (i == pattern_image_->GetWidth() - 1)) {
          dataOutBuffer_[dataOutPos++] = byte;
          checksum = (checksum + byte) & 0xFF;
          byte = 0;
        }
      }
    }

    //printf("Checksum %d\n", checksum);
    dataOutBuffer_[dataOutPos++] = checksum;
    
    size_t num_bytes = Write(&dataOutBuffer_[0], dataOutPos);
    assert(num_bytes == dataOutPos);
    /*
    if (serial_ != 0L && serial_->isOpen()) {
      int numBytesWritten = serial_->write(dataOutBuffer_, dataOutPos);
      assert(numBytesWritten == dataOutPos);
    }*/
  }

}