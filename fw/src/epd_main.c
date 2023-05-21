#include "epd_main.h"
#include "epd_lut.h"
#include "epd_regs.h"
#include "utils.h"

void EPD_W21_WriteCMD(unsigned char command);
void EPD_W21_WriteDATA(unsigned char data);

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

static void lut_GC(void);
static void lut_DU(void);
void EPD_init(void);
void PIC_display_GC(const unsigned char* picData);
void PIC_display_DU(const unsigned char* picData);
void PIC_display(unsigned char NUM);
void EPD_sleep(void);
void EPD_refresh(void);
void lcd_chkstatus(void);
void EPD_Reset(void);

static uint8_t first_image = 1;

/* using on-board LUTs doesn't work */
/* #define LUT_OTP */

#define IMG_DELAY_MS 500
int epd_main(void)
{
	EPD_Reset();
	EPD_init();
	PIC_display_GC(gImage_1);
	delay_ms(300);
	while(1)
	{
		for (int i=0; i<3; i++) {
			PIC_display_DU(gImage_1);
			delay_ms(IMG_DELAY_MS);
			PIC_display_DU(gImage_2);
			delay_ms(IMG_DELAY_MS);
			PIC_display_DU(gImage_3);
			delay_ms(IMG_DELAY_MS);
		}

		/* Last cycle can only display 2
		 * TODO: test the limit of partial-refreshes */
		PIC_display_DU(gImage_1);
		delay_ms(IMG_DELAY_MS);
		PIC_display_GC(gImage_2);
		delay_ms(IMG_DELAY_MS);

		EPD_sleep();
		delay_ms(300);
		EPD_Reset();
		EPD_init();

		PIC_display_GC(gImage_3);
		delay_ms(IMG_DELAY_MS);
	}
}

void EPD_Reset(void)
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

	EPD_W21_WriteCMD(EPD_CMD_CDI);
	EPD_W21_WriteDATA(val);
}

void EPD_init(void)
{
	uint8_t value;

	first_image = 1;

	EPD_W21_WriteCMD(EPD_CMD_PSR);
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
	EPD_W21_WriteDATA(value);//bit 3:Gate scan direction  bit 2:Source shift direction;
	/* What is this? */
	/* EPD_W21_WriteDATA(0x89); */

	EPD_W21_WriteCMD(EPD_CMD_PWR);
	EPD_W21_WriteDATA(BIT(EPD_PWR_VG_EN) | BIT(EPD_PWR_VS_EN));
	EPD_W21_WriteDATA(0x00); /* 20V (default) */
	EPD_W21_WriteDATA(0x3F); /* 15V (default) */
	EPD_W21_WriteDATA(0x3F); /* 15V (default) */
	EPD_W21_WriteDATA(0x03); /* 3V (value shouldn't matter) */

	EPD_W21_WriteCMD(EPD_CMD_BTST);
	/* phase A & B:
	 * - 6.58uS off time
	 * - strength 5
	 * - 40ms soft-start
	 */
	EPD_W21_WriteDATA(0x27);
	EPD_W21_WriteDATA(0x27);
	/* phase C:
	 * - 6.58us off time
	 * - strength 6
	 */
	EPD_W21_WriteDATA(0x2F);

	EPD_W21_WriteCMD(EPD_CMD_TRES);
	EPD_W21_WriteDATA(EPD_WIDTH);
	EPD_W21_WriteDATA(0);
	EPD_W21_WriteDATA(EPD_HEIGHT);

	EPD_W21_WriteCMD(EPD_CMD_GSST);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);
	EPD_W21_WriteDATA(0x00);

	write_cdi(true);
}

void PIC_display_GC(const unsigned char* picData)
{
	EPD_W21_WriteCMD(EPD_CMD_DTM2);
   	for(unsigned int i=0; i<IMG_BYTES; i++) {
  		EPD_W21_WriteDATA(*picData);
   		picData++;
   	}

	lut_GC();
	EPD_refresh();
	write_cdi(false);

	if(first_image) {
		first_image = 0;
	}
}

void PIC_display_DU(const unsigned char* picData)
{
	if(first_image) {
		/* one full refresh is necessary after boot */
		return;
	}

	EPD_W21_WriteCMD(EPD_CMD_DTM2);
	for(unsigned int i=0; i<IMG_BYTES; i++) {
		EPD_W21_WriteDATA(*picData);
		picData++;
	}

	lut_DU();
	EPD_refresh();
	write_cdi(false);
}

void EPD_refresh(void)
{
	EPD_W21_WriteCMD(EPD_CMD_AUTO);
	EPD_W21_WriteDATA(EPD_CMD_ON_REFRESH_OFF);

	while(isEPD_W21_BUSY == 0) {
		/* around 500ms in partial refresh, >1s for full refresh */
		k_msleep(10);
	}
}

void EPD_sleep(void)
{
	EPD_W21_WriteCMD (EPD_CMD_DSLP);
	EPD_W21_WriteDATA(EPD_DSLP_MAGIC);
}

static void write_lut(uint8_t const **p_lut)
{
	static bool swap = false;
	uint8_t LUT_LEN = 56;

#ifdef LUT_OTP
	return;
#endif

	EPD_W21_WriteCMD(EPD_CMD_LUTC);
	for (uint16_t i = 0; i < LUT_LEN; i++) EPD_W21_WriteDATA(p_lut[0][i]);

	EPD_W21_WriteCMD(EPD_CMD_LUTWW);
	for (uint16_t i = 0; i < LUT_LEN; i++) EPD_W21_WriteDATA(p_lut[1][i]);

	EPD_W21_WriteCMD(EPD_CMD_LUTK);
	for (uint16_t i = 0; i < LUT_LEN; i++) EPD_W21_WriteDATA(p_lut[4][i]);

	if (!swap || first_image) {
		swap = true;

		EPD_W21_WriteCMD(EPD_CMD_LUTKW);
		for (uint16_t i = 0; i < LUT_LEN; i++) EPD_W21_WriteDATA(p_lut[2][i]);

		EPD_W21_WriteCMD(EPD_CMD_LUTWK);
		for (uint16_t i = 0; i < LUT_LEN; i++) EPD_W21_WriteDATA(p_lut[3][i]);
	} else {
		swap = false;

		EPD_W21_WriteCMD(EPD_CMD_LUTWK);
		for (uint16_t i = 0; i < LUT_LEN; i++) EPD_W21_WriteDATA(p_lut[2][i]);

		EPD_W21_WriteCMD(EPD_CMD_LUTKW);
		for (uint16_t i = 0; i < LUT_LEN; i++) EPD_W21_WriteDATA(p_lut[3][i]);
	}
}

static void lut_GC(void)
{
	uint8_t const *p_lut_gc[5] = {lut_R20_GC, lut_R21_GC, lut_R22_GC, lut_R23_GC, lut_R24_GC};
	write_lut(p_lut_gc);
}

static void lut_DU(void)
{
	uint8_t const *p_lut_du[5] = {lut_R20_DU, lut_R21_DU, lut_R22_DU, lut_R23_DU, lut_R24_DU};
	write_lut(p_lut_du);
}

void EPD_W21_WriteCMD(unsigned char command)
{
  	EPD_W21_CS_0;
	EPD_W21_DC_0;		/* command */
	mspi_write_byte(command);
	EPD_W21_CS_1;
	EPD_W21_DC_1;
}

void EPD_W21_WriteDATA(unsigned char data)
{
  	EPD_W21_CS_0;
	EPD_W21_DC_1;		/* data */
	mspi_write_byte(data);
	EPD_W21_CS_1;
	EPD_W21_DC_1;
}
