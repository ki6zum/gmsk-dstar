RX SN display 
This is software that will display the values of the RXSN pin of the CMX589.  It can be used
to get an indication of the signal quality being received by the CMX589 chip. By adjusting the
input signal level an optimum Bit Error Rate (BER) and Signal to Noise ratio (SN) can be achieved.
It will run on either the Amega328 or the Mega versions of the Arduino boards. Just using the 
Arduino serial monitor window is good enough to view the data from the 


version 0.1
This is the initial release of the source code.  It should build and run on most any flavor of
Arduino board from the ATmega328 based boards all the way to Mega boards.  The code was built
using Arduino 1.0.1



The text below shows the output of an example run of the software as the board starts to receive
a DStar transmission and the signal level is adjusted to minimize the BER and Maximize the SN.


RX_SN_display  Copyright (C) 2013

This program comes with ABSOLUTELY NO WARRANTY;
This is free software, and you are welcome to redistribute it
under certain conditions;
Free SRAM at startup 
6221
% high: 4, BER: 0.00e+00, SN: 0.00
% high: 5, BER: 0.00e+00, SN: 0.00
% high: 4, BER: 0.00e+00, SN: 0.00
% high: 38, BER: 0.00e+00, SN: 0.00
% high: 46, BER: 1.24e-02, SN: 5.75
% high: 93, BER: 7.37e-06, SN: 12.17
% high: 87, BER: 3.17e-05, SN: 11.17
% high: 88, BER: 2.52e-05, SN: 11.33
% high: 93, BER: 7.37e-06, SN: 12.17
% high: 78, BER: 2.22e-04, SN: 9.75
% high: 92, BER: 1.00e-05, SN: 12.00
% high: 85, BER: 5.04e-05, SN: 10.83
% high: 96, BER: 2.95e-06, SN: 12.67
% high: 87, BER: 3.17e-05, SN: 11.17
% high: 93, BER: 7.37e-06, SN: 12.17
% high: 85, BER: 5.04e-05, SN: 10.83
% high: 90, BER: 1.59e-05, SN: 11.67
% high: 88, BER: 2.52e-05, SN: 11.33
% high: 98, BER: 1.60e-06, SN: 13.00
% high: 97, BER: 2.17e-06, SN: 12.83
% high: 99, BER: 1.18e-06, SN: 13.17
% high: 99, BER: 1.18e-06, SN: 13.17
% high: 100, BER: 8.69e-07, SN: 13.33
% high: 99, BER: 1.18e-06, SN: 13.17
% high: 98, BER: 1.60e-06, SN: 13.00
% high: 93, BER: 7.37e-06, SN: 12.17
% high: 100, BER: 8.69e-07, SN: 13.33
% high: 99, BER: 1.18e-06, SN: 13.17
