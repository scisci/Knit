#include "GifAnimation.h"
#include "ImageData.h"
#include "Log.h"

#include <vector>

GifFrame::GifFrame()
:duration_(0.0),
 left_(0),
 top_(0)
{
  image_ = ImageDataCreate();
}

GifFrame::GifFrame(const GifFrame& rhs)
:duration_(rhs.duration_),
 left_(rhs.left_),
 top_(rhs.top_)
{
  image_ = ImageDataCopy(rhs.image_);
}

GifFrame& GifFrame::operator= (const GifFrame& rhs)
{
  if (this != &rhs) // protect against invalid self-assignment
  {
    duration_ = rhs.duration_;
    left_ = rhs.left_;
    top_ = rhs.top_;
    image_ = ImageDataCopy(rhs.image_);
  }
  
  return *this;
}

void GifFrame::setImage(const ImageData& imgData)
{
  ImageDataTake(image_, &imgData);
}

//TODO: this seems dangerous, should we really take data here, obviously
// used for some optimization
void GifFrame::setImage(ImageData* imgData)
{
  if (imgData == 0L) return;
  
  ImageDataDestroy(image_);
  image_ = imgData;
}

const ImageData& GifFrame::getImage() const
{
  return *image_;
}

int GifFrame::getLeft() const
{
  return left_;
}

int GifFrame::getTop() const
{
  return top_;
}
void GifFrame::getPosition(int& left, int& top) const
{
  left = left_;
  top = top_;
}

void GifFrame::setPosition(int left, int top)
{
  left_ = left;
  top_ = top;
}

double GifFrame::getDuration() const
{
  return duration_;
}

void GifFrame::setDuration(double durationInSeconds)
{
  duration_ = durationInSeconds;
}

bool GifFrame::isValid() const
{
  return (image_->width > 0 && image_->height > 0 && image_->pixels != 0L);
}

GifFrame::~GifFrame()
{
  ImageDataDestroy(image_);
}

GifFrame GifAnimation::dummyFrame_;


void GifAnimation::cancel()
{
  MUTEX_LOCK(&mutex_);
  if (!ready_) cancel_ = true;
  MUTEX_UNLOCK(&mutex_);
}

bool GifAnimation::begin(ImageDataFormat format, int width, int height, int loops, int numFrames)
{
  if (width <= 0 || height <= 0) return false;
  
  MUTEX_LOCK(&mutex_);
  if (begun_)
  {
    MUTEX_UNLOCK(&mutex_);
    return false;
  }

  loops_ = loops;
  width_ = width;
  height_ = height;
  format_ = format;
  ready_ = false;
  
  if (numFrames > 0) frames_.reserve(numFrames);
  begun_ = true;
  
  // Broadcast the begun message
  if (scheduler_ != 0L && schBegunInt_ == SCHEDULER_INVALID_ID)
  {
    schBegunInt_ = scheduler_->schedule(&schBegunObs_, 0.0);
  }
  MUTEX_UNLOCK(&mutex_);

 
  return true;
}


bool GifAnimation::addFrame(const ImageData& imageData, double durationInSeconds, int left, int top, bool end)
{
  if (!isValid() || left < 0 || top < 0 || (imageData.width + left > width_) || 
      (imageData.height + top > height_)) return false;
  
  ImageData* img = 0L;
  
  if (imageData.format == format_) img = ImageDataCopy(&imageData);
  else img = ImageDataCopyWithFormat(format_, &imageData);
  
  if (img == 0L) return false;
  
  if (!addFrame(img, durationInSeconds, left, top, end))
  {
    ImageDataDestroy(img);
    return false;
  }
  
  return true;
}

bool GifAnimation::addFrame(ImageData* imgData, double durationInSeconds, int left, int top, bool end)
{
  if (!isValid() || left < 0 || top < 0 || imgData == 0L || 
      (imgData->width + left > width_) || (imgData->height + top > height_)) return false;

  if (imgData->pixels == 0L)
  {
    LOG(LOG_ERROR) << "GifAnimation::no gif pixels on add frame!";
  }
  
  if (imgData->format != format_)
  {
    ImageData* copy = ImageDataCopyWithFormat(format_, imgData);
    
    if (copy == 0L) return false;
    
    // We've made a copy to the correct format so dispose of given data
    ImageDataDestroy(imgData);
    
    imgData = copy;
  }
  
  MUTEX_LOCK(&mutex_);
  int numFrames = frames_.size();
  frames_.push_back(GifFrame());
  // Assign image data to the frame, the frame will take ownership
  frames_[numFrames].setImage(imgData);
  // Set gif info for the frame
  frames_[numFrames].setDuration(durationInSeconds);
  frames_[numFrames].setPosition(left, top);
  
  if (!durationValid_ && durationInSeconds > 0.0)
  {
    durationValid_ = true;
  }
  
  if (end)
  {
    ready_ = true;
    cancel_ = false; // Too late to cancel
  }

  //TODO: verify that this won't create a locked condition
  if (scheduler_ != 0L && schAddedInt_ == SCHEDULER_INVALID_ID)
  {
    LOG(LOG_DEBUG) << "GifAnimation::addFrame: scheduling on add event.";
    schAddedInt_ = scheduler_->schedule(&schAddedObs_, 0.0);
  }
  MUTEX_UNLOCK(&mutex_);

  return true;
}

void GifAnimation::notifyBegun(SchedulerIntervalID schId)
{
  MUTEX_LOCK(&mutex_);
  schBegunInt_ = SCHEDULER_INVALID_ID;
  //std::vector<GifAnimationObserver *> obsList(observers_);
  MUTEX_UNLOCK(&mutex_);
  /*
  
  for (int i = 0; i < obsList.size(); i++)
  {
    obsList[i]->gifAnimBegun(this);
  }*/
  notifyObservers<GifAnimation *>(&GifAnimationObserver::gifAnimBegun, this);
}

void GifAnimation::notifyAdded(SchedulerIntervalID schId)
{
  
  MUTEX_LOCK(&mutex_);
  schAddedInt_ = SCHEDULER_INVALID_ID;
  //std::vector<GifAnimationObserver *> obsList(observers_);
  MUTEX_UNLOCK(&mutex_);
/*
  for (int i = 0; i < obsList.size(); i++)
  {
    obsList[i]->gifAnimFramesAdded(this);
  }*/
  notifyObservers<GifAnimation *>(&GifAnimationObserver::gifAnimFramesAdded, this);
}

int GifAnimation::getNumFrames() const
{
  MUTEX_LOCK(&mutex_);
  int numFrames = frames_.size();
  MUTEX_UNLOCK(&mutex_);
  return numFrames;
}

const GifFrame& GifAnimation::getFrameAt(int index) const
{
  if (!isValid()) return dummyFrame_;
  
  MUTEX_LOCK(&mutex_);
  bool invalid = (index < 0 || index >= frames_.size());
  MUTEX_UNLOCK(&mutex_);
  
  if (invalid) return dummyFrame_;
  
  // Should be ok to return a frame in a threaded environment because it shouldn't
  // be cleared, only added to
  return frames_[index];
}

// Returns -1 for infinite looping
int GifAnimation::getNumLoops() const
{
  MUTEX_LOCK(&mutex_);
  int loops = loops_;
  MUTEX_UNLOCK(&mutex_);
  return loops;
}

