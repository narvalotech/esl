#include "epd_main.h"
#include "epd_lut.h"
#include "epd_regs.h"
#include "utils.h"

void EPD_W21_WriteCMD(unsigned char command);
void EPD_W21_WriteDATA(unsigned char data);

#define EPD_WIDTH   128UL
#define EPD_HEIGHT  250UL
#define EPD_W21_RST_0 mgpio_reset(DISP_RESET)
#define EPD_W21_RST_1 mgpio_set(DISP_RESET)
#define isEPD_W21_BUSY mgpio_read(DISP_BUSY)

static void lut_GC(void);//
static void lut_DU(void);//
void EPD_init(void);//
void PIC_display_GC(const unsigned char* picData);//
void PIC_display_DU(const unsigned char* picData);//
void PIC_display(unsigned char NUM);//
void EPD_sleep(void);//
void EPD_refresh(void);//
void lcd_chkstatus(void);//
void EPD_Reset(void);//

static u8 lut_flag = 0;	//
static u8 boot_flag = 0;//

/* using on-board LUTs doesn't work */
/* #define LUT_OTP */

/*********************************************************************************************

 -->  -->  --> ... --> 10DUGC,
EPD_WIDTH*EPD_HEIGHT/8byte1bit1
*********************************************************************************************/
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

/**/
void EPD_Reset(void)
{
	EPD_W21_RST_1;
	delay_ms(10);	//At least 10ms delay
	EPD_W21_RST_0;		// Module reset
	delay_ms(100);//At least 10ms delay
	EPD_W21_RST_1;
	delay_ms(100);	//At least 10ms delay
}


/**/
void EPD_init(void)
{
	uint8_t value;

	lut_flag = 0;
	boot_flag = 0;

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

	EPD_W21_WriteCMD(EPD_CMD_CDI);
	EPD_W21_WriteDATA(0xB7);
}

/**/
void PIC_display_GC(const unsigned char* picData)
{
	unsigned int i;
	if(!boot_flag)
	{
		boot_flag = 1;
	}
	else
	{
		EPD_W21_WriteCMD(EPD_CMD_CDI);
		EPD_W21_WriteDATA(0xD7);
	}
	EPD_W21_WriteCMD(EPD_CMD_DTM2);  //Start data transfer
   	for(i=0;i<4000;i++)
   	{
  		EPD_W21_WriteDATA(*picData);  //Transfer the actual displayed data
   		picData++;
   	}
	lut_GC();
	EPD_refresh();
}


/**/
void PIC_display_DU(const unsigned char* picData)
{
	unsigned int i;
	if(!boot_flag)
	{
		return ;//GC
	}
	else
	{
		EPD_W21_WriteCMD(EPD_CMD_CDI);
		EPD_W21_WriteDATA(0xD7);
	}

	EPD_W21_WriteCMD(EPD_CMD_DTM2);	  //Transfer new data
	for(i=0;i<EPD_WIDTH*EPD_HEIGHT/8;i++)
	{
		EPD_W21_WriteDATA(*picData);  //Transfer the actual displayed data
		picData++;
	}
	lut_DU();     		 //Transfer wavefrom data
	EPD_refresh();		//DISPLAY REFRESH
}

//
/* void PIC_display(unsigned char NUM) */
/* { */

/* 	unsigned int row, column; */
/*   	if(!boot_flag) */
/* 	{ */
/* 		boot_flag = 1; */
/* 	} */
/* 	else */
/* 	{ */
/* 		EPD_W21_WriteCMD(0X50); */
/* 		EPD_W21_WriteDATA(0xD7); */
/* 	} */

/* 	EPD_W21_WriteCMD(0x13);		     //Transfer new data */
/* 	for(column=0; column<EPD_HEIGHT; column++) */
/* 	{ */
/* 		for(row=0; row<EPD_WIDTH/8; row++) */
/* 		{ */
/* 			/\*10*\/ */
/* 			switch (NUM) */
/* 			{ */
/* 				case PIC_WHITE: */
/* 				EPD_W21_WriteDATA(0xFF); */
/* 				break; */

/* 				case PIC_BLACK: */
/* 				EPD_W21_WriteDATA(0x00); */
/* 				break; */
/* 			} */
/* 		} */
/* 	} */
/* 	lut_GC(); */
/* 	EPD_refresh(); */
/* } */

/**/
void EPD_refresh(void)
{
	EPD_W21_WriteCMD(EPD_CMD_AUTO);
	EPD_W21_WriteDATA(EPD_CMD_ON_REFRESH_OFF);

	while(isEPD_W21_BUSY == 0) {
		/* around 500ms in partial refresh, >1s for full refresh */
		k_msleep(10);
	}
}

/**/
void EPD_sleep(void)
{
	EPD_W21_WriteCMD (EPD_CMD_DSLP);
	EPD_W21_WriteDATA(EPD_DSLP_MAGIC);
}

//LUT download
static void lut_GC(void)
{
#ifdef LUT_OTP
	return;
#endif
	u8 count;
	EPD_W21_WriteCMD(EPD_CMD_LUTC);
	for(count=0;count<56;count++)
		{EPD_W21_WriteDATA(lut_R20_GC[count]);}

	EPD_W21_WriteCMD(EPD_CMD_LUTWW);
	for(count=0;count<56;count++)
		{EPD_W21_WriteDATA(lut_R21_GC[count]);}

	EPD_W21_WriteCMD(EPD_CMD_LUTK);
	for(count=0;count<56;count++)
		{EPD_W21_WriteDATA(lut_R24_GC[count]);}
	if(lut_flag == 0)
	{
		EPD_W21_WriteCMD(EPD_CMD_LUTKW);
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R22_GC[count]);}

		EPD_W21_WriteCMD(EPD_CMD_LUTWK);
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R23_GC[count]);}
		lut_flag=1;
	}
	else
	{
		EPD_W21_WriteCMD(EPD_CMD_LUTKW);
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R23_GC[count]);}

		EPD_W21_WriteCMD(EPD_CMD_LUTWK);							//wb w
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R22_GC[count]);}
		lut_flag=0;
	}
}

//LUT download
static void lut_DU(void)
{
#ifdef LUT_OTP
	return;
#endif
	u8 count;
	EPD_W21_WriteCMD(EPD_CMD_LUTC);
	for(count=0;count<56;count++)
		{EPD_W21_WriteDATA(lut_R20_DU[count]);}

	EPD_W21_WriteCMD(EPD_CMD_LUTWW);
	for(count=0;count<56;count++)
		{EPD_W21_WriteDATA(lut_R21_DU[count]);}

	EPD_W21_WriteCMD(EPD_CMD_LUTK);
	for(count=0;count<56;count++)
		{EPD_W21_WriteDATA(lut_R24_DU[count]);}

	if (lut_flag==0)
	{
		EPD_W21_WriteCMD(EPD_CMD_LUTKW);
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R22_DU[count]);}

		EPD_W21_WriteCMD(EPD_CMD_LUTWK);
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R23_DU[count]);}
		lut_flag=1;
	}
	else
	{
		EPD_W21_WriteCMD(EPD_CMD_LUTKW);
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R23_DU[count]);}

		EPD_W21_WriteCMD(EPD_CMD_LUTWK);
		for(count=0;count<56;count++)
			{EPD_W21_WriteDATA(lut_R22_DU[count]);}
		lut_flag=0;
	}
}


/*************************SPI***********************************************************/

#define EPD_W21_MOSI_0	mgpio_reset(DISP_MOSI)
#define EPD_W21_MOSI_1	mgpio_set(DISP_MOSI)

#define EPD_W21_CLK_0	mgpio_reset(DISP_SCK)
#define EPD_W21_CLK_1	mgpio_set(DISP_SCK)

#define EPD_W21_CS_0	mgpio_reset(DISP_CS)
#define EPD_W21_CS_1	mgpio_set(DISP_CS)

#define EPD_W21_DC_0	mgpio_reset(DISP_DC)
#define EPD_W21_DC_1	mgpio_set(DISP_DC)

//SPI
void SPI_Write(unsigned char value)
{
	mspi_write_byte(value);
	/* unsigned char i; */

	/* for(i=0; i<8; i++) */
	/* { */
	/* 	EPD_W21_CLK_0; */

	/* 	if(value & 0x80) */
	/* 		EPD_W21_MOSI_1; */
	/* 	else */
	/* 		EPD_W21_MOSI_0; */
	/* 	value = (value << 1); */
	/* 	EPD_W21_CLK_1; */

	/* } */
}
/*SPI*/
void EPD_W21_WriteCMD(unsigned char command)
{

  	EPD_W21_CS_0;
	EPD_W21_DC_0;		// command write
	SPI_Write(command);
	EPD_W21_CS_1;
	EPD_W21_DC_1;		//
}
/*SPI*/
void EPD_W21_WriteDATA(unsigned char data)
{

	EPD_W21_MOSI_0;
  	EPD_W21_CS_0;
	EPD_W21_DC_1;		// data write
	SPI_Write(data);
	EPD_W21_CS_1;
	EPD_W21_DC_1;		//
	EPD_W21_MOSI_0;
}

/**********************************

	u8 data = 0;
	EPD_W21_WriteCMD(0x71);	//status bit read
	data = EPD_read();	//
**********************************/
/* unsigned char EPD_read(void) */
/* { */
/* 	unsigned char i; */
/* 	unsigned char DATA_BUF; */

/* 	EPD_W21_CS_0; */
/*       delay_us(5); */
/* 	SDA_IN();			// */
/* 	EPD_W21_DC_1;		// data read */
/*       delay_us(5); */
/* 	EPD_W21_CLK_0; */
/* 	delay_us(5); */
/*       for(i=0;i<8;i++ ) */
/* 	{ */
/* 		DATA_BUF=DATA_BUF<<1; */
/* 		DATA_BUF|=READ_SDA;	//READ_SDA */
/* 		EPD_W21_CLK_1; */
/* 		delay_us(5); */
/* 		EPD_W21_CLK_0; */
/*         	delay_us(5); */
/*       } */
/*       delay_us(5); */
/* 	EPD_W21_CS_1; */
/* 	EPD_W21_DC_1; */
/* 	SDA_OUT();		// */
/*       return DATA_BUF; */
/* } */

/***************************************************************************************///GC
