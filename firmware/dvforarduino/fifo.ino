/*
DVforArduino - Copyright (C) 2013
The transmit and receive fifos

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

#include "fifo.h"

volatile uint8_t TXfifo[TXfifosize];
volatile uint8_t RXfifo[RXfifosize];
volatile uint8_t TXfifohead,TXfifotail,TXfifolevel;
volatile uint8_t RXfifohead,RXfifotail,RXfifolevel;

//init queues
void TXfifoinit (void) { TXfifolevel=0;TXfifohead=0;TXfifotail=0; }
void RXfifoinit (void) { TXfifolevel=0;TXfifohead=0;TXfifotail=0; }

//    empty queue = 1 else 0
int TXfifoempty(void) { return(TXfifolevel==0); }
int RXfifoempty(void) { return(RXfifolevel==0); }

//    full queue = 1 else 0
int TXfifofull(void) { return(TXfifolevel==TXfifosize); }
int RXfifofull(void) { return(RXfifolevel==RXfifosize); }

// how full is queue
int TXfifolen(void) { return(TXfifolevel); }
int RXfifolen(void) { return(RXfifolevel); }

// insert element
uint8_t TXfifoput(uint8_t next) {
   if(TXfifolevel==TXfifosize) {return(0);}
   else {
       TXfifo[TXfifohead]=next;
       ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
           {
           TXfifolevel++;
           TXfifohead=(TXfifohead+1)%TXfifosize;
           }
       return(1);
   }   
}
uint8_t RXfifoput(uint8_t next) {
   if(RXfifolevel==RXfifosize) {return(0);}
   else {
       RXfifo[RXfifohead]=next;
       RXfifolevel++;
       RXfifohead=(RXfifohead+1)%RXfifosize;
       return(1);
   }   
}

// return next element
uint8_t TXfifoget(void){
   uint8_t get;
   if (TXfifolevel>0) {
       get=TXfifo[TXfifotail];
       TXfifotail=(TXfifotail+1)%TXfifosize;
       TXfifolevel--;
       return(get);
   }   
}   
uint8_t RXfifoget(void){
   uint8_t get;
   if (RXfifolevel>0) {
       get=RXfifo[RXfifotail];
       ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
           {
           RXfifotail=(RXfifotail+1)%RXfifosize;
           RXfifolevel--;
           }       
       return(get);
   }   
}   

