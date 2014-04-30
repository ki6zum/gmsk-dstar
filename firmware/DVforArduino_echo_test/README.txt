DVforArduino Echo Test
This is a simple application that records serveral seconds of audio and then plays it back again.
This is useful when trying to optimize the the input and output signal levels.  When the board is recoring
the COS light will be on and when it is playing back, the PTT light will be on. You may have to comment out
the the line 
#define txinvert true
if your radio only recognizes the transmission as FM instead of DV.

version 0.1
This is the initial release of the source code.  It should build and run on most any flavor of
Arduino board from the ATmega328 based boards all the way to Mega boards.  The code was built
using Arduino 1.0.5

