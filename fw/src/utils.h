#include <nrf.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>

#ifndef UTILS_H_
#define UTILS_H_

#define DISP_MOSI 24
#define DISP_SCK 22
#define DISP_CS 20
#define DISP_DC 17
#define DISP_RESET 15
#define DISP_BUSY 13

static inline void mgpio_set(uint32_t bit)
{
	NRF_P0->OUTSET = BIT(bit);
}

static inline void mgpio_reset(uint32_t bit)
{
	NRF_P0->OUTCLR = BIT(bit);
}

static inline bool mgpio_read(uint32_t bit)
{
	return NRF_P0->IN & BIT(bit);
}

#define delay_us(d) k_busy_wait(d)
#define delay_ms(d) k_msleep(d)
#define DELAY_S(d) k_sleep(K_SECONDS(d))

void mspi_write_byte(uint8_t byte);

#endif // UTILS_H_
