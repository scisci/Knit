//
//  KnitEvent.h
//  Knit
//
//  Created by x on 10/24/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#ifndef __Knit__KnitEvent__
#define __Knit__KnitEvent__


/*!
  \enum KnitCommandType
  \brief Generic commands returned from the Arduino knit controller.
*/
enum KnitEventType {
  // Knit async task complete (such as uploading/downloading)
  kKnitEventDone = ('D' << 8) | 'N',
  // There was an error when sending data to the arduino
  kKnitEventError = ('E' << 8) | 'R',
  // The arduino received the last bit of data OK
  kKnitEventGood = ('G' << 8) | 'D',
  // The arduino initialized and can receive commands
  kKnitEventInit = ('I' << 8) | 'N',
  // The arduino sends a 2-byte value which is the needle position of carriage.
  kKnitEventPosition = ('P' << 8) | 'S',
  // The arduino wrote data to the machine and is ready for more data
  kKnitEventReady = ('R' << 8) | 'D',
  // The machine finished a row at the left
  kKnitEventRowLeft = ('R' << 8) | 'L',
  // The machine finished a row at the right
  kKnitEventRowRight = ('R' << 8) | 'R',
  // The machine start a row at the left
  kKnitEventStartRowLeft = ('S' << 8) | 'L',
  // The machine started a row at the right
  kKnitEventStartRowRight = ('S' << 8) | 'R',
  // The arduino restarted.
  kKnitEventStart = ('S' << 8) | 'T',
};

class KnitEvent;
class PositionUpdateKnitCommand;

class KnitEventHandler {
public:
  virtual ~KnitEventHandler() {}
  virtual bool HandleDoneEvent(const KnitEvent* event) { return false; }
  virtual bool HandleErrorEvent(const KnitEvent* event) { return false; }
  virtual bool HandleGoodEvent(const KnitEvent* event) { return false; }
  virtual bool HandleInitEvent(const KnitEvent* event) { return false; }
  virtual bool HandleReadyEvent(const KnitEvent* event) { return false; }
  virtual bool HandleFinishedRowLeftEvent(const KnitEvent* event) { return false; }
  virtual bool HandleFinishedRowRightEvent(const KnitEvent* event) { return false; }
  virtual bool HandleStartedRowLeftEvent(const KnitEvent* event) { return false; }
  virtual bool HandleStartedRowRightEvent(const KnitEvent* event) { return false; }
  virtual bool HandleStartEvent(const KnitEvent* event) { return false; }
  virtual bool HandlePositionEvent(const PositionUpdateKnitCommand* event) { return false; }
};


enum KnitParseStatus {
  kKnitParseStatusReady,
  kKnitParseStatusDone,
  kKnitParseStatusError
};


class KnitEvent {
public:
  KnitEvent(KnitEventType type, bool requires_parse=false)
  :type_(type),
   requires_parse_(requires_parse)
  {}
  
  virtual ~KnitEvent() {}

  virtual KnitParseStatus Parse(unsigned char byte)
  {
    return kKnitParseStatusError;
  }
  
  // Dispatch the event to the handler, return true if the event was handled.
  virtual bool Dispatch(KnitEventHandler* handler) const
  {
    switch (type_) {
      case kKnitEventDone: return handler->HandleDoneEvent(this);
      case kKnitEventError: return handler->HandleErrorEvent(this);
      case kKnitEventGood: return handler->HandleGoodEvent(this);
      case kKnitEventInit: return handler->HandleInitEvent(this);
      case kKnitEventReady: return handler->HandleReadyEvent(this);
      case kKnitEventRowLeft: return handler->HandleFinishedRowLeftEvent(this);
      case kKnitEventRowRight: return handler->HandleFinishedRowRightEvent(this);
      case kKnitEventStartRowLeft: return handler->HandleStartedRowLeftEvent(this);
      case kKnitEventStartRowRight: return handler->HandleStartedRowRightEvent(this);
      case kKnitEventStart: return handler->HandleStartEvent(this);
      default: return false;
    }
  }
  
  KnitEventType GetType() const
  {
    return type_;
  }
  
protected:
  unsigned char GetInitCheckSum() const
  {
    unsigned char checksum = 'K';
    checksum = checksum + ((type_ >> 8) & 0xFF);
    checksum = checksum + ((type_ & 0xFF));
    return checksum;
  }
  
private:
  KnitEventType type_;
  bool requires_parse_;
};

class PositionUpdateKnitCommand : public KnitEvent {
public:
  PositionUpdateKnitCommand()
  :KnitEvent(kKnitEventPosition, true),
   parse_pos_(0),
   position_(0),
   checksum_(GetInitCheckSum())
  {}
  
  int GetPosition() const
  {
    return position_;
  }

  virtual KnitParseStatus Parse(unsigned char byte)
  {
    if (++parse_pos_ < 3) { // Only accepts two bytes
      position_ |= byte << ((2 - parse_pos_) * 8); // MSB
      checksum_ = (checksum_ + byte) & 0xFF;
      return kKnitParseStatusReady;
    }
    
    // If we're here then we're at the checksum
    return checksum_ == byte ? kKnitParseStatusDone : kKnitParseStatusError;
  }
  
  virtual bool Dispatch(KnitEventHandler* handler) const
  {
    return handler->HandlePositionEvent(this);
  }
  
private:
  int parse_pos_;
  int position_;
  unsigned char checksum_;
};



#endif /* defined(__Knit__KnitEvent__) */
