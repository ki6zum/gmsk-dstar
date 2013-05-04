/*
DVforArduino - Copyright (C) 2013
The header file for the transmit and receive fifos

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

#define TXfifosize 128
#define RXfifosize 128
extern volatile uint8_t TXfifo[];
extern volatile uint8_t RXfifo[];
extern volatile uint8_t TXfifohead,TXfifotail,TXfifolevel;
extern volatile uint8_t RXfifohead,RXfifotail,RXfifolevel;

//init queues
extern void TXfifoinit (void);
extern void RXfifoinit (void);

//    empty queue = 1 else 0
extern int TXfifoempty(void);
extern int RXfifoempty(void);

//    full queue = 1 else 0
extern int TXfifofull(void);
extern int RXfifofull(void);

// how full is queue
extern int TXfifolen(void);
extern int RXfifolen(void);

// insert element
extern uint8_t TXfifoput(uint8_t);
extern uint8_t RXfifoput(uint8_t);

// return next element
extern uint8_t TXfifoget(void);
extern uint8_t RXfifoget(void);

