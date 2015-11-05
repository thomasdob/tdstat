/*
 * USB Potentiostat/galvanostat firmware
 *
 * This code makes use of Signal 11's M-Stack USB stack to implement 
 * communication through raw USB bulk transfers. Commands are received
 * as ASCII strings on EP1 OUT. They are then interpreted and executed;
 * they either change the state of output pins, or cause data to be read
 * from / written to the MCP3422 (ADC) or MAX5217 (DAC) using a software
 * I2C implementation (hardware I2C was tested but did not work properly).
 * The resulting data, or an "OK" message, is sent as a reply on EP1 IN.
 * The USB service is interrupt-driven.
 *
 * Thomas Dobbelaere
 * CoCooN research group
 * 2015-08-27
 */

#include "usb.h"
#include <xc.h>
#include <string.h>
#include "usb_config.h"
#include "usb_ch9.h"
#include "i2c_software.h"

// PIC16F1459 configuration bit settings:
// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = OFF       // Internal/External Switchover Mode (Internal/External Switchover Mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is disabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config CPUDIV = NOCLKDIV// CPU System Clock Selection Bit (NO CPU system divide)
#pragma config USBLSCLK = 48MHz // USB Low SPeed Clock Selection bit (System clock expects 48 MHz, FS/LS USB CLKENs divide-by is set to 8.)
#pragma config PLLMULT = 3x     // PLL Multipler Selection Bit (3x Output Frequency Selected)
#pragma config PLLEN = ENABLED  // PLL Enable Bit (3x or 4x PLL Enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

#define MODE_SW LATAbits.LATA5
#define MODE_SW_TRIS TRISAbits.TRISA5
#define CELL_ON_PIN LATAbits.LATA4
#define CELL_ON_TRIS TRISAbits.TRISA4
#define CELL_ON  1
#define CELL_OFF 0
#define POTENTIOSTATIC  0
#define GALVANOSTATIC  1
#define PIN_INPUT  1
#define PIN_OUTPUT 0
#define RANGE1 0x04 // RC2 high, everything else low
#define RANGE2 0x08 // RC3 high, everything else low
#define RANGE3 0x10 // RC4 high, everything else low
#define RANGE4 0x20 // RC5 high, everything else low

static const uint8_t* received_data;
static uint8_t received_data_length;
static uint8_t* transmit_data;
static uint8_t transmit_data_length;

void MAX5217_load_code(uint8_t value_hi, uint8_t value_lo)
{
	StartI2C();
	WriteI2C(0x3A); // I2C address (for writing)
	WriteI2C(0x01); // CODE_LOAD command
	WriteI2C(value_hi); // DAC value <15:8>
	WriteI2C(value_lo); // DAC value <7:0>
	StopI2C();
}

void MCP3422_config(uint8_t config_byte)
{
	StartI2C();
	WriteI2C(0xD0); // I2C address (for writing)
	WriteI2C(config_byte); // CODE_LOAD command
	StopI2C();
}

void MCP3422_read3(uint8_t* byte1, uint8_t* byte2, uint8_t* config_byte)
{
	StartI2C();
	WriteI2C(0xD1); // I2C address (for reading)
	*byte1 = ReadI2C();
	*byte2 = ReadI2C();
	*config_byte = ReadLastI2C();
	StopI2C();
}

void MCP3422_read4(uint8_t* byte1, uint8_t* byte2, uint8_t* byte3, uint8_t* config_byte)
{
	StartI2C();
	WriteI2C(0xD1); // I2C address (for reading)
	*byte1 = ReadI2C();
	*byte2 = ReadI2C();
	*byte3 = ReadI2C();
	*config_byte = ReadLastI2C();
	StopI2C();
}

void command_unknown()
{
	const uint8_t* reply = "?";
    strcpy(transmit_data, reply);
    transmit_data_length = strlen(reply);
}

void send_OK()
{
	const uint8_t* reply = "OK";
    strcpy(transmit_data, reply);
    transmit_data_length = strlen(reply);
}

void command_cell_on()
{
	CELL_ON_PIN = CELL_ON;
	send_OK();
}

void command_cell_off()
{
	CELL_ON_PIN = CELL_OFF;
	send_OK();
}

void command_mode_potentiostatic()
{
	MODE_SW = POTENTIOSTATIC;
	send_OK();
}

void command_mode_galvanostatic()
{
	MODE_SW = GALVANOSTATIC;
	send_OK();
}

void command_range1()
{
	LATC = RANGE1;
	send_OK();
}

void command_range2()
{
	LATC = RANGE2;
	send_OK();
}

void command_range3()
{
	LATC = RANGE3;
	send_OK();
}

void command_range4()
{
	LATC = RANGE4;
	send_OK();
}

void command_set_dac(uint8_t value_hi, uint8_t value_lo)
{
	MAX5217_load_code(value_hi, value_lo);
	send_OK();
}

void command_config_adc(uint8_t config_byte)
{
	MCP3422_config(config_byte);
	send_OK();
}

void command_read_adc_3bytes()
{
	uint8_t byte1, byte2, config_byte;
	MCP3422_read3(&byte1, &byte2, &config_byte);
	transmit_data[0]=byte1;
	transmit_data[1]=byte2;
	transmit_data[2]=config_byte;
	transmit_data_length=3;
}

void command_read_adc_4bytes()
{
	uint8_t byte1, byte2, byte3, config_byte;
	MCP3422_read4(&byte1, &byte2, &byte3, &config_byte);
	transmit_data[0]=byte1;
	transmit_data[1]=byte2;
	transmit_data[2]=byte3;
	transmit_data[3]=config_byte;
	transmit_data_length=4;
}

void interpret_command() {
    if (received_data_length == 7 && strncmp(received_data,"CELL ON",7) == 0)
        command_cell_on();
    else if (received_data_length == 8 && strncmp(received_data,"CELL OFF",8) == 0)
        command_cell_off();
    else if (received_data_length == 14 && strncmp(received_data,"POTENTIOSTATIC",14) == 0)
        command_mode_potentiostatic();
    else if (received_data_length == 13 && strncmp(received_data,"GALVANOSTATIC",13) == 0)
        command_mode_galvanostatic();
    else if (received_data_length == 7 && strncmp(received_data,"RANGE 1",7) == 0)
        command_range1();
	else if (received_data_length == 7 && strncmp(received_data,"RANGE 2",7) == 0)
        command_range2();
	else if (received_data_length == 7 && strncmp(received_data,"RANGE 3",7) == 0)
        command_range3();
	else if (received_data_length == 7 && strncmp(received_data,"RANGE 4",7) == 0)
        command_range4();
	else if (received_data_length == 9 && strncmp(received_data,"DACSET ",7) == 0)
		command_set_dac(received_data[7],received_data[8]);
	else if (received_data_length == 9 && strncmp(received_data,"ADCCONF ",8) == 0)
		command_config_adc(received_data[8]);
	else if (received_data_length == 8 && strncmp(received_data,"ADCREAD3",8) == 0)
		command_read_adc_3bytes();
	else if (received_data_length == 8 && strncmp(received_data,"ADCREAD4",8) == 0)
		command_read_adc_4bytes();
	else
        command_unknown();
}

int main(void)
{
	OSCCONbits.IRCF = 0b1111; // 0b1111 = 16MHz HFINTOSC postscaler
    ANSELA = 0x00; // digital I/O on PORTA
    ANSELB = 0x00; // digital I/O on PORTB
    ANSELC = 0x00; // digital I/O on PORTC
    APFCON = 0x00; // a little bit of voodoo
    MODE_SW_TRIS = PIN_OUTPUT;
    MODE_SW = POTENTIOSTATIC; // initialize mode to potentiostatic
    CELL_ON_TRIS = PIN_OUTPUT;
    CELL_ON_PIN = CELL_OFF; // initialize cell to off position
    TRISC = 0x00; // all outputs on PORTC
    LATC = RANGE1; // initialize range to range 1

	// Enable Active clock-tuning from the USB
	ACTCONbits.ACTSRC = 1; // 1=USB
	ACTCONbits.ACTEN = 1;

	// Configure interrupts
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
	
	// Initialize USB
	usb_init();

	while (1)
	{
		if (usb_is_configured() && usb_out_endpoint_has_data(1)) // wait for data received from host
		{
			if (!usb_in_endpoint_halted(1))
			{
				while (usb_in_endpoint_busy(1)) // wait for EP1 IN to become free
					;
				received_data_length = usb_get_out_buffer(1, &received_data);
				transmit_data = usb_get_in_buffer(1);
				interpret_command(); // this reads received_data and sets transmit_data and transmit_data_length
				usb_send_in_buffer(1, transmit_data_length); // send the data back
			}
			usb_arm_out_endpoint(1);
		}
	}

	return 0;
}

void interrupt isr()
{
	usb_service();
}
