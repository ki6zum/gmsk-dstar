/*
DVforArduino - Copyright (C) 2013
The pin definitions for the software running on the Arduino to run the DV for Arduino GMSK board.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
Boston, MA  02110-1301, USA.
*/

// Pin 13 has an LED connected on most Arduino boards.
#define pinLED 13
#define pinRXCLK 2
#define pinTXCLK 3
#define pinSN 4
#define pinRXD 5
#define pinTXD 6
#define pinCOS 7
#define pinPTT 8
#define pinPLLACQ 9
#define pinRXDCACQ 10
#define pinCOSLED 11
#define pinDebug 12

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)
#define pinDebugPort PORTB
#define pinDebugPin 4
#define pinSNPort PIND
#define pinSNPin 4
#define pinRXDPort PIND
#define pinRXDPin 5
#define pinTXDPort PIND
#define pinTXDPin 6
#define pinCOSPort PIND
#define pinCOSPin 7
#define pinPTTPort PIND
#define pinPTTPin 8

#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define pinDebugPort PORTB
#define pinDebugPin 6
#define pinSNPort PING
#define pinSNPin 5
#define pinRXDPort PINE
#define pinRXDPin 3
#define pinTXDPort PINH
#define pinTXDPin 3
#define pinCOSPort PINH
#define pinCOSPin 4
#define pinPTTPort PINH
#define pinPTTPin 5
#endif


// Atmega328/Atmega1280 pin mapping
// Arduinio  328   1280
//     0     PD0   PE0
//     1     PD1   PE1
//     2     PD2   PE4
//     3     PD3   PE5
//     4     PD4   PG5
//     5     PD5   PE3
//     6     PD6   PH3
//     7     PD7   PH4
//     8     PD8   PH5
//     9     PB1   PH6
//     10    PB2   PB4
//     11    PB3   PB5
//     12    PB4   PB6
//     13    PB5   PB7
