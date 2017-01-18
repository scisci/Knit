//
//  PassapProtocol.cpp
//  Knit
//
//  Created by Daniel Riley on 8/9/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#include "PassapProtocol.h"

#include "d2bd/SerialPort.h"
#include "d2bd/SerialPortManager.h"
#include "d2bd/GifReader.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <assert.h>

class MultiColorImageTechnique : public KnitTechnique {
public:
  MultiColorImageTechnique()
  :row_(0),
   color_(0),
   delegate_(nullptr)
  {}
  
  virtual ~MultiColorImageTechnique() {}
  
  virtual void Init(KnitProtocolDelegate* delegate, const d2bd::IndexedImage* image)
  {
    delegate_ = delegate;
    image_ = image->Clone();
  }
  
  virtual void SetPosition(unsigned int row, unsigned int color)
  {
    row_ = row;
    color_ = color;
    
    delegate_->Update(row_, color_);
  }
  
  virtual bool HandleFinishedRowRightEvent(const KnitEvent* command)
  {
    if (++color_ >= image_->GetPalletteSize()) {
      color_ = 0;
      if (++row_ >= image_->GetHeight()) {
        // Done, dispatch stop command.
        
      }
      
      
    }
    
    delegate_->Update(row_, color_);
    return true;
  }
  
private:
  std::unique_ptr<d2bd::IndexedImage> image_;
  unsigned int row_;
  unsigned int color_;
  KnitProtocolDelegate* delegate_;
};

PassapProtocol::PassapProtocol()
:row_(0),
 color_(0)
{
  // Set this so we don't do anything if we receive Ready by accident
  pattern_upload_.status = kKnitUploadStatusComplete;
}


void PassapProtocol::SetPatternImage(const d2bd::IndexedImage* image)
{
  // TODO: mutex image data.
  pattern_image_ = image->Clone();
}

void PassapProtocol::SetPosition(int row, int color)
{
  // Make sure we are in knitting mode and update the knitting technique with
  // the current position. Technically one would have to do some fiddling with
  // the physical machine to get it to start at a particular position, so for
  // now this is just used for starting the technique.
  ChangeMode('K'); // Change back to program mode.
  SetStartPosition();
  
  if (technique_.get() == nullptr) {
    std::unique_ptr<MultiColorImageTechnique> tech(new MultiColorImageTechnique());
    tech->Init(delegate_, pattern_image_.get());
    technique_.reset(tech.release());
  }
  
  technique_->SetPosition(row, color);
}

bool PassapProtocol::PrepareUpload(const d2bd::IndexedImage* image)
{
  if (image == nullptr) {
    return false;
  } else {
    pattern_upload_.num_colors = image->GetPalletteSize();
    pattern_upload_.width = image->GetWidth();
    pattern_upload_.height = image->GetHeight();
    pattern_upload_.row_offset = 0;
    pattern_upload_.bytes_uploaded = 0;
    
    if (pattern_upload_.num_colors > 4) {
      pattern_upload_.num_colors = 4;
    } else if (pattern_upload_.num_colors <= 2) {
      pattern_upload_.num_colors = 0; // Passap protocol, 1 or 2 colors is 0
    }
    
    if (pattern_upload_.width > 180) {
      pattern_upload_.width = 180;
    }
    
    if (pattern_upload_.height > 254) { // Use 254 since some patterns can only have even number
      pattern_upload_.height = 254;
    }
    
    const unsigned char* upload_data = image->GetData();
    
    // Build up the header.
    std::stringstream ss;
    ss << pattern_upload_.num_colors << ' ';
    ss << std::setw(3) << std::setfill(' ') << pattern_upload_.width << ' ';
    ss << std::setw(3) << std::setfill(' ') << pattern_upload_.height << ' ';
    
    for (int i = 0; i < pattern_upload_.height * pattern_upload_.width; ++i) {
      int x = i % pattern_upload_.width;
      int y = i / pattern_upload_.width;
      
      unsigned char color_index = upload_data[y * pattern_upload_.width + x];
      
      if (color_index > 3) {
        color_index = 3;
      }
      
      ss << (int)color_index;
    }
    
    ss << (char)3;
    
    pattern_upload_.pattern_data = ss.str();
    
    return true;
  }
}

KnitUploadStatus PassapProtocol::UploadBlank()
{
  if (pattern_image_.get() != nullptr) {
    d2bd::IndexedImage image(1, 1, pattern_image_->GetPalletteSize());
    image.FillData(0);
    if (PrepareUpload(&image)) {
      WritePacket();
      pattern_upload_.status = kKnitUploadStatusWaiting;
      return pattern_upload_.status;
    }
  }
  
  return kKnitUploadStatusError;
}


KnitUploadStatus PassapProtocol::Upload()
{
  if (PrepareUpload(pattern_image_.get())) {
    // We want to write the first packet.
    WritePacket();

    pattern_upload_.status = kKnitUploadStatusWaiting;
    return pattern_upload_.status;
  }
  
  return kKnitUploadStatusError;
}


size_t PassapProtocol::WritePacket()
{
  size_t start = pattern_upload_.bytes_uploaded;
  size_t size = pattern_upload_.pattern_data.size() - start;
  
  //printf("Trying to write %lu bytes.\n", size);
  size_t bytes_written = SendDataBuffer((unsigned char *)&pattern_upload_.pattern_data[start], size);
  //printf("Wrote  %lu : %lu / %lu bytes.\n", bytes_written, pattern_upload_.bytes_uploaded, pattern_upload_.pattern_data.size());
  pattern_upload_.bytes_uploaded += bytes_written;
  if (pattern_upload_.bytes_uploaded >= pattern_upload_.pattern_data.size()) {
    printf("Sent all data from comp.\n");
    pattern_upload_.status = kKnitUploadStatusComplete;
  }

  return bytes_written;
}

size_t PassapProtocol::SendDataBuffer(const unsigned char* buffer, size_t buffer_size)
{
  size_t bytes_written = 0;
  
  if (buffer_size > 0) {
    const size_t max_packet_size = 32;
    unsigned char packet[max_packet_size];
    
    size_t max_size = max_packet_size - 6; // KDB 2 byte size and checksum
    size_t pck_size = buffer_size > max_size ? max_size : buffer_size;
    
    packet[0] = 'K';
    packet[1] = 'D';
    packet[2] = 'B';
    packet[3] = (pck_size >> 8) & 0xFF;
    packet[4] = pck_size & 0xFF;
    
    memcpy(&packet[5], buffer, pck_size);
    
    unsigned char checksum = 0;
    for (int i = 0; i < pck_size + 5; ++i) {
      checksum = checksum + packet[i] & 0xFF;
    }
    
    // Insert checksum at end
    packet[pck_size + 5] = checksum;
    
    size_t written = Write(&packet[0], pck_size + 6);
    
    if (written != pck_size + 6) {
      printf("Failed to write data buffer, maybe serial port not open.\n");
    } else {
      bytes_written = pck_size;
    }
  }
  
  return bytes_written;

}

void PassapProtocol::ChangeMode(char mode)
{
  unsigned char buffer[3];
  buffer[0] = 'C';
  buffer[1] = 'M';
  buffer[2] = mode;
  SendDataBuffer(&buffer[0], 3);
}

void PassapProtocol::SetStartPosition()
{
  // Send start position message
  SendDataBuffer((const unsigned char*)"SP", 2);
}

void PassapProtocol::StopMachine()
{
  // Send start position message
  SendDataBuffer((const unsigned char*)"HT", 2);
}

int PassapProtocol::GetBaudRate() const
{
  return 115200;
}

d2bd::SerialPortOptions PassapProtocol::GetSerialPortOptions() const
{
  d2bd::SerialPortOptions options;
  options.timeout = 2.0;
  return options;
}

bool PassapProtocol::HandleReadyEvent(const KnitEvent* event)
{
  if (pattern_upload_.status == kKnitUploadStatusWaiting) {
    double progress = (double)pattern_upload_.bytes_uploaded / pattern_upload_.pattern_data.size();
    printf("Download progress: %f\n", progress * 100.0);
  
    WritePacket();
  }
  return true;
}

bool PassapProtocol::HandleDoneEvent(const KnitEvent* event)
{
  if (pattern_upload_.status == kKnitUploadStatusComplete) {
    // Upload was successful, all done now
    printf("Download complete.\n");
    ChangeMode('P'); // Change back to program mode.

  }
  return true;
}

bool PassapProtocol::HandleErrorEvent(const KnitEvent* event)
{
  if (pattern_upload_.status == kKnitUploadStatusWaiting) {
    pattern_upload_.status = kKnitUploadStatusError;
  }
  
  return true;
}

void PassapProtocol::HandleEvent(const KnitEvent* event)
{
  if (!event->Dispatch(this)) {
    if (technique_.get() != nullptr) {
      event->Dispatch(technique_.get());
    }
  }
}