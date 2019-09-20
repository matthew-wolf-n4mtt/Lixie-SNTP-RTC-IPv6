/* Lixie-SNTP-IPv6.h
 * Author: Matthew J. Wolf  <matthew.wolf.hpsdr@speciosus.net>
 * Date: ??-JAN-19
 * 
 * Header file for Lixie-SNTP-IPv6
 * 
 * - Version 0.1
 *   = Initial version
 * 
 * Copyright (c) 2019 Matthew J. Wolf
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files  
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the 
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// Preprocessor Defines
// - From Lixie-arduino "ntp_clock"
#define DATA_PIN   5
#define NUM_LIXIES 6

// The UDP port number to send NTP packets to
const uint8_t NTP_PORT = 123;

// Data structure for an NTP packet, based on RFC4330 
typedef struct ntpStructure {
    // Leap Indicator, Version Number and Mode fields
    uint8_t flags;

    // Stratum number; distance from a primary reference time source
    uint8_t stratum;

    // Max Poll interval (exponent of 2 in seconds)
    uint8_t poll;

    // Precision of the system clock (exponent of 2 in seconds)
    int8_t precision;

    // Total roundtrip delay to the primary reference source
    int32_t rootDelay;

    // Maximum error due to the clock frequency tolerance
    uint32_t rootDispersion;

    // Identifier for the upstream reference source
    uint32_t referenceIdentifer;

    // The time the server clock was last set or corrected (in seconds)
    uint32_t referenceTimestampSeconds;

    // The time the server clock was last set or corrected (fractions of a second)
    uint32_t referenceTimestampFraction;

    // The time the the request departed the client for the server (in seconds)
    uint32_t originateTimestampSeconds;

    // The time the the request departed the client for the server (fractions of a second)
    uint32_t originateTimestampFraction;

    // The time at which the request arrived at the server or the reply arrived at
    //    the client (in seconds)
    uint32_t receiveTimestampSeconds;

    // The time at which the request arrived at the server or the reply arrived at
    // the client (fractions of a second)
    uint32_t receiveTimestampFraction;

    // The time at which the request departed the client or the reply departed
    // the server (in seconds)
    uint32_t transmitTimestampSeconds;

    // The time at which the request departed the client or the reply departed
    // the server (fractions of a second)
    uint32_t transmitTimestampFraction;
} __attribute__((__packed__)) ntpType;
