#ifndef _state_h
#define _state_h

#include "imageMeta.h"
#include "imageParams.h"
#include "preferences.h"
#include "logger.h"

#include <deque>

/*
userState object lives in the image, and is created using
pointers from the metadata and buffers.

The roll is responsible for calling the update state in each image
Every time a change occurs (post-timer) the UI will call
the roll to add a state for each image.
Unified state number (per roll) to track where in the vector
we are. If we're less than size-1, and add, then clear the rest
of the stack

If the user undo, then move the state number back one and apply
that state to all images.

If user redo, move the state number forward one and apply that state

Function (roll) for checking state status.
Are we at the front, or the back of the state? enable/disable
buttons/react accordingly.

*/

class userState {

  public:
  userState(){};
  ~userState(){};

  void setPtrs(imageMetadata* meta, imageParams* params, bool* reRdnr);

  void updateState();
  int getState(){return statePos;}
  void setState(int pos);

  void undoState();
  void redoState();

  int stateSize();
  bool undoAvail();
  bool redoAvail();


  private:

  int statePos = -1;

  imageMetadata *imgMeta = nullptr;
  imageParams *imgParam = nullptr;
  bool* reRender = nullptr;

  std::deque<imageMetadata> metaStates;
  std::deque<imageParams> paramStates;



};

#endif
