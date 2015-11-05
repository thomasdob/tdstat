#include "i2c_software.h"

void StartI2C()
{
  CLOCK_HI;                    // release clock pin to float high
  DATA_LAT = 0;                // set data pin latch to 0
  DATA_LOW;                    // set pin to output to drive low
  _delay(100);
}

void StopI2C()
{
  SCLK_LAT = 0;                // set clock pin latch to 0
  CLOCK_LOW;                   // set clock pin to output to drive low
  DATA_LAT = 0;                // set data pin latch to 0
  DATA_LOW;                    // set data pin output to drive low
  _delay(100);                 // user may need to modify based on Fosc
  CLOCK_HI;                    // release clock pin to float high
  _delay(100);                 // user may need to modify based on Fosc
  DATA_HI;                     // release data pin to float high
  _delay(100);                 // user may need to modify based on Fosc
}

uint8_t ReadI2C()
{
  uint8_t I2C_BUFFER = 0;      // data to be read in
  uint8_t BIT_COUNTER = 8;     // set bit count for byte 
  SCLK_LAT = 0;                // set clock pin latch to 0
  DATA_HI;                     // release data line to float high
  do
  {
    CLOCK_LOW;                 // set clock pin output to drive low
    _delay(100);               // user may need to modify based on Fosc
    CLOCK_HI;                  // release clock line to float high
    _delay(100);               // user may need to modify based on Fosc

    I2C_BUFFER <<= 1;          // shift composed byte by 1
    I2C_BUFFER &= 0xFE;        // clear bit 0

    if ( DATA_PIN )            // is data line high
     I2C_BUFFER |= 0x01;       // set bit 0 to logic 1
   
  } while ( --BIT_COUNTER );   // stay until 8 bits have been acquired
  CLOCK_LOW;                   //------------------*
  DATA_LAT = 0;                //                  *
  DATA_LOW;                    //   Generate Ack   *
  _delay(100);                 //                  *
  CLOCK_HI;                    //                  *
  _delay(100);                 //------------------*
  return I2C_BUFFER ;          // return with data
}

uint8_t ReadLastI2C()
{
  uint8_t I2C_BUFFER = 0;      // data to be read in
  uint8_t BIT_COUNTER = 8;     // set bit count for byte 
  SCLK_LAT = 0;                // set clock pin latch to 0
  DATA_HI;                     // release data line to float high
  do
  {
    CLOCK_LOW;                 // set clock pin output to drive low
    _delay(100);               // user may need to modify based on Fosc
    CLOCK_HI;                  // release clock line to float high
    _delay(100);               // user may need to modify based on Fosc

    I2C_BUFFER <<= 1;          // shift composed byte by 1
    I2C_BUFFER &= 0xFE;        // clear bit 0

    if ( DATA_PIN )            // is data line high
     I2C_BUFFER |= 0x01;       // set bit 0 to logic 1
   
  } while ( --BIT_COUNTER );   // stay until 8 bits have been acquired
  CLOCK_LOW;                   //------------------*
  _delay(100);                 //   Generate Nack  *
  CLOCK_HI;                    //                  *
  _delay(100);                 //------------------*
  return I2C_BUFFER ;          // return with data
}

void WriteI2C(uint8_t data_out)
{
  uint8_t BIT_COUNTER = 8;        // initialize bit counter
  uint8_t I2C_BUFFER = data_out;  // data to be sent out
  SCLK_LAT = 0;                   // set clock pin latch to 0
  do
  {
	  CLOCK_LOW;                  // set clock pin output to drive low
	  if ( I2C_BUFFER & 0x80 )    // if MSB set, transmit out logic 1
	  {
		DATA_HI;                  // release data line to float high 
	  }
	  else                        // transmit out logic 0
	  {
		DATA_LAT = 0;             // set data pin latch to 0
		DATA_LOW;                 // set data pin output to drive low 
	  }
	  _delay(100);              // user may need to modify based on Fosc
	  CLOCK_HI;                 // release clock line to float high 
	  _delay(100);              // user may need to modify based on Fosc

	  I2C_BUFFER <<= 1;
	  BIT_COUNTER --;           // reduce bit counter by 1
  } while (BIT_COUNTER);        // stay in transmit loop until byte sent
  CLOCK_LOW;                      // 
  _delay(100);                    //
  CLOCK_HI;                       // waits for Ack from device
  _delay(100);                    //
}
