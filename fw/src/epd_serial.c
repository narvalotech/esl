#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#define DT_DRV_COMPAT nvl_epd_uart

extern void esl_cfb_init(void);

static int uart_epd_init(const struct device *dev)
{
	/* TODO: we might want to init the whole EPD driver here */
	esl_cfb_init();

	return 0;
}

static int uart_epd_poll_in(const struct device *dev, unsigned char *c)
{
	unsigned int ret = 0;	/* TODO: read from real UART */

	/* -1 if buf empty */
	return ret ? 0 : -1;
}

extern int esl_cfb_out(int c);
static void uart_epd_poll_out(const struct device *dev, unsigned char c)
{
	esl_cfb_out(c);
}

static const struct uart_driver_api uart_epd_driver_api = {
	.poll_in = uart_epd_poll_in,
	.poll_out = uart_epd_poll_out,
};

DEVICE_DT_INST_DEFINE(0,
		      uart_epd_init,
		      NULL,
		      NULL,
		      NULL,
		      /* Initialize UART device before UART console. */
		      PRE_KERNEL_1,
		      CONFIG_SERIAL_INIT_PRIORITY,
		      &uart_epd_driver_api);