/* Lixie-SNTP-RTC-IPv6.ino
   Author: Matthew J. Wolf  <matthew.wolf.hpsdr@speciosus.net>
   Date: ??-JULY-19

   Lixie-SNTP-IPv6:
   A arduino sketch for a six digit Lixie US eastern time clock.
   The sketch uses the internet protocol version 6 (IPv6)
   SNTP client with a DS3231 real time clock.

   - Version 1.0
     = Initial version

   SNTP is a simple subset of the full NTP protocol.

   See RFC4330 for details of the protocol:
   - https://tools.ietf.org/html/rfc4330

   This example demonstrates of the basics of an SNTP client;
   A real-world client should enforce RFC4330 properly, such as
   the randomized time before sending a query at startup.

   This example uses a static MAC address, please update the MAC with your own static MAC address.

   Get your own Random Locally Administered MAC Address here:
   https://www.hellion.org.uk/cgi-bin/randmac.pl

   Based on:
   Lixie-arduino example "ntp_clock"
   https://github.com/connornishijima/Lixie-arduino/tree/master/examples/ESP8266/ntp_clock

   EtherSia example "SNTPClient"
   https://github.com/njh/EtherSia/tree/master/examples/SNTPClient


   Libraries that are used in this sketch:
   -Lixie     : https://github.com/connornishijima/Lixie-arduino
    -= Dependencies:
       -== FastLED
   -Time      : http://playground.arduino.cc/code/time
   -EtherSia  : https://github.com/njh/EtherSia
   -Time      : http://playground.arduino.cc/code/time
   -DS3232RTC : https://github.com/JChristensen/DS3232RTC
   -= Dependencies:
      -== Time
   -Timezone  : http://github.com/JChristensen/Timezone
   -= Dependencies:
      -== Time

   To Do:
   - Button to switch between 12 hour and 24 hour time formats.
   - Add display of calendar date.
   - Function when Ethernet not connected.
     -- Use RTC time.
   - Function when DNS resolution fails.
     -- Use RTC time.

   Copyright (c) 2019 Matthew J. Wolf

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files
   (the "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit
   persons to whom the Software is furnished to do so, subject to the
   following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// BOF preprocessor bug prevent - insert me on top of your arduino-code
#if 1
__asm volatile ("nop");
#endif

// Header includes
#include <Lixie.h>     // Lixie Library
#include <TimeLib.h>   // Time Library
#include <EtherSia.h>  // IPv6 Library
#include <DS3232RTC.h> // DS3231 DS2331 Library
#include <Timezone.h>  // Timezone Library
#include "Lixie-SNTP-RTC-IPv6.h" // Local Defines

// DEBUG - Define for output on serial port.
#define DEBUG

// TWELVE or TWENTYFOUR - Defines for clock display format.
// - Only uncomment one of them.
// TWELVE     - 12 hour format clock display.
// TWENTYFOUR - 24 hour format clock display.
#define TWELVE
//#define TWENTYFOUR

// Global Variables
// NTP Server:
static const char ntpServerName[] = "ntp.speciosus.net";

// How often to send NTP packets - RFC4330 says this shouldn't be less than 15 seconds
const uint32_t DEFAULT_POLLING_INTERVAL = 15;

// Set to true when a DNS lookup needs to be performed
boolean needNewServer = true;

// The time that the next NTP request should be sent
unsigned long nextRequest = millis();

// Time Zone
// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);

char version[] = "v1.0 -- ?? July 2019";

// Global Data Structures
// - Number of Lixie digits
Lixie lix(DATA_PIN, NUM_LIXIES);

// - EtherSia Wiznet W5100 ethernet interface (with Chip Select connected to Pin 10)
EtherSia_W5100 ether(10);

// - Define a UDP socket to send packets from
UDPSocket udp(ether);

void setup() {
  // Initialize Lixie digits
  lix.begin();

  // Lixie green on connection success
  lix.color(0, 255, 0);
  lix.write(9999);
  delay(500);

  // Lixie - Reset colors to default
  lix.color(255, 255, 255);
  lix.clear();

  // Locally Administered Unicast MAC Address
  // Generated at https://www.hellion.org.uk/cgi-bin/randmac.pl
  MACAddress macAddress("d6:53:08:82:5b:ce");

#ifdef DEBUG
  // Setup serial port
  Serial.begin(9600);

  Serial.println("Lixie-SNTP-RTC-IPv6");
  Serial.println(version);
  Serial.println("Copyright 2019 Matthew J Wolf");
  Serial.println("Licensed under MIT license");
#endif

  // Start Ethernet
  if (ether.begin(macAddress) == false) {
#ifdef DEBUG
    Serial.println("Failed to configure Ethernet");
#endif
  }

}

void loop() {

  unsigned long posixTime;
  unsigned long displayTime;

  tmElements_t tm;

  // How often to send NTP packets - RFC4330 says this shouldn't be less than 15 seconds
  unsigned long pollingInterval = DEFAULT_POLLING_INTERVAL;

  // process packets
  ether.receivePacket();

  // Do we need to lookup the IP address of a new server?
  // Using DNS lookup uses less program memory that using a IPv6 128 bit address.
  if (needNewServer) {
    if (udp.setRemoteAddress(ntpServerName, NTP_PORT)) {

      needNewServer = false;
    } else {
      // Something went wrong, sleep for a minute, then try again
      delay(60000);
      return;
    }
  }

  if (udp.havePacket()) {

    // Convert the payload into the NTP structure above
    ntpType *ntpPacket = (ntpType*)udp.payload();

    // Extract the transmit timestamp from the packet
    // The ntohl() function converts from network byte-order to native byte-order
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800.
    // Subtract Unix start time to convert to POSIX / Unix epoch
    posixTime = ntohl(ntpPacket->transmitTimestampSeconds) - 2208988800UL;

    // Get tm time structure from NTP data.
    ntpUtcTm(posixTime, tm);

    // Get calendar date from NTP data.
    ntpGregorianCalTm(posixTime, tm);

#ifdef DEBUG
    Serial.println();
    Serial.println("--NTP--");
    Serial.print("UTC: ");
    Serial.print(tm.Hour, DEC);
    Serial.print(":");
    Serial.print(tm.Minute, DEC);
    Serial.print(":");
    Serial.print(tm.Second, DEC);
    Serial.print(" ");
    Serial.print(tm.Month, DEC);
    Serial.print("-");
    Serial.print(tm.Day, DEC);
    Serial.print("-");
    Serial.println(tm.Year + 1970, DEC);
#endif

    // Write NTP date to real time clock (RTC)
    // -NTP date and time changed to local time zone.
    RTC.set(myTZ.toLocal(makeTime(tm)));

    // Success, server is working; reset the polling interval to default
    pollingInterval = DEFAULT_POLLING_INTERVAL;
    nextRequest = millis() + (pollingInterval * 1000);

  } else {
    // Reject any other incoming packets
    ether.rejectPacket();
  }

  if ((long)(millis() - nextRequest) >= 0) {

    // Set the NTP packet to all-zeros
    ntpType ntpPacket;
    memset(&ntpPacket, 0, sizeof(ntpPacket));

    // Set NTP header flags (Leap Indicator=Not Synced, Version=4, Mode=Client)
    ntpPacket.flags = 0xe3;
    udp.send(&ntpPacket, sizeof(ntpPacket));

    // Exponential back-off; double the polling interval
    // This prevents the server from being overloaded if it is having problems
    pollingInterval *= 2;

    // Set the time of the next request to: now + polling interval
    nextRequest = millis() + (pollingInterval * 1000);
  }

  // Display time

  // Read real time clock
  RTC.read(tm);

#ifdef TWELVE
  // Hour
  if (tm.Hour > 12)
    displayTime = tm.Hour - 12;
  else if (tm.Hour == 0) {
    displayTime = 12;
  } else {
    displayTime = tm.Hour;
  }

  // Minute
  displayTime = (displayTime * 100) + tm.Minute;
  // Second
  displayTime = (displayTime * 100) + tm.Second;

#ifdef TWELVE || defined (DEBUG)
  Serial.println();
  Serial.print("DT:  ");
  Serial.println(displayTime);
#endif

  lix.write(displayTime);

#endif // end of 12 hour ifdef

#ifdef TWENTYFOUR
  // 24 Hour Format
  // - The Lixie Library write() fuction will not
  //   display a leading zero. When the hour is 0
  //   push individual Lixie digits then refresh
  //   the display.
  if ( tm.Hour == 0) {

#ifdef TWENTYFOUR || defined (DEBUG)
    Serial.println();
    Serial.println("Zero Hour: pushing individual"
                   " digits");
#endif

    // Hour
    lix.push_digit(tm.Hour / 10);
    lix.push_digit(tm.Hour % 10);
    // Minute
    lix.push_digit(tm.Minute / 10);
    lix.push_digit(tm.Minute % 10);
    // Second
    lix.push_digit(tm.Second / 10);
    lix.push_digit(tm.Second % 10);

    lix.show();

  } else {

    // Hour
    displayTime = tm.Hour;

    // Add the minute
    displayTime = (displayTime * 100) + tm.Minute;

    // Add the seconds
    displayTime = (displayTime * 100) + tm.Second;

#ifdef TWENTYFOUR || defined (DEBUG)
    Serial.println();
    Serial.print("DT:  ");
    Serial.println(displayTime);
#endif

    lix.write(displayTime);
  }

#endif // 24 hour display ifdef



#ifdef DEBUG
  Serial.print("RTC: ");
  Serial.print(tm.Hour, DEC);
  Serial.print(':');
  Serial.print(tm.Minute, DEC);
  Serial.print(':');
  Serial.print(tm.Second, DEC);
  Serial.print(" ");
  Serial.print(tm.Month, DEC);
  Serial.print('-');
  Serial.print(tm.Day, DEC);
  Serial.print('-');
  Serial.println(tm.Year + 1970, DEC);
#endif

  delay(500);

}

// Function to convert NTP posix time into a tm structure.
// - Algorithm from From EtherSia "SNTPClient" displayTime()
// Inputs:
//        posix - NTP posix time
//        &tm   - Address of Time library tm structure
void ntpUtcTm(unsigned long posix, tmElements_t &tm)
{

  // Get the current UTC hour
  // 86400 seconds in a 24 hours (a day)
  // 3600 seconds in hour
  tm.Hour = (posix  % 86400L) / 3600;

  // Get the UTC minute
  // 3600 seconds in hour
  tm.Minute = (posix % 3600) / 60;

  // Get the UTC seconds
  // 60 seconds in a minute
  tm.Second = (posix % 60);

}

// Function to get the gregorian calendar from the NTP posix date.
// - The gregorian date is placed into a tm structure.
// - References
// https://www.jotform.com/help/443-Mastering-Date-and-Time-Calculation
// https://www.epochconverter.com
// Inputs:
//        posix - NTP posix time
//        &tm   - Address of Time library tm structure
void ntpGregorianCalTm(unsigned long posix, tmElements_t &tm)
{
  time_t local_time = posix;

  // Get Month
  // Total number of months.
  // 2629743 seconds in month.
  // Total number of months: posix / 2629743
  // 12 Months in a year
  // Total number of months % 12
  // Add 1 for humand number
  tm.Month = ((posix / 2629743L) % 12) + 1;

  // Get Day of month
  // Using Time library becauce of leap years.
  tm.Day = day(local_time);

  // Get Year since 1970
  // 31556926 seconds in year
  tm.Year = (posix / 31556926L);
}
