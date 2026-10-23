#pragma once
#include <Arduino.h>

// Quick settings on the display: swipe down from the top edge of any screen.
// Brightness, clock format, night mode and screen time, with sliding pages.
bool quickTouch(bool down, int x, int y);   // every loop, upright coordinates; true while it owns the touch
bool quickActive();                          // the panel is open (or opening/closing)
bool quickWantsMic();                        // the sound page (with its live meter) is showing
bool quickWantsFrame();                      // something on the panel changed or is moving
void quickCompose(uint16_t *dst);            // draw the panel's current frame into a full-screen buffer
void quickAbort();                           // close at once (e.g. an update is starting)
bool quickJustClosed();                      // true once after the panel closed (the screen should redraw)
extern bool quickLog;                        // print each panel gesture over USB ("log")
void quickDebug(int pageId);                 // USB debug: open the panel at once on a page (-1 = menu)
