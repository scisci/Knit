//
//  KnitSerial.cpp
//  Knit
//
//  Created by Daniel Riley on 3/25/13.
//  Copyright (c) 2013 Daniel Riley. All rights reserved.
//
#include "KnitController.h"
#include "KnitSerial.h"


SerialObserver::SerialObserver(KnitController* controller)
:bufferPos_(0),
 bufferLength_(0),
 controller_(controller),
 parsePos_(0),
 port_(nullptr)
{
  MUTEX_INITIALIZE(&serialMutex_);
  COND_INITIALIZE(&serialCond_);
}

SerialObserver::~SerialObserver()
{
  MUTEX_DESTROY(&serialMutex_);
  COND_DESTROY(&serialCond_);
}

void SerialObserver::PushCommand(std::unique_ptr<KnitEvent> event)
{
  controller_->addCommand(std::unique_ptr<KnitCommand>(
      new KnitHandleEventCommand(std::move(event))));
  parseCommand_ = 0;
  parsePos_ = 0;
}

void SerialObserver::ParseCommand(std::unique_ptr<KnitEvent> command)
{
  current_command_ = std::move(command);
}

void SerialObserver::HandlePortReceivedBytes(const unsigned char* buffer, unsigned int bufferSize)
{
  std::string str;
  str.assign(buffer, buffer + bufferSize);
  
  //printf("%s", str.c_str());
  
  for (int i = 0; i < bufferSize; ++i) {
    unsigned char byte = buffer[i];
    
    if (parsePos_ == 0) {
      if (byte == 'K') {
        parsePos_ = 1;
        parseCommand_ = 0;
      }
    } else if (parsePos_ < 3) {
      parseCommand_ |= byte << ((2 - parsePos_) * 8);
      if (++parsePos_ == 3) {
        // Analyze the command
        switch (parseCommand_) {
          case kKnitEventError:
          case kKnitEventGood:
          case kKnitEventInit:
          case kKnitEventRowLeft:
          case kKnitEventRowRight:
          case kKnitEventStart:
          case kKnitEventReady:
          case kKnitEventDone:
          case kKnitEventStartRowLeft:
          case kKnitEventStartRowRight:
            //printf("\nPushed command\n");
            // Create generic knit command and push it to the controller
            PushCommand(std::unique_ptr<KnitEvent>(
                new KnitEvent((KnitEventType)parseCommand_)));
            break;
          
          case kKnitEventPosition:
            //printf("\nStarted parsing command\n");
            ParseCommand(std::unique_ptr<KnitEvent>(
                new PositionUpdateKnitCommand()));
            break;
          default:
            parsePos_ = 0;
            parseCommand_ = 0;
        }
      }
    }else if (parsePos_ >= 3) {
      KnitParseStatus status = kKnitParseStatusError;
      
      if (current_command_ != nullptr) {
        status = current_command_->Parse(byte);
        
        if (status == kKnitParseStatusDone) {
          PushCommand(std::move(current_command_));
        }
      }
      
      if (status != kKnitParseStatusReady) {
        parsePos_ = 0;
        parseCommand_ = 0;
      }
    }
  }
}
