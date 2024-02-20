#ifndef ATMEAGA_I2C_H
#define ATMEAGA_I2C_H

#include <stdint.h>

int send_data(uint8_t addr, uint8_t *data, uint8_t len);
void twi_init(void);
#endif
