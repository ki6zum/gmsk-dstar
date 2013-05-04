/*
RX_SN_display - Copyright (C) 2013
A simple program that monitors the RX_SN pin the of the CMX589 chip to determine current bit error rates

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

#include <stdint.h>
#include "pindefs.h"

/*
#define CLR(x,y) (x&=(~(1<<y)))
#define SET(x,y) (x|=(1<<y))
#define GET(x,y) ((x>>y)&1)
#define _BV(bit) (1 << (bit))
#define sbi(port,bit) (port)|=(1<<(bit))
*/

#define GET(x,y) ((x>>y)&1)
#define SET(x,y) x |= _BV(y) //set bit - using bitwise OR operator 
#define CLR(x,y) x &= ~(_BV(y)) //clear bit - using bitwise AND operator
#define INV(x,y) x ^= _BV(y) //toggle bit - using bitwise XOR operator
#define is_high(x,y) (x & _BV(y) == _BV(y)) //check if the y'th bit of register 'x' is high ... test if its AND with 1 is 1

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif 
#ifndef inb
#define inb(sfr, bit) ((_SFR_BYTE(sfr)>>bit)&1)
#endif

//% high, BER, S/N (dB)
// values taken from graphs in CMX589 data sheet and intermediate points were interpolated
   typedef struct RX_SN {
       int percentHigh;
       float BER;
       float SN;
   } rx_sn;

   rx_sn rxsn[] = {
        0, 0, 0,
        1, 0, 0,
        2, 0, 0,
        3, 0, 0,
        4, 0, 0,
        5, 0, 0,
        6, 0, 0,
        7, 0, 0,
        8, 0, 0,
        9, 0, 0,
        10, 0, 0,
        11, 0, 0,
        12, 0, 0,
        13, 0, 0,
        14, 0, 0,
        15, 0, 0,
        16, 0, 0,
        17, 0, 0,
        18, 0, 0,
        19, 0, 0,
        20, 0, 0,
        21, 0, 0,
        22, 0, 0,
        23, 0, 0,
        24, 0, 0,
        25, 0, 0,
        26, 0, 0,
        27, 0, 0,
        28, 0, 0,
        29, 0, 0,
        30, 0, 0,
        31, 0, 0,
        32, 0, 0,
        33, 0, 0,
        34, 0, 0,
        35, 0, 0,
        36, 0, 0,
        37, 0, 0,
        38, 0, 0,
        39, 0, 0,
        40, 1.78E-02, 5.000,
        41, 1.68E-02, 5.125,
        42, 1.58E-02, 5.250,
        43, 1.49E-02, 5.375,
        44, 1.40E-02, 5.500,
        45, 1.32E-02, 5.625,
        46, 1.24E-02, 5.750,
        47, 1.17E-02, 5.875,
        48, 1.10E-02, 6.000,
        49, 1.03E-02, 6.100,
        50, 9.68E-03, 6.200,
        51, 9.09E-03, 6.300,
        52, 8.53E-03, 6.400,
        53, 8.00E-03, 6.500,
        54, 7.28E-03, 6.625,
        55, 6.63E-03, 6.750,
        56, 6.04E-03, 6.875,
        57, 5.50E-03, 7.000,
        58, 4.91E-03, 7.125,
        59, 4.39E-03, 7.250,
        60, 3.92E-03, 7.375,
        61, 3.50E-03, 7.500,
        62, 3.04E-03, 7.625,
        63, 2.65E-03, 7.750,
        64, 2.30E-03, 7.875,
        65, 2.00E-03, 8.000,
        66, 1.72E-03, 8.125,
        67, 1.48E-03, 8.250,
        68, 1.28E-03, 8.375,
        69, 1.10E-03, 8.500,
        70, 9.45E-04, 8.625,
        71, 8.12E-04, 8.750,
        72, 6.98E-04, 8.875,
        73, 6.00E-04, 9.000,
        74, 4.92E-04, 9.167,
        75, 4.03E-04, 9.333,
        76, 3.30E-04, 9.500,
        77, 2.71E-04, 9.625,
        78, 2.22E-04, 9.750,
        79, 1.83E-04, 9.875,
        80, 1.50E-04, 10.000,
        81, 1.22E-04, 10.167,
        82, 9.86E-05, 10.333,
        83, 8.00E-05, 10.500,
        84, 6.35E-05, 10.667,
        85, 5.04E-05, 10.833,
        86, 4.00E-05, 11.000,
        87, 3.17E-05, 11.167,
        88, 2.52E-05, 11.333,
        89, 2.00E-05, 11.500,
        90, 1.59E-05, 11.667,
        91, 1.26E-05, 11.833,
        92, 1.00E-05, 12.000,
        93, 7.37E-06, 12.167,
        94, 5.43E-06, 12.333,
        95, 4.00E-06, 12.500,
        96, 2.95E-06, 12.667,
        97, 2.17E-06, 12.833,
        98, 1.60E-06, 13.000,
        99, 1.18E-06, 13.167,
        100, 8.69E-07, 13.333
      };

uint16_t rxsnCount;
uint16_t rxsnSum;
uint8_t rxsnValid;

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

// the setup routine runs once when you press reset:
void setup() {      
//char string[64];
// initialize the digital pins as input or output
//  pinMode(pinLED, OUTPUT);    
  pinMode(pinSN, INPUT);    
  pinMode(pinRXD, INPUT);    
  pinMode(pinTXD, OUTPUT);    
  pinMode(pinCOS, INPUT);    
  pinMode(pinPTT, OUTPUT);    
  pinMode(pinPLLACQ, OUTPUT);    
  pinMode(pinCOSLED, OUTPUT);
  pinMode(pinDebug, OUTPUT);

//  CLR(pinPTTPort,pinPTTPin);
  digitalWrite(pinPTT, LOW);
//  CLR(pinDebugPort,pinDebugPin);
  digitalWrite(pinDebug, LOW);

  digitalWrite(pinCOSLED, LOW);
  digitalWrite(pinPLLACQ, 1);

    rxsnCount = 0;
    rxsnSum = 0;
    
    Serial.begin(115200);
    Serial.flush();
    Serial.println("RX_SN_display  Copyright (C) 2013\n");
    Serial.println("This program comes with ABSOLUTELY NO WARRANTY;");
    Serial.println("This is free software, and you are welcome to redistribute it");
    Serial.println("under certain conditions;");
    Serial.println("Free SRAM at startup ");
    Serial.println(freeRam(),DEC);

// initialize the tx and rx interrupts
    attachInterrupt(0, rxINT, FALLING);
}



// the loop routine runs over and over again forever:
void loop() {
char string[64];

// when the count gets to 1000 use the rx_sn sum to display the BER and S/N
    if (rxsnCount==1000) {
        sprintf(string,"%% high: %d, BER: ", rxsn[rxsnSum/10].percentHigh);
        Serial.print(string);
        dtostre(rxsn[rxsnSum/10].BER, string, 2, NULL);
        Serial.print(string);
        Serial.print(", SN: ");
        dtostrf(rxsn[rxsnSum/10].SN, 4, 2, string);
        Serial.println(string);
        rxsnSum = 0;  
        rxsnCount = 0;  
    }

} // end "loop"

/*
        if (toggle==0)
            CLR(pinTXDPort,pinTXDPin);
        else
            SET(pinTXDPort,pinTXDPin);
        toggle ^= 1;
*/    






void rxINT()
{
uint8_t SN;

    // Read the state of the data lines
    // using the PIND method to read the pin takes a LOT less time than digitalRead
//    SN  = GET(pinSNPort,pinSNPin);
//    SN = digitalRead(pinSN);

    // sum the RX_SN bits for 1000 bit times then do nothing until main loop uses the data and resets the count
    if (rxsnCount++<1000) {
        rxsnSum+=digitalRead(pinSN);
    }

} // end rxint


/*
void exampleheader()
{
unsigned char testdata[]={
		0x00,0x00,0x00,
		0x44,0x49,0x52,0x45,0x43,0x54,0x20,0x20,	// "DIRECT  "
		0x44,0x49,0x52,0x45,0x43,0x54,0x20,0x20,	// "DIRECT  "
		0x43,0x51,0x43,0x51,0x43,0x51,0x20,0x20,	// "CQCQCQ  "
		0x37,0x4d,0x33,0x54,0x4a,0x5a,0x20,0x43,	// "7M3TJZ C"
		0x20,0x20,0x20,0x20,				// "    "
		0x00,0x00,					// CRC
		0x00						// DUMMY for convolution
		};



unsigned char	bb[83];
unsigned char	temp[700],dd[41];

unsigned short	crc_dstar_ffff;

int	i,j,k;

//  Encode Final  Out put 

	for (i = 0 ; i < 83 ; i++)
	{
		printf ("%2.2x ",bb[i]);
	}	

	printf ("\n");

// Decode Step 1. DeScramble 
// same as scramble of encode 
	for (i = 0 ; i < 83 ; i++)
	{
		bb[i] = bb[i] ^ scramble[i];		
	}	

// Decode Step 2. DeInterleave 
	interleave_decode (temp, bb);

// Decode Step 3. DeConvolustion (Viterbi method) 
	viterbi (dd,temp);

	for (i = 0 ; i < 41 ; i++)
	{
		printf ("%2.2x ",dd[i]);
	}

}

*/








