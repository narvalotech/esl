#define DT_DRV_COMPAT nvl_epd_spi

#include "epd_main.h"
#include "epd_lut.h"
#include "epd_regs.h"
#include "splash.h"
#include "utils.h"
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <nrf.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(epd, CONFIG_DISPLAY_LOG_LEVEL);

void epd_write_cmd(uint8_t command);
void epd_write_data(uint8_t data);

#define EPD_WIDTH   128UL
#define EPD_HEIGHT  250UL
#define IMG_BYTES (EPD_WIDTH * EPD_HEIGHT / 8)

static void load_lut_full(void);
static void load_lut_partial(void);
void epd_init(void);
void epd_display_full(const uint8_t* picData);
void epd_display_partial(const uint8_t* picData);
void epd_dsleep(void);
void epd_refresh(void);
void epd_reset(void);

static uint8_t first_image;
static bool blanking;
/* static uint8_t framebuffer[EPD_HEIGHT][EPD_WIDTH/8]; */
static uint8_t framebuffer[EPD_HEIGHT * EPD_WIDTH / 8];

/* using on-board LUTs doesn't work */
/* #define LUT_OTP */

void epd_reset(void)
{
	mgpio_set(DISP_RESET);
	delay_ms(10);

	mgpio_reset(DISP_RESET);
	delay_ms(100);

	mgpio_set(DISP_RESET);
	delay_ms(100);
}

void write_cdi(bool boot)
{
	uint8_t val = boot ? 0xB7 : 0xD7;

	epd_write_cmd(EPD_CMD_CDI);
	epd_write_data(val);
}

void epd_init(void)
{
	uint8_t value;

	first_image = 1;

	epd_write_cmd(EPD_CMD_PSR);
	value = BIT(EPD_PSR_RST_N) |
		BIT(EPD_PSR_SHD_N) |
		/* uncomment to rotate 180deg */
		/* BIT(EPD_PSR_SHL) | */
		/* BIT(EPD_PSR_UD) | */
		BIT(EPD_PSR_KWR) |
		BIT(EPD_PSR_RES0) |
		BIT(EPD_PSR_RES1) ;
#ifndef LUT_OTP
	value |= BIT(EPD_PSR_REG);
#endif
	epd_write_data(value);

	epd_write_cmd(EPD_CMD_PWR);
	epd_write_data(BIT(EPD_PWR_VG_EN) | BIT(EPD_PWR_VS_EN));
	epd_write_data(0x00); /* 20V (default) */
	epd_write_data(0x3F); /* 15V (default) */
	epd_write_data(0x3F); /* 15V (default) */
	epd_write_data(0x03); /* 3V (value shouldn't matter) */

	epd_write_cmd(EPD_CMD_BTST);
	/* phase A & B:
	 * - 6.58uS off time
	 * - strength 5
	 * - 40ms soft-start
	 */
	epd_write_data(0x27);
	epd_write_data(0x27);
	/* phase C:
	 * - 6.58us off time
	 * - strength 6
	 */
	epd_write_data(0x2F);

	epd_write_cmd(EPD_CMD_TRES);
	epd_write_data(EPD_WIDTH);
	epd_write_data(0);
	epd_write_data(EPD_HEIGHT);

	epd_write_cmd(EPD_CMD_GSST);
	epd_write_data(0x00);
	epd_write_data(0x00);
	epd_write_data(0x00);

	write_cdi(true);
}

void epd_display_full(const uint8_t* picData)
{
	epd_write_cmd(EPD_CMD_DTM2);
	epd_write_data_stream((uint8_t*)picData, IMG_BYTES);

	load_lut_full();
	epd_refresh();

	if(first_image) {
		first_image = 0;
		write_cdi(false);
	}
}

void epd_display_partial(const uint8_t* picData)
{
	if(first_image) {
		epd_display_full(picData);
		/* one full refresh is necessary after boot */
		return;
	}

	int64_t t0 = k_uptime_get();
	epd_write_cmd(EPD_CMD_DTM2);
	epd_write_data_stream((uint8_t*)picData, IMG_BYTES);
	int64_t t1 = k_uptime_get();

	load_lut_partial();
	int64_t t2 = k_uptime_get();
	epd_refresh();
	LOG_DBG("data %lldms lut %lldms refresh %lldms", t1-t0, t2-t1, k_uptime_get()-t2);
}

void epd_refresh(void)
{
	epd_write_cmd(EPD_CMD_AUTO);
	epd_write_data(EPD_CMD_ON_REFRESH_OFF);

	while(mgpio_read(DISP_BUSY) == 0) {
		/* around 500ms in partial refresh, >1s for full refresh */
		k_msleep(10);
	}
}

void epd_dsleep(void)
{
	epd_write_cmd (EPD_CMD_DSLP);
	epd_write_data(EPD_DSLP_MAGIC);
}

static void write_lut(uint8_t const **p_lut)
{
	static bool swap = false;
	uint8_t LUT_LEN = 56;

#ifdef LUT_OTP
	return;
#endif

	epd_write_cmd(EPD_CMD_LUTC);
	epd_write_data_stream(p_lut[0], LUT_LEN);

	epd_write_cmd(EPD_CMD_LUTWW);
	epd_write_data_stream(p_lut[1], LUT_LEN);

	epd_write_cmd(EPD_CMD_LUTK);
	epd_write_data_stream(p_lut[4], LUT_LEN);

	if (!swap || first_image) {
		swap = true;

		epd_write_cmd(EPD_CMD_LUTKW);
		epd_write_data_stream(p_lut[2], LUT_LEN);

		epd_write_cmd(EPD_CMD_LUTWK);
		epd_write_data_stream(p_lut[3], LUT_LEN);
	} else {
		swap = false;

		epd_write_cmd(EPD_CMD_LUTWK);
		epd_write_data_stream(p_lut[2], LUT_LEN);

		epd_write_cmd(EPD_CMD_LUTKW);
		epd_write_data_stream(p_lut[3], LUT_LEN);
	}
}

static void load_lut_full(void)
{
	uint8_t const *p_lut_gc[5] = {lut_R20_GC, lut_R21_GC, lut_R22_GC, lut_R23_GC, lut_R24_GC};
	write_lut(p_lut_gc);
}

static void load_lut_partial(void)
{
	uint8_t const *p_lut_du[5] = {lut_R20_DU, lut_R21_DU, lut_R22_DU, lut_R23_DU, lut_R24_DU};
	write_lut(p_lut_du);
}

void epd_write_cmd(uint8_t command)
{
	mgpio_reset(DISP_DC);		/* command */
	mspi_write_byte(command);
	mgpio_set(DISP_DC);
}

void epd_write_data(uint8_t data)
{
	mgpio_set(DISP_DC);		/* data */
	mspi_write_byte(data);
	mgpio_set(DISP_DC);
}

void epd_write_data_stream(const uint8_t *data, size_t len)
{
	#define CHUNK 256
	mgpio_set(DISP_DC);		/* data */
	for (size_t i=0; i<len; ) {
		size_t clen = MIN(CHUNK, len-i);
		mspi_write_buf((uint8_t *)data + i, clen);
		i+=clen;
	}
	mgpio_set(DISP_DC);
}

static int epd_read(const struct device *dev, const uint16_t x, const uint16_t y,
		    const struct display_buffer_descriptor *desc, void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *epd_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int epd_set_brightness(const struct device *dev,
			      const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int epd_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int epd_set_orientation(const struct device *dev,
			       const enum display_orientation
			       orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int epd_set_pixel_format(const struct device *dev,
				const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

/* EPD seems to use !blanking as `partial refresh` as a convention in zephyr */
static int epd_blanking_off(const struct device *dev)
{
	blanking = false;

	return 0;
}

static int epd_blanking_on(const struct device *dev)
{
	blanking = true;

	return 0;
}

void writeOutPixel(uint8_t *buf, uint32_t x, uint32_t y)
{
    uint32_t p = EPD_WIDTH * y + x;
    buf[p / 8] |= BIT((p & 0x7U));
}

static int epd_write(const struct device *dev, const uint16_t x, const uint16_t y,
		     const struct display_buffer_descriptor *desc,
		     const void *buf)
{
	/* x/y: start of buffer, upper left corner */
	/* TODO: bounds checking */
	/* uint16_t x_end = x + desc->width - 1; */
	/* uint16_t y_end = y + desc->height - 1; */

	LOG_DBG("x %u, y %u, height %u, width %u, pitch %u size %u",
		x, y, desc->height, desc->width, desc->pitch, desc->buf_size);

	__ASSERT(desc->height <= desc->pitch, "Pitch is smaller than width");
	/* lazy. TODO: support partial updates with framebuffer AND with direct write */
	__ASSERT_NO_MSG(desc->width == EPD_HEIGHT);
	__ASSERT_NO_MSG(desc->height == EPD_WIDTH);

	int64_t start = k_uptime_get();
	/* This way we only have to use the OR operation to draw the image */
	memset(framebuffer, 0, sizeof(framebuffer));

	uint8_t const *input = buf;

	/* Transform buffer from landscape V-aligned to portrait H-aligned */
	for (int y=0; y<desc->height; y++) {
		for (int x=0; x<desc->width; x++) {
			uint32_t dy = x;
			uint32_t dx = (desc->height-1) - y;
			if (input[(desc->width) * (y / 8) + x] & BIT((y & 0x7)))
				writeOutPixel(framebuffer, dx, dy);
		}
	}

	int64_t end = k_uptime_get();

	/* LOG_HEXDUMP_WRN(buf, 4000, "buf"); */
	/* LOG_HEXDUMP_WRN(framebuffer, sizeof(framebuffer), "fb"); */

	LOG_DBG("start HW write");
	if (!blanking) {
		epd_display_partial((uint8_t*)framebuffer);
	} else {
		epd_display_full((uint8_t*)framebuffer);
	}
	LOG_DBG("end HW write");
	LOG_DBG("transform %lldms send %lldms", end-start, k_uptime_get()-end);

	return 0;
}

static void epd_get_capabilities(const struct device *dev,
				 struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = EPD_HEIGHT;
	caps->y_resolution = EPD_WIDTH;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	/* caps->screen_info = SCREEN_INFO_EPD; */
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
	/* CFB module requires SCREEN_INFO_MONO_VTILED to be set to draw text.
	 * This is actually not the case for us, we are HTILED. This forces us
	 * to do extra work and change the orientation on the fly at the time of
	 * write. TODO: fix this upstream.
	 */
	caps->screen_info |= SCREEN_INFO_MONO_VTILED;
}

static struct display_driver_api epd_driver_api = {
	.blanking_on = epd_blanking_on,
	.blanking_off = epd_blanking_off,
	.write = epd_write,
	.read = epd_read,
	.get_framebuffer = epd_get_framebuffer,
	.set_brightness = epd_set_brightness,
	.set_contrast = epd_set_contrast,
	.get_capabilities = epd_get_capabilities,
	.set_pixel_format = epd_set_pixel_format,
	.set_orientation = epd_set_orientation,
};

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

void mspi_write_buf(uint8_t *buf, size_t len)
{
	const struct spi_buf tx = {.buf = buf, .len = len};
	const struct spi_buf_set tx_bufs = {.buffers = &tx, .count = 1};

	int err = spi_transceive_dt(&spi_dev, &tx_bufs, NULL);
	if (err) {
		LOG_ERR("SPI transfer failed: %d", err);
		k_panic();
	}
}

static int display_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	init_gpio();
	init_spi();

	epd_reset();
	epd_init();

	return 0;
}

/* TODO:
 * - allow multiple instances
 * - put device on bus
 * - use DTS for config/gpio
 */
DEVICE_DT_INST_DEFINE(0, &display_driver_init,
		      NULL, NULL, NULL,
		      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,
		      &epd_driver_api);
