#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>

#include "framebuffer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fb, 4);

/* calibrated for creep.ttf */
#define FWIDTH 5
#define FHEIGHT 12		/* kde says 11 */

/* TODO: use DTS to fetch this */
#define MAXLINE (128 / 12)	/* 10 */
#define MAXCOL (250 / 5)	/* 50 */

/* This could be k_malloc'd to be dynamic */
static char cbuf[MAXLINE][MAXCOL + 1] = {0};

int framebuffer_setup(const struct device *dev)
{
	uint16_t rows;
	uint8_t ppt;
	uint8_t font_width;
	uint8_t font_height;

	if (!device_is_ready(dev)) {
		LOG_ERR("Device %s not ready", dev->name);
		return -ENODEV;
	}

	if (display_set_pixel_format(dev, PIXEL_FORMAT_MONO10) != 0) {
		LOG_ERR("Failed to set required pixel format");
		return -EINVAL;
	}

	LOG_INF("Initialized %s", dev->name);

	if (cfb_framebuffer_init(dev)) {
		LOG_ERR("Framebuffer initialization failed!");
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
		LOG_INF("font width %d, font height %d",
		       font_width, font_height);
		break;		/* use the first (smallest) font */
	}

	LOG_INF("x_res %d, y_res %d, ppt %d, rows %d, cols %d",
	       cfb_get_display_parameter(dev, CFB_DISPLAY_WIDTH),
	       cfb_get_display_parameter(dev, CFB_DISPLAY_HEIGH),
	       ppt,
	       rows,
	       cfb_get_display_parameter(dev, CFB_DISPLAY_COLS));

	LOG_INF("setup ok\n");

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

struct fb_state {
	uint32_t col;
	uint32_t line;
	bool vt100;
};

static struct fb_state fb;

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
			fb.vt100 = false;
			LOG_DBG("VT:advance-%d\n", vt_r0);
			for (int i=0; i<vt_r0; i++) framebuffer_write_char(' ');
		}
		/* eat the char */
	}

	fb.vt100 = false;
	vt_r0 = 0;
}

int framebuffer_write_char(int c)
{
	LOG_DBG("%02x(%c) ", c, c);
	switch (c) {
		case '\e':
			fb.vt100 = true;
			LOG_DBG("`vt`");
			break;
		case '\n':
			fb.line++;
			fb.col = 0;
			LOG_DBG("\n");
		case '\r':
			fb.col = 0;
			break;
		default:
			if (fb.vt100) {
				handle_vt100(c);
				return c;
			} else {
				cbuf[fb.line][fb.col] = c;
				fb.col++;
			}
	}

	if (fb.col >= MAXCOL-1) {
		fb.line++;
		fb.col = 0;
	}

	if (fb.line >= MAXLINE && fb.col == 0) {
		fb.line = MAXLINE-1;
		scroll_up();
		LOG_DBG("\n###\n");
	}

	return c;
}

void framebuffer_reset(void)
{
	memset(cbuf, ' ', sizeof(cbuf) - 1);

	/* first line is half cut off */
	framebuffer_write_char('\n');
}
