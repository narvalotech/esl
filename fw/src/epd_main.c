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
static uint8_t framebuffer[EPD_WIDTH][EPD_HEIGHT];

/* using on-board LUTs doesn't work */
/* #define LUT_OTP */

/* This won't really be 10ms, as the BUSY line is asserted for a couple 100ms */
#define IMG_DELAY_MS 10
int epd_main(void)
{
	while(1)
	{
		epd_reset();
		epd_init();
		epd_display_full(drawing);
		delay_ms(2000);

		epd_display_full(gImage_2);
		delay_ms(IMG_DELAY_MS);

		for (int i=0; i<3; i++) {
			epd_display_partial(gImage_1);
			delay_ms(IMG_DELAY_MS);
			epd_display_partial(gImage_2);
			delay_ms(IMG_DELAY_MS);
			epd_display_partial(gImage_3);
			delay_ms(IMG_DELAY_MS);
		}

		epd_dsleep();
		delay_ms(IMG_DELAY_MS);
	}
}

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
		BIT(EPD_PSR_SHL) |
		BIT(EPD_PSR_UD) |
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
   	for(int i=0; i<IMG_BYTES; i++) {
  		epd_write_data(*picData);
   		picData++;
   	}

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
		/* one full refresh is necessary after boot */
		return;
	}

	epd_write_cmd(EPD_CMD_DTM2);
	for(int i=0; i<IMG_BYTES; i++) {
		epd_write_data(*picData);
		picData++;
	}

	load_lut_partial();
	epd_refresh();
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
	for (uint16_t i = 0; i < LUT_LEN; i++) epd_write_data(p_lut[0][i]);

	epd_write_cmd(EPD_CMD_LUTWW);
	for (uint16_t i = 0; i < LUT_LEN; i++) epd_write_data(p_lut[1][i]);

	epd_write_cmd(EPD_CMD_LUTK);
	for (uint16_t i = 0; i < LUT_LEN; i++) epd_write_data(p_lut[4][i]);

	if (!swap || first_image) {
		swap = true;

		epd_write_cmd(EPD_CMD_LUTKW);
		for (uint16_t i = 0; i < LUT_LEN; i++) epd_write_data(p_lut[2][i]);

		epd_write_cmd(EPD_CMD_LUTWK);
		for (uint16_t i = 0; i < LUT_LEN; i++) epd_write_data(p_lut[3][i]);
	} else {
		swap = false;

		epd_write_cmd(EPD_CMD_LUTWK);
		for (uint16_t i = 0; i < LUT_LEN; i++) epd_write_data(p_lut[2][i]);

		epd_write_cmd(EPD_CMD_LUTKW);
		for (uint16_t i = 0; i < LUT_LEN; i++) epd_write_data(p_lut[3][i]);
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

/* struct display_buffer_descriptor { */
/* 	/\** Data buffer size in bytes *\/ */
/* 	uint32_t buf_size; */
/* 	/\** Data buffer row width in pixels *\/ */
/* 	uint16_t width; */
/* 	/\** Data buffer column height in pixels *\/ */
/* 	uint16_t height; */
/* 	/\** Number of pixels between consecutive rows in the data buffer *\/ */
/* 	uint16_t pitch; */
/* }; */

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

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");

	for (int yi = y; yi < desc->height; yi++)
	for (int xi = x; xi < desc->width; xi++) {
		framebuffer[xi][yi] = ((uint8_t**)buf)[xi][yi];
	}


	LOG_DBG("start HW write");
	if (!blanking) {
		epd_display_partial((uint8_t*)framebuffer);
	} else {
		epd_display_full((uint8_t*)framebuffer);
	}
	LOG_DBG("end HW write");

	return 0;
}

static void epd_get_capabilities(const struct device *dev,
				 struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = EPD_WIDTH;
	caps->y_resolution = EPD_HEIGHT;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
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

static int display_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	init_gpio();
	init_spi();

	epd_reset();
	epd_init();

	return 0;
}

DEVICE_DEFINE(display_epd, "EPD", &display_driver_init,
	      NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,
	      &epd_driver_api);
