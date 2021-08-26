// Debug helper library - define NODEBUG to entirely remove debug code
#include "debug.h"

// System libraries
#include <Watchy.h>
#include <EEPROM.h>

// Fonts
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

// My libraries for this watchface
#include "watchface/face4.h" // Background watchface onto which we draw hands etc
#include "degtosink.h" // fast approximate trig functions
#include "TimeNTP.h" // NTP lookup

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
            uint16_t wb = (bw == GxEPD_BLACK? GxEPD_WHITE: GxEPD_BLACK); // Inverse colour for box
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
        void drawHand(int angle, int len, int thickness, int colour, int x=100, int y=100) {
          int x0, y0, x1, y1, x2, y2;
          // x2,y2 is location of tip of hand
          x2 = x + (sinK(angle) * len / 1000);
          y2 = y - (cosK(angle) * len / 1000);
          if (thickness == 1) {
            // Just draw a line to the tip
            display.drawLine(x, y, x2, y2, colour);
          }
          else {
            // Draw two triangles, apex1 at center, apex2 at tip, bases same two points
            int diff = thickness-1; // edges of base triangle point to neighbouring minutes to generate thickness
            x0 = x + (sinK(angle-6*diff) * len/2 / 1000);
            y0 = y - (cosK(angle-6*diff) * len/2 / 1000);
            x1 = x + (sinK(angle+6*diff) * len/2 / 1000);
            y1 = y - (cosK(angle+6*diff) * len/2 / 1000);
            // DEBMSG(String(x0)+","+String(y0)+" "+String(x1)+","+String(y1));
            display.fillTriangle(x, y, x0, y0, x1, y1, colour);
            display.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
          }
        }

        //--------------- NTP, sync, timezone --------------
        // Interface function for writing NTP status messages
        void NtpStatus(String msg) {
          statusMsg(msg, 162, 25);
        }

        // Check how long since last sync and get time from NTP if it's time to do so
        time_t NtpSync(bool doitNow) {
          // Can't use static because deepSleep starts from scratch each time. 
          // Need to store persistent values (lastSync) in flash memory using EEPROM libary
          time_t lastSync, timeNow=makeTime(currentTime), retVal=0;
          const int nvmAddr = 0; // address in EEPROM we're using
          const int updateDelayMs = 1500; // how long to allow for the update of the watchface. Adjust this to get hand movement at correct time

          // retrieve last sync time from flash memory
          EEPROM.get(nvmAddr, lastSync);
          //DEBMSG("lastSync (from flash):"+String(lastSync));
          const int syncInterval = 12*SECS_PER_HOUR - 2; // seconds between synchronising RTC with NTP. Subtract 2s or it may wait another hour

          //DEBMSG("timeNow:"+String(timeNow)+", doitNow:"+String(doitNow));
          // only try to sync at five past the hour in case we're away from WiFi
          // which would make the NTP call fail every time and drain the battery
          if (doitNow || lastSync > timeNow || 
            (timeNow - lastSync >= syncInterval && currentTime.Minute == 5)) {
            int mSecs;
            time_t t = getNtpTime(mSecs);
            if (t != 0) {
              time_t rt = RTC.get();
              // Set RTC time to NTP time, setting it fast by enough to allow for the time to draw and display the watchface
              delay(3000-mSecs-updateDelayMs); // 3000ms is the 3s we add in RTC.set to advance the clock, must be whole # of seconds
              RTC.set(t+3);
              // Debug
              DEBMSG("NTP:"+String(hour(t))+":"+String(minute(t))+":"+String(second(t))+"."+String(mSecs));
              DEBMSG("RTC:"+String(hour(rt))+":"+String(minute(rt))+":"+String(second(rt)));
              DEBMSG("OFS:"+String(t-rt));
              DEBVAL((int)(millis()%1000)-mSecs);
              // store time of sync
              EEPROM.put(nvmAddr, t);
              EEPROM.commit();
              retVal=t;
            }
            else {
              DEBMSG("NTP fail");
              NtpStatus("fail");
            }
          }
          else {
            int sToSync = syncInterval-(makeTime(currentTime)-lastSync);
            if (sToSync > SECS_PER_HOUR)
              NtpStatus(String(sToSync / SECS_PER_HOUR)+"h");
            else if (sToSync > SECS_PER_MIN)
              NtpStatus(String(sToSync / SECS_PER_MIN)+"m");
            else if (sToSync > 0)
              NtpStatus(String(sToSync)+"s");
            else NtpStatus("wait");
          }
        return(retVal); // sync time or 0 if sync fails
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
          statusMsg("BST",2,14);
          ct += SECS_PER_HOUR;
          breakTime(ct, currentTime);
        } else
          statusMsg("GMT",2,14);
        return (ct);
        }

        //-------------- Draw watchface ----------------
        
        void drawWatchFace(){ //override Watchy method

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
            drawHand((currentTime.Hour*30 + currentTime.Minute/2), 45, 4, GxEPD_WHITE,85,85);
            
            // minute hand
            drawHand(currentTime.Minute*6, 75, 3, GxEPD_BLACK,85,85);
            drawHand(currentTime.Minute*6, 75, 2, GxEPD_WHITE,85,85);
            
            // second hand
            drawHand(currentTime.Second*6, 20, 1, GxEPD_WHITE,85,85);

            // circle in centre
            display.fillCircle(85,85,6,GxEPD_WHITE);

        }

        //--------- draw seconds watchface ------------

        // This displays a watchface that includes seconds. Unlike the standard one, control is always in this function
        // timing is by delay() and deepSleep is not used. This takes far more battery so there is a timeout as well as an exit button.
        void secondWatchface() {

          // Clear the screen
          display.init(0, false);
          display.setFullWindow();
          display.fillScreen(GxEPD_BLACK);
          
          unsigned long long millis0 = millis();

          // Find what time the RTC switches seconds
          int tickMillis=-1;
          time_t startSec = RTC.get();
          while (true) { 
            if (RTC.get() != startSec) { // Tick just happened
              tickMillis = millis() % 1000;
              break;
            }
          }
          //DEBVAL(tickMillis);
          // Show watchface till the BACK button is pressed or a timeout is reached
          const static int timeoutMs = 5 * 60 * 1000;
          String timeLine, stopwatchLine;
          while (millis() < millis0 + timeoutMs && digitalRead(BACK_BTN_PIN) == 0) {
            // Read time from RTC and adjust for DST
            RTC.read(currentTime);
            adjustForDst();
            
            // Tricky bit: have to allow for the fact that this display is faster than the main one so would tick too soon as RTC is set fast
            // Here we can only handle the whole seconds. We handle the fractional seconds using delayMillis in the final delay() call
            time_t ct = makeTime(currentTime) - 1; const static int delayMillis = 200;
            
            // Print time
            int h = hour(ct), m = minute(ct), s = second(ct);
#define TWO_DIGITS(x) ((x < 10? "0": "") + String(x))
            timeLine = TWO_DIGITS(h) +":"+ TWO_DIGITS(m) +":"+ TWO_DIGITS(s);
            textInBox(&FreeSansBold24pt7b, timeLine, GxEPD_WHITE, 1, 0, 10, false); // centre top
 
            // Print countdown bar
            const static int x0 = 10, w = 180, x2 = x0+w;
            const static int ht = 40, y0 = 120-ht-10;
            // Markers on bar
            for (int i = 0; i<=60; i+=5) {
              bool major = i%15 == 0;
              display.fillRect(x0-1 + i*w/60, y0-(major? 15: 5), 2, (major? 15: 5), GxEPD_WHITE);
            }
            // Now the bar itself
            int x1 = x0 + (second(ct) * w) / 60;
            display.drawRect(x0, y0, x1-x0, ht, GxEPD_WHITE);
            display.fillRect(x1, y0, x2-x1, ht, GxEPD_WHITE);

            // Display stopwatch
            int elapsedS = (millis() - millis0 + 2000) / 1000; // add on two seconds for delay in getting to display
            stopwatchLine = TWO_DIGITS(hour(elapsedS))+":"+TWO_DIGITS(minute(elapsedS))+":"+TWO_DIGITS(second(elapsedS));
            textInBox(&FreeSansBold24pt7b, stopwatchLine, GxEPD_BLACK, 1, 2, 5, true); // centre bottom

            display.display(true);
            if (digitalRead(BACK_BTN_PIN) == 1) break; // Just to make it a bit quicker to sense the button

            // Erase from display ready for next display
            display.fillScreen(GxEPD_BLACK);
            //textInBox(&FreeSansBold24pt7b, topLine, GxEPD_BLACK, 1, 0, 10, false); // centre top
            //textInBox(&FreeSansBold24pt7b, TWO_DIGITS(s), GxEPD_BLACK, 1, 1, 10, false); // centre centre
            //display.fillRect(x0, y0, w, ht, GxEPD_BLACK); // Erase previous countdown bar

            // Wait for next second
            int timeToTick = tickMillis - (millis() % 1000);
            if (timeToTick < 0) timeToTick += 1000;
            //DEBVAL(timeToTick);
            if (timeToTick < 1000) delay(timeToTick+delayMillis); // delayMillis is to match the fractional part of the display delay of the normal watchface
          }
          
        }
        
        //------------ Handle buttons -------
        void watchfaceDownButton() {
          // Immediate feedback
          vibMotor(100, 2);

          // Switch to second-by-second display until exit or timeout
          secondWatchface();
          
          // Erase everything to help avoid ghosting
          display.fillScreen(GxEPD_BLACK);
          
          // Back to watchface (following lines copied from line 160 Watchy.cpp)
          RTC.alarm(ALARM_2); // reset alarm flag in RTC
          RTC.read(currentTime); // read current time ready for showing the watchface
          showWatchFace(false); // full refresh
        }

};


MyFirstWatchFace m; //instantiate your watchface

void setup() {
  DEBSTART;
  EEPROM.begin(4);
  m.init(); //call init in setup
}

void loop() {
    // this should never run, Watchy deep sleeps after init(). Waking from deep sleep restarts from scratch.
}
