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
#include <nrf.h>
#include "utils.h"

#if 1
extern void epd_main(void);

static void init_gpio(void)
{
	uint32_t outputs = GP(DISP_MOSI) | GP(DISP_SCK) | GP(DISP_CS) | GP(DISP_DC) | GP(DISP_RESET);
	uint32_t inputs = GP(DISP_BUSY);

	NRF_P0->DIRSET = outputs;
	NRF_P0->DIRCLR = inputs;
}

#define INST NRF_SPI0

static void init_spi(void)
{
	INST->PSEL.SCK =   DISP_SCK;
	INST->PSEL.MOSI =  DISP_MOSI;
	INST->PSEL.MISO =  0x80000000; /* not connected */
	/* INST->PSEL.CSN =   0x80000000; /\* not connected *\/ */

	INST->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M2;
	INST->CONFIG = 0;

	INST->ENABLE = 1UL;
	/* INST->ENABLE = 7UL; */
	/* INST->TASKS_STOP = 1; */
	/* INST->RXD.MAXCNT = 0; */
}

int main(void)
{
	init_gpio();
	init_spi();
	epd_main();

	return 0;
}
#endif

#if 0

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
	if (!device_is_ready(dev)) {
		printf("Device %s not ready\n", dev->name);
		return 0;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		printf("Failed to set required pixel format\n");
		return 0;
	}

	printf("Initialized %s\n", dev->name);

	if (cfb_framebuffer_init(dev)) {
		printf("Framebuffer initialization failed!\n");
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
		printf("font width %d, font height %d\n",
		       font_width, font_height);
	}

	printf("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH),
	       cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH),
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	while (1) {
		for (int i = 0; i < rows; i++) {
			cfb_framebuffer_clear(dev, false);
			if (cfb_print(dev,
				      "0123456789mMgj!\"§$%&/()=",
				      0, i * ppt)) {
				printf("Failed to print a string\n");
				continue;
			}

			cfb_framebuffer_finalize(dev);
			k_sleep(K_MSEC(100));
		}
	}
	return 0;
}
#endif

#endif
