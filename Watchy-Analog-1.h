#ifndef WATCHY_ANALOG_1
#define WATCHY_ANALOG_1

// Whether the watch case is rotated so can rotate the screen and buttons
#define CASE_ROTATED true
#define ROTATED_BACK_BTN_PIN (CASE_ROTATED? DOWN_BTN_PIN: BACK_BTN_PIN)

// Which watchface to use; they all define watchfaceBitmap
#include "watchface/face4.h" // Background watchface onto which we draw hands etc

// Centre of dial for this watchface and radius of centre dot
#define DIAL_XC 85
#define DIAL_YC 85
#define DIAL_CR 8

// Number of hand styles implemented
#define HAND_STYLE_N 3

// Current hand style in non-volatile memory
RTC_DATA_ATTR uint8_t handStyle = 0;

// Macros for access to the settings array
#define HAND_LEN_H (handSettings[handStyle][0])
#define HAND_WID_H (handSettings[handStyle][1])
#define HAND_BORD_H (handSettings[handStyle][2])

#define HAND_LEN_M (handSettings[handStyle][3])
#define HAND_WID_M (handSettings[handStyle][4])
#define HAND_BORD_M (handSettings[handStyle][5])

#define HAND_LEN_S (handSettings[handStyle][6])
#define HAND_WID_S (handSettings[handStyle][7])
#define HAND_BORD_S (handSettings[handStyle][8])
const static uint8_t handSettings [HAND_STYLE_N][9] = {
  // Style 0
  {45,10,0, 75,8,2, 20,1,0},
  // Style 1
  {45,10,0, 75,8,2,  20,1,0},
  // Style 2
  {45,10,0, 75,8,2,  20,1,0}
};

// Macro to invert colour
#define INVERT(x) (x == GxEPD_BLACK? GxEPD_WHITE: GxEPD_BLACK)

// How long to allow for the update of the watchface. Adjust this to get hand movement at correct time
#define UPDATE_DELAY_MS 1500

// How long between connecting to NTP for sync. Subtract 2s or it may wait another hour
#define SYNC_INTERVAL (24*SECS_PER_HOUR - 2)
// Hour for first sync attempt (after at least SYNC_INTERVAL seconds have elapsed)
#define SYNC_HOUR 2
// Minute for sync attempts
#define SYNC_MINUTE 5

// Define the names and offsets from NTP time for our normal and daylight saving timezones
#define NORM_TZ "GMT"
#define NORM_TZ_ADD 0
#define DST_TZ "BST"
#define DST_TZ_ADD SECS_PER_HOUR

// Add leading zero for single digit numbers
#define TWO_DIGITS(x) ((x < 10? "0": "") + String(x))

// Minimum timeout for second display (ms). It will stay on this long or till the next 15 minute mark whichever is longer
#define SEC_TIMEOUT_MS (2 * 60 * 1000)

#endif
