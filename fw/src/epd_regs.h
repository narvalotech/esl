#ifndef EPD_REGS_H_
#define EPD_REGS_H_

#define EPD_CMD_PSR 0x00
enum {
	EPD_PSR_RST_N = 0,	/* 1: reset 0: no reset */
	EPD_PSR_SHD_N,		/* 1: booster on */
	EPD_PSR_SHL,		/* 1: shift dir right */
	EPD_PSR_UD,		/* 1: scan dir up */
	EPD_PSR_KWR,		/* 1: b/w 0: b/w/r */
	EPD_PSR_REG,		/* 1: LUT from register 0: OTP */
	EPD_PSR_RES0,		/* resolution lsb */
	EPD_PSR_RES1,		/* resolution msb */
} EPD_PSR;
/* resolutions:
 * - 0b00: 96x230
 * - 0b01: 96x252
 * - 0b10: 128x296
 * - 0b11: 160x296
 */

#define EPD_CMD_PWR 0x01
enum {
	EPD_PWR_VG_EN = 0,	/* 1: internal DCDC for VSH/L */
	EPD_PWR_VS_EN,		/* 1: internal DCDC for VGH/L */
} EPD_PWR;
/* 5 bytes of data. Next 4 bytes:
 * - VGHL_LV[3:0]: 20V - [value]
 * - VSH[5:0]
 * - VSL[5:0]
 * - VDHR[5:0] (red pixels)
 * */

/* Power off sequence setting */
#define EPD_CMD_PFS 0x03
#define EPD_PFS_T 4
/* T_VDS_OFF[1:0]: t_off
 * - 0b00: 1 frame (default)
 * - 0b01: 2 frames
 * - 0b10: 3 frames
 * - 0b11: 4 frames
 */

/* Booster soft-start */
#define EPD_CMD_BTST 0x06
/* 3 bytes, see datasheet */

/* PLL control */
#define EPD_CMD_PLL 0x30
/* 1 byte, see datasheet */

#define EPD_CMD_TCON 0x60
/* 1 byte, see datasheet */

#define EPD_CMD_VCDS 0x82

/* Power saving (during refresh) */
#define EPD_CMD_PWS 0xe3

/* Temperature sensor enable */
#define EPD_CMD_TSE 0x41
#define EPD_TSE_TSE 7		/* 1: external sensor */
/* TO[3:0] temp offset in C (2s compl) */

/* Resolution setting, overrides PSR */
#define EPD_CMD_TRES 0x61
/* 1st byte:
 * - HRES[7:0] horiz res (x10)
 * 2nd + 3rd bytes:
 * - VRES[8:0] vert res
 */

/* Resolution start gate/source position */
#define EPD_CMD_GSST 0x65
/* 3 bytes, same sch as TRES */

/* VCOM and Data interval */
#define EPD_CMD_CDI 0x50
/* - CDI[3:0]
 * - DDX[1:0]
 * - VBD[1:0]
 * see datasheet
 */

#define EPD_CMD_DSLP 0x07
#define EPD_DSLP_MAGIC 0xa5

/* Data start transmission 2 */
#define EPD_CMD_DTM2 0x13

/* Auto sequence */
#define EPD_CMD_AUTO 0x17
#define EPD_CMD_ON_REFRESH_OFF 0xa5
#define EPD_CMD_ON_REFRESH_OFF_DSLEEP 0xa7

/* VCOM LUT */
#define EPD_CMD_LUTC 0x20
/* 81-byte command */

/* W2W LUT */
#define EPD_CMD_LUTWW 0x21
/* 57-byte command */

/* B2W LUT */
#define EPD_CMD_LUTKW 0x22
/* 81-byte command */

/* W2B LUT */
#define EPD_CMD_LUTWK 0x23
/* 81-byte command */

/* B2B LUT */
#define EPD_CMD_LUTK 0x24

#endif // EPD_REGS_H_
