#include "i2c_hardware.h"

#define TIMEOUT 65535

void InitializeI2C()
{ 
  // a little bit of voodoo
  TRISBbits.TRISB4 = 0;
  TRISBbits.TRISB6 = 0;
  LATBbits.LATB4 = 0;
  LATBbits.LATB6 = 0;
  
  TRISBbits.TRISB4 = 1;   // Set SDA (PORTB,4) pin to input
  TRISBbits.TRISB6 = 1;   // Set SCL (PORTB,6) pin to input
  
  SSPCON = 0x28;          // SSPEN=1, SSP1M<3:0> = binary 1000
  SSPCON3 = 0x00;
  
  SSPSTATbits.CKE = 0;    // Standard I2C voltage levels
  SSPSTATbits.SMP = 1;    // Slew rate control off
  
  SSPADD = 239;           // Set 100 kHz baud rate (for Fosc=48MHz)
}

void IdleI2C() 
{
	uint16_t i = 0;
	while ( ( ( (SSPCON2 & 0x1F) | (SSPSTATbits.R_nW) ) != 0 ) && (i<TIMEOUT) )
		i++;
	if(i==TIMEOUT)
		StopI2C();
}

void StartI2C()
{
	SSPCON2bits.SEN = 1;  // initiate bus start condition
}

void StopI2C()
{
	SSPCON2bits.PEN = 1;  // initiate bus stop condition
}

signed char WriteI2C(uint8_t data_out)
{
	SSPBUF = data_out;           // write single byte to SSP1BUF
	if ( SSPCONbits.WCOL )       // test if write collision occurred
		return -1;               // if WCOL bit is set return negative #
	else
	{
		uint16_t i = 0;
		while((SSPSTATbits.BF) && (i<TIMEOUT))
			i++;                 // wait until write cycle is complete 
		if(i==TIMEOUT)
		{
			StopI2C();
			return -3;       // timeout
		}
		IdleI2C();               // ensure module is idle
		if (SSPCON2bits.ACKSTAT) // test for ACK condition received
			return -2;           //Return NACK
		else
			return 0 ;           //Return ACK
	}
}

uint8_t ReadI2C()
{
	SSPCON2bits.RCEN = 1;        // enable master for 1 byte reception
	uint16_t i = 0;
	while ((!SSPSTATbits.BF) && (i<TIMEOUT))
		i++;                     // wait until byte received
	if (i==TIMEOUT)
	{
		StopI2C();
		return 0;                // timeout
	}
	uint8_t byte_read = SSPBUF;
	IdleI2C();
	return byte_read;        // return with read byte 
}

void AckI2C()
{
	SSPCON2bits.ACKDT = 0;         // set acknowledge bit state for ACK
	SSPCON2bits.ACKEN = 1;         // initiate bus acknowledge sequence
}

void NotAckI2C()
{
	SSPCON2bits.ACKDT = 1;          // set acknowledge bit for not ACK
	SSPCON2bits.ACKEN = 1;          // initiate bus acknowledge sequence
}
