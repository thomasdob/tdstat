#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <xc.h>

void InitializeI2C();
void IdleI2C();
void StartI2C();
void StopI2C();
signed char WriteI2C(uint8_t data_out);
uint8_t ReadI2C();
void AckI2C();
void NotAckI2C();
