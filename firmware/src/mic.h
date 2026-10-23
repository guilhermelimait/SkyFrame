#pragma once
#include <Arduino.h>

// The board's microphone (ES7210 over I2S), used only to measure how loud the room is.
// Nothing is recorded, stored or sent: each block of sound becomes one loudness number and is discarded.
bool micStart();                 // set up the audio chip and start listening (call from the main task)
void micStop();
bool micRunning();
float micLevel(int ch = -1);     // recent loudness in dBFS (about -90 = silence, 0 = loudest); ch 0/1, or -1 = louder
float micFloor();                // the room's background noise, learned over ~15 s
float micMargin(int sense);      // how far above the background a sound must be: 16 / 10 / 6 dB
bool micHears(int sense);        // louder than the background: sense 0 low, 1 medium, 2 high
