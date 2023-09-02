#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "framebuffer.h"

#define DT_DRV_COMPAT nvl_epd_uart

static int uart_epd_init(const struct device *dev)
{
	/* TODO: we might want to init the framebuffer + the display driver here */
	framebuffer_reset();

	return 0;
}

static int uart_epd_poll_in(const struct device *dev, unsigned char *c)
{
	/* Use console UART as input device for now */
	const struct device *const console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	return uart_poll_in(console, c);
}

extern int esl_cfb_out(int c);
static void uart_epd_poll_out(const struct device *dev, unsigned char c)
{
	framebuffer_write_char(c);
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
