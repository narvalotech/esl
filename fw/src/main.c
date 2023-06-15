/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/spi.h>
#include <nrf.h>
#include "utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, 4);

#if 0
extern void epd_main(void);

static void init_gpio(void)
{
	uint32_t outputs = BIT(DISP_DC) | BIT(DISP_RESET);
	uint32_t inputs = BIT(DISP_BUSY);

	NRF_P0->DIRSET = outputs;
	NRF_P0->DIRCLR = inputs;

	/* connect input buffer */
	NRF_P0->PIN_CNF[DISP_BUSY] = 0;
}

#define SPI_OP SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE

const struct spi_dt_spec spi_dev = SPI_DT_SPEC_GET(DT_NODELABEL(se0213q04), SPI_OP, 0);

static void init_spi(void)
{
	if(spi_is_ready_dt(&spi_dev)) {
		LOG_INF("spi initialized");
	} else {
		LOG_ERR("spi not initialized");
		k_panic();
	}
}

void mspi_write_byte(uint8_t byte)
{
	uint8_t buf[1] = {byte};
	const struct spi_buf tx = {.buf = buf, .len = 1};
	const struct spi_buf_set tx_bufs = {.buffers = &tx, .count = 1};

	int err = spi_transceive_dt(&spi_dev, &tx_bufs, NULL);
	if (err) {
		LOG_ERR("SPI transfer failed: %d", err);
		k_panic();
	}
}

int main(void)
{
	init_gpio();
	init_spi();
	epd_main();

	return 0;
}
#endif

#if 1

#if !defined(CONFIG_DISPLAY)

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
	     "Console device is not ACM CDC UART device");

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;

	if (usb_enable(NULL)) {
		return 0;
	}

	/* Poll if the DTR flag was set */
	while (!dtr) {
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(100));
	}

	while (1) {
		printk("Hello World! %s\n", CONFIG_ARCH);
		k_sleep(K_SECONDS(1));
	}
}

/* void setup_usb(void) */
/* { */
/* 	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console)); */
/* 	uint32_t dtr = 0; */

/* 	if (usb_enable(NULL)) { */
/* 		k_oops(); */
/* 	} */

/* 	/\* Poll if the DTR flag was set *\/ */
/* 	while (!dtr) { */
/* 		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr); */
/* 		/\* Give CPU resources to low priority threads. *\/ */
/* 		k_sleep(K_MSEC(100)); */
/* 	} */
/* } */

#else
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

int main(void)
{
	const struct device *dev;
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;

	/* setup_usb(); */

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	/* dev = DEVICE_DT_GET(display_epd); */
	if (!device_is_ready(dev)) {
		printk("Device %s not ready\n", dev->name);
		return 0;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		printk("Failed to set required pixel format\n");
		return 0;
	}

	printk("Initialized %s\n", dev->name);

	if (cfb_framebuffer_init(dev)) {
		printk("Framebuffer initialization failed!\n");
		return 0;
	}

	cfb_framebuffer_clear(dev, true);

	display_blanking_off(dev);

	rows = cfb_get_display_parameter(dev, CFB_DISPLAY_ROWS);
	ppt = cfb_get_display_parameter(dev, CFB_DISPLAY_PPT);

	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(dev, idx);
		printk("font width %d, font height %d\n",
		       font_width, font_height);
	}

	printk("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH),
	       cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH),
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	while (1) {
		for (int i = 0; i < rows; i++) {
			cfb_framebuffer_clear(dev, false);
			if (cfb_print(dev,
				      "hello",
				      0, i * ppt)) {
				printk("Failed to print a string\n");
				continue;
			}

			cfb_framebuffer_finalize(dev);
			/* k_sleep(K_MSEC(100)); */
		}
	}
	return 0;
}
#endif

#endif
