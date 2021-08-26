#ifndef NODEBUG
#define DEBSTART { Serial.begin(9600); delay(500); }
#define DEBVAL(a) { Serial.print(#a); Serial.print(":"); Serial.println(a); }
#define DEBMSG(m) Serial.println(m)
#endif
#ifdef NODEBUG
#define DEBSTART
#define DEBVAL(a)
#define DEBMSG(m)
#endif
