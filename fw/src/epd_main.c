#include "epd_main.h"
#include "epd_regs.h"
#include "utils.h"

void EPD_W21_WriteCMD(unsigned char command);
void EPD_W21_WriteDATA(unsigned char data);

#define EPD_WIDTH   128UL
#define EPD_HEIGHT  250UL
#define EPD_W21_RST_0 mgpio_reset(DISP_RESET)
#define EPD_W21_RST_1 mgpio_set(DISP_RESET)
#define isEPD_W21_BUSY mgpio_read(DISP_BUSY)

const unsigned char lut_R20_GC[];	//GC
const unsigned char lut_R21_GC[];
const unsigned char lut_R22_GC[];
const unsigned char lut_R23_GC[];
const unsigned char lut_R24_GC[];
const unsigned char lut_R20_DU[];	//DU
const unsigned char lut_R21_DU[];
const unsigned char lut_R22_DU[];
const unsigned char lut_R23_DU[];
const unsigned char lut_R24_DU[];

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

	/* GPIO_Configuration(); */
	EPD_Reset();                  //
	EPD_init(); 		      //  GC
	PIC_display_GC(gImage_1);	//1
	delay_ms(300);
	while(1)
	{
		PIC_display_DU(gImage_1);     //DU1
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_2);     //DU2
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_3);     //DU3
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_1);     //DU4
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_2);     //DU5
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_3);     //DU6
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_1);     //DU7
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_2);     //DU8
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_3);     //DU9
		delay_ms(IMG_DELAY_MS);
		PIC_display_DU(gImage_1);     //DU10
		delay_ms(IMG_DELAY_MS);
		PIC_display_GC(gImage_2);     //GC110
		delay_ms(IMG_DELAY_MS);

		EPD_sleep();	//
		delay_ms(300);
		//
		EPD_Reset();                //
		//
		EPD_init();
		//GC
		PIC_display_GC(gImage_3);   //EPD_picture
		delay_ms(IMG_DELAY_MS);
	}

}

/*GPIO*/
/* void GPIO_Configuration(void) */
/* { */
/*   GPIO_InitTypeDef GPIO_InitStructure; */

/*   RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE,ENABLE); */
/* //============================================================================= */
/* //LED -> PE12   D/C-->PE15	RST -- PE14 */
/* //============================================================================= */
/*   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_15 |GPIO_Pin_14; */
/*   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; */
/*   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; */
/*   GPIO_Init(GPIOE, &GPIO_InitStructure); */

/* //SDA--->PD10  SCK-->PD9  CS-->PD8 */
/*   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10;		//Port configuration */
/*   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; */
/*   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; */
/*   GPIO_Init(GPIOD, &GPIO_InitStructure); */

/* //BUSY--->PE13     KEY -- PE11 */
/*   GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_11|GPIO_Pin_13 ; */
/*   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;		//busy,busy */
/*   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz; */
/*   GPIO_Init(GPIOE, &GPIO_InitStructure);				//Initialize GPIO */

/* } */


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

	/* uses default value, TODO: remove */
	EPD_W21_WriteCMD(EPD_CMD_PFS);
	EPD_W21_WriteDATA(0 << EPD_PFS_T);

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

	/* uses default value, TODO: remove */
	EPD_W21_WriteCMD(EPD_CMD_PLL);
	EPD_W21_WriteDATA(0x09); /* 50Hz */

	/* uses default value, TODO: remove */
	EPD_W21_WriteCMD(EPD_CMD_TCON);
	EPD_W21_WriteDATA(0x22);

	/* might not be necessary */
	EPD_W21_WriteCMD(EPD_CMD_VCDS);
	EPD_W21_WriteDATA(0x00);

	/* might not be necessary */
	EPD_W21_WriteCMD(EPD_CMD_PWS);
	EPD_W21_WriteDATA(0x00);

	/* uses default value, TODO: remove */
	EPD_W21_WriteCMD(EPD_CMD_TSE);
	EPD_W21_WriteDATA(0x00);

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
const unsigned char lut_R20_GC[] = // VCOM 108=80bytes
{
0x01,	0x00,	0x14,	0x14,	0x01,	0x01,	0x00,	0x01,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
const unsigned char  lut_R21_GC[] = // W2W 78=56bytes
{
0x01,	0x60,	0X14,	0x14,	0x01,	0x01,	0x00,	0x01,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};

const unsigned char  lut_R22_GC[] = // B2W 108=80bytes
{
0x01,	0x60,	0X14,	0X14,	0x01,	0x01,	0x00,	0x01,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
const unsigned char  lut_R23_GC[] = // W2B 108=80bytes
{
0x01,	0x90,	0X14,	0X14,	0x01,	0x01,	0x00,	0x01,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
const unsigned char  lut_R24_GC[] = // B2B 108=80bytes
{
0x01,	0x90,	0X14,	0X14,	0x01,	0x01,	0x00,	0x01,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};


// DU 300ms
const unsigned char lut_R20_DU[] = // VCOM 108=80bytes
{
0x01,	0x00,	0X14,	0x01,	0x01,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
const unsigned char  lut_R21_DU[] = // W2W 78=56bytes
{
0x01,	0x20,	0X14,	0x01,	0x01,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
const unsigned char  lut_R22_DU[] = // B2W 108=80bytes
{
0x01,	0x80,	0X14,	0x01,	0x01,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
const unsigned char  lut_R23_DU[] = // W2B 108=80bytes
{
0x01,	0x40,	0X14,	0x01,	0x01,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
const unsigned char  lut_R24_DU[] = // B2B 108=80bytes
{
0x01,	0x00,	0X14,	0x01,	0x01,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,	0x00,
};
