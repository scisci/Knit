//
//  KnitTechnique.h
//  Knit
//
//  Created by x on 10/24/14.
//  Copyright (c) 2014 Daniel Riley. All rights reserved.
//

#ifndef __Knit__KnitTechnique__
#define __Knit__KnitTechnique__

#include "KnitEvent.h"

class KnitProtocolDelegate;

// Class that understands what is attempting to be done on the knitting machine
// and the context and updates itself and the machine accordingly.
class KnitTechnique : public KnitEventHandler {
public:
  virtual ~KnitTechnique() {}
  virtual void SetPosition(unsigned int row, unsigned int color) = 0;
};



#endif /* defined(__Knit__KnitTechnique__) */
