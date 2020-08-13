#include "common.h"
#include <vpad/input.h>
#include <padscore/wpad.h>
#include <padscore/kpad.h>

// This is wrapper around all input types
// It also abstracts it to navigational actions to provide consistency in the interface and simplicity in the code

void initializeInputs();
void updateInputs();

bool navigatedUp();
bool navigatedDown();
bool navigatedLeft();
bool navigatedRight();

bool pressedOk();
bool pressedStart();
bool pressedBack();