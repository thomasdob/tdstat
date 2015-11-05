#ifndef __SWI2C16_H
#define __SWI2C16_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <xc.h>

#define  DATA_LOW   TRISBbits.TRISB4 = 0; // define macro for data pin output
#define  DATA_HI    TRISBbits.TRISB4 = 1; // define macro for data pin input
#define  DATA_LAT   LATBbits.LATB4        // define macro for data pin latch
#define  DATA_PIN   PORTBbits.RB4         // define macro for data pin

#define  CLOCK_LOW  TRISBbits.TRISB6 = 0; // define macro for clock pin output
#define  CLOCK_HI   TRISBbits.TRISB6 = 1; // define macro for clock pin input
#define  SCLK_LAT   LATBbits.LATB6        // define macro for clock pin latch
#define  SCLK_PIN   PORTBbits.RB6         // define macro for clock pin

void StartI2C();
void StopI2C();
uint8_t ReadI2C();
uint8_t ReadLastI2C();
void WriteI2C(uint8_t data_out);

#endif
