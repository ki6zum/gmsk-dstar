/*
DVforArduino Echo Test version 0.1 - Copyright (C) 2014
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

// BOF preprocessor bug prevent - insert me on top of your arduino-code
#if 1
__asm volatile ("nop");
#endif

#define pinLED 13
#define pinRXCLK 2
#define pinTXCLK 3
#define pinRXD 5
#define pinTXD 6
#define pinPTT 8
#define pinPLLACQ 9
#define pinCOSLED 11

// Mega boards have 8k or 16k of SRAM, 328 based boards have only 2k of SRAM
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define BUFSIZE 6*1024
#else
#define BUFSIZE 1024
#endif

#define txinvert true

uint8_t buffer[BUFSIZE];
int byteCount;
int rxbitCount;   
int txbitCount;   
int rxbyte;   
int txbyte;   
int txrx;   

void setup(void)
{
  Serial.begin(115200);
  Serial.flush();
  Serial.println("DVforArduino Echo Test 0.1 Copyright (C) 2013\n");
  Serial.println("This program comes with ABSOLUTELY NO WARRANTY;");
  Serial.println("This is free software, and you are welcome to redistribute it under certain conditions;");
  Serial.print("Free Ram: ");
  Serial.println(freeRam(),DEC);

// set all the pin modes and initial states
  pinMode(pinRXD, INPUT);    
  pinMode(pinTXD, OUTPUT);    
  pinMode(pinPTT, OUTPUT);    
  pinMode(pinPLLACQ, OUTPUT);    
  pinMode(pinCOSLED, OUTPUT);
  digitalWrite(pinPTT, LOW);
  digitalWrite(pinCOSLED, LOW);
  digitalWrite(pinPLLACQ, HIGH);

// initialize the counter variables
  byteCount = 0;
  rxbitCount = 0;   
  txbitCount = 0;   
  rxbyte = 0;   
  txbyte = 0;   
  txrx=0;

// initialize the tx and rx interrupts
  attachInterrupt(0, rxINT, FALLING);
  attachInterrupt(1, txINT, RISING);
}

int freeRam () {                        // Count free ram, thanks to KI6ZUM
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}  


void loop(void)
{
// reset the counter back to zero when we get to the end
  if (byteCount >= BUFSIZE) {
    // toggle between transmit and receive modes
    if (txrx == 0) {
      txrx = 1;
    }
    else {
      txrx = 0;
    }
    rxbitCount = 0;
    txbitCount = 0;
    byteCount = 0;
  }
  if (txrx == 0)
    Serial.print("receive ");
  else
    Serial.print("transmit ");
  Serial.print("loop: ");
  Serial.println(byteCount);

  delay(1000);
}

void rxINT()
{
    // if tx mode then ignore interrupt
    if (txrx == 1)
      return;
      
    // only do something if the counter is still within the buffer space
    if (byteCount <= BUFSIZE) {
      // turn on COS LED a short time after starting recording 
      if (byteCount == 100)
        digitalWrite(pinCOSLED, HIGH);
      // turn off COS LED a short time before end of buffer
      else if (byteCount == BUFSIZE-100)
        digitalWrite(pinCOSLED, LOW);
  
      // save the received bit
      buffer[byteCount] = (buffer[byteCount] << 1) | digitalRead(pinRXD);

      rxbitCount++;
      if (rxbitCount == 8) {
        rxbitCount = 0;
        byteCount++; 
      }
    }
} // end rxINT

void txINT()
{
    // if rx mode then ignore interrupt
    if (txrx == 0)
      return;

    // only do something if the counter is still within the buffer space
    if (byteCount <= BUFSIZE) {
      // turn on PTT a short time after starting transmit
      if (byteCount == 100)
        digitalWrite(pinPTT, HIGH);
      // turn off PTT a short time before end of buffer
      else if (byteCount == BUFSIZE-100)
        digitalWrite(pinPTT, LOW);

      // output the previously recorded bit
#ifdef txinvert
        digitalWrite(pinTXD, (buffer[byteCount] >> (7-(txbitCount)))&0x1 ? HIGH:LOW);
#elif         
        digitalWrite(pinTXD, (buffer[byteCount] >> (7-(txbitCount)))&0x1 ? LOW:HIGH);
#endif        

      txbitCount++;
      if (txbitCount == 8) {
        txbitCount = 0;
        byteCount++; 
      }
    }
} // end txINT



