//
//  KnitSerial.h
//  Knit
//
//  Created by Daniel Riley on 3/25/13.
//  Copyright (c) 2013 Daniel Riley. All rights reserved.
//

#ifndef __Knit__KnitSerial__
#define __Knit__KnitSerial__

#include "d2bd/SerialPort.h"
#include "d2bd/Threads.h"


class KnitController;
class KnitEvent;

class SerialObserver {
public:
  SerialObserver(KnitController* controller);
  virtual ~SerialObserver();
  void HandlePortReceivedBytes(const unsigned char* buffer, unsigned int bufferSize);
  
  void SetSerialPort(d2bd::SerialPort* port)
  {
    if (port_ != nullptr) {
      port_->SetReceivedBytesHandler(nullptr);
      port_->close();
    }
    
    port_ = port;
    
    if (port_ != nullptr) {
      port_->SetReceivedBytesHandler(
          boost::bind(&SerialObserver::HandlePortReceivedBytes, this, _1, _2));
    }
  }
  
  ssize_t Write(const unsigned char* buffer, size_t bufferSize)
  {
    if (port_ != nullptr) {
      return port_->queueBytes(buffer, bufferSize);
    }
    
    return 0;
  }
  
  bool IsOpen() const
  {
    return port_ != nullptr && port_->isOpen();
  }
  
private:
  void PushCommand(std::unique_ptr<KnitEvent> command);
  void ParseCommand(std::unique_ptr<KnitEvent> command);
  
  boost::signals2::scoped_connection port_connection_;
  
  std::unique_ptr<KnitEvent> current_command_;
  KnitController* controller_;
  d2bd::SerialPort* port_;
  d2bd::MutexHandle serialMutex_;
  d2bd::ConditionHandle serialCond_;
  unsigned char serialBuffer_[1024];
  int bufferPos_;
  int bufferLength_;
  int parseCommand_;
  int parsePos_;
};


#endif /* defined(__Knit__KnitSerial__) */
