//*****************************************************************************
// Filename : 'lcdncurses.c'
// Title    : Lcd ncurses stub functionality for emuchron emulator
//*****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <ncurses.h>
#include "lcdncurses.h"

// This is ugly:
// Why can't we include the GLCD_* defs below from ks0108conf.h and ks0108.h?
// Defs TRUE and FALSE are defined in both "avrlibtypes.h" (loaded via
// "ks0108.h") and <ncurses.h> and... they have different values for TRUE.
// To make sure that we build our ncurses lcd stub using the proper defs we
// must build our ncurses interface independent from the avr environment.
// And because of this we have to duplicate common glcd defines like lcd panel
// pixel sizes in here.
// This also means that we can 'communicate' with the outside world using only
// common data types such as int, char, etc and whatever we define locally and
// expose via our header.
#define GLCD_XPIXELS		128
#define GLCD_YPIXELS		64
#define GLCD_CONTROLLER_XPIXELS	64
#define GLCD_CONTROLLER_YPIXELS	64
#define GLCD_NUM_CONTROLLERS \
  ((GLCD_XPIXELS + GLCD_CONTROLLER_XPIXELS - 1) / GLCD_CONTROLLER_XPIXELS)
#define GLCD_FALSE		0
#define GLCD_TRUE		1

// Fixed ncurses xterm geometry requirements
#define NCURSES_X_PIXELS 	258
#define NCURSES_Y_PIXELS 	66

// This is me
extern const char *__progname;

// Definition of a structure holding ncurses lcd device statistics
typedef struct _lcdNcurStats_t
{
  long long bitCnf;		// Lcd bits leading to ncurses update
  long long byteReq;		// Lcd bytes processed
} lcdNcurStats_t;

// Definition of a structure to hold ncurses window related data
typedef struct _lcdNcurCtrl_t
{
  WINDOW *winCtrl;		// The controller ncurses window
  unsigned char display;	// Indicates if controller display is active
  unsigned char startLine;	// Controller data display line offset
  unsigned char reverse;	// Indicates the normal/reverse state
  unsigned char flush;		// Indicates the flush state
} lcdNcurCtrl_t;

// The ncurses controller windows data
lcdNcurCtrl_t lcdNcurCtrl[GLCD_NUM_CONTROLLERS];

// A private copy of the window image from which we draw our ncurses window 
static unsigned char lcdNcurImage[GLCD_XPIXELS][GLCD_YPIXELS / 8];

// The init parameters
static lcdNcurInitArgs_t lcdNcurInitArgs;
static int deviceActive = GLCD_FALSE;

// Data needed for ncurses stub lcd device
static FILE *ttyFile = NULL;
static SCREEN *ttyScreen;
static WINDOW *winBorder;

// An Emuchron ncurses lcd pixel
static char lcdNcurPixel[] = "  ";

// Statistics counters on ncurses
static lcdNcurStats_t lcdNcurStats;
static unsigned char lcdBacklight = 16;

// Timestamps used to verify existence of ncurses tty
static struct timeval tvNow;
static struct timeval tvThen;

// Local function prototypes
static void lcdNcurDrawModeSet(unsigned char controller, unsigned char color);
static void lcdNcurRedraw(unsigned char controller, int startY, int rows);

//
// Function: lcdNcurBacklightSet
//
// Set backlight in lcd display in ncurses window (dummy)
//
void lcdNcurBacklightSet(unsigned char backlight)
{
  int pairNew = 1 + backlight;
  int i;

  // No need to update when brightness is unsupported or remains unchanged
  if (lcdNcurInitArgs.useBacklight == GLCD_FALSE || lcdBacklight == backlight)
    return;
 
  // Assign new paint color and repaint all controller windows
  lcdBacklight = backlight;
  for (i = 0; i < GLCD_NUM_CONTROLLERS; i++)
  {
    wattron(lcdNcurCtrl[i].winCtrl, COLOR_PAIR(pairNew));
    lcdNcurRedraw(i, 0, GLCD_YPIXELS);
  }
}

//
// Function: lcdNcurCleanup
//
// Shut down the lcd display in ncurses window
//
void lcdNcurCleanup(void)
{
  // Nothing to do if the ncurses environment is not initialized
  if (deviceActive == GLCD_FALSE)
    return;

  // Reset settings applied during init of ncurses display
  nocbreak();
  echo();
  curs_set(1);

  // End the ncurses session.
  // Okay... it turns out that the combination of ncurses, readline and
  // using ncurses endwin() on the ncurses tty makes the mchron command
  // terminal go haywire after exiting mchron, requiring to use the 'reset'
  // command to make it function properly again. It turns out that omitting
  // endwin() keeps the mchron terminal in a proper state. So, what to do?
  // Implement ending an ncurses session according specs (= use endwin())
  // and then prohibit the use of the readline library, or, don't call endwin()
  // so we can use the readline library thereby deviating from the official
  // ncurses specs.
  // The verdict: The readline library is awesome! Sorry endwin()...
  //endwin();

  // Since we're not calling endwin() we clear the display ourselves
  clear();
  refresh();

  // Unlink ncurses from tty
  delscreen(ttyScreen);
  fclose(ttyFile);
  ttyFile = NULL;
  deviceActive = GLCD_FALSE;
}

//
// Function: lcdNcurDataWrite
//
// Set content of lcd display in ncurses window. The controller has decided
// that the new data differs from the current lcd data.
//
void lcdNcurDataWrite(unsigned char x, unsigned char y, unsigned char data)
{
  unsigned char pixel = 0;
  unsigned char controller = x / GLCD_CONTROLLER_XPIXELS;
  int posX = (x - controller * GLCD_CONTROLLER_XPIXELS) * 2;
  int posY = y * 8;
  unsigned char lcdByte = lcdNcurImage[x][y];

  // Sync y with startline
  if (posY >= lcdNcurCtrl[controller].startLine)
    posY = posY - lcdNcurCtrl[controller].startLine;
  else
    posY = GLCD_YPIXELS - lcdNcurCtrl[controller].startLine + posY;

  // Statistics
  lcdNcurStats.byteReq++;

  // Sync internal window image and force window buffer flush 
  lcdNcurImage[x][y] = data;
  lcdNcurCtrl[controller].flush = GLCD_TRUE;

  // Process each individual bit of byte in display
  for (pixel = 0; pixel < 8; pixel++)
  {
    // Check if pixel has changed
    if ((lcdByte & 0x1) != (data & 0x1))
    {
      // Statistics
      lcdNcurStats.bitCnf++;

      // Only draw when the controller display is on
      if (lcdNcurCtrl[controller].display == GLCD_TRUE)
      {
        // Switch between normal/reverse mode and draw pixel
        lcdNcurDrawModeSet(controller, data & 0x1);
        mvwprintw(lcdNcurCtrl[controller].winCtrl, posY, posX,
          lcdNcurPixel);
      }
    }

    // Shift to next pixel
    lcdByte = (lcdByte >> 1);
    data = (data >> 1);
    posY++;
    if (posY >= GLCD_YPIXELS)
      posY = posY - GLCD_YPIXELS;
  }
}

//
// Function: lcdNcurDisplaySet
//
// Switch controller display off or on
//
void lcdNcurDisplaySet(unsigned char controller, unsigned char display)
{
  // Sync display state and update window
  if (lcdNcurCtrl[controller].display != display)
  {
    lcdNcurCtrl[controller].display = display;
    if (display == 0)
    {
      // Clear out the controller window
      werase(lcdNcurCtrl[controller].winCtrl);
      lcdNcurCtrl[controller].flush = GLCD_TRUE;
    }
    else
    {
      // Repaint the entire controller window
      lcdNcurRedraw(controller, 0, GLCD_YPIXELS);
    }
  }
}

//
// Function: lcdNcurDrawModeSet
//
// Set the ncurses draw color
//
static void lcdNcurDrawModeSet(unsigned char controller, unsigned char inverse)
{
  // Switch between normal and reverse state when needed
  if (inverse == 1)
  {
    if (lcdNcurCtrl[controller].reverse == GLCD_FALSE)
    {
      // Switch to reverse state
      wattron(lcdNcurCtrl[controller].winCtrl, A_REVERSE);
      lcdNcurCtrl[controller].reverse = GLCD_TRUE;
    }
  }
  else
  {
    if (lcdNcurCtrl[controller].reverse == GLCD_TRUE)
    {
      // Switch to normal state
      wattroff(lcdNcurCtrl[controller].winCtrl, A_REVERSE);
      lcdNcurCtrl[controller].reverse = GLCD_FALSE;
    }
  }
}

//
// Function: lcdNcurFlush
//
// Flush the lcd display in ncurses window
//
void lcdNcurFlush(void)
{
  int i;
  int refreshDone = GLCD_FALSE;
  struct stat buffer;

  // Check the ncurses tty if the previous check was in a
  // preceding second 
  (void)gettimeofday(&tvNow, NULL);
  if (tvNow.tv_sec > tvThen.tv_sec)
  {
    // Sync check time and check ncurses tty
    memcpy(&tvThen, &tvNow, sizeof(size_t));
    if (stat(lcdNcurInitArgs.tty, &buffer) != 0)
    {
      // The ncurses tty is gone so we'll force mchron to exit
      lcdNcurInitArgs.winClose();
    }
  }

  // Dump only when activity has been signalled since last refresh
  for (i = 0; i < GLCD_NUM_CONTROLLERS; i++)
  {
    if (lcdNcurCtrl[i].flush == GLCD_TRUE)
    {
      // Flush changes in ncurses window but do not redraw yet
      refreshDone = GLCD_TRUE;
      wnoutrefresh(lcdNcurCtrl[i].winCtrl);
      lcdNcurCtrl[i].flush = GLCD_FALSE;
    }
  }

  // Do the actual redraw
  if (refreshDone == GLCD_TRUE)
    doupdate();
}

//
// Function: lcdNcurInit
//
// Initialize the lcd display in ncurses window
//
int lcdNcurInit(lcdNcurInitArgs_t *lcdNcurInitArgsSet)
{
  int backlight;
  int i;
  FILE *fp;
  struct winsize sizeTty;
  struct stat statTty;
  int retVal = 0;

  // Nothing to do if the ncurses environment is already initialized
  if (deviceActive == GLCD_TRUE)
    return 0;

  // Copy ncursus init parameters
  memcpy(&lcdNcurInitArgs, lcdNcurInitArgsSet, sizeof(lcdNcurInitArgs_t));

  // Check if the ncurses tty is actually in use
  if (stat(lcdNcurInitArgs.tty, &statTty) != 0)
  {
    printf("%s: Destination ncurses tty \"%s\" is not in use\n", __progname,
      lcdNcurInitArgs.tty);
    return -1;
  }

  // Check if the ncurses tty has a minimum size
  fp = fopen(lcdNcurInitArgs.tty, "r");
  if (ioctl(fileno(fp), TIOCGWINSZ, (char *)&sizeTty) >= 0)
  {
    if (sizeTty.ws_col < NCURSES_X_PIXELS || sizeTty.ws_row < NCURSES_Y_PIXELS)
    {
      printf("%s: Destination ncurses tty size (%dx%d) is too small for use as\n",
        __progname, sizeTty.ws_col, sizeTty.ws_row);
      printf("mchron ncurses terminal (minimum size = %dx%d characters)\n",
        NCURSES_X_PIXELS, NCURSES_Y_PIXELS);
      fclose(fp);
      return -1;
    }
  }
  fclose(fp);

  // Init our window lcd image copy to blank
  memset(lcdNcurImage, 0, sizeof(lcdNcurImage));

  if (lcdNcurInitArgs.useBacklight == GLCD_TRUE)
  {
    // Make ncurses believe we're dealing with a 256 color terminal
    setenv("TERM", "xterm-256color", 1);
    // Open destination tty, assign to ncurses screen and allow using colors
    ttyFile = fopen(lcdNcurInitArgs.tty, "r+");
    ttyScreen = newterm("xterm-256color", ttyFile, ttyFile);
    start_color();
  }
  else
  {
    // Open destination tty and assign it to an ncurses screen
    ttyFile = fopen(lcdNcurInitArgs.tty, "r+");
    ttyScreen = newterm("xterm", ttyFile, ttyFile);
  }

  // Try to set the size of the xterm tty. We do this because for some
  // reason gdb makes ncurses use the size of the mchron terminal window
  // as the actual size, which is pretty weird.
  resize_term(NCURSES_Y_PIXELS, NCURSES_X_PIXELS);

  // Set ncurses tty to not wait for Enter key and not echo characters
  cbreak();
  noecho();

  // Hide the cursor in the tty
  curs_set(0);

  // Create the outer border window and the controller section windows
  winBorder = newwin(NCURSES_Y_PIXELS, NCURSES_X_PIXELS, 0, 0);
  for (i = 0; i < GLCD_NUM_CONTROLLERS; i++)
  {
    lcdNcurCtrl[i].winCtrl = newwin(GLCD_YPIXELS,
      GLCD_CONTROLLER_XPIXELS * 2, 1, 1 + i * GLCD_CONTROLLER_XPIXELS * 2);
    lcdNcurCtrl[i].display = GLCD_FALSE;
    lcdNcurCtrl[i].startLine = 0;
    lcdNcurCtrl[i].reverse = GLCD_FALSE;
    lcdNcurCtrl[i].flush = GLCD_FALSE;
  }

  // Setup backlight grey tone colors when requested
  if (lcdNcurInitArgs.useBacklight == GLCD_TRUE)
  {
    // Define black color
    init_color(COLOR_BLACK, 0, 0, 0);

    // Define the 17 backlight grey-tone schemes from dim to full brightness.
    // It will result in ncurses color pairs 0..17.
    for (i = 1; i <= 17; i++)
    {
      backlight = (int)(1000 * (6 + (float)(i - 1)) / 22);
      init_color(i + 127, backlight, backlight, backlight);
      init_pair(i, i + 127, COLOR_BLACK);
    }

    // Init full brightness color to controller windows
    wattron(lcdNcurCtrl[0].winCtrl, COLOR_PAIR(17));
    wattron(lcdNcurCtrl[1].winCtrl, COLOR_PAIR(17));

    // Assign medium brightness in the outer border window
    wattron(winBorder, COLOR_PAIR(7));
  }

  // Draw and show a box in the outer border window
  box(winBorder, 0 , 0);
  wrefresh(winBorder);

  // Set time reference for checking ncurses tty
  (void)gettimeofday(&tvThen, NULL);

  // We're initialized
  deviceActive = GLCD_TRUE;

  return retVal;
}

//
// Function: lcdNcurRedraw
//
// Redraw (a part of) the lcd display in ncurses window
//
static void lcdNcurRedraw(unsigned char controller, int startY, int rows)
{
  unsigned char x,y;
  unsigned char rowsToDo;
  unsigned char bitsToDo;
  unsigned char lcdByte;
  unsigned char lcdLine;
  int posY;

  // Redraw all x columns for (a limited set of) rows. We either start
  // at line 0 for a number of rows, or start at a non-0 line and draw
  // all up to the end of the window. We only need to draw the white
  // (or grey-tone) pixels.
  lcdNcurDrawModeSet(controller, 1);
  for (x = 0; x < GLCD_CONTROLLER_XPIXELS; x++)
  {
    rowsToDo = rows;
    posY = startY;
    lcdLine = (startY + lcdNcurCtrl[controller].startLine) %
      GLCD_CONTROLLER_YPIXELS;

    // Draw pixels in pixel byte
    for (y = (lcdLine >> 3); rowsToDo > 0; y = (y + 1) % 8)
    {
      // Get pixel byte and shift to the first pixel to be drawn
      lcdByte = lcdNcurImage[x + controller * GLCD_CONTROLLER_XPIXELS][y];
      lcdByte = (lcdByte >> (lcdLine & 0x7));

      // Draw all applicable pixel bit in byte
      for (bitsToDo = 8 - (lcdLine & 0x7); bitsToDo > 0 && rowsToDo > 0;
          bitsToDo--)
      {
        // Only draw when needed
        if ((lcdByte & 0x1) == 0x1)
          mvwprintw(lcdNcurCtrl[controller].winCtrl, posY, x * 2,
            lcdNcurPixel);

        // Shift to next pixel in byte and line in display
        lcdByte = (lcdByte >> 1);
        lcdLine++;
        posY++;
        rowsToDo--;
      }
    }
  }

  // Signal redraw
  lcdNcurCtrl[controller].flush = GLCD_TRUE;
}

//
// Function: lcdNcurStartLineSet
//
// Set controller display line offset
//
void lcdNcurStartLineSet(unsigned char controller, unsigned char startLine)
{
  int scroll;
  int fillStart;
  int rows;

  // If the display is off or when there's no change in the startline
  // there's no reason to redraw so only sync new value
  if (lcdNcurCtrl[controller].display == GLCD_FALSE ||
      lcdNcurCtrl[controller].startLine == startLine)
  {
    lcdNcurCtrl[controller].startLine = startLine;
    return;
  }

  // Determine the parameters that minimize ncurses scroll and redraw efforts
  if (lcdNcurCtrl[controller].startLine > startLine)
  {
    if (lcdNcurCtrl[controller].startLine - startLine < GLCD_YPIXELS / 2)
    {
      scroll = (int)startLine - (int)lcdNcurCtrl[controller].startLine;
      fillStart = 0;
      rows = -scroll;
    }
    else
    {
      scroll = (int)startLine - (int)lcdNcurCtrl[controller].startLine +
        GLCD_YPIXELS;
      fillStart = GLCD_YPIXELS - scroll;
      rows = scroll;
    }
  }
  else
  {
    if (startLine - lcdNcurCtrl[controller].startLine < GLCD_YPIXELS / 2)
    {
      scroll = (int)startLine - (int)lcdNcurCtrl[controller].startLine;
      fillStart = GLCD_YPIXELS - scroll;
      rows = scroll;
    }
    else
    {
      scroll = (int)startLine - (int)lcdNcurCtrl[controller].startLine -
        GLCD_YPIXELS;
      fillStart = 0;
      rows = -scroll;
    }
  }

  // Set new startline
  lcdNcurCtrl[controller].startLine = startLine;

  // Scroll the window up or down and fill-up emptied scroll space.
  // To avoid nasty scroll side effects that may result in scroll remnants at
  // the bottom of the window allow scrolling only during the actual scroll.
  scrollok(lcdNcurCtrl[controller].winCtrl, TRUE);
  wscrl(lcdNcurCtrl[controller].winCtrl, scroll);
  scrollok(lcdNcurCtrl[controller].winCtrl, FALSE);
  lcdNcurRedraw(controller, fillStart, rows);
}
  
//
// Function: lcdNcurStatsPrint
//
// Print interface statistics
//
void lcdNcurStatsPrint(void)
{
  printf("ncurses: lcdByteRx=%llu, ", lcdNcurStats.byteReq);
  if (lcdNcurStats.byteReq == 0)
    printf("bitEff=-%%\n");
  else
    printf("bitEff=%d%%\n",
      (int)(lcdNcurStats.bitCnf * 100 / (lcdNcurStats.byteReq * 8)));
}

//
// Function: lcdNcurStatsReset
//
// Reset interface statistics
//
void lcdNcurStatsReset(void)
{
  memset(&lcdNcurStats, 0, sizeof(lcdNcurStats_t));
}

