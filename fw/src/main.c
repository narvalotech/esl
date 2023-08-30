/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/spi.h>
#include <nrf.h>
#include "utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, 4);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

/* calibrated for creep.ttf */
#define FWIDTH 5
#define FHEIGHT 12
#define MAXLINE (128 / 12)
#define MAXCOL (250 / 5)

/* This could be k_malloc'd to be dynamic */
static char cbuf[MAXLINE][MAXCOL + 1] = {0};

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

	for (int idx = 0; idx < 2; idx++) {
		if (cfb_get_font_size(dev, idx, &font_width, &font_height)) {
			break;
		}
		cfb_framebuffer_set_font(dev, idx);
		printk("font width %d, font height %d\n",
		       font_width, font_height);
		break;
	}

	printk("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
	       cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH),
	       cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH),
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	printk("hello from main thread\n");

	while (1) {
		cfb_framebuffer_clear(dev, false);
		cbuf[MAXLINE-1][MAXCOL] = '\0';
		for (int line = 0; line < MAXLINE; line++) {
			cfb_draw_text(dev, cbuf[line], 0, line * FHEIGHT);
		}
		cfb_framebuffer_finalize(dev);
	}

	return 0;
}

static void scroll_up(void)
{
	for (int line = 0; line < (MAXLINE-1); line++) {
		memcpy(cbuf[line], cbuf[line + 1], MAXCOL);
	}
	memset(cbuf[MAXLINE-1], ' ', MAXCOL);
}

static int esl_cfb_out(int c)
{
	static uint32_t col = 0;
	static uint32_t line = 0;

	if ((c != '\n') && (c != '\r')) {
		cbuf[line][col] = c;
		col++;
	} else {
		line++;
		col = 0;
	}

	if (col >= MAXCOL-1) {
		line++;
		col = 0;
	}

	if (line >= MAXLINE && col == 0) {
		line = MAXLINE-1;
		scroll_up();
	}

	return c;
}

#if defined(CONFIG_CONSOLE_EPD) && defined(CONFIG_PRINTK)
extern void __printk_hook_install(int (*fn)(int c));
extern void __stdout_hook_install(int (*fn)(int c));

static int cfb_console_init(void)
{
	memset(cbuf, ' ', sizeof(cbuf) - 1);

	/* first line is half cut off */
	esl_cfb_out('\n');

#if defined(CONFIG_STDOUT_CONSOLE)
	__stdout_hook_install(esl_cfb_out);
#endif
	__printk_hook_install(esl_cfb_out);
	return 0;
}

SYS_INIT(cfb_console_init, POST_KERNEL, CONFIG_CONSOLE_INIT_PRIORITY);

#endif
