//
//  GifImageProvider.h
//  VideoPicMatic
//
//  Created by Daniel Riley on 2/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef GIF_IMAGE_PROVIDER
#define GIF_IMAGE_PROVIDER

#include "VideoClip.h"

class GifImageProvider
{
public:
  virtual ~GifImageProvider(){};
  virtual int getNumImages() const = 0;
  virtual int getWidth() const = 0;
  virtual int getHeight() const = 0;
  inline const ImageData* getImageDataAt(int index) { int left, top; return getImageDataAt(index, left, top); }
  virtual const ImageData* getImageDataAt(int index, int& left, int& top) = 0;
  inline const ImageData* getImageMaskAt(int index) { int left, top; return getImageMaskAt(index, left, top); }
  virtual const ImageData* getImageMaskAt(int index, int& left, int& top) { return 0L; }
  virtual const VideoMotionMap* getMotionMapAt(int index) { return 0L; }
  virtual double getDurationInSecondsAt(int index) { return 0.1; }
  virtual void clearCache() {}
};


// This doesn't own the clip only operates on it
class GifVideoClipImageProvider : public GifImageProvider
{
public:
  GifVideoClipImageProvider()
  :clip_(0L),
   cacheImage_(0L),
   cacheMap_(0L)
  {}
  
  virtual ~GifVideoClipImageProvider();
  
  void setClip(VideoClip* clip);
  
  void clearCache();
  
  virtual int getNumImages() const;
  
  virtual int getWidth() const;
  
  virtual int getHeight() const;
  
  virtual const ImageData* getImageDataAt(int index, int& left, int& top);
  
  virtual const VideoMotionMap* getMotionMapAt(int index);
  virtual double getDurationInSecondsAt(int index);
  
private:
  VideoClip* clip_;
  ImageData* cacheImage_;
  VideoMotionMap* cacheMap_;
};

#endif
