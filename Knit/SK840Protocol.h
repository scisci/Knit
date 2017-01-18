//
//  SK840.h
//  Knit
//
//  Created by Daniel Riley on 8/14/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#ifndef __Knit__SK840__
#define __Knit__SK840__


#include "KnitProtocol.h"
#include <memory>

#include "d2bd/SerialPort.h"


class SK840Protocol : public KnitProtocol {
public:
  SK840Protocol();
  //! Return the serial port baud rate for the protocol
  virtual int GetBaudRate() const;
  //! Return the serial port settings necessary for the protocol.
  virtual d2bd::SerialPortOptions GetSerialPortOptions() const;
  
  //! Sets the pattern image the knitting should work with
  virtual void SetPatternImage(const d2bd::IndexedImage* image);
  
  //! Callbacks from the arduino.
  virtual void HandleEvent(const KnitEvent* event);
  // Upload command from UI
  virtual KnitUploadStatus Upload() { return kKnitUploadStatusError; }
  virtual KnitUploadStatus UploadBlank() { return kKnitUploadStatusError; }

  // Set the current position
  virtual void SetPosition(int row, int color);
  
private:
  int row_;
  int color_;
  void QueuePosition(int row, int color);
  void next();
  void fillBuffer(KnitDirection direction);
  std::unique_ptr<d2bd::IndexedImage> pattern_image_;
};


#endif /* defined(__Knit__SK840__) */
