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

#define ELAPSED		0
#define REMAINING	1

// Get a subset of the global variables representing the Monochron state
extern volatile uint8_t mcClockNewTS, mcClockNewTM, mcClockNewTH;
extern volatile uint8_t mcClockOldTS, mcClockOldTM, mcClockOldTH;
extern volatile uint8_t mcClockOldDD, mcClockOldDM, mcClockOldDY;
extern volatile uint8_t mcClockNewDD, mcClockNewDM, mcClockNewDY;
extern volatile uint8_t mcClockInit;
extern volatile uint8_t mcAlarmSwitch, mcUpdAlarmSwitch;
extern volatile uint8_t mcClockTimeEvent;
extern volatile uint8_t mcBgColor, mcFgColor;
extern volatile uint8_t mcAlarmH, mcAlarmM, mcAlarming;
// Display mode Elapsed/Remaining
extern volatile uint8_t mcU8Util1;
static u08 i=6;
void cfoxCycle(void)
{
// static u08 i=6;
/*
if (i<=120 && mcClockNewTS==mcClockOldTS) {
    glcdFillRectangle(i,56,3,3,mcFgColor);
    if (i>6) glcdFillRectangle(i-10,56,3,3,mcBgColor);
    i=i+10; } else {
    if (i>10 && i<134) glcdFillRectangle(i-10,56,3,3,mcBgColor);
    i=6;    
}
*/
if ((i>=6 && i<=116) && mcClockNewTS==mcClockOldTS)
{
    glcdFillRectangle(i,56,3,3,mcFgColor);
    if (i>=16 && mcU8Util1 == ELAPSED) glcdFillRectangle(i-10,56,3,3,mcBgColor);
    if (i<=115 && mcU8Util1 == REMAINING) glcdFillRectangle(i+10,56,3,3,mcBgColor);
    if (mcU8Util1 == ELAPSED) {i=i+10;} else {i=i-10;}
} else {
    // if (i>16 && i<126 && mcU8Util1 == ELAPSED) glcdFillRectangle(i-10,56,3,3,mcBgColor);
    // if (i>16 && i<126 && mcU8Util1 == REMAINING) glcdFillRectangle(i+10,56,3,3,mcBgColor);
    glcdFillRectangle(6,56,113,3,mcBgColor);
    if (mcU8Util1 == ELAPSED) {i=6;} else {i=116;}   
}

// Alarming?
if (mcAlarming == GLCD_TRUE)
{
  glcdPutStr2(5,5,FONT_5X5P,"DEFCON 1!",mcFgColor);
}
// Update the alarm indicator area.
if (mcUpdAlarmSwitch == GLCD_TRUE)
{
  if (mcAlarmSwitch == ALARM_SWITCH_ON)
  {
    char msg[6];
    animValToStr(mcAlarmH, msg);
    msg[2] = ':';
    animValToStr(mcAlarmM, &msg[3]);
    glcdPutStr2(5, 5, FONT_5X5P, "ARMED", mcFgColor);
    glcdPutStr2(124-glcdGetWidthStr(FONT_5X5P, msg), 5, FONT_5X5P, msg, mcFgColor);
  } else {
    glcdFillRectangle2(5, 5, glcdGetWidthStr(FONT_5X5P, "DEFCON 1!"), 5, ALIGN_AUTO, FILL_BLANK, mcFgColor);
  }
}

  // Only if a time event or init is flagged we need to update the clock
  if (mcClockTimeEvent == GLCD_FALSE && mcClockInit == GLCD_FALSE)
    return; 
char hrsInfo[6];
char msInfo[14];

if (mcU8Util1==ELAPSED) {
glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, "GAME"))/2, 7, FONT_5X7N, "GAME", mcFgColor);
glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, " TIME ELAPSED "))/2, 18, FONT_5X7N, " TIME ELAPSED ", mcFgColor);
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

} else {
uint16_t eMins = mcClockNewTH * 60 + mcClockNewTM;
uint16_t aMins = mcAlarmH * 60 + mcAlarmM;
uint16_t dMins = eMins>=aMins ? (aMins+1440)-(eMins+1) : aMins-(eMins+1);
glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, "GAME"))/2, 7, FONT_5X7N, "GAME", mcFgColor);
glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, "TIME REMAINING"))/2, 18, FONT_5X7N, "TIME REMAINING", mcFgColor);
  // Put new hour, min, sec in a string and paint it on the lcd
  animValToStr(dMins/60, hrsInfo);
  hrsInfo[2] = ' ';
  hrsInfo[3] = 'H';
  hrsInfo[4] = 'R';
  hrsInfo[5] = 'S';
  animValToStr(dMins%60, msInfo);
  msInfo[2] = ' ';
  msInfo[3] = 'M';
  msInfo[4] = 'I';
  msInfo[5] = 'N';
  msInfo[6] = ' ';
  msInfo[7] = ' ';
  animValToStr(59-mcClockNewTS, &msInfo[8]);
  msInfo[10] = ' ';
  msInfo[11] = 'S';
  msInfo[12] = 'E';
  msInfo[13] = 'C';

  glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, hrsInfo))/2, 34, FONT_5X7N, hrsInfo, mcFgColor);
  glcdPutStr2((128-glcdGetWidthStr(FONT_5X7N, msInfo))/2, 45, FONT_5X7N, msInfo, mcFgColor);

}


}

void cfoxInit(u08 mode)
{
  // Initialize alarm switch.
  mcAlarmSwitch = ALARM_SWITCH_NONE;
  // Set Elapsed mode.
  mcU8Util1 = ELAPSED;
  // Start from scratch by clearing the lcd using the background color
  glcdClearScreen(mcBgColor);
  // Rectangular border.
  glcdRectangle(2,2,125,61,mcFgColor);
  //
}

void cfoxButton(u08 pressedButton)
{
  if (mcU8Util1 == ELAPSED) {
mcU8Util1 = REMAINING;
 } else {
 mcU8Util1 = ELAPSED;
}
}

