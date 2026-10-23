#pragma once
#include <Arduino.h>

#define LCD_W 480
#define LCD_H 480

void hwInit();
uint16_t *hwBack();     // buffer to draw the next frame into
uint16_t *hwFront();    // buffer currently on screen
void hwPresent();       // show the back buffer (waits for the swap)
void hwBacklight(int pct);
// Writing to flash briefly stops the panel from reading its picture (PSRAM), which leaves it out of step.
// Wrap flash writes in these: the backlight goes off, and the panel restarts cleanly afterwards.
void hwFlashBegin();
void hwFlashEnd();
int hwResync();         // restart the panel's scan-out at the next frame (0 = accepted)
bool hwTouch(int &x, int &y);   // true while a finger is down
bool hwHasTouch();
bool hwImuReady();                         // QMI8658 motion sensor found
bool hwImu(float &ax, float &ay, float &az);   // acceleration in g (gravity when still)
void hwI2cScan(Print &out);                // USB debug: list the chips on the sensor bus
