//
//  KnitController.h
//  Knit
//
//  Created by Daniel Riley on 3/24/13.
//  Copyright (c) 2013 Daniel Riley. All rights reserved.
//

#ifndef __Knit__KnitController__
#define __Knit__KnitController__

#include <iostream>
#include <fstream>
#include <vector>
#include "assert.h"

#include "d2bd/Threads.h"
#include "d2bd/GifReader.h"
#include "d2bd/SerialPortManager.h"
#include "KnitProtocol.h"
#include "KnitSerial.h"

enum KnitProtocolType {
  kKnitProtocolSK840,
  kKnitProtocolPassap,
  kKnitProtocolPassapDirect,
  kKnitprotocolCount
};

class KnitControllerObserver {
  virtual void colorChanged(int color) = 0;
  virtual void rowChanged(int row) = 0;
};

enum KnitCommandType {
  kKnitCommandUpload,
  kKnitCommandUploadBlank,
  kKnitCommandUpdateImage,
  kKnitCommandHandleEvent,
  kKnitCommandSetPosition,
};

class KnitCommand {
public:
  KnitCommand(KnitCommandType type)
  :type_(type)
  {}
  
  virtual ~KnitCommand() {}
  
  KnitCommandType GetType() const { return type_; }
private:
  KnitCommandType type_;
};

class KnitHandleEventCommand : public KnitCommand {
public:
  KnitHandleEventCommand(std::unique_ptr<KnitEvent> event)
  :KnitCommand(kKnitCommandHandleEvent),
   event_(std::move(event))
  {}
  
  const KnitEvent* GetEvent() const { return event_.get(); }
private:
  std::unique_ptr<KnitEvent> event_;
};

class KnitSetPositionCommand : public KnitCommand {
public:
  KnitSetPositionCommand(unsigned int row, unsigned int color)
  :KnitCommand(kKnitCommandSetPosition),
   row_(row),
   color_(color)
  {}

  unsigned int GetRow() const { return row_; }
  unsigned int GetColor() const { return color_; }
  
private:
  unsigned int row_;
  unsigned int color_;
};


class KnitController : public KnitProtocolDelegate {
public:
  typedef boost::signals2::signal<void (unsigned int)> RowChangedSignal;
  typedef boost::signals2::signal<void (unsigned int)> ColorChangedSignal;
  
  KnitController()
  :row_(UINT_MAX),
   color_(UINT_MAX),
   serial_(this),
   serial_index_(-1)
  {
    MUTEX_INITIALIZE(&commandMutex_);
    COND_INITIALIZE(&commandCond_);
    
    thread_.start(&KnitController::start_thread, this);
  }
  
  ~KnitController()
  {
    // Stop receiving.
    serial_.SetSerialPort(nullptr);
    
    
    MUTEX_DESTROY(&commandMutex_);
    COND_DESTROY(&commandCond_);
  }
  
  void SetProtocol(KnitProtocolType protocol);
  
  void SetProtocol(std::unique_ptr<KnitProtocol> protocol);
  
  void SetSerialPortIndex(int index);

  void InitSerialIfPossible();

  static void* start_thread(void* ctrl)
  {
    KnitController* _knitCtrl = static_cast<KnitController *>(ctrl);
    _knitCtrl->run();
    return 0;
  }
  
  void run();
  
  void loadImage(const std::string& path)
  {
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
    std::unique_ptr<d2bd::IndexedImage> image = d2bd::GifReader::ReadIndexedImage(file.rdbuf());
    
    // If a color in the image is not used, remove it.
    int i = 0;
    while (i < image->GetPalletteSize()) {
      if (!image->ContainsColor(i)) {
        image->RemoveColor(i, i == 0 ? 1 : 0);
      } else {
        ++i;
      }
    }
    
    if (image.get() != nullptr) {
      std::lock_guard<std::mutex> lock(image_mutex_);
      image_ = std::move(image);
      
      addCommand(std::unique_ptr<KnitCommand>(
        new KnitCommand(kKnitCommandUpdateImage)));
    } else {
      assert(0);
    }
  }
  
  void reset(int row=0, int color=0);
  void Upload();
  void UploadBlank();
  
  void SwapColors(int color_index_1, int color_index_2);
  

  void addCommand(std::unique_ptr<KnitCommand> command)
  {
    MUTEX_LOCK(&commandMutex_);
    commands_.push_back(std::move(command));
    COND_SIGNAL(&commandCond_);
    MUTEX_UNLOCK(&commandMutex_);
  }
  
  // Should only be called from the thread that calls loadImage
  const d2bd::IndexedImage* GetImage() const
  {
    return image_.get();
  }
  

  //! Delegate method for writing data to the serial port.
  virtual size_t Write(const unsigned char* data, size_t size)
  {
    return serial_.Write(data, size);
  }
  //! Delegate method for updating colors
  virtual void Update(unsigned int row, unsigned int color)
  {
    bool color_changed = false;
    bool row_changed = false;
    
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      
      if (color != color_) {
        color_ = color;
        color_changed = true;
      }
      
      if (row != row_) {
        row_ = row;
        row_changed = true;
      }
    }
    
    if (color_changed) {
      color_changed_signal_(color);
    }
    
    if (row_changed) {
      row_changed_signal_(row);;
    }
  }
  
  boost::signals2::connection ConnectRowChanged(
      const RowChangedSignal::slot_type& slot)
  {
    return row_changed_signal_.connect(slot);
  }
  
  boost::signals2::connection ConnectColorChanged(
      const ColorChangedSignal::slot_type& slot)
  {
    return color_changed_signal_.connect(slot);
  }

  
private:
  SerialObserver serial_;
  d2bd::Thread thread_;
  std::vector<std::unique_ptr<KnitCommand>> commands_;
  std::unique_ptr<d2bd::IndexedImage> image_;
  unsigned char dataOutBuffer_[1024];
  unsigned int row_;
  unsigned int color_;
  
  int serial_index_;
  
  std::unique_ptr<KnitProtocol> protocol_;

  d2bd::MutexHandle commandMutex_;
  d2bd::ConditionHandle commandCond_;
  
  mutable std::mutex data_mutex_;
  mutable std::mutex image_mutex_;
  
  
  RowChangedSignal row_changed_signal_;
  ColorChangedSignal color_changed_signal_;
};

#endif /* defined(__Knit__KnitController__) */
