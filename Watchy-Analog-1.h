#ifndef WATCHY_ANALOG_1
#define WATCHY~ANALOG_1
//#define NODEBUG

// Which watchface to use; they all define watchfaceBitmap
#include "watchface/face4.h" // Background watchface onto which we draw hands etc
// Centre of dial for this watchface and radius of centre dot
#define DIAL_CX 85
#define DIAL_CY 85
#define DIAL_CR 6
// Dimensions of hands
#define HAND_LEN_H 45
#define HAND_WID_H 25
#define HAND_BORD_H 0

#define HAND_LEN_M 75
#define HAND_WID_M 10
#define HAND_BORD_M 6

#define HAND_LEN_S 20
#define HAND_WID_S 1
#define HAND_BORD_S 0

// Invert colour
#define INVERT(x) (x == GxEPD_BLACK? GxEPD_WHITE: GxEPD_BLACK)

// How long to allow for the update of the watchface. Adjust this to get hand movement at correct time
#define UPDATE_DELAY_MS 1500

// How long between connecting to NTP for sync. Subtract 2s or it may wait another hour
#define SYNC_INTERVAL (12*SECS_PER_HOUR - 2)

// Define the names and offsets from NTP time for our normal and daylight saving timezones
#define NORM_TZ "GMT"
#define NORM_TZ_ADD 0
#define DST_TZ "BST"
#define DST_TZ_ADD SECS_PER_HOUR

// Add leading zero for single digit numbers
#define TWO_DIGITS(x) ((x < 10? "0": "") + String(x))

// Timeout for second display (ms)
#define SEC_TIMEOUT_MS (2 * 60 * 1000)

#endif
