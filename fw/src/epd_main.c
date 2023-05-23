#include "epd_main.h"
#include "epd_lut.h"
#include "epd_regs.h"
#include "splash.h"
#include "utils.h"

void epd_write_cmd(unsigned char command);
void epd_write_data(unsigned char data);

#define EPD_WIDTH   128UL
#define EPD_HEIGHT  250UL
#define IMG_BYTES (EPD_WIDTH * EPD_HEIGHT / 8)

#define EPD_W21_CS_0	mgpio_reset(DISP_CS)
#define EPD_W21_CS_1	mgpio_set(DISP_CS)

#define EPD_W21_DC_0	mgpio_reset(DISP_DC)
#define EPD_W21_DC_1	mgpio_set(DISP_DC)

#define EPD_W21_RST_0   mgpio_reset(DISP_RESET)
#define EPD_W21_RST_1   mgpio_set(DISP_RESET)

#define isEPD_W21_BUSY  mgpio_read(DISP_BUSY)

static void load_lut_full(void);
static void load_lut_partial(void);
void epd_init(void);
void epd_display_full(const unsigned char* picData);
void epd_display_partial(const unsigned char* picData);
void epd_dsleep(void);
void epd_refresh(void);
void epd_reset(void);

static uint8_t first_image;

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
	EPD_W21_RST_1;
	delay_ms(10);

	EPD_W21_RST_0;
	delay_ms(100);

	EPD_W21_RST_1;
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
	epd_write_data(value);//bit 3:Gate scan direction  bit 2:Source shift direction;
	/* What is this? */
	/* epd_write_data(0x89); */

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

void epd_display_full(const unsigned char* picData)
{
	epd_write_cmd(EPD_CMD_DTM2);
   	for(unsigned int i=0; i<IMG_BYTES; i++) {
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

void epd_display_partial(const unsigned char* picData)
{
	if(first_image) {
		/* one full refresh is necessary after boot */
		return;
	}

	epd_write_cmd(EPD_CMD_DTM2);
	for(unsigned int i=0; i<IMG_BYTES; i++) {
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

	while(isEPD_W21_BUSY == 0) {
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

void epd_write_cmd(unsigned char command)
{
	EPD_W21_DC_0;		/* command */
	mspi_write_byte(command);
	EPD_W21_DC_1;
}

void epd_write_data(unsigned char data)
{
	EPD_W21_DC_1;		/* data */
	mspi_write_byte(data);
	EPD_W21_DC_1;
}
