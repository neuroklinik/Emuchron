//*****************************************************************************
// Filename : 'cfox.c'
// Title    : Animation code for a very simple cfox clock
//*****************************************************************************

// We need the following includes to build for Linux Emuchron / Atmel Monochron
#ifdef EMULIN
#include "../emulator/stub.h"
#endif
#ifndef EMULIN
#include "../util.h"
#endif
#include "../ks0108.h"
#include "../monomain.h"
#include "../glcd.h"
#include "../anim.h"
#include "cfox.h"

// Get a subset of the global variables representing the Monochron state
extern volatile uint8_t mcClockNewTS, mcClockNewTM, mcClockNewTH;
extern volatile uint8_t mcClockOldTS, mcClockOldTM, mcClockOldTH;
extern volatile uint8_t mcClockOldDD, mcClockOldDM, mcClockOldDY;
extern volatile uint8_t mcClockNewDD, mcClockNewDM, mcClockNewDY;
extern volatile uint8_t mcClockInit;
extern volatile uint8_t mcAlarmSwitch;
extern volatile uint8_t mcClockTimeEvent;
extern volatile uint8_t mcBgColor, mcFgColor;

//
// Function: cfoxCycle
//
// Update the lcd display of a very simple clock.
// This function is called every clock cycle (75 msec).
//
void cfoxCycle(void)
{

static s08 i=6;
if (i<=120 && mcClockNewTS==mcClockOldTS) {
    glcdFillRectangle(i,56,3,3,mcFgColor);
    if (i>6) glcdFillRectangle(i-10,56,3,3,mcBgColor);
    i=i+10; } else {
    if (i>10 && i<134) glcdFillRectangle(i-10,56,3,3,mcBgColor);
    i=6;
}


//  char dtInfo[9];
char hrsInfo[6];
char msInfo[14];

  // Use the generic method to update the alarm info in the clock.
  // This includes showing/hiding the alarm time upon flipping the alarm
  // switch as well as flashing the alarm time while alarming/snoozing.
  // animAlarmAreaUpdate(2, 57, ALARM_AREA_ALM_ONLY);

  // Only if a time event or init is flagged we need to update the clock
  if (mcClockTimeEvent == GLCD_FALSE && mcClockInit == GLCD_FALSE)
    return;

  DEBUGP("Update cfox");

  // Put new hour, min, sec in a string and paint it on the lcd
  animValToStr(mcClockNewTH, hrsInfo);
  hrsInfo[2] = ' ';
  hrsInfo[3] = 'H';
  hrsInfo[4] = 'R';
  hrsInfo[5] = 'S';
  animValToStr(mcClockNewTM, msInfo);
  msInfo[2] = ' ';
  msInfo[3] = 'M';
  msInfo[4] = 'I';
  msInfo[5] = 'N';
  msInfo[6] = ' ';
  msInfo[7] = ' ';
  animValToStr(mcClockNewTS, &msInfo[8]);
  msInfo[10] = ' ';
  msInfo[11] = 'S';
  msInfo[12] = 'E';
  msInfo[13] = 'C';

  glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, hrsInfo))/2, 34, FONT_5X7N, hrsInfo, mcFgColor);
  glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, msInfo))/2, 45, FONT_5X7N, msInfo, mcFgColor);
/*
  // Only paint the date when it has changed or when initializing the clock
  if (mcClockNewDD != mcClockOldDD || mcClockNewDM != mcClockOldDM ||
      mcClockNewDY != mcClockOldDY || mcClockInit == GLCD_TRUE)
  {
    // Put new month, day, year in a string and paint it on the lcd
    animValToStr(mcClockNewDM, dtInfo);
    dtInfo[2] = '/';
    animValToStr(mcClockNewDD, &dtInfo[3]);
    dtInfo[5] = '/';
    animValToStr(mcClockNewDY, &dtInfo[6]);
    glcdPutStr2(41, 29, FONT_5X7N, dtInfo, mcFgColor);
  }
*/

}

//
// Function: cfoxInit
//
// Initialize the lcd display of a very simple clock.
// This function is called once upon clock initialization.
//
void cfoxInit(u08 mode)
{
  DEBUGP("Init cfox");

  // Start from scratch by clearing the lcd using the background color
  glcdClearScreen(mcBgColor);
  // Rectangular border.
  glcdRectangle(2,2,125,61,mcFgColor);
 glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, "GAME"))/2, 7, FONT_5X7N, "GAME", mcFgColor);
 glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, "TIME ELAPSED"))/2, 18, FONT_5X7N, "TIME ELAPSED", mcFgColor);

  // Force the alarm info area to init itself in animAlarmAreaUpdate()
  // upon the first call to cfoxCycle()
  // mcAlarmSwitch = ALARM_SWITCH_NONE;
}

