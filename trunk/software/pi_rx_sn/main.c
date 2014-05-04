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

#include <sys/time.h>
#include <stdio.h>
#include <wiringPi.h>
#include <stdint.h>
#include <stdbool.h>

#define pinRXCLK 6
#define pinSN 4
#define pinPTT 3
#define pinCOSLED 2

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

void txINT(void)
{
return;
}


void rxINT(void)
{
uint8_t SN;

    // sum the RX_SN bits for 1000 bit times then do nothing until main loop uses the data and resets the count
    if (rxsnCount++<1000) {
        rxsnSum+=digitalRead(pinSN);
    }
}


int main (void)
{
    printf("RasPi RX_SN display 0.1\n") ;
    printf("This program comes with ABSOLUTELY NO WARRANTY;\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions;\n");

    if (wiringPiSetup () == -1) {
        printf("wiringPin setup failed\n");
        return 0;
    }

    // setup the RXSN pin and the RXCLK pin to be in input mode
    pinMode(pinSN, INPUT);
    pinMode(pinRXCLK, INPUT);
    pinMode(pinCOSLED, OUTPUT);
    pinMode(pinPTT, OUTPUT);
    digitalWrite(pinPTT, LOW);
    digitalWrite(pinCOSLED, LOW);

    // setup the receive interrupt
    if (wiringPiISR(pinRXCLK, INT_EDGE_FALLING, rxINT) == -1) {
        printf("isr setup failed\n");
        return 0;
    }

    // set the counter and sum to zero
    rxsnCount = 0;
    rxsnSum = 0;

    while(1) {
        // when the count gets to 1000 use the rx_sn sum to display the BER and S/N
        if (rxsnCount==1000) {
            printf("%% high: %d, BER: %4.1e, SN: %.1f\n",
            rxsn[rxsnSum/10].percentHigh, rxsn[rxsnSum/10].BER, rxsn[rxsnSum/10].SN);
            rxsnSum = 0;
            rxsnCount = 0;
        }
    }

return 0;
}

