/*
DVforArduino version 0.1 - Copyright (C) 2013
This is the main controller software running on the Arduino to run the DV for Arduino GMSK board.
It will run on either the Amega328 or the Mega versions of the Arduino boards.

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

#include "pindefs.h"
#include <util/atomic.h>
#include "fifo.h"

#define RAWSIZE 12
// native arduino serial input and output buffers are 64 bytes maximum and bad things happen at the 63 byte point
#define SNMODE 1
#define RXMODE 0

#define INV(x,y) x ^= _BV(y) //toggle bit - using bitwise XOR operator

// rxframe is 589 to arduino to host
typedef struct RXFRAME589 {
    uint8_t header;
    uint8_t tx_fifo   : 7; // fifo for data going to 589
    uint8_t cos_pin   : 1;
    uint8_t rx_fifo   : 7; // fifo for data coming from 589
    uint8_t reserved  : 1;
    uint8_t rawbits[RAWSIZE];
    uint8_t crc;
   } rxframe589;

// txframe is host to arduino to 589
typedef struct TXFRAME589 {
    uint8_t header;
    uint8_t ptt       : 1;
    uint8_t pllacq    : 1;
    uint8_t cosled    : 1;
    uint8_t sn_mode   : 1;
    uint8_t txen      : 1;
    uint8_t rxen      : 1;
    uint8_t reset     : 1;
    uint8_t reserved1 : 1;
    uint8_t reserved2 : 8;
    uint8_t rawbits[RAWSIZE];
    uint8_t crc;
   } txframe589;

rxframe589 rxtesting;
txframe589 txtesting;
uint8_t rxMode;
uint8_t TxRxMode;

uint8_t crc8(uint8_t * buffer, uint8_t length)
{
uint8_t crc = 0;
uint8_t crc_feedback[16] = { 0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D };

//crc=0;
for (int i=0;i<length;i++)  {
    crc = (crc << 4) ^ crc_feedback[((crc     ) ^ buffer[i]) >> 4];
    crc = (crc << 4) ^ crc_feedback[((crc >> 4) ^ buffer[i]) & 0x0F];
}
return(crc);  
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup(void)
{
// set all the pin modes and initial states
  pinMode(pinSN, INPUT);    
  pinMode(pinRXD, INPUT);    
  pinMode(pinTXD, OUTPUT);    
  pinMode(pinCOS, INPUT);    
  pinMode(pinPTT, OUTPUT);    
  pinMode(pinPLLACQ, OUTPUT);    
  pinMode(pinCOSLED, OUTPUT);
  pinMode(pinDebug, OUTPUT);
  digitalWrite(pinPTT, LOW);
  digitalWrite(pinDebug, LOW);
  digitalWrite(pinCOSLED, LOW);
  digitalWrite(pinPLLACQ, HIGH);

  TXfifoinit ();
  RXfifoinit ();

  rxMode = RXMODE;
  TxRxMode = 0;  // init to = for RX by default
    
  Serial.begin(115200);
  Serial.flush();

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  Serial1.begin(115200);
  Serial1.flush();
  Serial1.println("DVforArduino  Copyright (C) 2013\n");
  Serial1.println("This program comes with ABSOLUTELY NO WARRANTY;");
  Serial1.println("This is free software, and you are welcome to redistribute it");
  Serial1.println("under certain conditions;");
  Serial1.println("Free SRAM at startup ");
  Serial1.println(freeRam(),DEC);
#endif

// initialize the tx and rx interrupts
  attachInterrupt(0, rxINT, FALLING);
  attachInterrupt(1, txINT, RISING);

}

void loop(void)
{
uint8_t inval;  
static uint8_t readcount = 0;  
static uint8_t inBuf[RAWSIZE+4];

 //look for packet data coming from computer via serial port
    if (Serial.available() > 0) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        //Serial1.print(" SAR:");
        //Serial1.print(Serial.available());
#endif
        // read the incoming byte:
        inval = Serial.read();
        // make sure the first byte is the header - if not then throw it away
        if ((readcount == 0) && (inval != 0x55)) { // 0x55 is header byte for host to arduino direction
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
            Serial1.println("SE");
#endif            
          // don't bother incrementing the read count
        }
        // if the count is at the start of the buffer and the correct header byte is found then start counting
        else if ((readcount == 0) && (inval == 0x55)) { 
            inBuf[readcount++] = inval;
        }
        // otherwise the count to get the packet has started, grab the data and if full packet received the process it
        else {
            // store the next byte in the packet
            inBuf[readcount++] = inval;
            // if the full count of bytes have been received then process the packet and move it to the TX fifo
            if (readcount == RAWSIZE+4) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
                //for (int i=0;i<RAWSIZE+4;i++) {
                //    Serial1.print(inBuf[i], HEX);
                //    Serial1.print(" ");
                //}
                //Serial1.println(" ");
#endif
                memcpy((uint8_t *)&txtesting, inBuf, RAWSIZE+4);
                // check the crc is ok and process the packet, if not throw it away and start looking for the header again
                if (crc8((uint8_t *)&txtesting, RAWSIZE+3)==txtesting.crc) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
                    //Serial1.print("CO");
#endif                    
                    // copy the raw bits from the payload to the TX fifo
                    for (int i=0;i<RAWSIZE;i++) {
                         TXfifo[TXfifohead]=txtesting.rawbits[i];
                         ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
                             {
                             TXfifolevel++;
                             TXfifohead=(TXfifohead+1)%TXfifosize;
                             }
                        }
                    // handle the PTT, PLLACQ, COSLED and SN_mode settings coming from the host
                    digitalWrite(pinPTT, txtesting.ptt ? HIGH:LOW);
                    digitalWrite(pinPLLACQ, txtesting.pllacq ? HIGH:LOW);
                    digitalWrite(pinCOSLED, txtesting.cosled ? HIGH:LOW);
                    //rxMode = txtesting.sn_mode;
                    
                    if (txtesting.txen == 1) {  
                        attachInterrupt(1, txINT, RISING);
                    }
                    else {
                        detachInterrupt(1);
                    }
                    if (txtesting.rxen == 1) {  
                        attachInterrupt(0, rxINT, RISING);
                    }
                    else {
                        detachInterrupt(1);
                    }
                } // end if crc of whole packet is ok
                else {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
                    Serial1.println("CE");
#endif
                }
                // reset the receive buffer back to zero to start looking for the start of the next packet   
                readcount = 0;
            } // end if full packet received in buffer
        } // end else - readcount > 0
    }  // end if serial data available

    // when the receive fifo (raw bits from 589) has at least 12 bytes available, send a packet to the host
    // 28 bytes of raw bits equals roughly 50ms
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    //Serial1.print(TXfifolevel);
    //Serial1.print("_");
    //Serial1.println(RXfifolevel);
#endif
    if (RXfifolevel > RAWSIZE) {
        rxtesting.header = 0xaa;  // 0xaa is header byte for arduino to host direction
        for (int i=0;i<RAWSIZE;i++) {
            rxtesting.rawbits[i] = RXfifo[RXfifotail];
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
                {
                RXfifotail=(RXfifotail+1)%RXfifosize;
                RXfifolevel--;
                }
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
            //Serial1.print(rxtesting.rawbits[i], HEX);
#endif
        }
        
        rxtesting.tx_fifo = TXfifolevel;
        rxtesting.rx_fifo = RXfifolevel;
        rxtesting.cos_pin = digitalRead(pinCOS);
        rxtesting.reserved = 0;
        rxtesting.crc = crc8((uint8_t *)&rxtesting, RAWSIZE+3);
        Serial.write((uint8_t *)&rxtesting, RAWSIZE+4);
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        //Serial1.print("SRt:");
        // note!! txavailable function is new one created in Arduino serial driver leave commented
        // unless you modify the driver too - it is only really useful when looking for buffer overflows
        //Serial1.print(Serial.txavailable());
#endif
    } // end if rx fifo has enough raw bits to send to the host

}

uint8_t rxdat;
uint8_t rxByte = 0, rxbitCount = 0;
void rxINT()
{
    // pick the data to put into the fifo - either RXDATA or RX_SN
    if (rxMode == RXMODE) {
        rxdat = digitalRead(pinRXD);
    }
    else {
        rxdat = digitalRead(pinSN);
    }

    // save the 8 bits before pushing into fifo
    rxByte = (rxByte << 1)|rxdat;
    rxbitCount++;
    if (rxbitCount == 8) {
        // make sure the fifo isn't full, if it is we can't do much other than throw away the data
        if (RXfifolevel < 120) {
            RXfifo[RXfifohead]=rxByte;
            RXfifolevel++;
            RXfifohead=(RXfifohead+1)%RXfifosize;
        }
        else {
          // do something here to signal rx fifo overflow
            INV(pinDebugPort,pinDebugPin);
            INV(pinDebugPort,pinDebugPin);
        }
        rxByte=0;  // not absolutely necessary as shifting should flush old data but good for the coder's sanity
        rxbitCount = 0;
    }
} // end rxint


uint8_t txdat;
uint8_t txByte = 0, txbitCount = 0;
void txINT()
{
    if (txbitCount == 8) {
        //make sure the fifo isn't empty - it shouldn't ever be empty
        if (TXfifolevel>0) {
             txByte=TXfifo[TXfifotail];
             TXfifotail=(TXfifotail+1)%TXfifosize;
             TXfifolevel--;
        } else {
           // do something here to signal tx fifo underflow
            txByte = 0;
            INV(pinDebugPort,pinDebugPin);
            INV(pinDebugPort,pinDebugPin);
        }
        txbitCount=0;
    }
    txdat = (txByte >> (7-txbitCount))&0x1;
    
    digitalWrite(pinTXD, txdat ? HIGH:LOW);

    txbitCount++;
} // end txINT


