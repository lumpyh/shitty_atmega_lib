#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>

#include "i2c.h"

#define START_SEND       0x08
#define RESTART_SEND     0x10
#define SLA_W_ACK        0x18
#define SLA_W_NACK       0x20
#define DATA_TX_ACK      0x28
#define DATA_TX_NACK     0x30
#define ARBITRATION_LOST 0x38

struct twi_tx_ctx {
	uint8_t read;
	uint8_t addr;
	uint8_t idx;
	uint8_t data_len;
	uint8_t data[256];
	uint8_t res;
	uint8_t in_progress;
} twi_tx_ctx;

int send_data(uint8_t addr, uint8_t *data, uint8_t len)
{
	twi_tx_ctx.read = 0;
	twi_tx_ctx.idx = 0;
	twi_tx_ctx.addr = addr;
	twi_tx_ctx.data_len = len;
	memcpy(&twi_tx_ctx.data, data, len);
	twi_tx_ctx.res = 0;
	twi_tx_ctx.in_progress = 1;

	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE);

	while (twi_tx_ctx.in_progress) {
		_delay_us(500);
	}
}

// twi
static uint8_t twi_get_status(void)
{
	return TWSR & 0xF8;
}

static void twi_set_data(uint8_t data)
{
	TWDR = data;
}

ISR(TWI_vect)
{
	uint8_t status = twi_get_status();
	uint8_t do_stop = 0;
	uint8_t do_restart = 0;

	if (!twi_tx_ctx.in_progress) {
		do_stop = 1;
		goto out;
	}

	switch (status)
	{
		case RESTART_SEND:
		case START_SEND:
			twi_set_data((twi_tx_ctx.addr << 1));
			break;
		case SLA_W_ACK:
		case DATA_TX_ACK:
			if (twi_tx_ctx.idx < twi_tx_ctx.data_len) {
				twi_set_data(twi_tx_ctx.data[twi_tx_ctx.idx++]);
			 } else {
				twi_tx_ctx.res = 0;
				twi_tx_ctx.in_progress = 0;
				do_stop = 1;
			}
			break;
		case SLA_W_NACK:
		case DATA_TX_NACK:
			twi_tx_ctx.res = -1;
			twi_tx_ctx.in_progress = 0;
			do_stop = 1;
			break;
		case ARBITRATION_LOST:
			do_restart = 1;
			break;
		default:
			twi_tx_ctx.res = -1;
			twi_tx_ctx.in_progress = 0;
			do_stop = 1;
			break;
	}

out:
	TWCR = (1 << TWINT) | (do_restart << TWSTA) | (do_stop << TWSTO) | (1 << TWEN) | (1 << TWIE);
	return;
}

void twi_init(void)
{
	TWBR  = 3;//set bitrate
	
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE);
}


