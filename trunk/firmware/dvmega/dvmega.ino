
/*
DV-MEGA version 01.60A - Copyright (C) 2013/2014 - Guus van Dooren (PE1PLM)
Digital voice node adapter. This firmware is made for the Arduino, 
in combination with a CMX589 GMSK modem chip. It will run on either 
the Amega328 or the Mega versions of the Arduino boards.

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

For questions, comments or suggestions please contact
Guus van Dooren, PE1PLM via E-mail : info@auria.nl

17-08-2013 - V00.1A Test procedures V01
23-08-2013 - V00.2A Header decoding implemented V02
27-08-2013 - V00.3A Comminucation with host V03
29-08-2013 - V00.4A Voice from radio to modem to host is okay V04
30-08-2013 - V00.5A Voice from host to modem to radio is working without header V05
06-09-2013 - V00.6A TX Buffer enlagerd to 0,85K
09-09-2013 - V01.0A First release under GNU GPL2
21-09-2013 - V01.1A ADF7021 tranceiver and optimized buffer control
05-10-2013 - V1.10A Header encoding fixed, long TX delay
17-10-2013 - V1.11A RX buffer modified
10-01-2014 - V1.12A Pre-release DV node adapter
01-01-2014 - V1.15V Temporarily version for VHF
12-01-2014 - V1.12B Some fixes to comply with DVRPTR Control Center
12-01-2014 - V1.53A Some fixes to comply with DVRPTR Control Center
17-01-2014 - V1.54A Merge of VHF and UHF and frequency setting
31-01-2014 - V1.55A Initialize both radio's on start
30-03-2014 - V1.60A status LED fixes, cleaned up formatting, minor code fixes, 
                    bit pattern checking more tolerant, unified AD7021/GMSK adapter code
*/

// BOF preprocessor bug prevent - insert me on top of your arduino-code
#if 1
__asm volatile ("nop");
#endif

//#define toggleDebug true
#define INV(x,y) x ^= _BV(y) //toggle bit - using bitwise XOR operator
// toggleDebug is hard coded for the UNO using PORTB-5 to signal - same pin as Arduino 13 which is the built-in LED pin
// pin definition HAS to be changed if you are using anything other than a ATMega328P based board! eg. INV(PORTB, 5)
//#define serial1Debug true

// uncomment only one of the three following defines
//#define ADF7021_eval true
#define GMSK_adapter true
//#define AD7021 true


#if defined(AD7021) || defined(ADF7021_eval)
#define pinAD7021_SCLK 7
#define pinAD7021_SDATA 10
#define pinAD7021_SREAD 11       // Not used
#define pinAD7021_SLE 12
#endif

#define pinRXD 5
#define pinTXD 6

#ifdef GMSK_adapter
#define pinRXCLK 2
#define pinTXCLK 3
#define pinSN 4                  // Not used
#define pinPTT 8
#define pinPLLACQ 9
#define pinCOSLED 11
#endif

#define pinLED 13

#define radio_tx_buf_len 850     // Size of radio tx ring buffer
#define radio_rx_buf_len 300     // Size of radio rx ring buffer
#define ser_in_buf_len 55        // Size of serial ring buffer
#define ser_out_buf_len 55       // Size of serial out buffer

static const uint16_t crc_table[256] PROGMEM = {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 
0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef, 
0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 
0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 
0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4, 
0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 
0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 
0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12, 
0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 
0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49, 
0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78, 
0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 
0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 
0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 
0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c, 
0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab, 
0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 
0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92, 
0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 
0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 
0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

  volatile byte input_rx_buffer;                                   // Temp input buffer from CMX589, 8 bit buffer is filled by RX_589.
  volatile byte input_rx_buffer_bit_counter = 0;                   // When 8 bits are received byte full wil be placed in radio_rx_buffer,
                                                                   // this is a ringbuffer.
  volatile int  radio_rx_buffer_counter = radio_rx_buf_len;        // pointer for end of buffer indication
  
  volatile byte radio_tx_buffer[radio_tx_buf_len + 1];             // ringbuffer for datastream from CMX589
  volatile int  radio_tx_buffer_in_counter = 0;                    // pointer for end of buffer indication
  volatile int  radio_tx_buffer_out_counter = 0;    
  volatile int  radio_tx_buffer_bit_counter = 0;
  volatile int  radio_tx_buffer_gen_counter = 0;
  
  volatile byte temp_shift_buffer[23];                             // Fifo buffer for data analisys. 21 X 8 bits can be shift right. 
  volatile int  temp_shift_buffer_counter = radio_rx_buf_len - 50; // When 8 bits are shifted a new byte pointed by counter will be
  volatile byte temp_shift_buffer_bit_counter = 0;                 // placed in temp_shift_buffer[0].

  volatile byte header_table[28][3];                               // table that holds header for descrambling an de-interleave
  volatile byte header_table_counter_y;                            // x and y keep track of actual possition.
  volatile byte header_table_counter_x;                            
  volatile int  header_table_bit_counter;
  
  volatile byte descramble_poly;                                   // Holds initial value for descrambling and convolutional decoding.
  
  volatile byte decoded_header[42];                                // Table that holds decoded header. 
  volatile byte decoded_header_byte;                               // Temp value used in procedure
  volatile int  decoded_header_counter;
 
  volatile byte serial_buffer[ser_in_buf_len + 1];                 // Input buffer for serial communication
  volatile int  serial_buffer_input_pointer = 0;
  volatile int  serial_buffer_output_pointer = 0;
 
  volatile byte serial_out[ser_out_buf_len + 1];                   // Output buffer for serial coomunication
  volatile int  table_counter;
  volatile int  serial_length;
  
  volatile byte frame_counter;                                     // Number of frames to detect 21st frame
  volatile byte frame_counter_nr;

  volatile byte packet_counter;                                    // Packet counter to stay synchronized with host
  volatile byte transmission_counter = 1;
  
  volatile byte rx_status = 0;                                     // Status of transmission
  volatile byte tx_status = 0;
  volatile boolean rx_int_flag = false;
  volatile byte stream_status_modem = 0;
  volatile byte stream_status_host = 0;
  volatile boolean tx_int_flag = false;
  
  volatile boolean rx_inverse = false;                             // RX TX inverse depend on type of tranceiver
  volatile boolean tx_inverse = false;
  
  volatile byte config_flags;                                      // Tranceiver parameters
  volatile byte config_modulation;
  volatile byte config_TX_delay_1;
  volatile byte config_TX_delay_2;
  volatile unsigned int TX_delay_counter;
  volatile byte config_status;
  
  volatile byte status_flags_hi = B00000000;                       // Stream parameters
  volatile byte status_flags_lo = B00000000;
  volatile byte status_rcv_buffer = 0;
  volatile byte status_tx_buffer = 0;
  volatile byte status_unsent_frames = 0;
  volatile byte status_total_packets_send = 0;
  volatile int  status_tx_bit_counter;
  volatile byte header_flag_1;
  volatile byte header_flag_2;
  volatile byte header_flag_3;
  volatile byte preamble_counter;
  unsigned int  crc;
  volatile unsigned int time_out_timer;

  volatile long AD7021_control_byte;                              // AD7021 related 
  volatile int  AD7021_counter;
  volatile boolean  modem_type;                                   // true = CMX589 - false = ADF7021
  volatile long frequency;
  volatile float devider;
  volatile byte N_devider;
  volatile word F_devider;
  volatile boolean band;      // false = VHF true = UHF  
  volatile long ADF7021_RX_REG0;
  volatile long ADF7021_TX_REG0;
  volatile unsigned int temp_value;  

  volatile byte tx_int_pin;

  volatile unsigned int synctimeout;

// D-Star bit order version of 0x55 0x55 0x55 0x55
#define  BIT_SYNC_DATA    0xAAAAAAAAU
#define  BIT_SYNC_MASK    0xFFFFFFFFU
#define  BIT_SYNC_ERRS    1U

// D-Star bit order version of 0x55 0x55 0x6E 0x0A
#define  FRAME_SYNC_DATA  0x55557650U
#define  FRAME_SYNC_MASK  0xFFFFFFFFU
#define  FRAME_SYNC_ERRS  1U

// D-Star bit order version of 0x55 0x2D 0x16
#define  DATA_SYNC_DATA   0x00AAB468U
#define  DATA_SYNC_MASK   0x00FFFFFFU
#define  DATA_SYNC_ERRS   0U

// D-Star bit order version of 0x55 0x55 0xC8 0x7A
#define  END_SYNC_DATA    0xAAAA135EU
#define  END_SYNC_MASK    0xFFFFFFFFU
#define  END_SYNC_ERRS    2U

static const uint8_t nibblebits[] PROGMEM = {0U, 1U, 1U, 2U, 1U, 2U, 2U, 3U, 1U, 2U, 2U, 3U, 2U, 3U, 3U, 4U};
static const uint8_t bytebits[] PROGMEM = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
  };

#if defined(SERIAL1_DEBUG) && (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__))
static const uint8_t HEXvalues[] PROGMEM = {'0', '1', '2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
#endif
static const char BINvalues[16][5] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", 
                        "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};


int freeRam () {                        // Count free ram, thanks to KI6ZUM
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}  

void setup(void)
{
// set all the pin modes and initial states
  pinMode(pinLED, OUTPUT); // default LED pin on most Arduino boards
  digitalWrite(pinLED, LOW);

#if defined(AD7021) || defined(ADF7021_eval)
  pinMode(pinAD7021_SCLK, OUTPUT);    
  pinMode(pinAD7021_SLE, OUTPUT);
  pinMode(pinAD7021_SDATA, OUTPUT);
//  pinMode(pinAD7021_SREAD, INPUT);        // Not used 
  digitalWrite(pinAD7021_SCLK, LOW);
  digitalWrite(pinAD7021_SLE, LOW);
  digitalWrite(pinAD7021_SDATA, LOW);

  Set_ADF7021_UHF();
#endif

#ifdef GMSK_adapter
  pinMode(pinRXCLK, INPUT);
  pinMode(pinTXCLK, INPUT); 
  pinMode(pinCOSLED, OUTPUT);
  pinMode(pinPLLACQ, OUTPUT);    
  pinMode(pinPTT, OUTPUT);    
  digitalWrite(pinCOSLED, LOW);
  digitalWrite(pinPLLACQ, HIGH);
  digitalWrite(pinPTT, LOW);
//  pinMode(pinSN, INPUT);         // Not used 
#endif

  pinMode(pinTXD, OUTPUT);    
  pinMode(pinRXD, INPUT);   
  digitalWrite(pinTXD, LOW);

  Serial.begin(115200);                  // set serial interface to host
  Serial.flush();

#if (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)) && defined(serial1Debug)
  Serial1.begin(115200);
  Serial1.flush();
  Serial1.print("Free Ram : ");                  
  Serial1.println(freeRam(),DEC);
  Serial1.write(13);
  Serial1.write(10);        
#endif
}

//======================================= Main loop =====================================================

void loop()
{
    //------------------------------------ Serial input -------------------------------------------------  
    // If serial char received, store in serial_buffer[x] and rotate buffer 1 pos acw
    // If valid message found Answer host and execute action, if wrong start char delete buffer
    // If buffer pointer reach max start at min, typical ringbuffer
    // For Linux OS Please change SERIAL_BUFFER_SIZE from 64 to 128 in HardwareSerial.cpp
    // File is located in \Arduino 1.0.X\arduino-1.0.X-windows\arduino-1.0.X\hardware\arduino\cores\arduino 
    
    if (Serial.available() > 0)
    {    
#if (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)) && defined(serial1Debug)
      //Serial1.write(Serial.available());
#endif     
      serial_buffer[serial_buffer_input_pointer] = Serial.read();      
      serial_buffer_input_pointer +=1; 
      if (serial_buffer_input_pointer > ser_in_buf_len) 
      {
        serial_buffer_input_pointer = ser_in_buf_len; 
      }
        
      if (serial_buffer[0] != 0xD0)
      {
        serial_buffer_input_pointer = 0; 
      }

      if ((serial_buffer[0] == 0xD0) && (serial_buffer_input_pointer > 4))  
      {
        if (serial_buffer_input_pointer > (serial_buffer[1] + 4))  
        {
          Answer_Host();           
        }    
      }  
    }
    //----------------------------------------------------------------------------------------------------
  
  if(tx_int_flag == true)      // Calculate unsent frames
  {
      if(radio_tx_buffer_in_counter > radio_tx_buffer_out_counter)
      {
        status_unsent_frames = ((radio_tx_buffer_in_counter - radio_tx_buffer_out_counter)/12);
      }  
      
      if(radio_tx_buffer_in_counter < radio_tx_buffer_out_counter)
      {
        status_unsent_frames = (((radio_tx_buffer_in_counter + radio_tx_buf_len) - radio_tx_buffer_out_counter)/12);
      }   
      
       if(radio_tx_buffer_in_counter == radio_tx_buffer_out_counter)
      {
        status_unsent_frames = 0;
      }  
  }  
  
  if(stream_status_modem > 0)
  {
    if(stream_status_modem == 1)
    {
      if(stream_status_host > 0)
      {
        temp_shift_buffer_counter = radio_rx_buf_len - 50;
        radio_rx_buffer_counter = radio_rx_buf_len;
        
        detachInterrupt(0);                               // Disable RX interrupt           
        bitClear(status_flags_hi,0);                      // RX off
        bitSet(status_flags_hi,1);                        // TX on

   #if defined(AD7021) || defined(ADF7021_eval)
        ADF7021_tx();
        attachInterrupt(0, TX_589, FALLING);     // Enable TX interrupt           
        digitalWrite(pinLED, HIGH);
   #endif
   #ifdef GMSK_adapter
        attachInterrupt(tx_int_pin, TX_589, RISING);     // Enable TX interrupt           
        digitalWrite(pinPTT, HIGH);                       // Set TX on   
   #endif
     
        TX_delay_counter = config_TX_delay_1 * 5;
        TX_delay_counter = TX_delay_counter + (config_TX_delay_2 * 5 * 255);
        stream_status_modem = 2;                          // Start TX delay
        tx_status = 2;
      }  
    }
    
    if(stream_status_modem == 2)
    {
      if (TX_delay_counter < 5 ) 
      {
        stream_status_modem = 3; 
      }
    }  
    
    if((stream_status_modem == 3)  && (stream_status_host > 1))
    {
      preamble_counter = 64;
      stream_status_modem = 4;
      tx_status = 3;
    }  

    if(stream_status_modem == 4)
    {     
      if(preamble_counter < 2) stream_status_modem = 5;  
    }
    
    if(stream_status_modem == 6)
    {     
      stream_status_modem = 7; 
      tx_status = 5; 
    }    
 
    if(stream_status_modem == 7)
    {    
      if(time_out_timer > 4800) 
        Set_RX();
      
      if(status_unsent_frames < 5) 
      {
        radio_tx_buffer_out_counter -= 60;
        if(radio_tx_buffer_out_counter < 1) radio_tx_buffer_out_counter = radio_tx_buf_len + radio_tx_buffer_out_counter;
        //radio_tx_buffer_in_counter = 240;
        //radio_tx_buffer_out_counter = 1;
      }  
    
      if(status_unsent_frames > 66) 
      {
        radio_tx_buffer_in_counter -= 60;
        if (radio_tx_buffer_in_counter < 1) 
        {
          radio_tx_buffer_in_counter = radio_tx_buf_len + radio_tx_buffer_in_counter;
        }
        //radio_tx_buffer_in_counter = 1;
        //radio_tx_buffer_out_counter = 240;
      }   
    }   
   
    if((stream_status_modem == 7) && (stream_status_host == 4))
    {
      stream_status_modem = 8;
      tx_status = 7;
    }  
     
    if(stream_status_modem == 9) 
      Set_RX();
  }
   
  if (rx_status < 6) 
  {
    if (radio_rx_buffer_counter > temp_shift_buffer_counter)
    {    
      if ((temp_shift_buffer_counter + radio_rx_buf_len) - radio_rx_buffer_counter > (radio_rx_buf_len-50)) 
        rx_int_flag = true ;
    }
  
    if (temp_shift_buffer_counter > radio_rx_buffer_counter)
    {
      if (temp_shift_buffer_counter - radio_rx_buffer_counter > (radio_rx_buf_len-50)) 
        rx_int_flag = true ;        
    }
  }
 
  if (rx_int_flag == true)
  {
    RX_buffer_shift();                                 // Rotate RX buffer
  
    // Wait for preamble
    if ((rx_status == 0) && (rx_int_flag == true)) // TODO is the rx_int_flag==true check really needed since the whole block is inside the same check?
    { 
      Check_for_preamble();
    }

    // TODO - make sure enough preamble bits are found, can't just find 32 and assume rest of 512 will follow
    // TODO - reset back to state zero if more than just a couple of bits in the preamble are incorrect after detecting the start of the preamble
    // TODO - figure out why alternate incoming streams work and the other alternate ones don't - why is the preamble full of blocks of non alternating bits
    
    // Wait for frame sync
    if ((rx_status == 1) && (rx_int_flag == true)) 
    {
      Check_for_frame_sync();
    }

    //Read radio header, descramble and de-interleave input
    if ((rx_status == 2) && (rx_int_flag == true))
    {  
      Read_radio_header();
      stream_status_modem = 0;
    }    
    
    // Convolutional decoding
    if ((rx_status == 3) && (rx_int_flag == true)) 
    {
      Convolutional_decode();
    }
    
    // Send decoded header to serial port
    if ((rx_status == 4) && (rx_int_flag == true))
    {   
      rx_status = 5;
    }
    
    // Shift frame sync out of buffer to clear space
    if ((rx_status == 5) && (rx_int_flag == true))
    {
      for (frame_counter = 1; frame_counter <= 16; frame_counter +=1) RX_buffer_shift(); 
      rx_status = 6;
      frame_counter = 0;   
      frame_counter_nr = 21;
      rx_int_flag = false;
    }
    
    // RX is up and running
    if (rx_status == 6 && rx_int_flag == true)
    {        
      frame_counter += 1;
        
       // Buffer correction, level control
       if ((frame_counter > 1) && (frame_counter < 25))     // 25 was 15
       {
         if (temp_shift_buffer_counter > radio_rx_buffer_counter)
         {    
           if (temp_shift_buffer_counter - radio_rx_buffer_counter > 20 )
           {
             for (int corr = 1; corr <= 32; corr++)   // 32 was 16
             {
               RX_buffer_shift();
               frame_counter += 1;
             }
           }
         }
         
        if (temp_shift_buffer_counter < radio_rx_buffer_counter)
        {    
          if ((temp_shift_buffer_counter + radio_rx_buf_len) - radio_rx_buffer_counter > 20 )
          {
            for (int corr = 1; corr <= 32; corr++)   // 32 was 16
            {
              RX_buffer_shift();
              frame_counter += 1;
            }
          }
        }
      }
   
      // Check termination flags
      if (frame_counter == 73)
      {
//        if (temp_shift_buffer[12] == B01010101
//         && temp_shift_buffer[11] == B01010101
//         && temp_shift_buffer[10] == B01010101 
//          && temp_shift_buffer[9] == B01010101 
//          && temp_shift_buffer[8] == B11001000 
//          && temp_shift_buffer[7] == B01111010)
        if ((pgm_read_byte(bytebits + (temp_shift_buffer[12] ^ B01010101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[11] ^ B01010101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[10] ^ B01010101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[9]  ^ B01010101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[8]  ^ B11001000))+
             pgm_read_byte(bytebits + (temp_shift_buffer[7]  ^ B01111010)))<2) // allow up to 1 bit mismatch
        {
          Set_RX();
          Send_EOT();
        }
      }
  
      // Check on gained bit      
      if (frame_counter_nr == 20 && frame_counter >=66)
      {      
//       if (temp_shift_buffer[11] == B01010101
//         && temp_shift_buffer[10] == B00101101 
//          && temp_shift_buffer[9] == B00010110)
       if ((pgm_read_byte(bytebits + (temp_shift_buffer[11] ^ B01010101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[10] ^ B00101101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[9]  ^ B00010110)))<2) // allow up to 1 bit mismatch
        {
          rx_int_flag = false;
          RX_buffer_shift();
        }
      }

// TODO - how to look for sync flag when state isn't zero - the case when the preamble and header is missed
// TODO - build all the header information from the interleaved data and then send the packet
    
      // Check sync flag
      if (frame_counter_nr == 20 && frame_counter ==73)
      { 
//        if (temp_shift_buffer[12] == B01010101
//           && temp_shift_buffer[11] == B00101101 
//           && temp_shift_buffer[10] == B00010110)
        if ((pgm_read_byte(bytebits + (temp_shift_buffer[12] ^ B01010101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[11] ^ B00101101))+
             pgm_read_byte(bytebits + (temp_shift_buffer[10] ^ B00010110)))<2) // allow up to 1 bit mismatch
        {
          // TODO - what should be done in here when the sync flag is found?
        }    
        // If 1 out of 3 is okay, accept and correct
        else if (temp_shift_buffer[12] == B01010101
              || temp_shift_buffer[11] == B00101101 
              || temp_shift_buffer[10] == B00010110)
        {
          temp_shift_buffer[12] = B01010101;
          temp_shift_buffer[11] = B00101101; 
          temp_shift_buffer[10] = B00010110;   
        }
          
        // Resync
        else
        {    
          Set_RX();            
          Send_EOT();
        }
      }
    
      // Send voice / data Packet to host
      if (frame_counter ==73)   
      {
        packet_counter = frame_counter_nr;
        Send_Packet_to_Host(); 
      }  
 
      // Count frames 
      if (frame_counter >= 96)
      {         
        frame_counter = 0;  
        frame_counter_nr += 1;      
        if (frame_counter_nr > 20)    
        {
          frame_counter_nr = 0;        
        }
      }
    }
  } // end if rx_int_flag == true

  rx_int_flag = false;    
  tx_int_flag = false; 
} // end main loop

#if defined(AD7021) || defined(ADF7021_eval)

//========================================== Send AD7021 byte ===================================================
void Send_AD7021_control()
{
  for(AD7021_counter = 31; AD7021_counter >= 0; AD7021_counter--)
  {
    if(bitRead(AD7021_control_byte, AD7021_counter) == HIGH)
    {
      digitalWrite(pinAD7021_SDATA, HIGH);   
    }  
    else
    {
      digitalWrite(pinAD7021_SDATA, LOW);
    }  
    
    digitalWrite(pinAD7021_SCLK, HIGH);
//    delayMicroseconds(5);
    digitalWrite(pinAD7021_SCLK, LOW);    
    // TODO - would code using INV help speed up the clock?
    //  INV(PORTB, 5); // fast invert pin example
  }
  
  digitalWrite(pinAD7021_SLE, HIGH);
  digitalWrite(pinAD7021_SLE, LOW);     
  digitalWrite(pinAD7021_SDATA, LOW);
}
#endif

//========================================== Set RX =============================================================
void Set_RX()
{
   #if defined(AD7021) || defined(ADF7021_eval)
      detachInterrupt(0);                      // Disable TX interrupt
      attachInterrupt(0, RX_589, RISING);      // Enable RX interrupt              
   #endif
   #ifdef GMSK_adapter
      detachInterrupt(1);                                // Disable TX interrupt
      attachInterrupt(tx_int_pin, RX_589, FALLING);      // Enable RX interrupt              
   #endif

      bitClear(status_flags_hi,0);             // RX on
      bitClear(status_flags_hi,1);             // TX off
         
   #if defined(AD7021) || defined(ADF7021_eval)
      ADF7021_rx();
      digitalWrite(pinLED, LOW);               // Set status LED off
   #endif
   #ifdef GMSK_adapter
      digitalWrite(pinPLLACQ, HIGH); 
      digitalWrite(pinPTT, LOW);               // Set TX off   
      digitalWrite(pinCOSLED, LOW);
   #endif

      radio_rx_buffer_counter = radio_rx_buf_len;
      temp_shift_buffer_counter = 75;
      input_rx_buffer_bit_counter = 0;
      temp_shift_buffer_bit_counter = 0;
      rx_status = 0;   
      frame_counter = 0;
      frame_counter_nr = 0;     
      status_total_packets_send = 0;
      status_unsent_frames = 0;
      stream_status_modem = 1;
      stream_status_host = 0;
      tx_status = 1;
      synctimeout = 0;
}

//========================================= Send EOT ============================================================
void Send_EOT()
{
    serial_out[2] = 0x03;
    serial_out[3] = 0x00;
    serial_out[4] = 0x1A;
    serial_out[5] = transmission_counter;   
    serial_out[6] = packet_counter;    

    serial_length = 6;
    Send_Serial();          

  #if defined(AD7021) || defined(ADF7021_eval)
    digitalWrite(pinLED, LOW); 
  #endif    
  #ifdef GMSK_adapter
    digitalWrite(pinPLLACQ, LOW); // not sure if this is needed here
    digitalWrite(pinCOSLED, LOW); 
  #endif
}  

//========================================= Send packet to host =================================================
void Send_Packet_to_Host()
{
    serial_out[2] = 0x13;
    serial_out[3] = 0x00;
    serial_out[4] = 0x19;
    serial_out[5] = transmission_counter;   
    serial_out[6] = packet_counter;    
    serial_out[7] = 0xE8;
    serial_out[8] = 0x03;   
    for (int d = 1; d <= 12; d++)
    {
      serial_out[8 + d] = temp_shift_buffer[22 - d];
    }   
    serial_out[21] = 0x00;
    serial_out[22] = 0x00; 

/*    
#if (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)) && defined(serial1Debug)      
    Serial1.print(radio_rx_buffer_counter); 
    Serial1.print("-"); 
    Serial1.print(temp_shift_buffer_counter); 
    Serial1.write(13);
    Serial1.write(10); 
#endif    
*/
    serial_length = 22;
    Send_Serial();        
    
}

//========================================= List of answers to host =============================================
void Answer_Host()
{ 
  if (serial_buffer[3] == 0x11)                      // Answer to get version
  {
    serial_out[2] = 0x11;
    serial_out[3] = 0x00;
    serial_out[4] = 0x91;
// TODO - what are these two bytes? are values needed for new version info?
    serial_out[5] = 0x22;          //2B
    serial_out[6] = 0x11;          //1.1
    serial_out[7] = 'D';
    serial_out[8] = 'V';
    serial_out[9] = '-';
    serial_out[10] = 'M';
    serial_out[11] = 'E';
    serial_out[12] = 'G';
    serial_out[13] = 'A';
    serial_out[14] = ' ';
    serial_out[15] = 'R';
    serial_out[16] = '1';
    serial_out[17] = '.';
    serial_out[18] = '1';
    serial_out[19] = '6';
    serial_out[20] = 'A';
  
    serial_length = 20;
    Send_Serial();   

/*
#if (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)) && defined(serial1Debug)      
      Serial1.print("Free Ram : ");                  
      Serial1.println(freeRam(),DEC);
      Serial1.write(13);
      Serial1.write(10);        
      Serial1.write(13);
      Serial1.write(10);        
      Serial1.write(13);
      Serial1.write(10);        

      Serial1.print("Get version");
      Serial1.write(13);
      Serial1.write(10);
#endif
*/

    stream_status_modem = 1;   // modem TX is idle 
    tx_status = 1;
    synctimeout = 0;
  }  
 
  if (serial_buffer[3] == 0x12)                      // Answer to get serial
  {
    serial_out[2] = 0x05;
    serial_out[3] = 0x00;
    serial_out[4] = 0x92;
    serial_out[5] = 0x13;
    serial_out[6] = 0x09;
    serial_out[7] = 0x10;
    serial_out[8] = 0x01;   
    serial_length = 8;
    Send_Serial();  
/*
#if (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)) && defined(serial1Debug)            
    Serial1.print("Get serial");
    Serial1.write(13);
    Serial1.write(10);
#endif      
*/
  }
  
  if (serial_buffer[3] == 0x14)                    // Answer to set config -> ACK
  {
    if (serial_buffer[4] == 0xC0) 
    {
      config_flags = serial_buffer[6];
      config_modulation = serial_buffer[7];
      config_TX_delay_1 = serial_buffer[8];
      config_TX_delay_2 = serial_buffer[9];   

   #if defined(AD7021) || defined(ADF7021_eval)
      modem_type = false;
      rx_inverse = false;
      tx_inverse = false;
      band = true;    
   #endif
   #ifdef GMSK_adapter
      if(bitRead(config_flags,0) == HIGH) rx_inverse = true; else rx_inverse = false;
      if(bitRead(config_flags,1) == HIGH) tx_inverse = true; else tx_inverse = false;   
      if(bitRead(config_flags,2) == HIGH) modem_type = false; else modem_type = true; 
      if(bitRead(config_flags,2) == HIGH) tx_int_pin = 0; else tx_int_pin = 1;
   #endif
    }
 
    if (serial_buffer[4] == 0xC1) 
     { 
   #if defined(AD7021) || defined(ADF7021_eval)
       CALC_REG0();
       ADF7021_ini();                        // Initiate ADF7021  
       attachInterrupt(0, RX_589, RISING);  // Attach RX interrupt
   #endif
   #ifdef GMSK_adapter
       attachInterrupt(tx_int_pin, RX_589, FALLING);  // Attach RX interrupt
   #endif
     }
     
    serial_out[2] = 0x02;
    serial_out[3] = 0x00;
    serial_out[4] = 0x94;
    serial_out[5] = 0x06; 
    
    serial_length = 5;
    Send_Serial();   
  }  
  
  if ((serial_buffer[3] == 0x10) && (serial_buffer[1] == 0x02))  // Answer to set status -> ACK
  {  
    config_status = serial_buffer[4];  
    serial_out[2] = 0x02;
    serial_out[3] = 0x00;
    serial_out[4] = 0x90;
    serial_out[5] = 0x06; 
    
    serial_length = 5;
    Send_Serial();   
  }
  
  if ((serial_buffer[3] == 0x10) && (serial_buffer[1] == 0x01))  // Answer to get status -> status flags 
  {  
    serial_out[2] = 0x08;
    serial_out[3] = 0x00;
    serial_out[4] = 0x90;
    serial_out[5] = 0x47; // status_flags_lo
    serial_out[6] = status_flags_hi;
    serial_out[7] = tx_status;
    serial_out[8] = 0x15;
    serial_out[9] = 0xFC;   
    serial_out[10] = status_unsent_frames;
    serial_out[11] = status_total_packets_send;

    serial_length = 11;
    Send_Serial();      
  }  
  
  // Start preamble
  if (serial_buffer[3] == 0x16)                   
  { 
    if(tx_inverse == true) digitalWrite(pinTXD, HIGH); else digitalWrite(pinTXD, LOW);        
    stream_status_host = 1;               // Send Preamble    
  }  
  
  // Read header from host
  if (serial_buffer[3] == 0x17)                   
  {
    transmission_counter = serial_buffer[4];
    packet_counter = serial_buffer[5];
    
    for(int rvp = 1; rvp <= 41; rvp++)      // Read uncoded header from host
    {
      decoded_header[rvp] = serial_buffer[rvp + 7];
    }  
                                          
    radio_tx_buffer_in_counter = 86;
    radio_tx_buffer_out_counter = 1;    
   
    Convolutional_encode();  
    
    Scramble_header();
    stream_status_host = 2;  
  }  
  
  // Read voive packet from host
  if (serial_buffer[3] == 0x19)                   
  { 
    time_out_timer = 0;   
    frame_counter_nr += 1;
    if(frame_counter_nr > 20) frame_counter_nr = 0;
    for(int rvp = 1; rvp <= 12; rvp++)
    {
/*
#if (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)) && defined(serial1Debug)       
      Serial1.write(serial_buffer[rvp + 7]);
#endif
*/
      radio_tx_buffer[radio_tx_buffer_in_counter] = serial_buffer[rvp + 7];
      radio_tx_buffer_in_counter += 1;
      if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1; 
    }  

    if(stream_status_host == 2) stream_status_host = 3;   
  }

  if (serial_buffer[3] == 0x1A)                   
  {  
    radio_tx_buffer_in_counter -= 1;
    if(radio_tx_buffer_in_counter == 0) radio_tx_buffer_in_counter = radio_tx_buf_len;
    radio_tx_buffer_in_counter -= 1;
    if(radio_tx_buffer_in_counter == 0) radio_tx_buffer_in_counter = radio_tx_buf_len;
    radio_tx_buffer_in_counter -= 1;
    if(radio_tx_buffer_in_counter == 0) radio_tx_buffer_in_counter = radio_tx_buf_len;

    radio_tx_buffer[radio_tx_buffer_in_counter] = B01010101;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
    radio_tx_buffer[radio_tx_buffer_in_counter] = B01010101;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
    radio_tx_buffer[radio_tx_buffer_in_counter] = B01010101;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
    radio_tx_buffer[radio_tx_buffer_in_counter] = B01010101;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
    
    radio_tx_buffer[radio_tx_buffer_in_counter] = B11001000;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
    radio_tx_buffer[radio_tx_buffer_in_counter] = B01111010;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
        
    radio_tx_buffer[radio_tx_buffer_in_counter] = B00000000;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
    radio_tx_buffer[radio_tx_buffer_in_counter] = B00000000;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;
    radio_tx_buffer[radio_tx_buffer_in_counter] = B00000000;
    radio_tx_buffer_in_counter += 1;
    if(radio_tx_buffer_in_counter > radio_tx_buf_len) radio_tx_buffer_in_counter = 1;   
    
    stream_status_host = 4;                        // Send stop word 
   }  
 
   if (serial_buffer[3] == 0x1F)      
     {
     if (serial_buffer[4] == 0xC0) 
     {
       serial_out[2] = 0x06;
       serial_out[3] = 0x00;
       serial_out[4] = 0x9F;
       serial_out[5] = 0xC0;
       serial_out[6] = 0x00;
       serial_out[7] = 0x00;
       serial_out[8] = 0x00;
       serial_out[9] = 0x00;       
       serial_length = 9;
       Send_Serial();       
     }
      
     if (serial_buffer[4] == 0xC1) 
     {
       serial_out[2] = 0x06;
       serial_out[3] = 0x00;
       serial_out[4] = 0x9F;
       serial_out[5] = 0xC1;
       serial_out[6] = 0x00;
       serial_out[7] = 0x00;
       serial_out[8] = 0x00;
       serial_out[9] = 0x00;       
       serial_length = 9;
       Send_Serial();       
     }    
   }  
 
  serial_buffer[0] = 0x00;
  serial_buffer[1] = 0x00;
  serial_buffer_input_pointer = 0;   
}

//========================================= Send message to host ================================================
void Send_Serial()
{
    serial_out[1] = 0xD0;
    CALC_CRC();
    for (int i = 1; i <= serial_length; i++)  
   {
     Serial.write (serial_out[i]);
   } 
}

//========================================= Calculate CRC =======================================================
void CALC_CRC()
{
    crc = 0x0000;
    for (int i = 1; i <= serial_length; i++)
    { 
      crc = (crc << 8 ) ^ (pgm_read_word(crc_table + ((crc >> 8) ^ serial_out[i])));
    }                  
    
    serial_out[serial_length + 2] = (crc & 0xFF);
    serial_out[serial_length + 1] = ((crc >> 8) & 0xFF);
    serial_length += 2;
}  

//========================================= Scramble header =====================================================
void Scramble_header()
{
    header_table_counter_x= 0;
    header_table_counter_y = 0;  
    descramble_poly = B11111111;      
    header_table_bit_counter = 0;  
    
    radio_tx_buffer[1] = B11101010;
    radio_tx_buffer[2] = B10100110;  
    radio_tx_buffer[3] = B00000000;
    
    radio_tx_buffer_bit_counter = 4;
    radio_tx_buffer_gen_counter = 3;
    
    byte bl;
    
    for(bl = 0; bl <= 7; bl++)
    {
      for(header_table_bit_counter = 0; header_table_bit_counter <= 27; header_table_bit_counter++)
      {
        if(bitRead(header_table[header_table_bit_counter][0], bl) == HIGH) 
        {
          bitSet(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter);
        }
        else
        {
          bitClear(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter); 
        }
 
        radio_tx_buffer_bit_counter += 1;
        if(radio_tx_buffer_bit_counter > 7) 
        {
          radio_tx_buffer_bit_counter = 0;
          radio_tx_buffer_gen_counter += 1;
        }     
      }  
    }
    
    for(bl = 0; bl <= 3; bl++)
    {
      for(header_table_bit_counter = 0; header_table_bit_counter <= 27; header_table_bit_counter++)
      {
        if(bitRead(header_table[header_table_bit_counter][1], bl) == HIGH) 
        {
          bitSet(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter);
        }
        else
        {
          bitClear(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter); 
        }
  
        radio_tx_buffer_bit_counter += 1;
        if(radio_tx_buffer_bit_counter > 7) 
        {
          radio_tx_buffer_bit_counter = 0;
          radio_tx_buffer_gen_counter += 1;
        }     
      }  
    }   
    
    for(bl = 4; bl <= 7; bl++)
    {
      for(header_table_bit_counter = 0; header_table_bit_counter <= 26; header_table_bit_counter++)
      {
        if(bitRead(header_table[header_table_bit_counter][1], bl) == HIGH) 
        {
          bitSet(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter);
        }
        else
        {
          bitClear(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter); 
        }
   
        radio_tx_buffer_bit_counter += 1;
        if(radio_tx_buffer_bit_counter > 7) 
        {
          radio_tx_buffer_bit_counter = 0;
          radio_tx_buffer_gen_counter += 1;
        }     
      }  
    }     
  
    for(bl = 0; bl <= 7; bl++)
    {
      for(header_table_bit_counter = 0; header_table_bit_counter <= 26; header_table_bit_counter++)
      {
        if(bitRead(header_table[header_table_bit_counter][2], bl) == HIGH) 
        {
          bitSet(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter);
        }
        else
        {
          bitClear(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter); 
        }
   
        radio_tx_buffer_bit_counter += 1;
        if(radio_tx_buffer_bit_counter > 7) 
        {
          radio_tx_buffer_bit_counter = 0;
          radio_tx_buffer_gen_counter += 1;
        }     
      }  
    }       
  
    radio_tx_buffer_bit_counter = 4;
    radio_tx_buffer_gen_counter = 3; 
    for(header_table_bit_counter = 1; header_table_bit_counter <= 660; header_table_bit_counter++)
    {
       descramble_poly = descramble_poly << 1;
       bitWrite(descramble_poly,0,((bitRead(descramble_poly, 4)) ^ (bitRead(descramble_poly, 7))));  
       bitWrite(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter,((bitRead(descramble_poly, 0)) ^ (bitRead(radio_tx_buffer[radio_tx_buffer_gen_counter],radio_tx_buffer_bit_counter))));    
       radio_tx_buffer_bit_counter += 1;
       if (radio_tx_buffer_bit_counter > 7)
       {
         radio_tx_buffer_bit_counter = 0;
         radio_tx_buffer_gen_counter += 1;                
       }
    }   
}  

//========================================= Convolutional encoding ==============================================
void Convolutional_encode()
{
    header_table_counter_x= 0;
    header_table_counter_y = 0;  
    descramble_poly = B00000000;    
    header_table_bit_counter = 0;
    byte uhbc = 0;
 
    for (decoded_header_counter = 1; decoded_header_counter <= 41; decoded_header_counter++)
    {
      for (uhbc = 0; uhbc <= 7; uhbc++)  
      {
        descramble_poly = descramble_poly << 1;       
        if (bitRead(decoded_header[decoded_header_counter],uhbc) == HIGH) bitSet(descramble_poly,0); else bitClear(descramble_poly,0);
        CONVOL_ENCODE();       
        if(header_table_bit_counter > 7)
        {
          header_table_bit_counter = 0;  
          header_table_counter_y += 1;
          if(header_table_counter_y > 2)
          {
            header_table_counter_y = 0;
            header_table_counter_x += 1;
          }  
        }  
      } 
    }  
    
    // Add 2 extra 0's
    descramble_poly = descramble_poly << 1;  
    bitClear(descramble_poly,0);
    CONVOL_ENCODE();    
   
    descramble_poly = descramble_poly << 1;    
    bitClear(descramble_poly,0);
    CONVOL_ENCODE();   
}  

//========================================= Encode routine ======================================================
void CONVOL_ENCODE()
{
     bitWrite(header_table[header_table_counter_x][header_table_counter_y], header_table_bit_counter,(bitRead(descramble_poly,0) ^ bitRead(descramble_poly,1) ^ bitRead(descramble_poly,2) ) );    
     header_table_bit_counter += 1;    
     bitWrite(header_table[header_table_counter_x][header_table_counter_y], header_table_bit_counter,(bitRead(descramble_poly,0) ^ bitRead(descramble_poly,2) ) ); 
     header_table_bit_counter += 1;      
}  

//========================================= Convolutional decoding ==============================================
void Convolutional_decode()
{
    header_table_counter_x= 0;
    header_table_counter_y = 0;
    descramble_poly = B00000000;  
 
    for (decoded_header_counter = 0; decoded_header_counter <= 40; decoded_header_counter++)
    {
      decoded_header_byte = B00000000;      
 
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],6) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],4) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],2) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],0) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();   
   
      header_table_counter_y += 1;
      if (header_table_counter_y > 2)
      {
        header_table_counter_y = 0;
        header_table_counter_x += 1;
      } 
    
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],6) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],4) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],2) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();
      if (bitRead(header_table[header_table_counter_x][header_table_counter_y],0) == HIGH) bitSet(descramble_poly, 7); else bitClear(descramble_poly, 7);
      CONVOL_DECODE();   
    
      header_table_counter_y += 1;
      if (header_table_counter_y > 2)
      {
        header_table_counter_y = 0;
        header_table_counter_x += 1;
      }  
      decoded_header[decoded_header_counter] = decoded_header_byte;
      }  
      rx_status = 4; 
  
      // Send decoded header
      serial_out[2] = 0x2F;
      serial_out[3] = 0x00;
      serial_out[4] = 0x17;
      serial_out[5] = transmission_counter; 
      serial_out[6] = packet_counter;
      serial_out[7] = 0x00;        
      serial_out[8] = 0x00;
      for (decoded_header_counter = 0; decoded_header_counter <= 40; decoded_header_counter++)
      {
        serial_out[9 + decoded_header_counter] = decoded_header[decoded_header_counter];
      }
              
      serial_length = 50;
      Send_Serial();     
}  

//========================================= Read radio header ===================================================
void Read_radio_header()
{
    if (bitRead(descramble_poly, 3) == HIGH && bitRead(descramble_poly, 6) == HIGH)
    {
      descramble_poly = descramble_poly << 1;
      bitClear(descramble_poly, 0);
    }    
    else if (bitRead(descramble_poly, 3) == LOW && bitRead(descramble_poly, 6) == LOW)
    {
      descramble_poly = descramble_poly << 1;
      bitClear(descramble_poly, 0);
    }  
    else
    {
      descramble_poly = descramble_poly << 1;
      bitSet(descramble_poly, 0);   
    }  
    
    if (bitRead(temp_shift_buffer[10], 0) == HIGH && bitRead(descramble_poly, 0) == HIGH)
    {
      bitClear(header_table[header_table_counter_x][header_table_counter_y], header_table_bit_counter);
    }      
    else if (bitRead(temp_shift_buffer[10], 0) == LOW && bitRead(descramble_poly, 0) == LOW)    
    {
      bitClear(header_table[header_table_counter_x][header_table_counter_y], header_table_bit_counter);
    }  
    else
    {
      bitSet(header_table[header_table_counter_x][header_table_counter_y], header_table_bit_counter);
    }  
    
    header_table_counter_x += 1;
    
    if (header_table_counter_y == 0 && header_table_counter_x > 27) 
    {
      header_table_counter_x = 0;
      header_table_bit_counter -= 1;
      if (header_table_bit_counter <0)
      {
        header_table_bit_counter = 7;
        header_table_counter_y += 1; 
      }
    }
   
    if (header_table_counter_y == 1 && header_table_counter_x > 27) 
    {
      header_table_counter_x = 0;
      header_table_bit_counter -= 1;
    }  
 
     if (header_table_counter_y == 1 && header_table_counter_x == 27 && header_table_bit_counter < 4) 
    {
      header_table_counter_x = 0;
      header_table_bit_counter -= 1;
    }    
     if (header_table_bit_counter <0)
      {
        header_table_bit_counter = 7;
        header_table_counter_y += 1; 
      }        
   
    if (header_table_counter_y == 2 && header_table_counter_x > 26) 
    {
      header_table_counter_x = 0;
      header_table_bit_counter -= 1;
      if (header_table_bit_counter <0)
      {
        rx_status =3; 
      }
    } 
}


//========================================= Wait for frame sync =================================================
void Check_for_frame_sync()
{
// unrolled method of counting number of unmatched bits when looking for frame sync
//   if ((pgm_read_byte(bytebits + (temp_shift_buffer[14] ^ B10101010))+
//        pgm_read_byte(bytebits + (temp_shift_buffer[13] ^ B10101010))+
//        pgm_read_byte(bytebits + (temp_shift_buffer[12] ^ B01101110))+
//        pgm_read_byte(bytebits + (temp_shift_buffer[11] ^ B00001010)))<3) // allow up to 2 bit mismatch
// original method of looking for frame sync      
  if ( temp_shift_buffer[14] == B10101010
    && temp_shift_buffer[13] == B10101010
    && temp_shift_buffer[12] == B01101110
    && temp_shift_buffer[11] == B00001010)
    {
      rx_status = 2;
      header_table_counter_y = 0;
      header_table_counter_x = 0;
      header_table_bit_counter = 7;
      descramble_poly = B11111111;
    
      // Send syncword detected
      packet_counter =0;
      transmission_counter +=1;
            
      serial_out[2] = 0x03;
      serial_out[3] = 0x00;
      serial_out[4] = 0x15;
      serial_out[5] = 0x00; 
      serial_out[6] = 0x00;

      serial_length = 6;
      Send_Serial();         
      
      serial_out[2] = 0x03;
      serial_out[3] = 0x00;
      serial_out[4] = 0x16;
      serial_out[5] = transmission_counter; 
      serial_out[6] = packet_counter;

      serial_length = 6;
      Send_Serial();     
    }
    else {
    // TODO - need some kind of time out if we can't find the frame sync
    //        perhaps a 512 count? otherwise it gets stuck here for ever  
      if (synctimeout++ == 1024) {
  #if defined(AD7021) || defined(ADF7021_eval)
        digitalWrite(pinLED, LOW);
  #endif        
  #ifdef GMSK_adapter
        digitalWrite(pinPLLACQ, HIGH);
        digitalWrite(pinCOSLED, LOW);
  #endif
        rx_status = 0;
      }
    }
}

//========================================= Wait for Preamble ===================================================
void Check_for_preamble()
{
//  if ( temp_shift_buffer[14] == B10101010
//    && temp_shift_buffer[13] == B10101010
//    && temp_shift_buffer[12] == B10101010
//    && temp_shift_buffer[11] == B10101010)
    if ((pgm_read_byte(bytebits + (temp_shift_buffer[14] ^ B10101010))+
         pgm_read_byte(bytebits + (temp_shift_buffer[13] ^ B10101010))+
         pgm_read_byte(bytebits + (temp_shift_buffer[12] ^ B10101010))+
         pgm_read_byte(bytebits + (temp_shift_buffer[11] ^ B10101010)))<3) // allow up to 2 bits mismatch
    {            
      rx_status = 1;
      bitSet(status_flags_hi, 0);        // Set receiving status   

   #if defined(AD7021) || defined(ADF7021_eval)
      digitalWrite(pinLED, HIGH);
   #endif
   #ifdef GMSK_adapter
      digitalWrite(pinPLLACQ, LOW);  
      digitalWrite(pinCOSLED, HIGH);
   #endif

      synctimeout = 0;
    }  
}

//========================================= Convolutional decoder ===============================================
void CONVOL_DECODE()
{
  if (bitRead(descramble_poly, 7) == HIGH && bitRead(descramble_poly, 2) == HIGH)
  {
    bitClear(descramble_poly, 0);
    decoded_header_byte = decoded_header_byte >> 1;
    bitClear(decoded_header_byte, 7);
    descramble_poly = descramble_poly << 1;   
  }
  else if (bitRead(descramble_poly, 7) == LOW && bitRead(descramble_poly, 2) == LOW)
  {
    bitClear(descramble_poly, 0);
    decoded_header_byte = decoded_header_byte >> 1;
    bitClear(decoded_header_byte, 7);
    descramble_poly = descramble_poly << 1;  
  }
  else
  {
    bitSet(descramble_poly, 0);
    decoded_header_byte = decoded_header_byte >> 1;
    bitSet(decoded_header_byte, 7);  
    descramble_poly = descramble_poly << 1;
  }    
}  

//============================ Rotate temp_shift_buffer ===========================================================
void RX_buffer_shift()
{
    temp_shift_buffer_bit_counter += 1;                                         // Increment curent possition
    
    if (temp_shift_buffer_bit_counter > 7)                                      // Check if byte is full
    {
      temp_shift_buffer_bit_counter = 0;                                        // If full clear bit counter
      temp_shift_buffer[0] = radio_tx_buffer[temp_shift_buffer_counter];   //!       // Read byte from ringbuffer
      temp_shift_buffer_counter -= 1;                                           // Relocate ringbuffer
      if (temp_shift_buffer_counter == 0) temp_shift_buffer_counter = radio_rx_buf_len;        // Reloop ringbuffer
    }

    // Place byte in front of RX shift buffer
    for (int RX_buffer_shift_counter = 22; RX_buffer_shift_counter >= 0; RX_buffer_shift_counter--)
    {
      temp_shift_buffer[RX_buffer_shift_counter] = temp_shift_buffer[RX_buffer_shift_counter] >> 1;
      bitWrite(temp_shift_buffer[RX_buffer_shift_counter], 7, bitRead(temp_shift_buffer[RX_buffer_shift_counter-1],0)); 
    }      
}  

//========================================= Interrupt routine TX clk from CMX589 =================================
void TX_589()
{
  time_out_timer += 1;  
  
    TX_delay_counter -= 1;
  
   // Send Preamble
   if(stream_status_modem == 4 || stream_status_modem == 5)              
   {        
     status_tx_bit_counter = 0;
     preamble_counter -= 1;

     if(digitalRead(pinTXD) == HIGH)
     {
       digitalWrite(pinTXD, LOW);
     }
     else
     {
       digitalWrite(pinTXD, HIGH);
     }    
    
    // Stop preamble, stops with 1, last 01010 is added to begin of framesync to get a full byte
    if(stream_status_modem ==5)
    {
      if(tx_inverse == false)
      {
        if(digitalRead(pinTXD) == HIGH) stream_status_modem = 6;
      }  
      else
      {
        if(digitalRead(pinTXD) == LOW) stream_status_modem = 6;
      }
    }
  }
  
    // Send Voice / Data
  if(stream_status_modem == 7 || stream_status_modem == 8)
  {
    status_tx_bit_counter += 1;
    if (status_tx_bit_counter >= 96)
    {
/*
#if (defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)) && defined(serial1Debug)          
      Serial1.print(status_unsent_frames);       
      Serial1.write(13);
      Serial1.write(10);  
#endif             
*/
      status_tx_bit_counter = 0;
      status_total_packets_send += 1;
      if(status_total_packets_send > 255) status_total_packets_send = 0;
    }
   
    if(bitRead(radio_tx_buffer[radio_tx_buffer_out_counter],radio_tx_buffer_bit_counter) == HIGH)
    {
      if(tx_inverse == true)
      {
        digitalWrite(pinTXD, LOW);
      }
      else
      {
        digitalWrite(pinTXD, HIGH);
      }      
    } 
    else
    {
      if(tx_inverse == true)
      {
        digitalWrite(pinTXD, HIGH);
      }
      else
      {
        digitalWrite(pinTXD, LOW);        
      }
    }
    
    radio_tx_buffer_bit_counter += 1;
    
    if (radio_tx_buffer_bit_counter == 8)
    {
      radio_tx_buffer_bit_counter = 0;
      radio_tx_buffer_out_counter += 1;
      if (radio_tx_buffer_out_counter > radio_tx_buf_len)
      {
        radio_tx_buffer_out_counter = 1;
      }
    }

    if(stream_status_modem == 8)
    {
      if(radio_tx_buffer_in_counter == radio_tx_buffer_out_counter) stream_status_modem = 9;  
    }
  }
    
  tx_int_flag = true;
}  

//========================================= Interrupt routine RX clk from CMX589 =================================
void RX_589()
{
  time_out_timer += 1;
        
  if (rx_inverse == false)                                              // Check data polarity
  {
    bitWrite(input_rx_buffer, 7, digitalRead(pinRXD));                    // Read RX data pin normal
  }
  else
  {
    bitWrite(input_rx_buffer, 7, !digitalRead(pinRXD));                   // Read RX data pin inverse
  }  
  input_rx_buffer_bit_counter += 1;                                     // Increment bit counter
  
  if (input_rx_buffer_bit_counter == 8 )                                // If byte is full copy byte to ringbuffer
  {
    radio_tx_buffer[radio_rx_buffer_counter] = input_rx_buffer;    //!     // Copy byte to ringbuffer
    radio_rx_buffer_counter -= 1;                                       // Rotate ringbuffer
    if (radio_rx_buffer_counter == 0) radio_rx_buffer_counter = radio_rx_buf_len;    // Loop buffer around if needed
    input_rx_buffer_bit_counter = 0;                                    //
    }  
  else
    {                                                                   // If byte is not full
    input_rx_buffer = input_rx_buffer >> 1;                             // Rotate input buffer 
  }
    
  rx_int_flag = true;                                                   // Set rx_int_flag for general purposes.
}  

#if defined(AD7021) || defined(ADF7021_eval)

//==================================================================================================================
void ADF7021_ini()
{
// TODO put both sets of register settings here proper 14.7456MHz crystal and for eval crystal
  #ifdef ADF7021_eval
  #endif
  #ifdef AD7021
  #endif
  
    AD7021_control_byte = 0x00575011; // 1
    Send_AD7021_control();

    AD7021_control_byte = 0x281500A3; // 3 RX rx werkend test commen
    Send_AD7021_control();

    AD7021_control_byte = ADF7021_RX_REG0;    
    Send_AD7021_control();    
    //AD7021_control_byte = 0x19614080; // 0
    //Send_AD7021_control();

    AD7021_control_byte = 0x00003155; // 5
    Send_AD7021_control();
 
    AD7021_control_byte = 0x0037B014; // 4  test common
    Send_AD7021_control(); 

    AD7021_control_byte = 0x0115D092; // 2 test common
    Send_AD7021_control();

    AD7021_control_byte = 0x000E000F; // 15
    Send_AD7021_control();  

    AD7021_control_byte = 0x050972C6; // 6
    Send_AD7021_control();

    AD7021_control_byte = 0x000231E9; // 9
    Send_AD7021_control();

    AD7021_control_byte = 0x3296355A; // 10
    Send_AD7021_control();

    AD7021_control_byte = 0x0000003B; // 11
    Send_AD7021_control();

    AD7021_control_byte = 0x0000010C; // 12
    Send_AD7021_control();

    AD7021_control_byte = 0x0000000D; // 13
    Send_AD7021_control();       
}  

//======================================================================================================================
void ADF7021_tx()
{
// TODO put both sets of register settings here proper 14.7456MHz crystal and for eval crystal
  #ifdef ADF7021_eval
  #endif
  #ifdef AD7021
  #endif

    AD7021_control_byte = 0x0027F092; // 2 tx
    Send_AD7021_control();
 
    AD7021_control_byte = 0x2B148123; // 3 tx
    Send_AD7021_control();
      
    AD7021_control_byte = 0x00593814; // 4 tx
    Send_AD7021_control();     
      
    //AD7021_control_byte = 0x11615550; // Set tx
    //Send_AD7021_control();     
       
    AD7021_control_byte = ADF7021_TX_REG0;         
    Send_AD7021_control();         
}

//======================================================================================================================
void ADF7021_rx()
{
// TODO put both sets of register settings here proper 14.7456MHz crystal and for eval crystal
  #ifdef ADF7021_eval
  #endif
  #ifdef AD7021
  #endif

    AD7021_control_byte = 0x00003155; // 5
    Send_AD7021_control();
    
    AD7021_control_byte = 0x0115D092; // 2 RX
    Send_AD7021_control();    
  
    AD7021_control_byte = 0x2B1500A3; // 3 RX
    Send_AD7021_control();    
    
    AD7021_control_byte = 0x0037B014; // 4 RX
    Send_AD7021_control();    
   
    //AD7021_control_byte = 0x19614080; // set rx
    //Send_AD7021_control();      
    AD7021_control_byte = ADF7021_RX_REG0;   
    Send_AD7021_control();      
}

//=========================================================================================================
void CALC_REG0()
{
// TODO put both sets of register settings here proper 14.7456MHz crystal and for eval crystal
  #ifdef ADF7021_eval
  #endif
  #ifdef AD7021
  #endif

    // RX
    temp_value = serial_buffer[7];
    frequency = temp_value * 1;
   
    temp_value = serial_buffer[8];
    frequency += temp_value * 256;
   
    temp_value = serial_buffer[9];
    frequency += temp_value * 65536;
 
    temp_value = serial_buffer[10];
    frequency += temp_value * 16777216;
     
    if(frequency < 200000000) band = false; else band = true;
  
    if(band == false)
    {    
      devider = (frequency - 100000) / (19680000.0 / 4.0);
    }
    else
    {    
      devider = (frequency - 100000) / (19680000.0 / 2.0);
    }
    
    N_devider = floor(devider);
    devider = (devider - N_devider) * 32768;
    F_devider = floor(devider + 0.5);
    
    ADF7021_RX_REG0 = 0x00000003;
    ADF7021_RX_REG0 = ADF7021_RX_REG0 << 8;
        
    //frequency = N_devider;
    ADF7021_RX_REG0 = ADF7021_RX_REG0 | N_devider;  //frequency;
    ADF7021_RX_REG0 = ADF7021_RX_REG0 << 15;
 
    //frequency = F_devider;
    ADF7021_RX_REG0 = ADF7021_RX_REG0 | F_devider; //frequency;
    ADF7021_RX_REG0 = ADF7021_RX_REG0 << 4;    
  
    // TX
    if(band == false)
    {    
     devider = frequency / (19680000.0 / 4.0);
    }
    else
    {    
     devider = frequency / (19680000.0 / 2.0);
    }
    
    N_devider = floor(devider);
    devider = (devider - N_devider) * 32768;
    F_devider = floor(devider + 0.5);
    
    ADF7021_TX_REG0 = 0x00000002;
    ADF7021_TX_REG0 = ADF7021_TX_REG0 << 8;
        
    //frequency = N_devider;
    ADF7021_TX_REG0 = ADF7021_TX_REG0 | N_devider ; //frequency;
    ADF7021_TX_REG0 = ADF7021_TX_REG0 << 15;
 
    //frequency = F_devider;
    ADF7021_TX_REG0 = ADF7021_TX_REG0 | F_devider; //frequency;
    ADF7021_TX_REG0 = ADF7021_TX_REG0 << 4;    
}  

//===============================================
void Set_ADF7021_UHF()
{
    serial_buffer[7] = 0x40;
    serial_buffer[8] = 0x78;
    serial_buffer[9] = 0xE7;
    serial_buffer[10] = 0x19;  
    band = true;
  
    CALC_REG0();
    ADF7021_ini();  
  
    detachInterrupt(0);
    detachInterrupt(1);    
   
    attachInterrupt(0, RX_589, RISING);  // Attach RX interrupt
}  

#endif


