//
//  PassapProtocol.h
//  Knit
//
//  Created by Daniel Riley on 8/9/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#ifndef __Knit__PassapProtocol__
#define __Knit__PassapProtocol__

#include "KnitProtocol.h"
#include <memory>

#include "d2bd/SerialPort.h"
#include "KnitTechnique.h"



struct PassapPatternUploadContext {
  /*! 
    The payload currently being sent to the device. This payload is a single 
    frame of data that the passap console can hold, which is a maximum of
    180x255 pixels. So if the image is longer the pattern needs to be broken
    up into rows.
  */
  std::string pattern_data;
  //! The number of colors in the image data
  size_t num_colors;
  //! The width of the image data cropped to 180px wide if necessary.
  size_t width;
  //! The height of the image data cropped to 255px if necessary.
  size_t height;
  //! The start offset in rows.
  size_t row_offset;
  //! The number of rows of the image data already uploaded into various patterns.
  size_t rows_uploaded;
  /*! 
    Number of bytes uploaded of the current pattern_data. This is done in chunks
    so as to not overflow the arduino which is the middleman.
  */
  size_t bytes_uploaded;
  // Result of last
  KnitUploadStatus status;
};

class PassapProtocol : public KnitProtocol {
public:
  PassapProtocol();
  //! Return the serial port baud rate for the protocol
  virtual int GetBaudRate() const;
  //! Return the serial port settings necessary for the protocol.
  virtual d2bd::SerialPortOptions GetSerialPortOptions() const;
  
  //! Sets the pattern image the knitting should work with
  virtual void SetPatternImage(const d2bd::IndexedImage* image);
  
  //! Callbacks from the arduino.
  virtual void HandleEvent(const KnitEvent* event);
  virtual bool HandleReadyEvent(const KnitEvent* command);
  virtual bool HandleDoneEvent(const KnitEvent* command);
  virtual bool HandleErrorEvent(const KnitEvent* command);
  // Upload command from UI
  virtual KnitUploadStatus Upload();
  virtual KnitUploadStatus UploadBlank();
  
  virtual void SetPosition(int row, int color);
  
protected:
  size_t WritePacket();
  
  bool PrepareUpload(const d2bd::IndexedImage* image);
  
  void ChangeMode(char mode);
  void SetStartPosition();
  void StopMachine();
  
  size_t SendDataBuffer(const unsigned char* buffer, size_t buffer_size);
  
  int row_;
  int color_;
  std::unique_ptr<d2bd::IndexedImage> pattern_image_;
  std::unique_ptr<KnitTechnique> technique_;
  PassapPatternUploadContext pattern_upload_;
};

#endif /* defined(__Knit__PassapProtocol__) */
