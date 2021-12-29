#ifndef DEBUG
#define DEBUG

// Define NODEBUG to remove all debugging code completely
//#define NODEBUG

#ifndef NODEBUG
#define DEBSTART { Serial.begin(500000); delay(500); }
#define DEBVAL(a) { Serial.print(#a); Serial.print(":"); Serial.println(a); }
#define DEBMSG(m) { Serial.println(m); }
// Screendump only works if you add in a line to GxEPD2_BW.h, just before the private: section. Otherwise getBuffer is not defined.
//    uint8_t *getBuffer() { return _buffer; } // to get screendump
#define DEBSCREENDUMP { uint8_t *p = display.getBuffer(); Serial.print("DEBSCREENDUMP");for (int i=0; i<200*200/8; i++) {if (i%25==0) Serial.println(""); Serial.print("0x"); if (p[i]<16) Serial.print("0"); Serial.print(p[i],HEX); Serial.print(",");}; Serial.println("\nEND"); }
#endif

#ifdef NODEBUG
#define DEBSTART
#define DEBVAL(a)
#define DEBMSG(m)
#define DEBSCREENDUMP
#endif

#endif
