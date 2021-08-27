#ifndef TimeNTP
#define TimeNTP
#include <TimeLib.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <Watchy.h>

// NTP Servers:
static const char ntpServerName[] = "pool.ntp.org";

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime(int &mSecs) {
  time_t t = 0;
  IPAddress ntpServerIP; // NTP server's ip address
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin();
    WiFi.waitForConnectResult(); // Relies on having configured a connection previously
  }
  if (WiFi.status() == WL_CONNECTED) {
    Udp.begin(localPort);
    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    // get a random server from the pool
    WiFi.hostByName(ntpServerName, ntpServerIP);
    sendNTPpacket(ntpServerIP);
    uint32_t beginWait = millis();
    // try for 1.5 seconds to get an NTP packet
    while (t == 0 && millis() - beginWait < 1500) {
      int size = Udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) {
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        t = secsSince1900 - 2208988800UL; // convert era for arduino
        // fractional seconds stored in next 4 bytes, 16bits is plenty for milliseconds
        mSecs =  (unsigned long)packetBuffer[44] << 8;
        mSecs |= (unsigned long)packetBuffer[45];
        mSecs = (mSecs*1000) >> 16;
      }
    }
  }
  WiFi.mode(WIFI_OFF);
  btStop();
  return(t); // returns 0 if unable to get the time
}
// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
#endif
