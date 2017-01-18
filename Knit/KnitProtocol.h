//
//  KnitProtocol.h
//  Knit
//
//  Created by Daniel Riley on 8/9/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#ifndef __Knit__KnitProtocol__
#define __Knit__KnitProtocol__

#include "d2bd/SerialPortManager.h"
#include "KnitEvent.h"

namespace d2bd {
  class IndexedImage;
}



enum KnitDirection {
  kKnitDirectionLeftToRight,
  kKnitDirectionRightToLeft
};

enum KnitUploadStatus {
  // The full pattern should be uploaded.
  kKnitUploadStatusComplete,
  // There was an error during the last upload command.
  kKnitUploadStatusError,
  // The pattern needs to wait for the user to trigger the console.
  kKnitUploadStatusWaiting
};


class KnitProtocolDelegate {
public:
  //! Delegate method for writing data to the serial port.
  virtual size_t Write(const unsigned char* data, size_t size) = 0;
  //! Delegate method for updating colors
  virtual void Update(unsigned int row, unsigned int color) = 0;
};



class KnitProtocol : public KnitEventHandler {
public:
  KnitProtocol()
  :delegate_(nullptr)
  {}
  
  virtual ~KnitProtocol() {}
  
  void SetDelegate(KnitProtocolDelegate* delegate)
  {
    delegate_ = delegate;
  }
  
  //! Return the serial port baud rate for the protocol
  virtual int GetBaudRate() const = 0;
  //! Return the serial port settings necessary for the protocol.
  virtual d2bd::SerialPortOptions GetSerialPortOptions() const = 0;
  //! Sets the pattern image the knitting should work with
  virtual void SetPatternImage(const d2bd::IndexedImage* image) = 0;
  
  //! Callbacks from the arduino.
  virtual void HandleEvent(const KnitEvent* event) = 0;
  // Upload command from UI
  virtual KnitUploadStatus Upload() = 0;
  
  virtual KnitUploadStatus UploadBlank() = 0;
  // Set the current position
  virtual void SetPosition(int row, int color) = 0;
  
protected:
  KnitProtocolDelegate* delegate_;
  
  size_t Write(const unsigned char* data, size_t size)
  {
    return delegate_->Write(data, size);
  }
  
  void Update(int row, int color)
  {
    delegate_->Update(row, color);
  }
  
};

#endif /* defined(__Knit__KnitProtocol__) */
