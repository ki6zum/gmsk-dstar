/*
DVforArduino host exerciser - Copyright (C) 2013
This is the software running on the host that controls the Arduino 
to run the DV for Arduino GMSK board. While this code was written for 
the Visual Studio environment, it should compile and run with minor 
changes on most other host platforms.

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

#include <windows.h>
#include <stdio.h>

typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#define RAWSIZE 12
#define SNMODE 1
#define RXMODE 0

// rxframe is 589 to arduino to host
typedef struct RXFRAME589 {
    uint8_t header;
    uint8_t tx_fifo   : 7;
    uint8_t cos_pin   : 1;
    uint8_t rx_fifo   : 7;
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

HANDLE openSerial(wchar_t * portString);
void closeSerial(HANDLE);
uint8_t crc8(uint8_t * buffer, uint8_t length);
uint8_t received[];


uint8_t crc8(uint8_t * buffer, uint8_t length)
{
uint8_t crc;
uint8_t crc_feedback[16] = { 0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D };

crc = 0;
for (int i=0;i<length;i++)  {
    crc = (crc << 4) ^ crc_feedback[((crc     ) ^ buffer[i]) >> 4];
    crc = (crc << 4) ^ crc_feedback[((crc >> 4) ^ buffer[i]) & 0x0F];
}
return(crc);  
}

rxframe589 rxtesting;
txframe589 txtesting;
uint8_t rxMode;

// serial port related functions
HANDLE openSerial(wchar_t * portString)
{
	DCB dcbCommPort;
	COMMTIMEOUTS CommTimeouts;
    HANDLE hComm = NULL;

    // Open the port using WIN32 API CreateFile().
    // Change COM1 for different port.
    // Leave the remaining parameters alone for simple
    // non-overlapped serial I/O.
    hComm = CreateFile(portString, GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
  
    // If port cannot open, print error message and return.
    if (hComm == INVALID_HANDLE_VALUE) 
		{
		hComm = NULL;
		return(FALSE);
		}

    // Get the existing DCB parameters
    dcbCommPort.DCBlength = sizeof(DCB);
    GetCommState(hComm, &dcbCommPort);
  
    // Use the WIN32 API BuildCommDCB() to load simple comm parameters
    // into DCB. If it fails, print error message and return.
    if(!BuildCommDCB(L"baud=115200 parity=N data=8 stop=1", &dcbCommPort)) 
		{
		CloseHandle(hComm);
		hComm = NULL;
		return(FALSE);
		}
  
    // Use the WIN32 API SetCommState() to set parameters from DCB.
    // If it fails, close hComm, print error message, and return.
    if(!SetCommState(hComm, &dcbCommPort)) 
		{
		CloseHandle(hComm);
		hComm = NULL;
		return (FALSE);
		}
  
    // Set CommTimeouts parameters.
    // See ReadFile() function later for details.
    CommTimeouts.ReadIntervalTimeout = MAXDWORD;
    CommTimeouts.ReadTotalTimeoutConstant = 0;
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;
    CommTimeouts.WriteTotalTimeoutConstant = 0;
    CommTimeouts.WriteTotalTimeoutMultiplier = 0;
  
    // Use the WIN32 API SetCommTimeouts() to set parameters from DCB.
    // If it fails, close hComm, print error message, and return.
    if(!SetCommTimeouts(hComm, &CommTimeouts)) 
		{
		CloseHandle(hComm);
		hComm = NULL;
		return (FALSE);
		}

	return(hComm);
}

void closeSerial(HANDLE hComm)
{
    CloseHandle(hComm);
    hComm = NULL;
}

uint32_t readSerial(HANDLE hComm, uint8_t * buffer, uint8_t length)
{
DWORD dwBytesRead;
int fSuccess;	

	fSuccess = ReadFile(hComm, buffer, length, &dwBytesRead, NULL);      
	if ((!fSuccess) || (dwBytesRead != length))
	{
		  printf("ReadFile(): Failed\n");
	}

return(dwBytesRead);
}

uint32_t writeSerial(HANDLE hComm, uint8_t * buffer, uint8_t length)
{
DWORD dwBytesWritten;
int fSuccess;	

	fSuccess = WriteFile(hComm, buffer, length, &dwBytesWritten, NULL);      
	if ((!fSuccess) || (dwBytesWritten != length))
	{
		  printf("WriteFile(): Failed\n");
	}

return(dwBytesWritten);
}

char numbers[16][5] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", 
                        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};

void main (void)
{
	HANDLE hComm;
	rxMode = RXMODE;
	hComm = openSerial(L"\\\\.\\COM17");
	DWORD dwErrors, dwBytesRead;
	COMSTAT stat;
	uint8_t inval=0, readcount=0, tx_fifo = 0xff, rx_fifo = 0xff;
	uint8_t inBuf[RAWSIZE+4];
	uint16_t bitcount = 0;
	uint8_t txInvert=TRUE, rxInvert=TRUE;

int x=0;
while (x==0) {
	dwBytesRead = 0;
	ClearCommError(hComm,&dwErrors,&stat); // always clear comm errors, as a buffer overrun (too many bytes unread) will stop any more events occuring 
	if (stat.cbInQue>0) // is there data to be read?
	{
        // read the incoming byte:
        dwBytesRead = readSerial(hComm, &inval, 1);
		//printf("%.2x ", inval);
        // make sure the first byte is the header - if not then it is a sync error so throw it away
        if ((readcount == 0) && (inval != 0xaa)) { // 0xaa is header byte for arduino to host direction
			printf("SE ");
        }
        // if the count is at the start of the buffer and the correct header byte is found then start counting
        else if ((readcount == 0) && (inval == 0xaa)) { 
            inBuf[readcount++] = inval;
        }
        // otherwise the count to get the packet has started, grab the data and if full packet received the process it
        else {
            // store the next byte in the packet
            inBuf[readcount++] = inval;
            // if the full count of bytes have been received then process the packet and move it to the TX fifo
            if (readcount == RAWSIZE+4) {
                // check the crc is ok and process the packet, if not throw it away and start looking for the header again
                if (crc8(inBuf, RAWSIZE+3)==inBuf[RAWSIZE+3]) {
					//printf("CO");
					memcpy(&rxtesting, inBuf, RAWSIZE+4);
					// invert the received bits if the radio needed it
					if (rxInvert == TRUE) {
						for (int j=0;j<RAWSIZE;j++)
						{
							rxtesting.rawbits[j]^=0xff;
						}
					}
                    // handle the COS value coming from the arduino
					//??? = rxtesting.cos_pin;
					// handle the fifo levels in the Arduino
					rx_fifo = rxtesting.rx_fifo;
					tx_fifo = rxtesting.tx_fifo;
					// process the raw incoming bits

                } // end if crc of whole packet is ok
				else
					printf("CE\n");
                // reset the receive buffer back to zero to start looking for the start of the next packet   
                readcount = 0;
            } // end if full packet received in buffer
        } // end else - readcount > 0
	} // end if receive data available

	if (tx_fifo < 32) {
		// fill up the packet and send it to the Arduino
		txtesting.header = 0x55;
		txtesting.pllacq = 1;
		txtesting.cosled = 0;
		txtesting.sn_mode = RXMODE;
		txtesting.txen = 1;
		txtesting.rxen = 1;
		memcpy(&txtesting.rawbits[0], &received[bitcount], RAWSIZE);
		// invert the bits here if the radio needs it
		if (txInvert == TRUE) {
			for (int j=0;j<RAWSIZE;j++)
			{
				txtesting.rawbits[j]^=0xff;
			}
		}
		txtesting.crc = crc8((uint8_t *)&txtesting, RAWSIZE+3);

		bitcount+=RAWSIZE;
		if (bitcount < 200) {
			txtesting.ptt = 0;
		}
		else if (bitcount < 4000) {
			txtesting.ptt = 1;
		}
		else if (bitcount < 4096) {
			txtesting.ptt = 0;
		}
		else {
			bitcount = 0;
		}

		writeSerial(hComm, (uint8_t *)&txtesting, RAWSIZE+4);
   		// reduce the apparent transmit to 589 fifo space on the Arduino by the amount of data just sent
		tx_fifo += RAWSIZE;
	}
	else {
		//printf("tx buffer full\n");
	}

} // end while x==0
}



uint8_t received[] = 
{0x7e, 
0xfa, 0x4b, 0x63, 0xb5, 0xad, 0x6a, 0x90, 0xca, 0xa5, 0xb2, 0x91, 0x6b, 0xd8, 0xca, 0x54, 0x4d, 
0x5c, 0x95, 0x92, 0x68, 0x94, 0xf3, 0x32, 0x55, 0x56, 0x64, 0x58, 0x5b, 0x9b, 0x4d, 0x9c, 0x62, 
0xdd, 0xc5, 0x51, 0x45, 0xd5, 0xca, 0x21, 0xca, 0x54, 0x93, 0x6b, 0x32, 0xc5, 0xda, 0xd4, 0xaf, 
0xa6, 0x83, 0x6a, 0xcb, 0x2d, 0xd9, 0x91, 0x9d, 0x56, 0xa6, 0xd4, 0xda, 0x6b, 0xea, 0x5c, 0xa9, 
0xad, 0x9d, 0xaf, 0x53, 0x6d, 0xea, 0x69, 0x2d, 0xb4, 0x09, 0x2f, 0xa9, 0xbb, 0x48, 0xae, 0xae, 
0xa9, 0x5b, 0x5c, 0x53, 0xb2, 0xb1, 0x76, 0x23, 0x28, 0xde, 0xea, 0xd4, 0x35, 0x5a, 0x79, 0x9b, 
0x6e, 0xda, 0x9d, 0x1f, 0x69, 0xad, 0x3b, 0xbe, 0xa2, 0x52, 0x9a, 0xab, 0x39, 0x91, 0x78, 0xd5, 
0x9d, 0x4b, 0x6a, 0xcb, 0x56, 0xa8, 0xd5, 0xf2, 0xb4, 0xb6, 0x8b, 0x13, 0x7a, 0x55, 0x35, 0xf2, 
0x95, 0xac, 0xd5, 0x6a, 0xa4, 0xd7, 0xd3, 0x6e, 0x8d, 0x58, 0xad, 0x2a, 0xe7, 0x65, 0xdd, 0x77, 
0x6b, 0x29, 0xd5, 0x5a, 0xd5, 0x50, 0xb9, 0x94, 0xbd, 0x93, 0x2b, 0x94, 0xa5, 0x55, 0x6d, 0x50, 
0xd4, 0xea, 0x21, 0x9a, 0xf6, 0xac, 0xd1, 0x6a, 0xb9, 0x5a, 0xa5, 0x6c, 0x3d, 0xd2, 0x8e, 0xa9, 
0x6d, 0x65, 0x52, 0xc5, 0xd5, 0x74, 0xd2, 0x93, 0x6a, 0x2d, 0xab, 0x55, 0xbb, 0x5b, 0x4a, 0x94, 
0xf5, 0x95, 0x89, 0x2d, 0xaa, 0xb3, 0x55, 0xe4, 0x4a, 0x91, 0xca, 0xea, 0xb2, 0xda, 0x4b, 0x6b, 
0x29, 0xb7, 0x56, 0x8b, 0x29, 0x02, 0xdd, 0x2d, 0x75, 0x2c, 0xea, 0x75, 0x59, 0x4c, 0x92, 0x26, 
0xa0, 0xb8, 0xf5, 0xfa, 0xb1, 0x27, 0x7e, 0x52, 0xdd, 0xa8, 0x54, 0xdb, 0x6e, 0xf4, 0xee, 0xdc, 
0x66, 0x35, 0xd5, 0x4b, 0x53, 0x11, 0xb6, 0xe2, 0x76, 0xaa, 0x74, 0xb6, 0xd1, 0xa3, 0x0f, 0xe5, 
0x35, 0xa2, 0x98, 0xb5, 0x55, 0x52, 0xeb, 0x6a, 0x6e, 0x9f, 0x15, 0x50, 0x75, 0x33, 0x5e, 0xd5, 
0x62, 0xdb, 0x23, 0x55, 0xbd, 0xfe, 0xd5, 0x1e, 0xe3, 0xf6, 0x44, 0xe5, 0x53, 0x92, 0x66, 0x2a, 
0xc5, 0xaa, 0x6d, 0xd0, 0xf5, 0x60, 0x5f, 0xd3, 0x2f, 0xd2, 0x0d, 0x66, 0x99, 0x72, 0x95, 0x53, 
0x3d, 0x54, 0xf6, 0xce, 0xd5, 0xf7, 0x8a, 0xa7, 0x2a, 0x57, 0xeb, 0x64, 0x2d, 0xcd, 0x4f, 0x62, 
0x61, 0x2a, 0x6e, 0xae, 0x9c, 0xb5, 0xac, 0x19, 0xda, 0xf9, 0x5a, 0xe9, 0x5e, 0x36, 0xe9, 0xae, 
0xb8, 0xed, 0x6b, 0x4b, 0xc7, 0x5f, 0x65, 0x54, 0x53, 0x18, 0xae, 0xad, 0xa3, 0x4b, 0x55, 0x24, 
0xd5, 0x55, 0x94, 0xb5, 0xee, 0x6c, 0xb9, 0x2a, 0x8a, 0xdd, 0xbb, 0xad, 0x57, 0xad, 0x8e, 0xb6, 
0xc5, 0x6c, 0xa9, 0xd5, 0xf5, 0xac, 0xd5, 0x75, 0x93, 0x29, 0x24, 0xcf, 0x5d, 0xb2, 0xb2, 0x9a, 
0xab, 0x96, 0x5a, 0xaf, 0x9a, 0xda, 0x52, 0xea, 0xb6, 0xcd, 0x06, 0xb7, 0xa4, 0xd2, 0xd5, 0x19, 
0xb3, 0xa9, 0x4a, 0xd6, 0xad, 0xb6, 0xff, 0x5a, 0xed, 0x48, 0x9a, 0x2a, 0xca, 0x97, 0x29, 0x3a, 
0xd4, 0xa9, 0x56, 0xde, 0xa5, 0x7e, 0x50, 0x99, 0xba, 0x51, 0x55, 0xb4, 0x95, 0x4a, 0x56, 0xd2, 
0x4b, 0x57, 0x76, 0x44, 0xba, 0x4f, 0x8d, 0x9d, 0xab, 0x9a, 0xd3, 0xd5, 0xaa, 0xa3, 0x24, 0x9d, 
0x6a, 0xae, 0xb7, 0x54, 0x16, 0xb3, 0xa4, 0x76, 0xc9, 0xca, 0xd5, 0x55, 0xa5, 0xdb, 0x6e, 0x8f, 
0x30, 0x4d, 0x95, 0x5d, 0xd6, 0xd8, 0xf7, 0xa5, 0x67, 0x96, 0x35, 0x59, 0xd5, 0x9e, 0x23, 0x9a, 
0xe1, 0x7e, 0xee, 0x1e, 0xa6, 0x4a, 0x6b, 0xc6, 0xc7, 0x6d, 0xa5, 0x56, 0x95, 0x9a, 0xa4, 0x2a, 
0xaa, 0xbe, 0x55, 0x54, 0xc4, 0x99, 0x5d, 0x66, 0x5b, 0x59, 0x5d, 0xaa, 0x54, 0xda, 0x3c, 0x0a, 
0xab, 0x69, 0x2e, 0xa7, 0xe9, 0x22, 0xef, 0x54, 0xd6, 0x2d, 0x2a, 0x27, 0xee, 0xea, 0x9b, 0x55, 
0x29, 0xeb, 0x4b, 0x5e, 0xab, 0xc6, 0xd3, 0x27, 0x00, 0x97, 0xb3, 0x6b, 0xd1, 0x67, 0x16, 0x6a, 
0x6b, 0x6b, 0x9d, 0xb9, 0xaa, 0x5d, 0x36, 0xae, 0xd5, 0xed, 0x5d, 0xe9, 0x34, 0xd5, 0x55, 0xd7, 
0x45, 0x6e, 0xf7, 0x55, 0x52, 0xa5, 0x44, 0x5a, 0x49, 0x25, 0xab, 0x5a, 0xba, 0x95, 0xdf, 0x53, 
0x59, 0x2d, 0x2b, 0x3d, 0x6b, 0x6b, 0x3d, 0xae, 0x4b, 0x2d, 0x99, 0x67, 0x6d, 0xaf, 0xd5, 0x53, 
0x57, 0x36, 0x3e, 0x5a, 0xab, 0xaf, 0x55, 0x72, 0x5d, 0xaa, 0xa6, 0xa9, 0xcb, 0x25, 0xda, 0x4b, 
0x15, 0x59, 0x23, 0xfc, 0x5a, 0xa9, 0x23, 0x59, 0x5f, 0x1a, 0x5b, 0xe1, 0xb5, 0x1a, 0x97, 0x6a, 
0x52, 0xc5, 0x2d, 0xea, 0xca, 0xa6, 0xd6, 0x25, 0x5b, 0x66, 0xa8, 0x86, 0x32, 0xa9, 0x3b, 0x95, 
0xba, 0xf5, 0xf6, 0x9a, 0x86, 0x75, 0xb4, 0x29, 0xb5, 0x76, 0xce, 0xe6, 0x43, 0x5a, 0xc5, 0x28, 
0x97, 0x75, 0xe1, 0x5e, 0x8b, 0x4c, 0xf2, 0xea, 0x85, 0x9d, 0xa9, 0x13, 0xaa, 0xa2, 0xf3, 0xd1, 
0x2f, 0xd6, 0xc6, 0x64, 0x8d, 0xc2, 0xb4, 0xff, 0x77, 0x4a, 0x69, 0x56, 0x95, 0xb7, 0x95, 0x2a, 
0x25, 0x2a, 0xd3, 0x60, 0xb5, 0x51, 0xea, 0x55, 0x9d, 0x57, 0x29, 0xaa, 0xad, 0xb5, 0xa4, 0xee, 
0x97, 0x15, 0xb2, 0x36, 0xb0, 0xda, 0x75, 0x54, 0xb6, 0x76, 0xa9, 0x52, 0x8a, 0xda, 0x3d, 0x2d, 
0xab, 0x35, 0xab, 0x7b, 0x16, 0xdb, 0xed, 0xaa, 0xed, 0x75, 0xb8, 0x91, 0x14, 0x53, 0x6f, 0x29, 
0x73, 0x25, 0xe9, 0x5a, 0x6a, 0xa9, 0x4a, 0x95, 0x63, 0x4b, 0xa6, 0x6c, 0xab, 0x5b, 0x75, 0xaa, 
0xe7, 0x45, 0xaa, 0xfa, 0x67, 0x2a, 0x81, 0xca, 0xf3, 0xb4, 0xb6, 0xb3, 0x5d, 0xbb, 0x5d, 0x8d, 
0x15, 0xd4, 0x2a, 0x0c, 0xaa, 0xea, 0xef, 0x56, 0xaa, 0x55, 0x5b, 0x8d, 0x46, 0xb7, 0x6d, 0x2a, 
0x55, 0xcc, 0xca, 0x95, 0x53, 0xad, 0x52, 0xda, 0x97, 0x4f, 0x77, 0xd5, 0x39, 0x52, 0x69, 0x5e, 
0x9a, 0xe9, 0xde, 0x31, 0x35, 0xd9, 0x56, 0xd5, 0x75, 0x5a, 0x98, 0x72, 0xe7, 0x2a, 0x97, 0x0b, 
0x35, 0x7d, 0x26, 0x5a, 0xe9, 0x62, 0xa4, 0x52, 0x55, 0xf2, 0xe5, 0x56, 0xd6, 0xf6, 0x8e, 0xf1, 
0x5d, 0x4d, 0x73, 0xd5, 0x04, 0x3a, 0x92, 0xed, 0xad, 0x50, 0x74, 0x9b, 0x5b, 0x53, 0x8c, 0xfd, 
0x6d, 0xb5, 0x12, 0xcf, 0x4d, 0x4d, 0x2e, 0xd2, 0xb2, 0xea, 0xc2, 0x5d, 0xbc, 0x95, 0xa5, 0x74, 
0xac, 0xfa, 0xb4, 0xa5, 0x54, 0xb7, 0x2a, 0xd7, 0x7b, 0x2e, 0xdc, 0xa5, 0xab, 0x35, 0x49, 0xdb, 
0x8e, 0xc2, 0x95, 0x4a, 0xaa, 0xe2, 0xe6, 0x5d, 0xe2, 0xaa, 0xba, 0xab, 0xa2, 0x50, 0x1b, 0xd0, 
0x4a, 0xf7, 0x55, 0x45, 0x99, 0x74, 0xc5, 0x75, 0x35, 0x6b, 0xaa, 0xa8, 0xd5, 0x4a, 0xd2, 0x29, 
0x50, 0xd7, 0x93, 0x2d, 0x6a, 0x93, 0x56, 0xeb, 0x6b, 0x16, 0xab, 0xe9, 0x90, 0x2d, 0xad, 0x96, 
0x5f, 0x3c, 0xb7, 0x4b, 0x57, 0x37, 0x2b, 0x77, 0x57, 0x5b, 0x53, 0x0a, 0xd5, 0x92, 0x4a, 0x73, 
0x5b, 0x55, 0xca, 0xd4, 0x95, 0x62, 0xfb, 0x5a, 0xa0, 0xa9, 0xd1, 0x98, 0x9d, 0x0c, 0xab, 0x37, 
0x74, 0xc4, 0x6f, 0x55, 0xbb, 0xd5, 0xe7, 0x55, 0x5f, 0xda, 0xb5, 0xdb, 0x37, 0x54, 0x2a, 0xab, 
0xbe, 0xdb, 0x50, 0x52, 0x53, 0x6d, 0x66, 0xc6, 0xa7, 0x5a, 0xa5, 0xde, 0xa9, 0x65, 0x5a, 0xaa, 
0xbf, 0x5e, 0xd5, 0x4d, 0x7d, 0xaa, 0xaa, 0x4f, 0x75, 0x53, 0x3b, 0x6f, 0x8f, 0x51, 0xad, 0x7a, 
0xab, 0x68, 0xc5, 0x55, 0xbe, 0xbb, 0xbe, 0x69, 0xd4, 0xee, 0xa6, 0xaa, 0xb9, 0x93, 0x55, 0xaa, 
0xab, 0xb5, 0x55, 0xca, 0x49, 0x76, 0xd4, 0x5b, 0xaa, 0xcb, 0xa8, 0xab, 0xf4, 0xbe, 0xaa, 0xdc, 
0x6a, 0xa5, 0xfa, 0x8c, 0xd7, 0x52, 0xbd, 0xa9, 0x4d, 0x55, 0x76, 0x4a, 0xd2, 0xaf, 0x2a, 0x5a, 
0x56, 0x4e, 0xdc, 0x48, 0x62, 0x2d, 0xab, 0x75, 0xf4, 0xae, 0xd5, 0x1d, 0x94, 0xd5, 0xa4, 0xed, 
0xdd, 0x37, 0xbb, 0x61, 0x2a, 0x8a, 0xad, 0x49, 0xa6, 0xaa, 0x98, 0xa9, 0x68, 0xeb, 0xb2, 0xcd, 
0x0a, 0x93, 0x14, 0xfb, 0x55, 0x71, 0x93, 0x6d, 0xb6, 0x13, 0x95, 0x13, 0xbd, 0x4a, 0xb5, 0x57, 
0x29, 0x15, 0x8b, 0xfa, 0xb6, 0xe5, 0x4b, 0xaa, 0x94, 0x55, 0x76, 0xab, 0x35, 0x5a, 0x55, 0x69, 
0xac, 0xb4, 0xe1, 0x66, 0x77, 0xf2, 0xdd, 0xb4, 0xd9, 0x37, 0x5e, 0xd9, 0x76, 0xd5, 0x77, 0x4d, 
0x94, 0xab, 0xb5, 0xb5, 0xaa, 0xf3, 0x4d, 0xa9, 0x95, 0x55, 0xa6, 0xb9, 0x2a, 0x8a, 0x95, 0xf6, 
0xb5, 0x67, 0x75, 0x5a, 0xc2, 0x5a, 0xa9, 0x65, 0x2d, 0x7b, 0x73, 0x56, 0xe8, 0xae, 0x3b, 0xd5, 
0x99, 0x55, 0x6c, 0xea, 0x54, 0x9d, 0x2a, 0x35, 0xc7, 0x55, 0x52, 0xa9, 0xa5, 0x55, 0x6a, 0x35, 
0x51, 0x6c, 0x56, 0xba, 0xf1, 0x6f, 0x5a, 0xda, 0x53, 0x2e, 0xfb, 0x53, 0x7d, 0x15, 0xbb, 0x5a, 
0xc8, 0xdf, 0xf0, 0x00, 0x4a, 0xff, 0x55, 0x55, 0x55, 0x7f, 0xf5, 0x5f, 0xfd, 0x55, 0x55, 0x55, 
0x5b, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 
0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 
0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 
0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 
0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x76, 0x50, 0x1c, 0xb3, 0xfb, 0x12, 0x02, 0x3c, 
0x92, 0x15, 0x05, 0x3a, 0x65, 0x38, 0xdd, 0xc4, 0x8d, 0x7a, 0xc4, 0x26, 0xd8, 0x6f, 0x40, 0x60, 
0x9b, 0x4c, 0x69, 0xd5, 0x4c, 0x4d, 0xc8, 0x33, 0xc1, 0x3f, 0xfa, 0xc2, 0xa0, 0x1a, 0x54, 0x0d, 
0x79, 0x1b, 0xc0, 0x24, 0xef, 0x05, 0xca, 0xe3, 0x57, 0x1e, 0x67, 0x69, 0xe6, 0xbb, 0x38, 0x71, 
0x3b, 0x67, 0xb9, 0xf5, 0xb3, 0x74, 0x0b, 0x85, 0xca, 0xd5, 0x74, 0x5c, 0xd4, 0xe0, 0xc8, 0xca, 
0x93, 0xa8, 0x2b, 0x56, 0x24, 0x0f, 0x52, 0xee, 0x7a, 0xf5, 0x92, 0x61, 0x4c, 0x77, 0x16, 0x45, 
0xc4, 0x2b, 0xf4, 0x80, 0xcc, 0x5a, 0xab, 0x46, 0x8e, 0x3b, 0x42, 0x54, 0xc8, 0x13, 0xc7, 0xc4, 
0x0b, 0x40, 0xcf, 0x6c, 0xd7, 0x52, 0x00, 0x45, 0x7a, 0xc0, 0x57, 0x42, 0x11, 0x50, 0xaf, 0x6c, 
0xdc, 0xff, 0x71, 0x54, 0x88, 0x75, 0xe4, 0x08, 0xcc, 0x78, 0xcf, 0x6c, 0xde, 0xb3, 0x04, 0x44, 
0x00, 0x30, 0xd4, 0x48, 0x38, 0x70, 0xaf, 0x6c, 0xdf, 0x77, 0x36, 0x45, 0x48, 0x53, 0xd7, 0x48, 
0x79, 0x54, 0xcf, 0x6c, 0xde, 0xfb, 0x64, 0x44, 0x44, 0x45, 0xe4, 0xcc, 0xfb, 0x50, 0xaf, 0x6c, 
0xd3, 0x96, 0x35, 0x94, 0x52, 0x91, 0x4f, 0x68, 0xe7, 0xfc, 0xcf, 0x6c, 0xd6, 0x97, 0x47, 0xc1, 
0xe0, 0xc0, 0x55, 0xa4, 0x46, 0xc0, 0xaf, 0x6c, 0xd4, 0xb0, 0x45, 0x00, 0xf4, 0x6d, 0x6d, 0x6e, 
0x78, 0xf6, 0x89, 0x4a, 0xf7, 0xb0, 0x07, 0x10, 0xf4, 0xdc, 0x4c, 0x2e, 0x09, 0xd6, 0x89, 0x4a, 
0xf4, 0xf3, 0x16, 0x55, 0x2f, 0xcf, 0x73, 0x04, 0xfe, 0x96, 0x89, 0x4a, 0xfb, 0xf7, 0x27, 0x04, 
0x7f, 0x57, 0xbb, 0xb6, 0xe3, 0x76, 0x89, 0x4a, 0xf4, 0xd1, 0x67, 0x50, 0xef, 0x20, 0x28, 0xd7, 
0x2f, 0x96, 0x89, 0x4a, 0xfb, 0x76, 0xa8, 0x61, 0xdd, 0xd3, 0xf7, 0x21, 0xae, 0x36, 0x89, 0x4a, 
0xf2, 0xf2, 0xfd, 0xfc, 0x9f, 0x86, 0x76, 0x63, 0x5a, 0x56, 0x89, 0x4a, 0xf9, 0x75, 0x9e, 0x39, 
0x9d, 0x80, 0xd0, 0x0f, 0xad, 0x96, 0x89, 0x4a, 0xfb, 0x35, 0xc5, 0xc9, 0xe9, 0xe5, 0xbd, 0x7a, 
0x12, 0x56, 0x89, 0x4a, 0xfa, 0xdd, 0x01, 0xc1, 0x0b, 0xf0, 0x9f, 0x50, 0xc6, 0xb6, 0x89, 0x4a, 
0xf2, 0xfa, 0x56, 0x69, 0x05, 0xea, 0x8c, 0x78, 0x0b, 0x86, 0x89, 0x4a, 0xf1, 0x92, 0x43, 0x50, 
0x2f, 0x48, 0x9e, 0x1e, 0x39, 0x56, 0x89, 0x4a, 0xf1, 0x5d, 0x17, 0xdc, 0xb7, 0xc4, 0x02, 0x9c, 
0x74, 0x8a, 0xab, 0x46, 0x8f, 0x59, 0x23, 0x45, 0x05, 0x6e, 0x10, 0x58, 0xe9, 0x3a, 0x4f, 0x2c, 
0x95, 0x93, 0x63, 0xdd, 0x8d, 0xe9, 0x95, 0x08, 0xbe, 0x90, 0xed, 0x05, 0xbe, 0x9e, 0x24, 0xc4, 
0x70, 0xfe, 0x6f, 0xb6, 0x6a, 0xba, 0x4b, 0x86, 0xbd, 0x5f, 0x76, 0xc4, 0x06, 0xba, 0x4d, 0xeb, 
0x1a, 0x5c, 0xcd, 0x8c, 0xdf, 0x99, 0x03, 0x55, 0xc5, 0x03, 0x20, 0xd8, 0x6d, 0x1a, 0x4f, 0x6e, 
0xb7, 0x59, 0x57, 0x45, 0xc3, 0x40, 0xfd, 0x46, 0xcb, 0x09, 0xcb, 0x86, 0xb0, 0xb1, 0x45, 0x83, 
0xfb, 0x12, 0x95, 0x1c, 0xa1, 0x4a, 0x43, 0x0e, 0x35, 0xfd, 0x42, 0x04, 0xf5, 0x4a, 0x0b, 0x1c, 
0x9d, 0xf0, 0xaf, 0x60, 0xbf, 0x53, 0x33, 0x1d, 0x7f, 0x86, 0x25, 0xe7, 0x5f, 0x5a, 0x47, 0x80, 
0xb5, 0x9f, 0x66, 0xdc, 0x6c, 0xfb, 0xde, 0x16, 0xfd, 0x68, 0x43, 0x04, 0x34, 0x3d, 0x37, 0xdc, 
0x08, 0x31, 0x73, 0xd4, 0x1d, 0xfa, 0x4f, 0x6c, 0xde, 0x54, 0x3a, 0xdc, 0x3a, 0x45, 0xe3, 0x9a, 
0x36, 0xcd, 0xc6, 0x0a, 0x5d, 0xbe, 0x17, 0x45, 0x32, 0x50, 0xd4, 0xdd, 0x67, 0x9a, 0x4a, 0x86, 
0x31, 0xb3, 0x88, 0x8d, 0xe3, 0x32, 0x34, 0xc9, 0xbd, 0x2b, 0xcf, 0x6c, 0xd9, 0x97, 0xae, 0xb5, 
0x39, 0x49, 0x31, 0x91, 0xa6, 0x4a, 0x4a, 0x05, 0xbb, 0xbf, 0xd8, 0xac, 0x13, 0xa4, 0x23, 0xf7, 
0xb8, 0x8b, 0xcf, 0x6c, 0xc8, 0xf9, 0x95, 0xe9, 0x25, 0xdf, 0xbd, 0xba, 0x04, 0x68, 0x4a, 0x2a, 
0xf1, 0xf5, 0x99, 0xc9, 0x67, 0x2a, 0x41, 0xa8, 0xeb, 0x76, 0x89, 0x4a, 0xf1, 0x71, 0xd0, 0xca, 
0x2f, 0x0b, 0x41, 0xa8, 0xe5, 0x56, 0x89, 0x4a, 0xfa, 0x9f, 0xa2, 0x51, 0xa5, 0x7a, 0xb9, 0xfd, 
0xbd, 0x36, 0x89, 0x4a, 0xfb, 0xb7, 0x85, 0xc0, 0xeb, 0x7c, 0x01, 0x5e, 0xb6, 0x0a, 0xab, 0x46, 
0x81, 0x91, 0xb4, 0x50, 0x09, 0x39, 0xb6, 0x20, 0xb7, 0x7a, 0x4f, 0x2c, 0x91, 0x55, 0x86, 0x60, 
0x09, 0x38, 0xa7, 0x2c, 0x24, 0x50, 0xed, 0x05, 0xb3, 0xb1, 0xef, 0x34, 0x5b, 0x28, 0x85, 0x2f, 
0xf3, 0x1a, 0x4b, 0x86, 0xb2, 0x7d, 0x99, 0xac, 0x3f, 0x75, 0x2d, 0xea, 0x51, 0xfc, 0xcd, 0x8c, 
0xd3, 0xd3, 0x8d, 0xac, 0xdf, 0xd9, 0xa1, 0x24, 0x0a, 0x7a, 0x4f, 0x6e, 0xb3, 0x7f, 0xfe, 0x41, 
0xe1, 0x26, 0x18, 0x6a, 0xa8, 0xd9, 0xcb, 0x86, 0xbb, 0x5d, 0x22, 0xd1, 0xcf, 0x61, 0x9e, 0xd4, 
0x0b, 0x8a, 0x43, 0x0e, 0x35, 0xfb, 0x06, 0xd4, 0x6b, 0x1b, 0xa5, 0x6a, 0xfd, 0x50, 0xaf, 0x60, 
0xb6, 0xb1, 0x26, 0x44, 0xc9, 0x97, 0x81, 0x8b, 0x1d, 0xca, 0x47, 0x80, 0xb6, 0xfd, 0x46, 0xdc, 
0xa9, 0x62, 0x3a, 0x42, 0x9a, 0x38, 0x43, 0x04, 0x35, 0x1b, 0x65, 0xcd, 0x24, 0x55, 0xcd, 0x9e, 
0xec, 0x4a, 0x4f, 0x6c, 0xd1, 0xd8, 0x10, 0xdd, 0xa4, 0x9c, 0x5b, 0x13, 0x0c, 0x9d, 0xc6, 0x0a, 
0x50, 0x18, 0x66, 0x44, 0x80, 0x3c, 0xe1, 0x16, 0xac, 0x5a, 0x4a, 0x86, 0x33, 0x7e, 0x74, 0x11, 
0x4a, 0x39, 0xc8, 0xfe, 0x05, 0xeb, 0xcf, 0x6c, 0xd1, 0x1c, 0x61, 0x11, 0x0b, 0x96, 0x3f, 0x66, 
0x60, 0xba, 0x4a, 0x05, 0xb7, 0x37, 0x35, 0x44, 0xe7, 0xeb, 0x53, 0x04, 0x8e, 0x9b, 0xcf, 0x6c, 
0xc4, 0x99, 0x15, 0xc5, 0x8f, 0xcc, 0x3a, 0x28, 0xcf, 0xd8, 0x4a, 0x2a, 0xfb, 0x9a, 0x05, 0xd5, 
0xff, 0x4b, 0x8a, 0x1b, 0x69, 0x86, 0x89, 0x4a, 0xf9, 0x31, 0xa1, 0x38, 0x55, 0xb4, 0xc3, 0x83, 
0x8b, 0x96, 0x89, 0x4a, 0xf8, 0x71, 0x82, 0x71, 0xc5, 0x48, 0x34, 0xb3, 0x24, 0xb6, 0x89, 0x4a, 
0xf3, 0x7b, 0x94, 0xf4, 0x89, 0x51, 0x09, 0x9d, 0xe5, 0xaa, 0xab, 0x46, 0x83, 0x9b, 0xf5, 0x60, 
0x51, 0xe0, 0xbd, 0x0b, 0xce, 0x8a, 0x4f, 0x2c, 0x93, 0x53, 0xb2, 0xf9, 0x39, 0xda, 0x36, 0x46, 
0xaa, 0x50, 0xed, 0x05, 0xb0, 0xdf, 0xa0, 0xfa, 0xb1, 0xf3, 0x05, 0x42, 0xa1, 0x7a, 0x4b, 0x86, 
0xb8, 0x75, 0xa1, 0xe6, 0x69, 0xd6, 0x27, 0xcc, 0x1a, 0xdc, 0xcd, 0x8c, 0xd9, 0x7d, 0xc1, 0x6f, 
0x09, 0x5b, 0xad, 0x41, 0x7a, 0x1a, 0x4f, 0x6e, 0xb2, 0x35, 0xa0, 0x60, 0x71, 0xa3, 0x03, 0x4d, 
0xe3, 0x19, 0xcb, 0x86, 0xb2, 0xf9, 0xb7, 0xf8, 0xd1, 0x9c, 0x8b, 0x04, 0x63, 0xca, 0x43, 0x0e, 
0x3b, 0x91, 0xd4, 0x6c, 0x71, 0x4c, 0x09, 0xa0, 0xb7, 0x70, 0xaf, 0x60, 0xba, 0x5b, 0xa7, 0xcd, 
0xb2, 0xc1, 0x44, 0x17, 0x2e, 0x3a, 0x47, 0x80, 0xbb, 0xff, 0x33, 0x5d, 0xfc, 0xae, 0xfc, 0xf4, 
0xde, 0x28, 0x43, 0x04, 0x3b, 0xf3, 0x53, 0xc4, 0x5c, 0xd3, 0x65, 0xf9, 0xcf, 0xca, 0x4f, 0x6c, 
0xd4, 0x57, 0x53, 0x54, 0x00, 0xf4, 0x74, 0x5b, 0x1f, 0xbd, 0xc6, 0x0a, 0x58, 0x7d, 0x46, 0x08, 
0x6f, 0x61, 0x0b, 0xaf, 0x46, 0x0a, 0x4a, 0x86, 0x3b, 0x7d, 0x46, 0xc4, 0xc8, 0x9b, 0x5e, 0x9e, 
0x1e, 0x1b, 0xcf, 0x6c, 0xdb, 0x3b, 0x27, 0xdc, 0x0a, 0x8c, 0xf1, 0xf2, 0x78, 0x6a, 0x4a, 0x05, 
0xbe, 0x35, 0x55, 0xd9, 0xa2, 0xd2, 0x68, 0x9b, 0xa2, 0x5b, 0xcf, 0x6c, 0xc2, 0x3a, 0x41, 0xc5, 
0x9e, 0x24, 0x52, 0x1a, 0xea, 0x98, 0x4a, 0x2a, 0xff, 0xd7, 0x23, 0x89, 0xb2, 0xc5, 0x6c, 0x82, 
0xf0, 0xf6, 0x89, 0x4a, 0xfd, 0x3b, 0x61, 0x90, 0xdc, 0x82, 0x5e, 0xe4, 0x32, 0x36, 0x89, 0x4a, 
0xfd, 0x36, 0x17, 0xdc, 0xde, 0x31, 0x5d, 0xd0, 0x0f, 0x56, 0x89, 0x4a, 0xff, 0x94, 0x37, 0x99, 
0xb1, 0xde, 0x3f, 0x6a, 0xa5, 0x1a, 0xab, 0x46, 0x84, 0x9e, 0x74, 0x00, 0xf5, 0x5d, 0xa9, 0xfe, 
0x03, 0xaa, 0x4f, 0x2c, 0x94, 0x10, 0x73, 0x11, 0x6d, 0x53, 0xa0, 0x15, 0x9a, 0x50, 0xed, 0x05, 
0xb7, 0x3c, 0x03, 0x94, 0x35, 0xfb, 0x14, 0x4b, 0x28, 0x7a, 0x4b, 0x86, 0xb5, 0x38, 0x26, 0x84, 
0x35, 0xb6, 0x37, 0x0b, 0xff, 0x6c, 0xcd, 0x8c, 0xd9, 0x9d, 0xa7, 0xe9, 0xd1, 0xec, 0x3b, 0x5b, 
0xf3, 0x1a, 0x4f, 0x6e, 0xbb, 0xfd, 0xb0, 0xe9, 0x21, 0x04, 0x8d, 0xf6, 0xae, 0x49, 0xcb, 0x86, 
0xb3, 0xfb, 0xda, 0xfc, 0x9b, 0x00, 0x8c, 0x9e, 0x02, 0x1a, 0x43, 0x0e, 0x34, 0x70, 0x8c, 0x90, 
0xeb, 0x30, 0x2a, 0x3a, 0xda, 0x50, 0xaf, 0x60, 0xbf, 0x5e, 0x85, 0x10, 0x0d, 0x90, 0xae, 0x0c, 
0x8d, 0x0a, 0x47, 0x80, 0xb5, 0x17, 0x52, 0xdc, 0x8d, 0x4e, 0xa4, 0x0c, 0x5d, 0x98, 0x43, 0x04, 
0x35, 0xf5, 0x81, 0xcd, 0xad, 0xdc, 0x29, 0x8a, 0x4f, 0x3a, 0x4f, 0x6c, 0xd5, 0xfe, 0x17, 0xd0, 
0x4f, 0xb8, 0x89, 0x82, 0x5a, 0xbd, 0xc6, 0x0a, 0x57, 0xf2, 0x22, 0xd0, 0x03, 0x3a, 0x89, 0xca, 
0x3e, 0x9a, 0x4a, 0x86, 0x34, 0x7d, 0x1d, 0x89, 0xe2, 0xe3, 0xd4, 0x3a, 0x36, 0xfb, 0xcf, 0x6c, 
0xd7, 0x19, 0x57, 0x80, 0x48, 0xe5, 0xf6, 0x5c, 0x24, 0x1a, 0x4a, 0x05, 0xb7, 0x94, 0x72, 0xd1, 
0x06, 0xd0, 0x5e, 0x56, 0x0f, 0xeb, 0xcf, 0x6c, 0xcd, 0x51, 0x21, 0x58, 0xa0, 0xab, 0xf1, 0xbc, 
0x9c, 0x58, 0x4a, 0x2a, 0xfc, 0x95, 0x47, 0xc0, 0x0c, 0x0a, 0x69, 0xbd, 0x0b, 0xa6, 0x89, 0x4a, 
0xf4, 0x7e, 0x72, 0xd1, 0x26, 0x34, 0xe1, 0x50, 0x4f, 0x7a, 0xaa, 0xaa, 0xaa, 0xa1, 0x35, 0xe0, 
0x28, 0x00, 0x00, 0x00, 0x01, 0x2a, 0xaa, 0xaa, 0xaa, 0x95, 0x55, 0x55, 0x57, 0xb5, 0x54, 0xa1, 
0xad, 0x5f, 0xab, 0xe0, 0x9b, 0xa9, 0xa8, 0x6a, 0xdb, 0x4b, 0x2d, 0xaa, 0xd5, 0x57, 0x19, 0x7b, 
0x5c, 0xe9, 0x17, 0x68, 0x49, 0x4e, 0x65, 0x6d, 0x55, 0x64, 0x66, 0xb3, 0xb2, 0x30, 0xb3, 0x58, 
0x59, 0x74, 0xaa, 0xa9, 0xa9, 0x4e, 0x66, 0x93, 0x55, 0xd6, 0xb3, 0xc9, 0x97, 0xe3, 0x2a, 0xd6, 
0xa5, 0xf5, 0x5a, 0xcb, 0x8d, 0x7b, 0x6b, 0x56, 0xf7, 0x43, 0x52, 0xb6, 0xaa, 0x55, 0x64, 0xae, 
0xae, 0x64, 0xa3, 0x96, 0x94, 0x76, 0xc9, 0x96, 0xaa, 0xbc, 0xc7, 0x55, 0x4e, 0xe9, 0xb6, 0x64, 
0xb2, 0xca, 0xd3, 0xa4, 0xe5, 0x76, 0xd3, 0x58, 0x2b, 0x25, 0x69, 0x5a, 0xe3, 0x05, 0xf5, 0x5f, 
0x1d, 0x89, 0x53, 0x4a, 0xad, 0xba, 0xb4, 0xaa, 0xa6, 0xeb, 0xad, 0xba, 0x98, 0xe0, 0xd5, 0xaa, 
0x96, 0x57, 0x52, 0x2f, 0x64, 0xb7, 0x54, 0xbe, 0x4a, 0xb4, 0xd6, 0x55, 0x25, 0x22, 0xee, 0xb4, 
0xd2, 0x13, 0xef, 0xa6, 0x92, 0xa2, 0xda, 0xab, 0xa9, 0x8a, 0xeb, 0x52, 0x29, 0xea, 0xae, 0x43, 
0xce, 0xa4, 0x5e, 0xf5, 0x55, 0x62, 0x91, 0xa6, 0xc3, 0x6c, 0x4d, 0x56, 0xad, 0x57, 0x5a, 0x21, 
0x55, 0x79, 0x5e, 0xb5, 0x52, 0x7a, 0x5a, 0xb5, 0x4a, 0x7a, 0xd5, 0x6b, 0xa6, 0x15, 0x6e, 0xae, 
0xcd, 0xab, 0xaa, 0x2c, 0x96, 0xb5, 0x2a, 0xb2, 0x48, 0xa6, 0xa5, 0x4c, 0xee, 0xfa, 0x93, 0x8f, 
0x35, 0xda, 0x49, 0x35, 0x11, 0x59, 0x7a, 0x4d, 0x65, 0x6e, 0xbf, 0x4a, 0xa2, 0x95, 0x2b, 0x74, 
0xa8, 0xd6, 0x56, 0x9a, 0x5d, 0x73, 0x55, 0x9a, 0xaa, 0xd5, 0x23, 0xd6, 0x4f, 0x15, 0xb5, 0xb3, 
0x55, 0x85, 0xe9, 0x79, 0xa9, 0x2e, 0x54, 0x2d, 0x2a, 0xea, 0x43, 0x5b, 0xad, 0xd5, 0xd7, 0xf2, 
0xea, 0x13, 0xa5, 0x0b, 0xca, 0x6d, 0x6d, 0x2a, 0xc2, 0x15, 0x6a, 0x9d, 0x26, 0xa2, 0x17, 0xb4, 
0xad, 0xfa, 0x53, 0xd5, 0xf9, 0x18, 0xa9, 0x6a, 0xae, 0xd5, 0x25, 0x4a, 0x92, 0xbb, 0x57, 0xc5, 
0x5e, 0x8a, 0xbb, 0x69, 0x5b, 0xaa, 0xd7, 0x56, 0xaa, 0xad, 0x5a, 0xd5, 0x58, 0xd2, 0xb2, 0xad, 
0xaa, 0xa6, 0xda, 0xa4, 0x17, 0x2a, 0x55, 0xad, 0xdd, 0x2e, 0x66, 0xd2, 0x6b, 0xd7, 0x99, 0x41, 
0x35, 0x54, 0xbb, 0x4a, 0x2b, 0xac, 0xa1, 0x54, 0xad, 0xee, 0xad, 0x2d, 0x65, 0x2b, 0xcc, 0xc4, 
0xae, 0xdf, 0x8f, 0x59, 0xcd, 0x95, 0x65, 0x6d, 0x94, 0xbd, 0x38, 0xa5, 0x1b, 0x52, 0xf5, 0xeb, 
0x9f, 0x73, 0x56, 0xb3, 0x32, 0xa0, 0xd6, 0xdd, 0x39, 0x4a, 0xad, 0x45, 0xbb, 0x6d, 0x9b, 0x9d, 
0xed, 0xd9, 0xad, 0xe6, 0x86, 0x88, 0xac, 0xaf, 0x26, 0x57, 0x4a, 0x5a, 0x6a, 0xda, 0xca, 0x8e, 
0xa9, 0x43, 0xbe, 0xb3, 0x88, 0x7b, 0x6d, 0x4d, 0xd3, 0x51, 0xab, 0x1a, 0x9d, 0xa6, 0xa7, 0x2d, 
0x57, 0x9a, 0x5b, 0x46, 0xb3, 0xdb, 0xb3, 0x32, 0xd5, 0x75, 0x5a, 0x8b, 0x25, 0x3b, 0x87, 0x21, 
0xa4, 0xbc, 0xea, 0x95, 0x7d, 0x5a, 0x42, 0x5e, 0xdb, 0x69, 0xad, 0x2a, 0xa9, 0x5e, 0xa6, 0xed, 
0x99, 0xe5, 0x31, 0x50, 0xd3, 0xaa, 0xe6, 0x9c, 0xa5, 0xa5, 0xcd, 0x50, 0xad, 0xd6, 0x56, 0x4b, 
0x66, 0x49, 0xa5, 0x7a, 0x82, 0xd2, 0x33, 0xad, 0x66, 0xd5, 0xaa, 0x72, 0xb7, 0x62, 0x52, 0xaa, 
0xef, 0x4d, 0xa2, 0xb5, 0x5a, 0xa6, 0xa5, 0x0e, 0x6a, 0xca, 0xda, 0xfd, 0x3a, 0xe4, 0x94, 0x4d, 
0xa8, 0x0a, 0xa2, 0xb4, 0xca, 0x89, 0x4a, 0xba, 0x52, 0xa5, 0x83, 0xcc, 0x92, 0xb5, 0xca, 0xea, 
0xc9, 0x3a, 0x9c, 0x5a, 0xca, 0xd5, 0x69, 0xaf, 0x62, 0xdb, 0x6a, 0x7c, 0xd3, 0x72, 0x9d, 0x5a, 
0x7b, 0xad, 0xd2, 0x73, 0x6a, 0xa2, 0x97, 0x43, 0xb9, 0x9f, 0x1c, 0xd5, 0x52, 0xad, 0x52, 0xcd, 
0xd9, 0x66, 0xfa, 0x76, 0x49, 0xfe, 0xed, 0x11, 0xa9, 0xf6, 0x52, 0xca, 0x16, 0xa7, 0xb3, 0xa9, 
0x69, 0xaf, 0x7a, 0xaf, 0x7a, 0xa9, 0x95, 0xcd, 0x9a, 0xd2, 0xf9, 0x62, 0xab, 0x15, 0x9a, 0xa7, 
0x4d, 0x55, 0x5a, 0xe9, 0x49, 0x76, 0xb6, 0x6a, 0x67, 0xb5, 0xde, 0xab, 0xea, 0xa9, 0x9a, 0x8f, 
0x4a, 0xbb, 0x67, 0x55, 0x48, 0xed, 0xa5, 0xac, 0xcf, 0x32, 0xd5, 0x6e, 0xba, 0xb6, 0x4a, 0x2d, 
0xed, 0x74, 0x6c, 0xfa, 0xaa, 0x80, 0xe6, 0x54, 0xa4, 0xf4, 0xba, 0xf6, 0xb6, 0x46, 0x99, 0x4a, 
0xbb, 0x5a, 0x9b, 0x6b, 0x92, 0xa2, 0xd3, 0xb3, 0xac, 0x2d, 0xf4, 0x75, 0x2a, 0xb4, 0x92, 0xee, 
0xa2, 0xa7, 0x0a, 0xb7, 0x6b, 0x7a, 0x74, 0xad, 0x54, 0x74, 0xba, 0x54, 0x36, 0xa6, 0x5d, 0xc7, 
0x95, 0x46, 0x55, 0xad, 0x9f, 0x5a, 0xd5, 0x4a, 0x46, 0x8b, 0x55, 0x86, 0xae, 0xd5, 0x26, 0xaa, 
0xcc, 0xda, 0x82, 0x89, 0x72, 0xf6, 0xcc, 0xa5, 0xd9, 0x72, 0x4b, 0xba, 0x9d, 0x8b, 0x35, 0xb2, 
0xd4, 0xb1, 0xed, 0x33, 0x32, 0xda, 0x15, 0x52, 0xae, 0xda, 0xe9, 0x33, 0x6a, 0xd3, 0x5d, 0x54, 
0x53, 0x14, 0xb2, 0x6a, 0x4e, 0x22, 0x92, 0x14, 0x2b, 0x7e, 0xdd, 0x2b, 0x6a, 0xe7, 0xb5, 0x6a, 
0x97, 0x1a, 0xd3, 0x4a, 0xdd, 0x56, 0xd5, 0xad, 0xf6, 0x2a, 0xf4, 0x7d, 0x65, 0x94, 0x5a, 0x95, 
0x25, 0x3b, 0x65, 0xab, 0x6a, 0x59, 0x33, 0x8a, 0x50, 0xcb, 0xc5, 0x1a, 0x8c, 0x9c, 0x34, 0xb4, 
0xaa, 0xf9, 0x2e, 0x97, 0x52, 0x3a, 0x54, 0xb5, 0x61, 0x21, 0x2c, 0xaf, 0x5a, 0x53, 0xb1, 0xca, 
0xb9, 0xd3, 0x49, 0x6b, 0x95, 0xc9, 0x52, 0xae, 0xba, 0xdd, 0x26, 0xca, 0x2b, 0x2e, 0xb2, 0xb6, 
0x4b, 0x6a, 0x6c, 0x94, 0x15, 0x33, 0xe9, 0x63, 0xbc, 0x91, 0x96, 0xab, 0x15, 0x6e, 0xf4, 0x56, 
0xae, 0xda, 0xd5, 0x68, 0x89, 0xe2, 0xcb, 0x95, 0x95, 0x57, 0x15, 0xa8, 0xe8, 0x99, 0x32, 0x9e, 
0x8b, 0x56, 0x6a, 0xb7, 0xba, 0x7a, 0x5a, 0xaf, 0x74, 0x9a, 0xa6, 0xb6, 0xd0, 0x11, 0xad, 0x36, 
0xcd, 0x6a, 0x09, 0x3e, 0x6a, 0x9a, 0x4c, 0xa9, 0x2f, 0x55, 0x9d, 0xfd, 0xc6, 0xed, 0x6e, 0x4b, 
0x2a, 0xbe, 0xf7, 0x4b, 0x37, 0x2a, 0x5a, 0x16, 0xb2, 0x57, 0x51, 0x4b, 0x6a, 0xaa, 0xbb, 0x69, 
0xa7, 0x12, 0xbd, 0x55, 0xba, 0xb5, 0x2d, 0x43, 0xcb, 0xc9, 0xa9, 0x29, 0xfd, 0x35, 0x52, 0x95, 
0xab, 0xa9, 0xa9, 0x4a, 0x7a, 0xdb, 0x77, 0xb3, 0x45, 0x57, 0xbf, 0x56, 0xa4, 0xc2, 0xba, 0x69, 
0x45, 0x5b, 0x9a, 0xdd, 0x4a, 0xab, 0x6b, 0xea, 0xaa, 0xd7, 0xa4, 0xfe, 0xaa, 0xac, 0x15, 0xa5, 
0x50, 0x5b, 0xed, 0x5e, 0xa4, 0xdb, 0x35, 0x72, 0xda, 0x4b, 0x6e, 0x4d, 0x5a, 0xcf, 0xa5, 0x50, 
0xbc, 0xa8, 0xaa, 0xac, 0x72, 0xaa, 0x9e, 0xfa, 0xba, 0x65, 0x59, 0x6b, 0x66, 0xe6, 0x55, 0x94, 
0x72, 0x10, 0xcb, 0xaa, 0xab, 0xee, 0xa8, 0xe3, 0xa5, 0x95, 0x36, 0xb7, 0x37, 0xb7, 0x19, 0x66, 
0xb5, 0x5b, 0x55, 0x5a, 0xb1, 0x4a, 0xb4, 0x7c, 0xa6, 0xfa, 0x6b, 0x55, 0xa5, 0xac, 0x81, 0x7c, 
0x8b, 0xea, 0x6b, 0x5b, 0x37, 0x44, 0xb7, 0x4d, 0xdc, 0xd6, 0x7a, 0xdc, 0x9a, 0x6a, 0x35, 0x47, 
0xb3, 0xf4, 0xb5, 0x57, 0x08, 0x3a, 0xa9, 0x76, 0xae, 0x77, 0xa9, 0xa1, 0x6b, 0x1d, 0x9a, 0xda, 
0xa8, 0x5e, 0x1b, 0xf9, 0x4c, 0x74, 0x94, 0x59, 0xaf, 0x39, 0x72, 0xb4, 0x8c, 0x34, 0x73, 0xd5, 
0x57, 0x1d, 0x55, 0xa5, 0xa8, 0x8a, 0xbb, 0xcd, 0xd5, 0xca, 0x64, 0xd5, 0x4d, 0x68, 0xa9, 0x79, 
0x4a, 0xa3, 0x55, 0xcd, 0xa4, 0x57, 0x55, 0x4b, 0xe5, 0x7a, 0x6a, 0xf5, 0x77, 0xaf, 0xd2, 0x48, 
0x10, 0x6c, 0x94, 0xac, 0x37, 0x6e, 0x59, 0x71, 0x2e, 0x8a, 0xaa, 0xdb, 0xd1, 0xdb, 0x63, 0xaa, 
0xb7, 0x9b, 0x8b, 0x24, 0x59, 0x26, 0xad, 0x76, 0x8d, 0xa8, 0xca, 0xad, 0x79, 0x1b, 0x83, 0x6a, 
0xab, 0x35, 0xbd, 0xca, 0xa8, 0xa5, 0x2d, 0x29, 0x5b, 0x35, 0xea, 0xf6, 0xa8, 0xaa, 0x6e, 0x8d, 
0xad, 0x4b, 0x26, 0x2b, 0x55, 0x49, 0x4d, 0xd9, 0x40, 0x72, 0xd7, 0x15, 0x51, 0xd7, 0x9a, 0x5b, 
0x2d, 0x15, 0x54, 0x5a, 0xb1, 0x0d, 0x55, 0x46, 0xdb, 0xfd, 0xb1, 0xb7, 0x5d, 0xce, 0xab, 0x4d, 
0x35, 0x55, 0x34, 0xd5, 0x32, 0xb4, 0xe2, 0x76, 0xf6, 0x55, 0x46, 0xaf, 0x25, 0x73, 0x7d, 0x54, 
0xbc, 0xb5, 0x35, 0xad, 0xca, 0xca, 0xd4, 0xd6, 0x07, 0x65, 0xaa, 0xa5, 0x2b, 0x55, 0x3a, 0x5a, 
0xbf, 0xd4, 0xfc, 0xba, 0x95, 0xab, 0x9e, 0x53, 0x3d, 0x5b, 0xaa, 0x6c, 0xcb, 0xdb, 0x5a, 0xae, 
0x52, 0xe5, 0x45, 0xbf, 0x71, 0x47, 0xa3, 0xa7, 0x2e, 0xad, 0xb5, 0x32, 0xe0, 0xd2, 0xbc, 0xcc, 
0xba, 0xea, 0xa5, 0x35, 0x63, 0x5a, 0xb2, 0xb3, 0xad, 0x2b, 0xe6, 0x6e, 0x5a, 0xca, 0xbd, 0xb7, 
0xb3, 0x96, 0xb4, 0xe5, 0x0a, 0xde, 0xde, 0x30, 0xa7, 0x66, 0xe7, 0x2f, 0x4b, 0x45, 0x2e, 0x7a, 
0xd4, 0x94, 0xda, 0x5d, 0x44, 0xb1, 0xa8, 0x6a, 0xa4, 0x56, 0x97, 0xdd, 0x5d, 0x4b, 0x34, 0x69, 
0xd8, 0xab, 0x75, 0x35, 0x65, 0x0d, 0x8c, 0x56, 0x90, 0x72, 0x95, 0x6a, 0x53, 0x9a, 0xbd, 0x7f, 
0x95, 0x7a, 0x2a, 0xec, 0xf8, 0x49, 0x4b, 0x47, 0x6b, 0xa5, 0x49, 0x51, 0xac, 0xa2, 0x8a, 0xe6, 
0xa9, 0x67, 0x19, 0x56, 0x54, 0xde, 0x68, 0x3d, 0xbd, 0x97, 0x15, 0xf5, 0x4a, 0xdc, 0x9a, 0x55, 
0xca, 0xe5, 0x5a, 0x73, 0xae, 0x4d, 0x37, 0x7a, 0x2d, 0x27, 0x1b, 0xe6, 0xa3, 0x6b, 0xe9, 0xca, 
0xd5, 0xd5, 0xd3, 0x69, 0x52, 0xb5, 0x6b, 0x6f, 0x5e, 0xb5, 0x5a, 0x91, 0x65, 0xca, 0x5c, 0x8d, 
0x9e, 0xd4, 0x2a, 0xdb, 0x3e, 0x2d, 0x94, 0xca, 0x9d, 0x62, 0x89, 0x6e, 0xb3, 0x4a, 0x31, 0xa9, 
0x59, 0x76, 0x16, 0x1a, 0xd5, 0xb4, 0x80, 0xab, 0x19, 0x7a, 0x45, 0x29, 0x54, 0xa1, 0xd5, 0xbd, 
0x45, 0x2c, 0xa9, 0x7f, 0x46, 0x92, 0xab, 0x81, 0x75, 0x5d, 0x54, 0xda, 0xb9, 0x55, 0xa2, 0x98, 
0xd4, 0xb5, 0x1e, 0x98, 0x53, 0x34, 0xe1, 0x69, 0x45, 0x6b, 0x57, 0x28, 0xb5, 0x50, 0xa9, 0xbd, 
0x6b, 0x3e, 0x14, 0x09, 0xa5, 0x4c, 0xb2, 0xea, 0x4f, 0xa3, 0xee, 0x6a, 0x2b, 0x53, 0x69, 0x6b, 
0xca, 0xb7, 0xa6, 0x7e, 0x48, 0xb3, 0xeb, 0xaa, 0xa4, 0x55, 0x35, 0xd1, 0x54, 0x46, 0xba, 0xaa, 
0xc6, 0x8e, 0x8e, 0x6a, 0xdb, 0x5b, 0x69, 0xab, 0x5d, 0xcb, 0x55, 0xb5, 0x2a, 0x7a, 0x6b, 0xbd, 
0x2c, 0xee, 0xce, 0xaa, 0xde, 0xbb, 0x17, 0x3b, 0xaa, 0xd2, 0xcf, 0x63, 0x56, 0xd2, 0x12, 0xa9, 
0x45, 0x4b, 0xb9, 0x92, 0xbc, 0xa9, 0xb5, 0x35, 0xab, 0xad, 0x67, 0x5b, 0x53, 0x5b, 0x49, 0x8d, 
0xa9, 0xae, 0x95, 0x52, 0xaa, 0x6d, 0x1b, 0x66, 0xca, 0x6f, 0x52, 0xa6, 0xcc, 0x7d, 0x01};
