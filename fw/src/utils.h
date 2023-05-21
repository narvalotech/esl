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

#define GP(x) (1UL << x)

static inline void mgpio_set(uint32_t bit)
{
	NRF_P0->OUTSET = GP(bit);
}

static inline void mgpio_reset(uint32_t bit)
{
	NRF_P0->OUTCLR = GP(bit);
}

static inline bool mgpio_read(uint32_t bit)
{
	return NRF_P0->IN & GP(bit);
}

#define delay_us(d) k_busy_wait(d)
#define delay_ms(d) k_msleep(d)
#define DELAY_S(d) k_sleep(K_SECONDS(d))

#define INST NRF_SPI0

static inline void mspi_write_byte(uint8_t byte)
{
	INST->EVENTS_READY = 0;
	INST->TXD = byte;

	while(INST->EVENTS_READY != 1) {
		__NOP();
	}
	(void)INST->RXD;
	INST->EVENTS_READY = 0;
}

/* #define INST NRF_SPIM3 */

/* static inline void mspi_write_byte(uint8_t byte) */
/* { */
/* 	static uint8_t myarray[10] = {0}; */

/* 	myarray[0] = byte; */

/* 	INST->TXD.MAXCNT = 1UL; */
/* 	INST->TXD.PTR = (uint32_t)myarray; */

/* 	INST->EVENTS_END = 0; */
/* 	INST->TASKS_START = 1; */

/* 	while(INST->EVENTS_END != 1) { */
/* 		__NOP(); */
/* 	} */
/* 	INST->EVENTS_END = 0; */
/* } */

#undef INST

#endif // UTILS_H_
