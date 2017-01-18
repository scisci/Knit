//
//  KnitController.cpp
//  Knit
//
//  Created by Daniel Riley on 3/24/13.
//  Copyright (c) 2013 Daniel Riley. All rights reserved.
//

#include "KnitController.h"
#include "KnitSerial.h"
#include "PassapProtocol.h"
#include "PassapDirectProtocol.h"
#include "SK840Protocol.h"




void KnitController::run()
{
  std::vector<std::unique_ptr<KnitCommand>> commands;
  
  for (;;) {
    MUTEX_LOCK(&commandMutex_);
    while (commands_.empty()) {
      COND_WAIT(&commandCond_, &commandMutex_);
    }
    commands = std::move(commands_);
    commands_.clear();
    MUTEX_UNLOCK(&commandMutex_);
    
    for (int i = 0; i < commands.size(); ++i) {
      // Debug
      switch (commands[i]->GetType()) {
        case kKnitCommandSetPosition:
        {
          KnitSetPositionCommand* position_command = dynamic_cast<KnitSetPositionCommand*>(commands[i].get());
          unsigned int row = position_command->GetRow();
          unsigned int color = position_command->GetColor();
          
          data_mutex_.lock();
          row_ = row;
          color_ = color;
          data_mutex_.unlock();
          
          row_changed_signal_(row_);
          color_changed_signal_(color_);
          
          if (protocol_.get() != nullptr) {
            protocol_->SetPosition(row_, color_);
          }
        }
        break;
        
        case kKnitCommandUpload:
        {
          if (protocol_.get() != nullptr) {
            protocol_->Upload();
          }
        }
        break;
        
        case kKnitCommandUpdateImage:
        {
          if (protocol_.get() != nullptr) {
            image_mutex_.lock();
            if (image_.get() != nullptr) {
              protocol_->SetPatternImage(image_.get());
            }
            image_mutex_.unlock();
          }
        }
        break;
        
        case kKnitCommandUploadBlank:
        {
          if (protocol_.get() != nullptr) {
            protocol_->UploadBlank();
          }
        }
        break;
        
        case kKnitCommandHandleEvent:
        {
          KnitHandleEventCommand* event_command = dynamic_cast<KnitHandleEventCommand*>(commands[i].get());
          switch (event_command->GetEvent()->GetType()) {
            case kKnitEventStart:
              printf("Device started.\n");
              break;
              
            case kKnitEventInit:
              printf("Device initialized.\n");
              break;
              
            case kKnitEventStartRowLeft:
              printf("Row started on left.\n");
              break;
              
            case kKnitEventStartRowRight:
              printf("Row started on right.\n");
              break;
              
            case kKnitEventRowLeft:
              printf("Row finished on left.\n");
              break;
              
            case kKnitEventRowRight:
              printf("Row finished on right.\n");
              break;
              
            case kKnitEventDone:
              printf("Knit async done (download/upload)\n");
              break;

            case kKnitEventGood:
              printf("Wrote data to device.\n");
              break;
              
            case kKnitEventReady:
              printf("Device is ready for more data.\n");
              break;
              
            case kKnitEventError:
              printf("Error writing to device.\n");
              break;
              
            case kKnitEventPosition:
              //printf("Position update.\n");
              break;
          }
          
          // Pass events to protocol.
          protocol_->HandleEvent(event_command->GetEvent());
        }
        break;
      }
    }
  }
}

void KnitController::SetProtocol(KnitProtocolType protocol)
{
  switch (protocol) {
    case kKnitProtocolPassap:
      SetProtocol(std::unique_ptr<KnitProtocol>(new PassapProtocol));
      break;
    case kKnitProtocolPassapDirect:
      SetProtocol(std::unique_ptr<KnitProtocol>(new PassapDirectProtocol));
      break;
    case kKnitProtocolSK840:
      SetProtocol(std::unique_ptr<KnitProtocol>(new SK840Protocol));
      break;
    default:
      printf("Protocol not supported!\n");
  }
}

void KnitController::SetProtocol(std::unique_ptr<KnitProtocol> protocol)
{
  // Stop receiving.
  serial_.SetSerialPort(nullptr);
  
  if (protocol_ != nullptr) {
    // Anything here?
    protocol_->SetDelegate(nullptr);
    
  }
  
  protocol_ = std::move(protocol);
  
  if (protocol_ != nullptr) {
    protocol_->SetDelegate(this);
    
    addCommand(std::unique_ptr<KnitCommand>(
        new KnitCommand(kKnitCommandUpdateImage)));
  }
  
  
  InitSerialIfPossible();
}

void KnitController::SetSerialPortIndex(int index)
{
  serial_.SetSerialPort(nullptr);
  serial_index_ = index;
  InitSerialIfPossible();
}

void KnitController::InitSerialIfPossible()
{
  if (serial_index_ > 0 && protocol_ != nullptr) {
    int baud = protocol_->GetBaudRate();
    d2bd::SerialPortOptions options = protocol_->GetSerialPortOptions();
    d2bd::SerialPort* port = d2bd::SerialPortManager::instance().OpenPort(
      serial_index_, baud, options);
    
    if (port == nullptr) {
      printf("Error opening serial port: %d, %d\n", serial_index_, baud);
    } else {
      serial_.SetSerialPort(port);
    }
  }
}



void KnitController::Upload()
{
  // TODO: currently this requires the machine to be at the right, but we
  // should detect the position of the carriage and fill left to right or
  // right to left.
  
  image_mutex_.lock();
  bool can_upload = image_ != nullptr;
  image_mutex_.unlock();
  
  if (can_upload && serial_.IsOpen()) {
    addCommand(std::unique_ptr<KnitCommand>(
        new KnitCommand(kKnitCommandUpdateImage)));
    addCommand(std::unique_ptr<KnitCommand>(
        new KnitCommand(kKnitCommandUpload)));
  }
}

void KnitController::UploadBlank()
{
  if (serial_.IsOpen()) {
    addCommand(std::unique_ptr<KnitCommand>(
        new KnitCommand(kKnitCommandUploadBlank)));
  }
}

void KnitController::SwapColors(int color_index_1, int color_index_2)
{
  std::lock_guard<std::mutex> lock(image_mutex_);
  
  if (image_ != nullptr) {
    image_->SwapColors(color_index_1, color_index_2);
  }
}


void KnitController::reset(int row, int color)
{
  // TODO: currently this requires the machine to be at the right, but we
  // should detect the position of the carriage and fill left to right or
  // right to left.
  addCommand(std::unique_ptr<KnitCommand>(
      new KnitSetPositionCommand(row, color)));
}
