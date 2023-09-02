/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, 4);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <stdio.h>

/* calibrated for creep.ttf */
#define FWIDTH 5
#define FHEIGHT 12		/* kde says 11 */
#define MAXLINE (128 / 12)	/* 10 */
#define MAXCOL (250 / 5)	/* 50 */

/* This could be k_malloc'd to be dynamic */
static char cbuf[MAXLINE][MAXCOL + 1] = {0};

void setup_usb(void)
{
#if CONFIG_USB_DEVICE_STACK
	const struct device *dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(dev)) {
		LOG_ERR("CDC ACM device not ready");
		return;
	}

	int ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB: %d", ret);
		return;
	}
#endif
}

int framebuffer_setup(const struct device *dev)
{
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;

	if (!device_is_ready(dev)) {
		printk("Device %s not ready\n", dev->name);
		return -ENODEV;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		printk("Failed to set required pixel format\n");
		return -EINVAL;
	}

	printk("Initialized %s\n", dev->name);

	if (cfb_framebuffer_init(dev)) {
		printk("Framebuffer initialization failed!\n");
		return -EIO;
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

	printk("setup ok\n");

	return 0;
}

void framebuffer_update(const struct device *dev)
{
	cfb_framebuffer_clear(dev, false);

	/* cfb takes a string as input */
	cbuf[MAXLINE-1][MAXCOL] = '\0';

	/* draw each line */
	for (int line = 0; line < MAXLINE; line++) {
		cfb_draw_text(dev, cbuf[line], 0, line * FHEIGHT);
	}

	/* write rendered framebuffer to display */
	cfb_framebuffer_finalize(dev);

	if (IS_ENABLED(CONFIG_NATIVE_APPLICATION)) {
		/* The real display takes ~600ms to update. The SDL-based
		 * emulated display on native builds will take 0 (zephyr)
		 * time.
		 *
		 * We need to sleep a bit to allow the native platform and
		 * display to do its bookkeeping.
		 */
		k_msleep(100);
	}
}

int main(void)
{
	setup_usb();

	const struct device *dev =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	framebuffer_setup(dev);

	printk("entering main loop\n");

	/* framebuffer rendering loop */
	while (1) {
		framebuffer_update(dev);
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

bool is_number(int c)
{
	return (c >= '0') && (c <= '9');
}

bool is_letter(int c)
{
	return ((c >= 'A') && (c <= 'Z')) ||
		((c >= 'a') && (c <= 'z'));
}

int esl_cfb_out(int c);
static bool vt100 = false;
void handle_vt100(int c)
{
	static int vt_r0 = 0;

	if (c == '[') {
		/* start of sequence.
		 * todo: assert first char after ESC */
		return;
	}

	if (is_number(c)) {
		vt_r0 *= 10;
		vt_r0 += c - '0';
		return;
	}

	/* todo: switch */
	if (is_letter(c)) {
		if (c == 'C') {
			/* advance cursor position */
			vt100 = false;
			printk("VT:advance-%d\n", vt_r0);
			for (int i=0; i<vt_r0; i++) esl_cfb_out(' ');
		}
		/* eat the char */
	}

	vt100 = false;
	vt_r0 = 0;
}

int esl_cfb_out(int c)
{
	static uint32_t col = 0;
	static uint32_t line = 0;

	printk("%02x(%c) ", c, c);
	/* printk("%c", c); */
	switch (c) {
		case '\e':
			vt100 = true;
			printk("`vt`");
			break;
		case '\n':
			line++;
			col = 0;
			printk("\n");
		case '\r':
			col = 0;
			break;
		default:
			if (vt100) {
				handle_vt100(c);
				return c;
			} else {
				cbuf[line][col] = c;
				col++;
			}
	}

	if (col >= MAXCOL-1) {
		line++;
		col = 0;
	}

	if (line >= MAXLINE && col == 0) {
		line = MAXLINE-1;
		scroll_up();
		printk("\n###\n");
	}

	return c;
}

void esl_cfb_init(void)
{
	memset(cbuf, ' ', sizeof(cbuf) - 1);

	/* first line is half cut off */
	esl_cfb_out('\n');
}
