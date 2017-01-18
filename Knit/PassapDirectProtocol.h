//
//  PassapDirectProtocol.h
//  Knit
//
//  Created by Daniel Riley on 8/15/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#ifndef __Knit__PassapDirectProtocol__
#define __Knit__PassapDirectProtocol__

#include "PassapProtocol.h"

#include <assert.h>

class PassapDirectProtocol : public PassapProtocol {
public:
  //! Return the serial port baud rate for the protocol
  virtual int GetBaudRate() const
  {
    return 1200;
  }
  
  //! Return the serial port settings necessary for the protocol.
  virtual d2bd::SerialPortOptions GetSerialPortOptions() const
  {
    d2bd::SerialPortOptions options;
    options.parity = d2bd::kSerialPortParityEven;
    options.timeout = 0.1;
    return options;
  }
  
  //! Callbacks from the arduino.
  virtual void HandleEvent(const KnitEvent* event)
  {
  }
  // Upload command from UI
  virtual KnitUploadStatus Upload()
  {
    if (PrepareUpload(pattern_image_.get())) {
      const char* buffer = &pattern_upload_.pattern_data.c_str()[0];
      size_t buffer_size = pattern_upload_.pattern_data.size();
      size_t bytes_written = Write((const unsigned char*)buffer, buffer_size);
      assert(bytes_written == buffer_size);
      return kKnitUploadStatusComplete;
    }
    
    return kKnitUploadStatusError;
  }
  
  virtual KnitUploadStatus UploadBlank()
  {
    // TODO:
    return kKnitUploadStatusError;
  }

};

#endif /* defined(__Knit__PassapDirectProtocol__) */
