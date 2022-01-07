// Debug helper library - define NODEBUG to entirely remove debug code
#include "debug.h"

// System libraries
#include <Watchy.h>

// Fonts
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// My libraries for this watchface
#include "Watchy-Analog-1.h" // Settings for variables in the watchface
#include "degtosink.h" // fast approximate trig functions
#include "TimeNTP.h" // NTP lookup

// Need to store persistent values in the ESP32's RTC memory to survive deepSleep
RTC_DATA_ATTR time_t lastSync=0, lastSyncAttempt=0; // Time of last successful NTP sync and time we last tried
RTC_DATA_ATTR int lastSyncErr; // Error in seconds at last sync. +ve means RTC was fast

class MyFirstWatchFace : public Watchy{ //inherit and extend Watchy class
    public:

        //--------------- Utility functions -----------------

        // this dumps the text onto the screen near the top, shifting down one line each time it's called
        int printDebugLine=1;
        void printDebug(String text) {
            display.setCursor(40, 40 + printDebugLine*20);
            display.setFont(&FreeSans9pt7b);
            display.print(text);
            printDebugLine++;
        }

        void statusMsg(String msg, int x, int y) {
          display.setCursor(x, y);
          display.setFont(&FreeSans9pt7b);
          display.setTextColor(GxEPD_BLACK);
          display.print(msg);          
        }

        // this prints text in a box that is the opposite colour to the text
        void textInBox(const GFXfont *font, String text, uint16_t bw, int xAlign, int yAlign, int pad, bool drawBox=true) {
            // Align is 0: Left/Top, 1: Centre, 2: Right/Bottom
            int16_t  x, y;
            uint16_t w, h;
            uint16_t wb = INVERT(bw); // Inverse colour for box
            display.setFont(font);
            display.getTextBounds(text, 10, 100, &x, &y, &w, &h);
            // Analyse the results
            int16_t offsX = x-10, offsY = y+h - 100;
            int16_t boxH = h + 2*pad, boxW = w + 2*pad;
            // printDebug(String(x)+","+String(y)+","+String(w)+","+String(h)); display.setFont(font);
            // Now find top left co-ordinate of box
            int16_t boxX = (xAlign == 0? 0: xAlign == 1? 100 - boxW/2: 200 - boxW);
            int16_t boxY = (yAlign == 0? 0: yAlign == 1? 100 - boxH/2: 200 - boxH);
            if (drawBox) display.fillRect(boxX, boxY, boxW, boxH, wb); // Draw the box
            display.setCursor(boxX + pad - offsX, boxY + pad + h - offsY); // Centre text in box
            display.setTextColor(bw);
            display.print(text);
        }
        
        // -----------  Draw a hand ----------------
        void drawHand(int angle, int len, int width, int colour, int xc=100, int yc=100, int border=0, uint8_t variant='M') {
          
          if (width == 0) return; // Nothing to draw

          int xt = xc + FROMK(sinK(angle) * len);
          int yt = yc - FROMK(cosK(angle) * len);
          
          if (width == 1) { // Just draw a line to the tip
            // xt,yt is location of tip of hand
            display.drawLine(xc, yc, xt, yt, colour);
            return;
          }
          
          // Special cases resolved, now the hard stuff
          int x0, y0, x1, y1, x2, y2, x3, y3;

          // Draw border round hand if required
          if (border > 0) {
            // Need border around hand. Just draw a slightly bigger hand in the opposite colour behind it
            drawHand(angle, len + 2*border, width+2*border, INVERT(colour), xc - FROMK(border*sinK(angle)), yc + FROMK(border*cosK(angle)), 0, variant);
          }

          if (handStyle == 0) { // flare out to midpoint then back to tip
            // Draw two triangles, apex1 at center, apex2 at tip, bases same two points
            x0 = xc + FROMK(sinK(angle-width) * len/2);
            y0 = yc - FROMK(cosK(angle-width) * len/2);
            x1 = xc + FROMK(sinK(angle+width) * len/2);
            y1 = yc - FROMK(cosK(angle+width) * len/2);
            // DEBMSG(String(x0)+","+String(y0)+" "+String(x1)+","+String(y1));
            display.fillTriangle(xc, yc, x0, y0, x1, y1, colour);
            display.fillTriangle(x0, y0, x1, y1, xt, yt, colour);
            if (variant == 'H') display.fillCircle((x0+x1)/2, (y0+y1)/2, 8, colour);
          }
          else if (handStyle == 1) { // oblong hands with tipLen length pointer
            // Draw three triangles, two to make an oblong hand, one to make the tip
            const int tipLen = 20;
            int handLen = len-tipLen;
            int dx = FROMK(cosK(angle)*width), xLen = FROMK(handLen * sinK(angle));
            x0 = xc - dx/2;
            x1 = x0 + dx;
            x2 = xc + xLen - dx/2;
            x3 = x2 + dx;
            int dy = FROMK(sinK(angle)*width), yLen = FROMK(handLen * cosK(angle));
            y0 = yc - dy/2;
            y1 = y0 + dy;
            y2 = yc - yLen - dy/2;
            y3 = y2 + dy;
            //DEBVAL(angle); DEBVAL(len);
            //DEBVAL(x0);DEBVAL(y0);DEBVAL(x1);DEBVAL(y1);
            //DEBVAL(x2);DEBVAL(y2);DEBVAL(x3);DEBVAL(y3);
            display.fillTriangle(x0, y0, x1, y1, x2, y2, colour); // First half of oblong
            display.fillTriangle(x1, y1, x2, y2, x3, y3, colour); // Second half
            display.fillTriangle(x2, y2, x3, y3, xt, yt, colour); // Tip
          }
          else if (handStyle == 2) { // oblong hands with semicircle tip and circle highlight
            // Draw two triangles to make an oblong hand, then one circle to make the tip
            int handLen = len - width/2;
            int dx = FROMK(cosK(angle)*width), xLen = FROMK(handLen * sinK(angle));
            x0 = xc - dx/2;
            x1 = x0 + dx;
            x2 = xc + xLen - dx/2;
            x3 = x2 + dx;
            xt = xc + xLen;
            int dy = FROMK(sinK(angle)*width), yLen = FROMK(handLen * cosK(angle));
            y0 = yc - dy/2;
            y1 = y0 + dy;
            y2 = yc - yLen - dy/2;
            y3 = y2 + dy;
            yt = yc - yLen;
            //DEBVAL(angle); DEBVAL(len);
            //DEBVAL(x0);DEBVAL(y0);DEBVAL(x1);DEBVAL(y1);
            //DEBVAL(x2);DEBVAL(y2);DEBVAL(x3);DEBVAL(y3);
            display.fillTriangle(x0, y0, x1, y1, x2, y2, colour); // First half of oblong
            display.fillTriangle(x1, y1, x2, y2, x3, y3, colour); // Second half
            display.fillCircle(xt, yt, width/2, colour); // Circle for tip
            if (width >= 4) display.fillCircle(xt, yt, width/4, INVERT(colour)); // Highlight in tip
            // Draw contrasting line in last part of hand
            int xl = xc + FROMK((handLen - 10)*sinK(angle));
            int yl = yc - FROMK((handLen - 10)*cosK(angle));
            display.drawLine(xl, yl, xt, yt, INVERT(colour));
          }
        }

        //--------------- NTP, sync, timezone --------------
        // Interface function for writing NTP status messages
        void NtpStatus(String msg) {
          statusMsg(msg, 162, 25);
        }
        
        // Check how long since last sync and get time from NTP if it's time to do so        
        time_t NtpSync(bool doitNow) {
          time_t timeNow=makeTime(currentTime), retVal=0;
          // See if we need to sync with NTP
          bool syncRequired = doitNow || // we've explicitly asked for a sync
                              lastSync > timeNow || lastSyncAttempt == 0; // means lost battery power to RTC or reloaded firmware
          if (!syncRequired) { // Nothing strange, so see if it's time to sync
            if (timeNow - lastSync >= SYNC_INTERVAL && currentTime.Minute == SYNC_MINUTE) { // Long enough since last sync and the right minute
                // If it's the first attempt wait till the defined hour, otherwise try every hour (not more often as would drain battery when away from WiFi)
                syncRequired = (lastSyncAttempt < lastSync? currentTime.Hour == SYNC_HOUR: true);
            }
          }
          if (syncRequired) {
            int mSecs;
            time_t t = getNtpTime(mSecs); // Returns the fractional part of the second in the mSecs parameter
            lastSyncAttempt = timeNow;
            if (t != 0) {
              // First wait a bit so when we set the RTC to t+3 it will be exactly UPDATE_DELAY_MS fast. 
              // That way the hands will move at exactly the right time. Set UPDATE_DELAY_MS in the .h file according to how complex the face is
              delay(3000-mSecs-UPDATE_DELAY_MS); 
              
              tmElements_t te, te2;
              // Save current RTC time to calculate the error later
              RTC.read(te);
              // Set the RTC 3 seconds fast (see above)
              breakTime(t+3, te2);RTC.set(te2); 
              // Update the persistent variables for this sync
              lastSync = t+3;
              lastSyncErr = makeTime(te) - (t+3); // Negative if RTC is running slow
              // Return the NTP time (not the RTC time)
              retVal=t;
            }
            else {
              DEBMSG("NTP fail");
              NtpStatus("fail");
            }
          }
          else {
            int sToSync = SYNC_INTERVAL-(makeTime(currentTime)-lastSync);
            if (sToSync > SECS_PER_HOUR)
              NtpStatus(String(sToSync / SECS_PER_HOUR)+"h");
            else if (sToSync > SECS_PER_MIN)
              NtpStatus(String(sToSync / SECS_PER_MIN)+"m");
            else if (sToSync > 0)
              NtpStatus(String(sToSync)+"s");
            else NtpStatus("wait");
          }
        // Whatever happens, display the most recent sync error
        String msg;
        if (lastSyncErr == 0) msg = "-"; // RTC was correct
        else if (abs(lastSyncErr)>100) msg = "--"; // Get a huge error the first time the watch is started up
        else msg = String(abs(lastSyncErr)) + (lastSyncErr <= 0? "s": "f"); // Indicate how fast or slow the RTC was
        statusMsg(msg, 165, 50);
        
        return(retVal); // sync NTP time or 0 if sync fails
        }

      // Simplified algorithm for detecting daylight saving time and correcting time accordingly
      // This works when DST is last Sunday of March to last Sunday of October and clocks change at 0300  
      time_t adjustForDst() {
        bool Dst;
        time_t ct = makeTime(currentTime);
        // First the easy cases
        if ((currentTime.Month > 3 && currentTime.Month < 10) ||
          (currentTime.Month == 10 && currentTime.Day < 28)) Dst = true;
        else if (currentTime.Month < 3 || 
          currentTime.Month > 10 || 
          (currentTime.Month == 3 && currentTime.Day < 28)) Dst = false;
        else { // complicated! Need the date of the last Sunday of Mar/Oct
          tmElements_t endMonth = currentTime;
          endMonth.Day = 31;
          time_t lastSunday = previousSunday(makeTime(endMonth));
          if (ct >= lastSunday + 3 * SECS_PER_HOUR) // After 3am on last Sunday of the month
            Dst = (currentTime.Month == 3);         // is DST for March, not for October
        }
        // Need to change status messages and "ct +=" line for different time zones
        if (Dst) {
          //DEBVAL(Dst);
          statusMsg(DST_TZ,2,14);
          ct += DST_TZ_ADD;
          breakTime(ct, currentTime);
        } else
          statusMsg(NORM_TZ,2,14);
          ct += NORM_TZ_ADD;
        return (ct);
        }

        //-------------- Draw watchface ----------------
        
        void drawWatchFace(){ //override Watchy method
            // Rotate screen by 180 degrees if watch case is rotated
            if (CASE_ROTATED) display.setRotation(2);

            // start with background image
            display.drawBitmap(0, 0, watchfaceBitmap, 200, 200, GxEPD_WHITE);
     
            // Sync with NTP if required
            NtpSync(false);

            // sort DST
            //DEBVAL(currentTime.Hour);
            adjustForDst();
            //DEBVAL(currentTime.Hour);
            
            // day number
            String dayNum = (currentTime.Day < 10 ? "0" : "") + String(currentTime.Day);
            textInBox(&FreeSansBold24pt7b, dayNum, GxEPD_BLACK, 2, 2, 0, false);

            // day name (e.g. Mon)
            String dayName = dayShortStr(currentTime.Wday);
            //dayName = dayName.substring(0,wDay.length() - 1);
            textInBox(&FreeSansBold24pt7b, dayName, GxEPD_BLACK, 0, 2, 0, false);

            // draw battery
            static int x0 = 175, w = 10;
            static int y0 = 105, h = 40, y2 = y0-h;
            static int minV=33, maxV=45; // min and max voltages in tenths
            // printDebug(String(getBatteryVoltage()));
            int y1 = y0 - ((getBatteryVoltage()*10 - minV) * (y0-y2) / (maxV-minV)); // scale the current voltage into the meter
            if (y1 > y0) { // battery bar is below the bottom of the meter
              textInBox(&FreeSansBold24pt7b, "X", GxEPD_BLACK, 2, 1, 2, false);
            }
            else { // draw the battery meter
              display.fillRect(x0, y1, w, y0-y1,GxEPD_BLACK);
              display.drawRect(x0, y2, w, y1-y2,GxEPD_BLACK);           
            }


            // hour hand
            drawHand((currentTime.Hour*30 + currentTime.Minute/2), HAND_LEN_H, HAND_WID_H, GxEPD_WHITE,DIAL_XC,DIAL_YC,HAND_BORD_H, 'H');
            
            // minute hand
            drawHand(currentTime.Minute*6, HAND_LEN_M, HAND_WID_M, GxEPD_WHITE,DIAL_XC,DIAL_YC,HAND_BORD_M, 'M');
            
            // second hand
            drawHand(currentTime.Second*6, HAND_LEN_S, HAND_WID_S, GxEPD_WHITE,DIAL_XC,DIAL_YC,HAND_BORD_S, 'S');

            // circle in centre
            display.fillCircle(DIAL_XC,DIAL_YC,DIAL_CR,  GxEPD_BLACK);
            display.fillCircle(DIAL_XC,DIAL_YC,DIAL_CR-2,GxEPD_WHITE);
            //DEBSCREENDUMP;

        }

        //--------- draw seconds watchface ------------
        // Print countdown bar
        void countdownBar(int x0, int y0, int n, int maxN, int ht, int minorTicks, int majorInt) {
          int w = 200-2*x0, x2 = x0+w;
          // Markers on bar
          for (int i = 0; i<=minorTicks; i++) {
            bool major = i%majorInt == 0;
            display.fillRect(x0-1 + i*w/minorTicks, y0-(major? 10: 3), 2, (major? 10: 3), GxEPD_WHITE);
          }
          // Now the bar itself
          int x1 = x0 + (n * w) / maxN;
          display.drawRect(x0, y0, x1-x0, ht, GxEPD_WHITE);
          display.fillRect(x1, y0, x2-x1, ht, GxEPD_WHITE);
        }

        // This displays a watchface that includes seconds. Unlike the standard one, control is always in this function
        // timing is by delay() and deepSleep is not used. This takes far more battery so there is a timeout as well as an exit button.
        void secondWatchface() {
          // Clear the screen
          display.init(0, false);
          display.setFullWindow();
          display.fillScreen(GxEPD_BLACK);
          // Rotate screen by 180 degrees if watch case is rotated
          if (CASE_ROTATED) display.setRotation(2);
          
          unsigned long long millis0 = millis();

          // Find what time the RTC switches seconds
          int tickMillis=-1;
          tmElements_t te;
          RTC.read(te);
          uint startSec = te.Second;
          while (true) {
            RTC.read(te);
            if (te.Second != startSec) { // Tick just happened
              tickMillis = millis() % 1000;
              break;
            }
          }
          //DEBVAL(tickMillis);
          // Show watchface till the BACK button is pressed or we go past the next 15 minute mark after the timeout
          String timeLine, stopwatchLine;
          while ((millis() - millis0 < SEC_TIMEOUT_MS || currentTime.Minute % 15 != 1) && digitalRead(ROTATED_BACK_BTN_PIN) == 0) {
            // Read time from RTC and adjust for DST
            RTC.read(currentTime);
            adjustForDst();
            
            // Tricky bit: have to allow for the fact that this display is faster than the main one so would tick too soon as RTC is set fast
            // Here we can only handle the whole seconds. We handle the fractional seconds using delayMillis in the final delay() call
            time_t ct = makeTime(currentTime) - 1; const static int delayMillis = 200;
            
            // Print time
            int h = hour(ct), m = minute(ct), s = second(ct);
            timeLine = TWO_DIGITS(h) +":"+ TWO_DIGITS(m) +":"+ TWO_DIGITS(s);
            textInBox(&FreeSansBold24pt7b, timeLine, GxEPD_WHITE, 0, 0, 10, false); // left top
 
            // Print countdown bar for minutes
            countdownBar(10, 60, (m%15)*60+s, 15*60, 30, 15, 5);
            // Print countdown bar for seconds
            countdownBar(10, 100, s, 60, 20, 12, 3);
            
            // Display elapsed time since entering seconds watchface
            int elapsedS = (millis() - millis0 + 2000) / 1000; // add on two seconds for delay in getting to display
            display.fillRect(0, 130, 200, 70, GxEPD_WHITE); // White background for elapsed time area
            display.setCursor(5, 153);
            display.setFont(&FreeSansBold12pt7b);
            display.setTextColor(GxEPD_BLACK);
            display.print("ELAPSED");          
            stopwatchLine = TWO_DIGITS(hour(elapsedS))+":"+TWO_DIGITS(minute(elapsedS))+":"+TWO_DIGITS(second(elapsedS));
            textInBox(&FreeSansBold24pt7b, stopwatchLine, GxEPD_BLACK, 0, 2, 5, false); // left bottom

            //DEBSCREENDUMP;
            display.display(true);
            if (digitalRead(ROTATED_BACK_BTN_PIN) == 1) break; // Just to make it a bit quicker to sense the button

            // Erase from display ready for next display
            display.fillScreen(GxEPD_BLACK);

            // Wait for next second
            int timeToTick = tickMillis - (millis() % 1000);
            if (timeToTick < 0) timeToTick += 1000;
            //DEBVAL(timeToTick);
            if (timeToTick < 1000) delay(timeToTick+delayMillis); // delayMillis is to match the fractional part of the display delay of the normal watchface
          }
        // Finished with seconds display, back to normal watchface. 
        // First give feedback
        vibMotor(100, 2);
        // Erase everything to help avoid ghosting
        display.fillScreen(GxEPD_BLACK);
        
        // Back to watchface (following lines copied from line 194 Watchy.cpp)
        RTC.read(currentTime); // read current time ready for showing the watchface
        showWatchFace(false); // full refresh          
        }
        
        //------------ Handle buttons -------
        void processButton(int b) {
          // Immediate feedback
          vibMotor(100, 2);

          if ((b == 2 && CASE_ROTATED) || (b == 1 && !CASE_ROTATED)) {
            // Switch to second-by-second display until exit or timeout
            secondWatchface();
          }
          else {
            handStyle = (handStyle + 1) % HAND_STYLE_N;
            // Refresh watchface (following lines copied from line 194 Watchy.cpp)
            RTC.read(currentTime); // read current time ready for showing the watchface
            showWatchFace(false); // full refresh            
          }
        }
        void watchfaceDownButton() {
          processButton(1);
        }
        
        void watchfaceBackButton() {
          processButton(2);
        }

};


MyFirstWatchFace m; //instantiate your watchface

void setup() {
  DEBSTART;
  m.init(); //call init in setup
}

void loop() {
    // this should never run, Watchy deep sleeps after init(). Waking from deep sleep restarts from scratch.
}
