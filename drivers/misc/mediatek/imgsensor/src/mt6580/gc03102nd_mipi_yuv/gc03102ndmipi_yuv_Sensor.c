/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of MediaTek Inc. (C) 2005
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 *  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
 *  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
 *
 *****************************************************************************/

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gc0310yuv_Sensor.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *   V1.1.0
 *
 * Author:
 * -------
 *   travis
 *
 *=============================================================
 *             HISTORY
 * Below this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 *   
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *=============================================================
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "gc03102ndmipi_yuv_Sensor.h"
#include "gc03102ndmipi_yuv_Camera_Sensor_para.h"
#include "gc03102ndmipi_yuv_CameraCustomized.h"

#include <mach/gpio_const.h>
#include <mt-plat/mt_gpio.h>


//#define GC03102NDMIPIYUV_DEBUG

#ifdef GC03102NDMIPIYUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

//#define DEBUG_SENSOR_GC03102NDMIPI//T_flash Tuning
extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

static void GC03102NDMIPI_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
   char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};	
	 iWriteRegI2C(puSendCmd , 2, GC03102NDMIPI_WRITE_ID);
}

static kal_uint16 GC03102NDMIPI_read_cmos_sensor(kal_uint8 addr)
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, GC03102NDMIPI_READ_ID);
	
  return get_byte;
}


#ifdef DEBUG_SENSOR_GC03102NDMIPI
#define GC03102NDMIPI_OP_CODE_INI		0x00		/* Initial value. */
#define GC03102NDMIPI_OP_CODE_REG		0x01		/* Register */
#define GC03102NDMIPI_OP_CODE_DLY		0x02		/* Delay */
#define GC03102NDMIPI_OP_CODE_END	0x03		/* End of initial setting. */
static kal_uint16 fromsd;

typedef struct
{
	u16 init_reg;
	u16 init_val;	/* Save the register value and delay tick */
	u8 op_code;		/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
} GC03102NDMIPI_initial_set_struct;

GC03102NDMIPI_initial_set_struct GC03102NDMIPI_Init_Reg[5000];

static u32 strtol(const char *nptr, u8 base)
{

	printk("GC03102NDMIPI___%s____\n",__func__); 

	u8 ret;
	if(!nptr || (base!=16 && base!=10 && base!=8))
	{
		printk("GC03102NDMIPI %s(): NULL pointer input\n", __FUNCTION__);
		return -1;
	}
	for(ret=0; *nptr; nptr++)
	{
		if((base==16 && *nptr>='A' && *nptr<='F') || 
				(base==16 && *nptr>='a' && *nptr<='f') || 
				(base>=10 && *nptr>='0' && *nptr<='9') ||
				(base>=8 && *nptr>='0' && *nptr<='7') )
		{
			ret *= base;
			if(base==16 && *nptr>='A' && *nptr<='F')
				ret += *nptr-'A'+10;
			else if(base==16 && *nptr>='a' && *nptr<='f')
				ret += *nptr-'a'+10;
			else if(base>=10 && *nptr>='0' && *nptr<='9')
				ret += *nptr-'0';
			else if(base>=8 && *nptr>='0' && *nptr<='7')
				ret += *nptr-'0';
		}
		else
			return ret;
	}
	return ret;
}

static u8 GC03102NDMIPI_Initialize_from_T_Flash()
{
	//FS_HANDLE fp = -1;				/* Default, no file opened. */
	//u8 *data_buff = NULL;
	u8 *curr_ptr = NULL;
	u32 file_size = 0;
	//u32 bytes_read = 0;
	u32 i = 0, j = 0;
	u8 func_ind[4] = {0};	/* REG or DLY */

	printk("GC03102NDMIPI ___%s____ start\n",__func__); 

	struct file *fp; 
	mm_segment_t fs; 
	loff_t pos = 0; 
	static u8 data_buff[10*1024] ;

	fp = filp_open("/mnt/sdcard/GC03102NDMIPI_sd.txt", O_RDONLY , 0); 
	if (IS_ERR(fp)) 
	{ 
		printk("0310MIPI create file error,IS_ERR(fp)=%s \n",IS_ERR(fp));  
		return -1; 
	} 
	else
	{
		printk("0310MIPI create file error,IS_ERR(fp)=%s \n",IS_ERR(fp));  
	}
	fs = get_fs(); 
	set_fs(KERNEL_DS); 

	file_size = vfs_llseek(fp, 0, SEEK_END);
	vfs_read(fp, data_buff, file_size, &pos); 
	//printk("%s %d %d\n", buf,iFileLen,pos); 
	filp_close(fp, NULL); 
	set_fs(fs);

	/* Start parse the setting witch read from t-flash. */
	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;				

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}

		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}
		//printk(" curr_ptr1 = %s\n",curr_ptr);
		memcpy(func_ind, curr_ptr, 3);


		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			curr_ptr += 6;				/* Skip "REG(0x" or "DLY(" */
			GC03102NDMIPI_Init_Reg[i].op_code = GC03102NDMIPI_OP_CODE_REG;

			GC03102NDMIPI_Init_Reg[i].init_reg = strtol((const char *)curr_ptr, 16);
			curr_ptr += 5;	/* Skip "00, 0x" */

			GC03102NDMIPI_Init_Reg[i].init_val = strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */

		}
		else									/* DLY */
		{
			/* Need add delay for this setting. */ 
			curr_ptr += 4;	
			GC03102NDMIPI_Init_Reg[i].op_code = GC03102NDMIPI_OP_CODE_DLY;

			GC03102NDMIPI_Init_Reg[i].init_reg = 0xFF;
			GC03102NDMIPI_Init_Reg[i].init_val = strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
		}
		i++;


		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;
	}

	/* (0xFFFF, 0xFFFF) means the end of initial setting. */
	GC03102NDMIPI_Init_Reg[i].op_code = GC03102NDMIPI_OP_CODE_END;
	GC03102NDMIPI_Init_Reg[i].init_reg = 0xFF;
	GC03102NDMIPI_Init_Reg[i].init_val = 0xFF;
	i++;
	//for (j=0; j<i; j++)
	printk("GC03102NDMIPI %x  ==  %x\n",GC03102NDMIPI_Init_Reg[j].init_reg, GC03102NDMIPI_Init_Reg[j].init_val);

	/* Start apply the initial setting to sensor. */
#if 1
	for (j=0; j<i; j++)
	{
		if (GC03102NDMIPI_Init_Reg[j].op_code == GC03102NDMIPI_OP_CODE_END)	/* End of the setting. */
		{
			printk("GC03102NDMIPI REG OK -----------------END!\n");
		
			break ;
		}
		else if (GC03102NDMIPI_Init_Reg[j].op_code == GC03102NDMIPI_OP_CODE_DLY)
		{
			msleep(GC03102NDMIPI_Init_Reg[j].init_val);		/* Delay */
			printk("GC03102NDMIPI REG OK -----------------DLY!\n");			
		}
		else if (GC03102NDMIPI_Init_Reg[j].op_code == GC03102NDMIPI_OP_CODE_REG)
		{
			GC03102NDMIPI_write_cmos_sensor(GC03102NDMIPI_Init_Reg[j].init_reg, GC03102NDMIPI_Init_Reg[j].init_val);
			printk("GC03102NDMIPI REG OK!--------1--------REG(0x%x,0x%x)\n",GC03102NDMIPI_Init_Reg[j].init_reg, GC03102NDMIPI_Init_Reg[j].init_val);			
			printk("GC03102NDMIPI REG OK!--------2--------REG(0x%x,0x%x)\n",GC03102NDMIPI_Init_Reg[j].init_reg, GC03102NDMIPI_Init_Reg[j].init_val);			
			printk("GC03102NDMIPI REG OK!--------3--------REG(0x%x,0x%x)\n",GC03102NDMIPI_Init_Reg[j].init_reg, GC03102NDMIPI_Init_Reg[j].init_val);			
		}
		else
		{
			printk("GC03102NDMIPI REG ERROR!\n");
		}
	}
#endif
	printk("GC03102NDMIPI ___%s____ end\n",__func__); 
	return 1;	
}

#endif

/*******************************************************************************
 * // Adapter for Winmo typedef
 ********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool   GC03102NDMIPI_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 GC03102NDMIPI_dummy_pixels = 0, GC03102NDMIPI_dummy_lines = 0;
kal_bool   GC03102NDMIPI_MODE_CAPTURE = KAL_FALSE;
kal_bool   GC03102NDMIPI_NIGHT_MODE = KAL_FALSE;

kal_uint32 GC03102NDMIPI_isp_master_clock;

kal_uint8 GC03102NDMIPI_sensor_write_I2C_address = GC03102NDMIPI_WRITE_ID;
kal_uint8 GC03102NDMIPI_sensor_read_I2C_address = GC03102NDMIPI_READ_ID;

UINT8 GC03102NDMIPIPixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT GC03102NDMIPISensorConfigData;

#define GC03102NDMIPI_SET_PAGE0 	GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00)
#define GC03102NDMIPI_SET_PAGE1 	GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01)
#define GC03102NDMIPI_SET_PAGE2 	GC03102NDMIPI_write_cmos_sensor(0xfe, 0x02)
#define GC03102NDMIPI_SET_PAGE3 	GC03102NDMIPI_write_cmos_sensor(0xfe, 0x03)

/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPI_SetShutter
 *
 * DESCRIPTION
 *	This function set e-shutter of GC03102NDMIPI to change exposure time.
 *
 * PARAMETERS
 *   iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC03102NDMIPI_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_GC03102NDMIPI_Shutter */


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPI_read_Shutter
 *
 * DESCRIPTION
 *	This function read e-shutter of GC03102NDMIPI .
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC03102NDMIPI_Read_Shutter(void)
{
    	kal_uint8 temp_reg1, temp_reg2;
	kal_uint16 shutter;

	temp_reg1 = GC03102NDMIPI_read_cmos_sensor(0x04);
	temp_reg2 = GC03102NDMIPI_read_cmos_sensor(0x03);

	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

	return shutter;
} /* GC03102NDMIPI_read_shutter */


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPI_write_reg
 *
 * DESCRIPTION
 *	This function set the register of GC03102NDMIPI.
 *
 * PARAMETERS
 *	addr : the register index of GC03102NDMIPI
 *  para : setting parameter of the specified register of GC03102NDMIPI
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC03102NDMIPI_write_reg(kal_uint32 addr, kal_uint32 para)
{
	GC03102NDMIPI_write_cmos_sensor(addr, para);
} /* GC03102NDMIPI_write_reg() */


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPI_read_cmos_sensor
 *
 * DESCRIPTION
 *	This function read parameter of specified register from GC03102NDMIPI.
 *
 * PARAMETERS
 *	addr : the register index of GC03102NDMIPI
 *
 * RETURNS
 *	the data that read from GC03102NDMIPI
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 GC03102NDMIPI_read_reg(kal_uint32 addr)
{
	return GC03102NDMIPI_read_cmos_sensor(addr);
} /* OV7670_read_reg() */


/*************************************************************************
* FUNCTION
*	GC03102NDMIPI_awb_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/
static void GC03102NDMIPI_awb_enable(kal_bool enalbe)
{	 
	kal_uint16 temp_AWB_reg = 0;

	temp_AWB_reg = GC03102NDMIPI_read_cmos_sensor(0x42);
	
	if (enalbe)
	{
		GC03102NDMIPI_write_cmos_sensor(0x42, (temp_AWB_reg |0x02));
	}
	else
	{
		GC03102NDMIPI_write_cmos_sensor(0x42, (temp_AWB_reg & (~0x02)));
	}

}


/*************************************************************************
* FUNCTION
*	GC03102NDMIPI_GAMMA_Select
*
* DESCRIPTION
*	This function is served for FAE to select the appropriate GAMMA curve.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void GC03102NDMIPIGammaSelect(kal_uint32 GammaLvl)
{
	switch(GammaLvl)
	{
		case GC03102NDMIPI_RGB_Gamma_m1:						//smallest gamma curve
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
			GC03102NDMIPI_write_cmos_sensor(0xbf, 0x06);
			GC03102NDMIPI_write_cmos_sensor(0xc0, 0x12);
			GC03102NDMIPI_write_cmos_sensor(0xc1, 0x22);
			GC03102NDMIPI_write_cmos_sensor(0xc2, 0x35);
			GC03102NDMIPI_write_cmos_sensor(0xc3, 0x4b);
			GC03102NDMIPI_write_cmos_sensor(0xc4, 0x5f);
			GC03102NDMIPI_write_cmos_sensor(0xc5, 0x72);
			GC03102NDMIPI_write_cmos_sensor(0xc6, 0x8d);
			GC03102NDMIPI_write_cmos_sensor(0xc7, 0xa4);
			GC03102NDMIPI_write_cmos_sensor(0xc8, 0xb8);
			GC03102NDMIPI_write_cmos_sensor(0xc9, 0xc8);
			GC03102NDMIPI_write_cmos_sensor(0xca, 0xd4);
			GC03102NDMIPI_write_cmos_sensor(0xcb, 0xde);
			GC03102NDMIPI_write_cmos_sensor(0xcc, 0xe6);
			GC03102NDMIPI_write_cmos_sensor(0xcd, 0xf1);
			GC03102NDMIPI_write_cmos_sensor(0xce, 0xf8);
			GC03102NDMIPI_write_cmos_sensor(0xcf, 0xfd);
			break;
		case GC03102NDMIPI_RGB_Gamma_m2:
			GC03102NDMIPI_write_cmos_sensor(0xBF, 0x08);
			GC03102NDMIPI_write_cmos_sensor(0xc0, 0x0F);
			GC03102NDMIPI_write_cmos_sensor(0xc1, 0x21);
			GC03102NDMIPI_write_cmos_sensor(0xc2, 0x32);
			GC03102NDMIPI_write_cmos_sensor(0xc3, 0x43);
			GC03102NDMIPI_write_cmos_sensor(0xc4, 0x50);
			GC03102NDMIPI_write_cmos_sensor(0xc5, 0x5E);
			GC03102NDMIPI_write_cmos_sensor(0xc6, 0x78);
			GC03102NDMIPI_write_cmos_sensor(0xc7, 0x90);
			GC03102NDMIPI_write_cmos_sensor(0xc8, 0xA6);
			GC03102NDMIPI_write_cmos_sensor(0xc9, 0xB9);
			GC03102NDMIPI_write_cmos_sensor(0xcA, 0xC9);
			GC03102NDMIPI_write_cmos_sensor(0xcB, 0xD6);
			GC03102NDMIPI_write_cmos_sensor(0xcC, 0xE0);
			GC03102NDMIPI_write_cmos_sensor(0xcD, 0xEE);
			GC03102NDMIPI_write_cmos_sensor(0xcE, 0xF8);
			GC03102NDMIPI_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC03102NDMIPI_RGB_Gamma_m3:			
			GC03102NDMIPI_write_cmos_sensor(0xbf , 0x0b);
			GC03102NDMIPI_write_cmos_sensor(0xc0 , 0x17);
			GC03102NDMIPI_write_cmos_sensor(0xc1 , 0x2a);
			GC03102NDMIPI_write_cmos_sensor(0xc2 , 0x41);
			GC03102NDMIPI_write_cmos_sensor(0xc3 , 0x54);
			GC03102NDMIPI_write_cmos_sensor(0xc4 , 0x66);
			GC03102NDMIPI_write_cmos_sensor(0xc5 , 0x74);
			GC03102NDMIPI_write_cmos_sensor(0xc6 , 0x8c);
			GC03102NDMIPI_write_cmos_sensor(0xc7 , 0xa3);
			GC03102NDMIPI_write_cmos_sensor(0xc8 , 0xb5);
			GC03102NDMIPI_write_cmos_sensor(0xc9 , 0xc4);
			GC03102NDMIPI_write_cmos_sensor(0xca , 0xd0);
			GC03102NDMIPI_write_cmos_sensor(0xcb , 0xdb);
			GC03102NDMIPI_write_cmos_sensor(0xcc , 0xe5);
			GC03102NDMIPI_write_cmos_sensor(0xcd , 0xf0);
			GC03102NDMIPI_write_cmos_sensor(0xce , 0xf7);
			GC03102NDMIPI_write_cmos_sensor(0xcf , 0xff);
			break;
			
		case GC03102NDMIPI_RGB_Gamma_m4:
			GC03102NDMIPI_write_cmos_sensor(0xBF, 0x0E);
			GC03102NDMIPI_write_cmos_sensor(0xc0, 0x1C);
			GC03102NDMIPI_write_cmos_sensor(0xc1, 0x34);
			GC03102NDMIPI_write_cmos_sensor(0xc2, 0x48);
			GC03102NDMIPI_write_cmos_sensor(0xc3, 0x5A);
			GC03102NDMIPI_write_cmos_sensor(0xc4, 0x6B);
			GC03102NDMIPI_write_cmos_sensor(0xc5, 0x7B);
			GC03102NDMIPI_write_cmos_sensor(0xc6, 0x95);
			GC03102NDMIPI_write_cmos_sensor(0xc7, 0xAB);
			GC03102NDMIPI_write_cmos_sensor(0xc8, 0xBF);
			GC03102NDMIPI_write_cmos_sensor(0xc9, 0xCE);
			GC03102NDMIPI_write_cmos_sensor(0xcA, 0xD9);
			GC03102NDMIPI_write_cmos_sensor(0xcB, 0xE4);
			GC03102NDMIPI_write_cmos_sensor(0xcC, 0xEC);
			GC03102NDMIPI_write_cmos_sensor(0xcD, 0xF7);
			GC03102NDMIPI_write_cmos_sensor(0xcE, 0xFD);
			GC03102NDMIPI_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC03102NDMIPI_RGB_Gamma_m5:
			GC03102NDMIPI_write_cmos_sensor(0xBF, 0x10);
			GC03102NDMIPI_write_cmos_sensor(0xc0, 0x20);
			GC03102NDMIPI_write_cmos_sensor(0xc1, 0x38);
			GC03102NDMIPI_write_cmos_sensor(0xc2, 0x4E);
			GC03102NDMIPI_write_cmos_sensor(0xc3, 0x63);
			GC03102NDMIPI_write_cmos_sensor(0xc4, 0x76);
			GC03102NDMIPI_write_cmos_sensor(0xc5, 0x87);
			GC03102NDMIPI_write_cmos_sensor(0xc6, 0xA2);
			GC03102NDMIPI_write_cmos_sensor(0xc7, 0xB8);
			GC03102NDMIPI_write_cmos_sensor(0xc8, 0xCA);
			GC03102NDMIPI_write_cmos_sensor(0xc9, 0xD8);
			GC03102NDMIPI_write_cmos_sensor(0xcA, 0xE3);
			GC03102NDMIPI_write_cmos_sensor(0xcB, 0xEB);
			GC03102NDMIPI_write_cmos_sensor(0xcC, 0xF0);
			GC03102NDMIPI_write_cmos_sensor(0xcD, 0xF8);
			GC03102NDMIPI_write_cmos_sensor(0xcE, 0xFD);
			GC03102NDMIPI_write_cmos_sensor(0xcF, 0xFF);
			break;
			
		case GC03102NDMIPI_RGB_Gamma_m6:										// largest gamma curve
			GC03102NDMIPI_write_cmos_sensor(0xBF, 0x14);
			GC03102NDMIPI_write_cmos_sensor(0xc0, 0x28);
			GC03102NDMIPI_write_cmos_sensor(0xc1, 0x44);
			GC03102NDMIPI_write_cmos_sensor(0xc2, 0x5D);
			GC03102NDMIPI_write_cmos_sensor(0xc3, 0x72);
			GC03102NDMIPI_write_cmos_sensor(0xc4, 0x86);
			GC03102NDMIPI_write_cmos_sensor(0xc5, 0x95);
			GC03102NDMIPI_write_cmos_sensor(0xc6, 0xB1);
			GC03102NDMIPI_write_cmos_sensor(0xc7, 0xC6);
			GC03102NDMIPI_write_cmos_sensor(0xc8, 0xD5);
			GC03102NDMIPI_write_cmos_sensor(0xc9, 0xE1);
			GC03102NDMIPI_write_cmos_sensor(0xcA, 0xEA);
			GC03102NDMIPI_write_cmos_sensor(0xcB, 0xF1);
			GC03102NDMIPI_write_cmos_sensor(0xcC, 0xF5);
			GC03102NDMIPI_write_cmos_sensor(0xcD, 0xFB);
			GC03102NDMIPI_write_cmos_sensor(0xcE, 0xFE);
			GC03102NDMIPI_write_cmos_sensor(0xcF, 0xFF);
			break;
		case GC03102NDMIPI_RGB_Gamma_night:									//Gamma for night mode
			GC03102NDMIPI_write_cmos_sensor(0xBF, 0x0B);
			GC03102NDMIPI_write_cmos_sensor(0xc0, 0x16);
			GC03102NDMIPI_write_cmos_sensor(0xc1, 0x29);
			GC03102NDMIPI_write_cmos_sensor(0xc2, 0x3C);
			GC03102NDMIPI_write_cmos_sensor(0xc3, 0x4F);
			GC03102NDMIPI_write_cmos_sensor(0xc4, 0x5F);
			GC03102NDMIPI_write_cmos_sensor(0xc5, 0x6F);
			GC03102NDMIPI_write_cmos_sensor(0xc6, 0x8A);
			GC03102NDMIPI_write_cmos_sensor(0xc7, 0x9F);
			GC03102NDMIPI_write_cmos_sensor(0xc8, 0xB4);
			GC03102NDMIPI_write_cmos_sensor(0xc9, 0xC6);
			GC03102NDMIPI_write_cmos_sensor(0xcA, 0xD3);
			GC03102NDMIPI_write_cmos_sensor(0xcB, 0xDD);
			GC03102NDMIPI_write_cmos_sensor(0xcC, 0xE5);
			GC03102NDMIPI_write_cmos_sensor(0xcD, 0xF1);
			GC03102NDMIPI_write_cmos_sensor(0xcE, 0xFA);
			GC03102NDMIPI_write_cmos_sensor(0xcF, 0xFF);
			break;
		default:
			//GC03102NDMIPI_RGB_Gamma_m1
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
			GC03102NDMIPI_write_cmos_sensor(0xbf , 0x0b);
			GC03102NDMIPI_write_cmos_sensor(0xc0 , 0x17);
			GC03102NDMIPI_write_cmos_sensor(0xc1 , 0x2a);
			GC03102NDMIPI_write_cmos_sensor(0xc2 , 0x41);
			GC03102NDMIPI_write_cmos_sensor(0xc3 , 0x54);
			GC03102NDMIPI_write_cmos_sensor(0xc4 , 0x66);
			GC03102NDMIPI_write_cmos_sensor(0xc5 , 0x74);
			GC03102NDMIPI_write_cmos_sensor(0xc6 , 0x8c);
			GC03102NDMIPI_write_cmos_sensor(0xc7 , 0xa3);
			GC03102NDMIPI_write_cmos_sensor(0xc8 , 0xb5);
			GC03102NDMIPI_write_cmos_sensor(0xc9 , 0xc4);
			GC03102NDMIPI_write_cmos_sensor(0xca , 0xd0);
			GC03102NDMIPI_write_cmos_sensor(0xcb , 0xdb);
			GC03102NDMIPI_write_cmos_sensor(0xcc , 0xe5);
			GC03102NDMIPI_write_cmos_sensor(0xcd , 0xf0);
			GC03102NDMIPI_write_cmos_sensor(0xce , 0xf7);
			GC03102NDMIPI_write_cmos_sensor(0xcf , 0xff);
			break;
	}
}

/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPI_config_window
 *
 * DESCRIPTION
 *	This function config the hardware window of GC03102NDMIPI for getting specified
 *  data of that window.
 *
 * PARAMETERS
 *	start_x : start column of the interested window
 *  start_y : start row of the interested window
 *  width  : column widht of the itnerested window
 *  height : row depth of the itnerested window
 *
 * RETURNS
 *	the data that read from GC03102NDMIPI
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC03102NDMIPI_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* GC03102NDMIPI_config_window */


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPI_SetGain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *   iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC03102NDMIPI_SetGain(kal_uint16 iGain)
{
	return iGain;
}


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPI_NightMode
 *
 * DESCRIPTION
 *	This function night mode of GC03102NDMIPI.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static void GC03102NDMIPINightMode(kal_bool bEnable)
{
	if (bEnable)
	{	
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
		if(GC03102NDMIPI_MPEG4_encode_mode == KAL_TRUE)
			GC03102NDMIPI_write_cmos_sensor(0x3c, 0x30);
		else
			GC03102NDMIPI_write_cmos_sensor(0x3c, 0x30);
             		GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);	
			GC03102NDMIPI_NIGHT_MODE = KAL_TRUE;
	}
	else 
	{
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
		if(GC03102NDMIPI_MPEG4_encode_mode == KAL_TRUE)
			GC03102NDMIPI_write_cmos_sensor(0x3c, 0x20);
		else
			GC03102NDMIPI_write_cmos_sensor(0x3c, 0x20);
           	       GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);				   
			GC03102NDMIPI_NIGHT_MODE = KAL_FALSE;
	}
} /* GC03102NDMIPI_NightMode */

/*************************************************************************
* FUNCTION
*	GC03102NDMIPI_Sensor_Init
*
* DESCRIPTION
*	This function apply all of the initial setting to sensor.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
*************************************************************************/
static void GC03102NDMIPI_Sensor_Init(void)
{

  /////////////////////////////////////////////////
	/////////////////	system reg	/////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xfe,0xf0);
	GC03102NDMIPI_write_cmos_sensor(0xfe,0xf0);
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xfc,0x0e);
	GC03102NDMIPI_write_cmos_sensor(0xfc,0x0e);
	GC03102NDMIPI_write_cmos_sensor(0xf2,0x80);
	GC03102NDMIPI_write_cmos_sensor(0xf3,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xf7,0x1b);
	GC03102NDMIPI_write_cmos_sensor(0xf8,0x04); 
	GC03102NDMIPI_write_cmos_sensor(0xf9,0x8e);
	GC03102NDMIPI_write_cmos_sensor(0xfa,0x11);
	/////////////////////////////////////////////////      
	///////////////////   MIPI   ////////////////////      
	/////////////////////////////////////////////////      
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x03);
	GC03102NDMIPI_write_cmos_sensor(0x40,0x08);
	GC03102NDMIPI_write_cmos_sensor(0x42,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x43,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x01,0x03);
	GC03102NDMIPI_write_cmos_sensor(0x10,0x84);
                                        
	GC03102NDMIPI_write_cmos_sensor(0x01,0x03);             
	GC03102NDMIPI_write_cmos_sensor(0x02,0x00);             
	GC03102NDMIPI_write_cmos_sensor(0x03,0x94);             
	GC03102NDMIPI_write_cmos_sensor(0x04,0x01);            
	GC03102NDMIPI_write_cmos_sensor(0x05,0x00);             
	GC03102NDMIPI_write_cmos_sensor(0x06,0x80);             
	GC03102NDMIPI_write_cmos_sensor(0x11,0x1e);             
	GC03102NDMIPI_write_cmos_sensor(0x12,0x00);      
	GC03102NDMIPI_write_cmos_sensor(0x13,0x05);             
	GC03102NDMIPI_write_cmos_sensor(0x15,0x10);                                                                    
	GC03102NDMIPI_write_cmos_sensor(0x21,0x10);             
	GC03102NDMIPI_write_cmos_sensor(0x22,0x01);             
	GC03102NDMIPI_write_cmos_sensor(0x23,0x10);                                             
	GC03102NDMIPI_write_cmos_sensor(0x24,0x02);                                             
	GC03102NDMIPI_write_cmos_sensor(0x25,0x10);                                             
	GC03102NDMIPI_write_cmos_sensor(0x26,0x03);                                             
	GC03102NDMIPI_write_cmos_sensor(0x29,0x02);                                             
	GC03102NDMIPI_write_cmos_sensor(0x2a,0x0a);                                             
	GC03102NDMIPI_write_cmos_sensor(0x2b,0x04);                                             
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);
	
	/////////////////////////////////////////////////
	/////////////////  CISCTL reg	/////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0x00,0x2f);
	GC03102NDMIPI_write_cmos_sensor(0x01,0x0f);
	GC03102NDMIPI_write_cmos_sensor(0x02,0x04);
	GC03102NDMIPI_write_cmos_sensor(0x03,0x03);
	GC03102NDMIPI_write_cmos_sensor(0x04,0x50);
	GC03102NDMIPI_write_cmos_sensor(0x09,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x0a,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x0b,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x0c,0x04);
	GC03102NDMIPI_write_cmos_sensor(0x0d,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x0e,0xe8);
	GC03102NDMIPI_write_cmos_sensor(0x0f,0x02);
	GC03102NDMIPI_write_cmos_sensor(0x10,0x88);
	GC03102NDMIPI_write_cmos_sensor(0x16,0x00);	
	GC03102NDMIPI_write_cmos_sensor(0x17,0x17);
	GC03102NDMIPI_write_cmos_sensor(0x18,0x1a);
	GC03102NDMIPI_write_cmos_sensor(0x19,0x14);
	GC03102NDMIPI_write_cmos_sensor(0x1b,0x48);
	GC03102NDMIPI_write_cmos_sensor(0x1e,0x6b);
	GC03102NDMIPI_write_cmos_sensor(0x1f,0x28);
	GC03102NDMIPI_write_cmos_sensor(0x20,0x8b);//0x89 travis20140801
	GC03102NDMIPI_write_cmos_sensor(0x21,0x49);
	GC03102NDMIPI_write_cmos_sensor(0x22,0xb0);
	GC03102NDMIPI_write_cmos_sensor(0x23,0x04);
	GC03102NDMIPI_write_cmos_sensor(0x24,0x16);
	GC03102NDMIPI_write_cmos_sensor(0x34,0x20);
	
	/////////////////////////////////////////////////
	////////////////////   BLK	 ////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0x26,0x23);
	GC03102NDMIPI_write_cmos_sensor(0x28,0xff);
	GC03102NDMIPI_write_cmos_sensor(0x29,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x33,0x10); 
	GC03102NDMIPI_write_cmos_sensor(0x37,0x20);
	GC03102NDMIPI_write_cmos_sensor(0x38,0x10);
	GC03102NDMIPI_write_cmos_sensor(0x47,0x80);
	GC03102NDMIPI_write_cmos_sensor(0x4e,0x66);
	GC03102NDMIPI_write_cmos_sensor(0xa8,0x02);
	GC03102NDMIPI_write_cmos_sensor(0xa9,0x80);
	
	/////////////////////////////////////////////////
	//////////////////	ISP reg   ///////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0x40,0xff);
	GC03102NDMIPI_write_cmos_sensor(0x41,0x21);
	GC03102NDMIPI_write_cmos_sensor(0x42,0xcf);
	GC03102NDMIPI_write_cmos_sensor(0x44,0x02);
	GC03102NDMIPI_write_cmos_sensor(0x45,0xa8); 
	GC03102NDMIPI_write_cmos_sensor(0x46,0x02); 
	GC03102NDMIPI_write_cmos_sensor(0x4a,0x11);
	GC03102NDMIPI_write_cmos_sensor(0x4b,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x4c,0x20);
	GC03102NDMIPI_write_cmos_sensor(0x4d,0x05);
	GC03102NDMIPI_write_cmos_sensor(0x4f,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x50,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x55,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x56,0xe0);
	GC03102NDMIPI_write_cmos_sensor(0x57,0x02);
	GC03102NDMIPI_write_cmos_sensor(0x58,0x80);
	
	/////////////////////////////////////////////////
	///////////////////   GAIN   ////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0x70,0x70);
	GC03102NDMIPI_write_cmos_sensor(0x5a,0x84);
	GC03102NDMIPI_write_cmos_sensor(0x5b,0xc9);
	GC03102NDMIPI_write_cmos_sensor(0x5c,0xed);
	GC03102NDMIPI_write_cmos_sensor(0x77,0x74);
	GC03102NDMIPI_write_cmos_sensor(0x78,0x40);
	GC03102NDMIPI_write_cmos_sensor(0x79,0x5f);
	
	///////////////////////////////////////////////// 
	///////////////////   DNDD  /////////////////////
	///////////////////////////////////////////////// 
	GC03102NDMIPI_write_cmos_sensor(0x82,0x14); 
	GC03102NDMIPI_write_cmos_sensor(0x83,0x0b);
	GC03102NDMIPI_write_cmos_sensor(0x89,0xf0);
	
	///////////////////////////////////////////////// 
	//////////////////   EEINTP  ////////////////////
	///////////////////////////////////////////////// 
	GC03102NDMIPI_write_cmos_sensor(0x8f,0xaa); 
	GC03102NDMIPI_write_cmos_sensor(0x90,0x8c); 
	GC03102NDMIPI_write_cmos_sensor(0x91,0x90);
	GC03102NDMIPI_write_cmos_sensor(0x92,0x03); 
	GC03102NDMIPI_write_cmos_sensor(0x93,0x03); 
	GC03102NDMIPI_write_cmos_sensor(0x94,0x05); 
	GC03102NDMIPI_write_cmos_sensor(0x95,0x65); 
	GC03102NDMIPI_write_cmos_sensor(0x96,0xf0); 
	
	///////////////////////////////////////////////// 
	/////////////////////  ASDE  ////////////////////
	///////////////////////////////////////////////// 
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);

	GC03102NDMIPI_write_cmos_sensor(0x9a,0x20);
	GC03102NDMIPI_write_cmos_sensor(0x9b,0x80);
	GC03102NDMIPI_write_cmos_sensor(0x9c,0x40);
	GC03102NDMIPI_write_cmos_sensor(0x9d,0x80);
	 
	GC03102NDMIPI_write_cmos_sensor(0xa1,0x33);
 	GC03102NDMIPI_write_cmos_sensor(0xa2,0x32);
	GC03102NDMIPI_write_cmos_sensor(0xa4,0x50);
	GC03102NDMIPI_write_cmos_sensor(0xa5,0x20);
	GC03102NDMIPI_write_cmos_sensor(0xaa,0x38); 
	GC03102NDMIPI_write_cmos_sensor(0xac,0x66);
	 
	
	/////////////////////////////////////////////////
	///////////////////   GAMMA   ///////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);//default
	GC03102NDMIPI_write_cmos_sensor(0xbf,0x08);
	GC03102NDMIPI_write_cmos_sensor(0xc0,0x16);
	GC03102NDMIPI_write_cmos_sensor(0xc1,0x28);
	GC03102NDMIPI_write_cmos_sensor(0xc2,0x41);
	GC03102NDMIPI_write_cmos_sensor(0xc3,0x5a);
	GC03102NDMIPI_write_cmos_sensor(0xc4,0x6c);
	GC03102NDMIPI_write_cmos_sensor(0xc5,0x7a);
	GC03102NDMIPI_write_cmos_sensor(0xc6,0x96);
	GC03102NDMIPI_write_cmos_sensor(0xc7,0xac);
	GC03102NDMIPI_write_cmos_sensor(0xc8,0xbc);
	GC03102NDMIPI_write_cmos_sensor(0xc9,0xc9);
	GC03102NDMIPI_write_cmos_sensor(0xca,0xd3);
	GC03102NDMIPI_write_cmos_sensor(0xcb,0xdd);
	GC03102NDMIPI_write_cmos_sensor(0xcc,0xe5);
	GC03102NDMIPI_write_cmos_sensor(0xcd,0xf1);
	GC03102NDMIPI_write_cmos_sensor(0xce,0xfa);
	GC03102NDMIPI_write_cmos_sensor(0xcf,0xff);
                                 
/* 
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);//big gamma
	GC03102NDMIPI_write_cmos_sensor(0xbf,0x08);
	GC03102NDMIPI_write_cmos_sensor(0xc0,0x1d);
	GC03102NDMIPI_write_cmos_sensor(0xc1,0x34);
	GC03102NDMIPI_write_cmos_sensor(0xc2,0x4b);
	GC03102NDMIPI_write_cmos_sensor(0xc3,0x60);
	GC03102NDMIPI_write_cmos_sensor(0xc4,0x73);
	GC03102NDMIPI_write_cmos_sensor(0xc5,0x85);
	GC03102NDMIPI_write_cmos_sensor(0xc6,0x9f);
	GC03102NDMIPI_write_cmos_sensor(0xc7,0xb5);
	GC03102NDMIPI_write_cmos_sensor(0xc8,0xc7);
	GC03102NDMIPI_write_cmos_sensor(0xc9,0xd5);
	GC03102NDMIPI_write_cmos_sensor(0xca,0xe0);
	GC03102NDMIPI_write_cmos_sensor(0xcb,0xe7);
	GC03102NDMIPI_write_cmos_sensor(0xcc,0xec);
	GC03102NDMIPI_write_cmos_sensor(0xcd,0xf4);
	GC03102NDMIPI_write_cmos_sensor(0xce,0xfa);
	GC03102NDMIPI_write_cmos_sensor(0xcf,0xff);
*/	

/*
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);//small gamma
	GC03102NDMIPI_write_cmos_sensor(0xbf,0x08);
	GC03102NDMIPI_write_cmos_sensor(0xc0,0x18);
	GC03102NDMIPI_write_cmos_sensor(0xc1,0x2c);
	GC03102NDMIPI_write_cmos_sensor(0xc2,0x41);
	GC03102NDMIPI_write_cmos_sensor(0xc3,0x59);
	GC03102NDMIPI_write_cmos_sensor(0xc4,0x6e);
	GC03102NDMIPI_write_cmos_sensor(0xc5,0x81);
	GC03102NDMIPI_write_cmos_sensor(0xc6,0x9f);
	GC03102NDMIPI_write_cmos_sensor(0xc7,0xb5);
	GC03102NDMIPI_write_cmos_sensor(0xc8,0xc7);
	GC03102NDMIPI_write_cmos_sensor(0xc9,0xd5);
	GC03102NDMIPI_write_cmos_sensor(0xca,0xe0);
	GC03102NDMIPI_write_cmos_sensor(0xcb,0xe7);
	GC03102NDMIPI_write_cmos_sensor(0xcc,0xec);
	GC03102NDMIPI_write_cmos_sensor(0xcd,0xf4);
	GC03102NDMIPI_write_cmos_sensor(0xce,0xfa);
	GC03102NDMIPI_write_cmos_sensor(0xcf,0xff);
*/
	/////////////////////////////////////////////////
	///////////////////   YCP  //////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xd0,0x40);
	GC03102NDMIPI_write_cmos_sensor(0xd1,0x28); //32
	GC03102NDMIPI_write_cmos_sensor(0xd2,0x28); //32
	GC03102NDMIPI_write_cmos_sensor(0xd3,0x40); 
	GC03102NDMIPI_write_cmos_sensor(0xd6,0xf2);
	GC03102NDMIPI_write_cmos_sensor(0xd7,0x1b);
	GC03102NDMIPI_write_cmos_sensor(0xd8,0x18);
	GC03102NDMIPI_write_cmos_sensor(0xdd,0x03); 
	
	/////////////////////////////////////////////////
	////////////////////   AEC   ////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x05,0x30); 
	GC03102NDMIPI_write_cmos_sensor(0x06,0x75); 
	GC03102NDMIPI_write_cmos_sensor(0x07,0x40); 
	GC03102NDMIPI_write_cmos_sensor(0x08,0xb0); 
	GC03102NDMIPI_write_cmos_sensor(0x0a,0xc5); 
	GC03102NDMIPI_write_cmos_sensor(0x0b,0x11); 
	GC03102NDMIPI_write_cmos_sensor(0x0c,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x12,0x52); 
	GC03102NDMIPI_write_cmos_sensor(0x13,0x38); 
	GC03102NDMIPI_write_cmos_sensor(0x18,0x95); 
	GC03102NDMIPI_write_cmos_sensor(0x19,0x96); 
	GC03102NDMIPI_write_cmos_sensor(0x1f,0x15); 
	GC03102NDMIPI_write_cmos_sensor(0x20,0xb0); 
	GC03102NDMIPI_write_cmos_sensor(0x3e,0x40); 
	GC03102NDMIPI_write_cmos_sensor(0x3f,0x57); 
	GC03102NDMIPI_write_cmos_sensor(0x40,0x7d); 
	GC03102NDMIPI_write_cmos_sensor(0x03,0x60);
	GC03102NDMIPI_write_cmos_sensor(0x44,0x02);
	
	/////////////////////////////////////////////////
	////////////////////   AWB   ////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0x1c,0x91); 
	GC03102NDMIPI_write_cmos_sensor(0x21,0x15); 
	GC03102NDMIPI_write_cmos_sensor(0x50,0x80); 
	GC03102NDMIPI_write_cmos_sensor(0x56,0x04); 
	GC03102NDMIPI_write_cmos_sensor(0x59,0x08); 
	GC03102NDMIPI_write_cmos_sensor(0x5b,0x02);
	GC03102NDMIPI_write_cmos_sensor(0x61,0x8d); 
	GC03102NDMIPI_write_cmos_sensor(0x62,0xa7); 
	GC03102NDMIPI_write_cmos_sensor(0x63,0xd0); 
	GC03102NDMIPI_write_cmos_sensor(0x65,0x06);
	GC03102NDMIPI_write_cmos_sensor(0x66,0x06); 
	GC03102NDMIPI_write_cmos_sensor(0x67,0x84); 
	GC03102NDMIPI_write_cmos_sensor(0x69,0x08);
	GC03102NDMIPI_write_cmos_sensor(0x6a,0x25);
	GC03102NDMIPI_write_cmos_sensor(0x6b,0x01); 
	GC03102NDMIPI_write_cmos_sensor(0x6c,0x00); 
	GC03102NDMIPI_write_cmos_sensor(0x6d,0x02); 
	GC03102NDMIPI_write_cmos_sensor(0x6e,0xf0); 
	GC03102NDMIPI_write_cmos_sensor(0x6f,0x80); 
	GC03102NDMIPI_write_cmos_sensor(0x76,0x80);
	GC03102NDMIPI_write_cmos_sensor(0x78,0xaf); 
	GC03102NDMIPI_write_cmos_sensor(0x79,0x75);
	GC03102NDMIPI_write_cmos_sensor(0x7a,0x40);
	GC03102NDMIPI_write_cmos_sensor(0x7b,0x50);	
	GC03102NDMIPI_write_cmos_sensor(0x7c,0x0c);
	 

    GC03102NDMIPI_write_cmos_sensor(0xfe,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x90,0xc9);
	GC03102NDMIPI_write_cmos_sensor(0x91,0xbe);
	GC03102NDMIPI_write_cmos_sensor(0x92,0xe6);
	GC03102NDMIPI_write_cmos_sensor(0x93,0xca);
	GC03102NDMIPI_write_cmos_sensor(0x95,0x23);
	GC03102NDMIPI_write_cmos_sensor(0x96,0xe7);
	GC03102NDMIPI_write_cmos_sensor(0x97,0x43);
	GC03102NDMIPI_write_cmos_sensor(0x98,0x24);
	GC03102NDMIPI_write_cmos_sensor(0x9a,0x43);
	GC03102NDMIPI_write_cmos_sensor(0x9b,0x24);
	GC03102NDMIPI_write_cmos_sensor(0x9c,0xc4);
	GC03102NDMIPI_write_cmos_sensor(0x9d,0x44);
	GC03102NDMIPI_write_cmos_sensor(0x9f,0xc7);
	GC03102NDMIPI_write_cmos_sensor(0xa0,0xc8);
	GC03102NDMIPI_write_cmos_sensor(0xa1,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xa2,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x86,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x87,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x88,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x89,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xa4,0xb9);
	GC03102NDMIPI_write_cmos_sensor(0xa5,0xa0);
	GC03102NDMIPI_write_cmos_sensor(0xa6,0xb8);
	GC03102NDMIPI_write_cmos_sensor(0xa7,0x95);
	GC03102NDMIPI_write_cmos_sensor(0xa9,0xbf);
	GC03102NDMIPI_write_cmos_sensor(0xaa,0x8c);
	GC03102NDMIPI_write_cmos_sensor(0xab,0x9d);
	GC03102NDMIPI_write_cmos_sensor(0xac,0x80);
	GC03102NDMIPI_write_cmos_sensor(0xae,0xb7);
	GC03102NDMIPI_write_cmos_sensor(0xaf,0x9e);
	GC03102NDMIPI_write_cmos_sensor(0xb0,0xc8);
	GC03102NDMIPI_write_cmos_sensor(0xb1,0x97);
	GC03102NDMIPI_write_cmos_sensor(0xb3,0xb7);
	GC03102NDMIPI_write_cmos_sensor(0xb4,0x7f);
	GC03102NDMIPI_write_cmos_sensor(0xb5,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xb6,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x8b,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x8c,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x8d,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x8e,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x94,0x55);
	GC03102NDMIPI_write_cmos_sensor(0x99,0xa6);
	GC03102NDMIPI_write_cmos_sensor(0x9e,0xaa);
	GC03102NDMIPI_write_cmos_sensor(0xa3,0x0a);
	GC03102NDMIPI_write_cmos_sensor(0x8a,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xa8,0x55);
	GC03102NDMIPI_write_cmos_sensor(0xad,0x55);
	GC03102NDMIPI_write_cmos_sensor(0xb2,0x55);
	GC03102NDMIPI_write_cmos_sensor(0xb7,0x05);
	GC03102NDMIPI_write_cmos_sensor(0x8f,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xb8,0xcb);
	GC03102NDMIPI_write_cmos_sensor(0xb9,0x9b);
	/////////////////////////////////////////////////
	////////////////////   CC    ////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x01);
                                 
	/*GC03102NDMIPI_write_cmos_sensor(0xd0,0x38);//skin red
	GC03102NDMIPI_write_cmos_sensor(0xd1,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xd2,0x02);
	GC03102NDMIPI_write_cmos_sensor(0xd3,0x04);
	GC03102NDMIPI_write_cmos_sensor(0xd4,0x38);
	GC03102NDMIPI_write_cmos_sensor(0xd5,0x12);*/
                                 
      
	/*GC03102NDMIPI_write_cmos_sensor(0xd0,0x38);//skin white
	GC03102NDMIPI_write_cmos_sensor(0xd1,0xfd);
	GC03102NDMIPI_write_cmos_sensor(0xd2,0x06);
	GC03102NDMIPI_write_cmos_sensor(0xd3,0xf0);
	GC03102NDMIPI_write_cmos_sensor(0xd4,0x40);
	GC03102NDMIPI_write_cmos_sensor(0xd5,0x08);    */          
	GC03102NDMIPI_write_cmos_sensor(0xd0,0x34);
	GC03102NDMIPI_write_cmos_sensor(0xd1,0x04);
	GC03102NDMIPI_write_cmos_sensor(0xd2,0x06);
	GC03102NDMIPI_write_cmos_sensor(0xd3,0xfd);
	GC03102NDMIPI_write_cmos_sensor(0xd4,0x40);
	GC03102NDMIPI_write_cmos_sensor(0xd5,0x00);
	
/*                       
	GC03102NDMIPI_write_cmos_sensor(0xd0,0x38);//guodengxiang
	GC03102NDMIPI_write_cmos_sensor(0xd1,0xf8);
	GC03102NDMIPI_write_cmos_sensor(0xd2,0x06);
	GC03102NDMIPI_write_cmos_sensor(0xd3,0xfd);
	GC03102NDMIPI_write_cmos_sensor(0xd4,0x40);
	GC03102NDMIPI_write_cmos_sensor(0xd5,0x00);
*/
	GC03102NDMIPI_write_cmos_sensor(0xd6,0x30);
	GC03102NDMIPI_write_cmos_sensor(0xd7,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xd8,0x0a);
	GC03102NDMIPI_write_cmos_sensor(0xd9,0x16);
	GC03102NDMIPI_write_cmos_sensor(0xda,0x39);
	GC03102NDMIPI_write_cmos_sensor(0xdb,0xf8);

	/////////////////////////////////////////////////
	////////////////////   LSC   ////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x01);
	GC03102NDMIPI_write_cmos_sensor(0xc1,0x3c);
	GC03102NDMIPI_write_cmos_sensor(0xc2,0x50);
	GC03102NDMIPI_write_cmos_sensor(0xc3,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xc4,0x40);
	GC03102NDMIPI_write_cmos_sensor(0xc5,0x30);
	GC03102NDMIPI_write_cmos_sensor(0xc6,0x30);
	GC03102NDMIPI_write_cmos_sensor(0xc7,0x10);
	GC03102NDMIPI_write_cmos_sensor(0xc8,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xc9,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xdc,0x20);
	GC03102NDMIPI_write_cmos_sensor(0xdd,0x10);
	GC03102NDMIPI_write_cmos_sensor(0xdf,0x00);
	GC03102NDMIPI_write_cmos_sensor(0xde,0x00);
	
	/////////////////////////////////////////////////
	///////////////////  Histogram	/////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0x01,0x10);
	GC03102NDMIPI_write_cmos_sensor(0x0b,0x31);
	GC03102NDMIPI_write_cmos_sensor(0x0e,0x50);
	GC03102NDMIPI_write_cmos_sensor(0x0f,0x0f);
	GC03102NDMIPI_write_cmos_sensor(0x10,0x6e);
	GC03102NDMIPI_write_cmos_sensor(0x12,0xa0);
	GC03102NDMIPI_write_cmos_sensor(0x15,0x60);
	GC03102NDMIPI_write_cmos_sensor(0x16,0x60);
	GC03102NDMIPI_write_cmos_sensor(0x17,0xe3);
	
	/////////////////////////////////////////////////
	//////////////	Measure Window	  ///////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xcc,0x0c); 
	GC03102NDMIPI_write_cmos_sensor(0xcd,0x10);
	GC03102NDMIPI_write_cmos_sensor(0xce,0xa0);
	GC03102NDMIPI_write_cmos_sensor(0xcf,0xe6);
	
	/////////////////////////////////////////////////
	/////////////////	dark sun   //////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0x45,0xf7);
	GC03102NDMIPI_write_cmos_sensor(0x46,0xff);
	GC03102NDMIPI_write_cmos_sensor(0x47,0x15);
	GC03102NDMIPI_write_cmos_sensor(0x48,0x03); 
	GC03102NDMIPI_write_cmos_sensor(0x4f,0x60);

	//////////////////banding//////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x05,0x02);
	GC03102NDMIPI_write_cmos_sensor(0x06,0xd1); //HB
	GC03102NDMIPI_write_cmos_sensor(0x07,0x00);
	GC03102NDMIPI_write_cmos_sensor(0x08,0x22); //VB
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x01);
	GC03102NDMIPI_write_cmos_sensor(0x25,0x00); //step 
	GC03102NDMIPI_write_cmos_sensor(0x26,0x6a); 
	GC03102NDMIPI_write_cmos_sensor(0x27,0x02); //20fps
	GC03102NDMIPI_write_cmos_sensor(0x28,0x7c);  
	GC03102NDMIPI_write_cmos_sensor(0x29,0x03); //12.5fps
	GC03102NDMIPI_write_cmos_sensor(0x2a,0xba); 
	GC03102NDMIPI_write_cmos_sensor(0x2b,0x06); //7.14fps
	GC03102NDMIPI_write_cmos_sensor(0x2c,0x36); 
	GC03102NDMIPI_write_cmos_sensor(0x2d,0x07); //5.55fps
	GC03102NDMIPI_write_cmos_sensor(0x2e,0x74);
	GC03102NDMIPI_write_cmos_sensor(0x3c,0x20);
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00);
	
	/////////////////////////////////////////////////
	///////////////////   MIPI	 ////////////////////
	/////////////////////////////////////////////////
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x03);
	GC03102NDMIPI_write_cmos_sensor(0x10,0x94);
	GC03102NDMIPI_write_cmos_sensor(0xfe,0x00); 
	
}


static void GC0310ZGXGetGpioStatus(void)
{
     int ret;
     printk("ZGX gc0310ZGX2ndGetGpioStatus\n");
     mt_set_gpio_mode(GPIO65, GPIO_MODE_00);
     mt_set_gpio_dir(GPIO65, GPIO_DIR_IN);
     ret = mt_get_gpio_in(GPIO65);
     printk("zgx_gpio_ret_2nd = %d\n",ret);
}


static UINT32 GC03102NDMIPIGetSensorID(UINT32 *sensorID)
{
    int  retry = 3; 
    // check if sensor ID correct
    do {
        *sensorID=((GC03102NDMIPI_read_cmos_sensor(0xf0)<< 8)|GC03102NDMIPI_read_cmos_sensor(0xf1));
        if (*sensorID == GC0310_SENSOR_ID)
            break; 
        SENSORDB("Read Sensor ID Fail = 0x%04x\n", *sensorID); 
        retry--; 
    } while (retry > 0);

    if (*sensorID != GC0310_SENSOR_ID) {
        *sensorID = 0xFFFFFFFF; 
        return ERROR_SENSOR_CONNECT_FAIL;
    }
	else
		{
			
			GC0310ZGXGetGpioStatus();
			if(mt_get_gpio_in(GPIO65))
				{
					printk("zgx error 2nd start\n");
					*sensorID = 0xFFFFFFFF;
					return ERROR_SENSOR_CONNECT_FAIL;
			}

	}
    return ERROR_NONE;    
}




/*************************************************************************
* FUNCTION
*	GC03102NDMIPI_Write_More_Registers
*
* DESCRIPTION
*	This function is served for FAE to modify the necessary Init Regs. Do not modify the regs
*     in init_GC03102NDMIPI() directly.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void GC03102NDMIPI_Write_More_Registers(void)
{

}


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPIOpen
 *
 * DESCRIPTION
 *	This function initialize the registers of CMOS sensor
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static UINT32 GC03102NDMIPIOpen(void)
{
	volatile signed char i;
	kal_uint16 sensor_id=0;

	printk("<Jet> Entry GC03102NDMIPIOpen!!!\r\n");

	Sleep(10);


	//  Read sensor ID to adjust I2C is OK?
	for(i=0;i<3;i++)
	{
		sensor_id = ((GC03102NDMIPI_read_cmos_sensor(0xf0) << 8) | GC03102NDMIPI_read_cmos_sensor(0xf1));
		if(sensor_id != GC0310_SENSOR_ID)  
		{
			SENSORDB("GC03102NDMIPI Read Sensor ID Fail[open] = 0x%x\n", sensor_id); 
			return ERROR_SENSOR_CONNECT_FAIL;
		}
	}
	
	SENSORDB("GC03102NDMIPI_ Sensor Read ID OK \r\n");
	GC03102NDMIPI_Sensor_Init();

#ifdef DEBUG_SENSOR_GC03102NDMIPI  
		struct file *fp; 
		mm_segment_t fs; 
		loff_t pos = 0; 
		static char buf[60*1024] ;

		printk("GC03102NDMIPI open debug \n");

		fp = filp_open("/mnt/sdcard/GC03102NDMIPI_sd.txt", O_RDONLY , 0); 

		if (IS_ERR(fp)) 
		{ 
			fromsd = 0;   
			printk("GC03102NDMIPI open file error\n");
		} 
		else 
		{
			fromsd = 1;
			printk("GC03102NDMIPI open file ok\n");
	
			filp_close(fp, NULL); 
			set_fs(fs);
		}

		if(fromsd == 1)
		{
			printk("GC03102NDMIPI open from t!\n");
			GC03102NDMIPI_Initialize_from_T_Flash();
		}
		else
		{
			printk("GC03102NDMIPI open not from t!\n");		
		}

#endif
	GC03102NDMIPI_Write_More_Registers();

	return ERROR_NONE;
} /* GC03102NDMIPIOpen */


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPIClose
 *
 * DESCRIPTION
 *	This function is to turn off sensor module power.
 *
 * PARAMETERS
 *	None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static UINT32 GC03102NDMIPIClose(void)
{
    return ERROR_NONE;
} /* GC03102NDMIPIClose */


/*************************************************************************
 * FUNCTION
 * GC03102NDMIPIPreview
 *
 * DESCRIPTION
 *	This function start the sensor preview.
 *
 * PARAMETERS
 *	*image_window : address pointer of pixel numbers in one period of HSYNC
 *  *sensor_config_data : address pointer of line numbers in one period of VSYNC
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
static UINT32 GC03102NDMIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{



    if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        RETAILMSG(1, (TEXT("Camera Video preview\r\n")));
        GC03102NDMIPI_MPEG4_encode_mode = KAL_TRUE;
       
    }
    else
    {
        RETAILMSG(1, (TEXT("Camera preview\r\n")));
        GC03102NDMIPI_MPEG4_encode_mode = KAL_FALSE;
    }

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC03102NDMIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC03102NDMIPIPreview */


/*************************************************************************
 * FUNCTION
 *	GC03102NDMIPICapture
 *
 * DESCRIPTION
 *	This function setup the CMOS sensor in capture MY_OUTPUT mode
 *
 * PARAMETERS
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
UINT32 GC03102NDMIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    GC03102NDMIPI_MODE_CAPTURE=KAL_TRUE;

    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC03102NDMIPISensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC03102NDMIPI_Capture() */



static UINT32 GC03102NDMIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    pSensorResolution->SensorVideoWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorVideoHeight=IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
} /* GC03102NDMIPIGetResolution() */


static UINT32 GC03102NDMIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_MIPI;//MIPI setting
    pSensorInfo->CaptureDelayFrame = 2;
    pSensorInfo->PreviewDelayFrame = 2;
    pSensorInfo->VideoDelayFrame = 4;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    default:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
	//MIPI setting
	pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_1_LANE; 	
	pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
	pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14;
	pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
	pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
	pSensorInfo->SensorHightSampling = 0;	// 0 is default 1x 
	pSensorInfo->SensorPacketECCOrder = 1;

        break;
    }
    GC03102NDMIPIPixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &GC03102NDMIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC03102NDMIPIGetInfo() */


static UINT32 GC03102NDMIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    default:
	 GC03102NDMIPIPreview(pImageWindow, pSensorConfigData);
        break;
    }


    return TRUE;
}	/* GC03102NDMIPIControl() */

static BOOL GC03102NDMIPI_set_param_wb(UINT16 para)
{

	switch (para)
	{
		case AWB_MODE_OFF:

		break;
		
		case AWB_MODE_AUTO:
			GC03102NDMIPI_awb_enable(KAL_TRUE);
		break;
		
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			GC03102NDMIPI_awb_enable(KAL_FALSE);
			GC03102NDMIPI_write_cmos_sensor(0x77, 0x8c); //WB_manual_gain 
			GC03102NDMIPI_write_cmos_sensor(0x78, 0x50);
			GC03102NDMIPI_write_cmos_sensor(0x79, 0x40);
		break;
		
		case AWB_MODE_DAYLIGHT: //sunny
			GC03102NDMIPI_awb_enable(KAL_FALSE);
			GC03102NDMIPI_write_cmos_sensor(0x77, 0x74); 
			GC03102NDMIPI_write_cmos_sensor(0x78, 0x52);
			GC03102NDMIPI_write_cmos_sensor(0x79, 0x40);			
		break;
		
		case AWB_MODE_INCANDESCENT: //office
			GC03102NDMIPI_awb_enable(KAL_FALSE);
			GC03102NDMIPI_write_cmos_sensor(0x77, 0x48);
			GC03102NDMIPI_write_cmos_sensor(0x78, 0x40);
			GC03102NDMIPI_write_cmos_sensor(0x79, 0x5c);
		break;
		
		case AWB_MODE_TUNGSTEN: //home
			GC03102NDMIPI_awb_enable(KAL_FALSE);
			GC03102NDMIPI_write_cmos_sensor(0x77, 0x40);
			GC03102NDMIPI_write_cmos_sensor(0x78, 0x54);
			GC03102NDMIPI_write_cmos_sensor(0x79, 0x70);
		break;
		
		case AWB_MODE_FLUORESCENT:
			GC03102NDMIPI_awb_enable(KAL_FALSE);
			GC03102NDMIPI_write_cmos_sensor(0x77, 0x40);
			GC03102NDMIPI_write_cmos_sensor(0x78, 0x42);
			GC03102NDMIPI_write_cmos_sensor(0x79, 0x50);
		break;
		
		default:
		return FALSE;
	}

	return TRUE;
} /* GC03102NDMIPI_set_param_wb */


static BOOL GC03102NDMIPI_set_param_effect(UINT16 para)
{
	kal_uint32  ret = KAL_TRUE;

	switch (para)
	{
		case MEFFECT_OFF:
			GC03102NDMIPI_write_cmos_sensor(0x43 , 0x00);
		break;
		
		case MEFFECT_SEPIA:
			GC03102NDMIPI_write_cmos_sensor(0x43 , 0x02);
			GC03102NDMIPI_write_cmos_sensor(0xda , 0xd0);
			GC03102NDMIPI_write_cmos_sensor(0xdb , 0x28);
		break;
		
		case MEFFECT_NEGATIVE:
			GC03102NDMIPI_write_cmos_sensor(0x43 , 0x01);
		break;
		
		case MEFFECT_SEPIAGREEN:
			GC03102NDMIPI_write_cmos_sensor(0x43 , 0x02);
			GC03102NDMIPI_write_cmos_sensor(0xda , 0xc0);
			GC03102NDMIPI_write_cmos_sensor(0xdb , 0xc0);
		break;
		
		case MEFFECT_SEPIABLUE:
			GC03102NDMIPI_write_cmos_sensor(0x43 , 0x02);
			GC03102NDMIPI_write_cmos_sensor(0xda , 0x50);
			GC03102NDMIPI_write_cmos_sensor(0xdb , 0xe0);
		break;

		case MEFFECT_MONO:
			GC03102NDMIPI_write_cmos_sensor(0x43 , 0x02);
			GC03102NDMIPI_write_cmos_sensor(0xda , 0x00);
			GC03102NDMIPI_write_cmos_sensor(0xdb , 0x00);
		break;
		default:
			ret = FALSE;
	}

	return ret;

} /* GC03102NDMIPI_set_param_effect */


static BOOL GC03102NDMIPI_set_param_banding(UINT16 para)
{
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00); 
			GC03102NDMIPI_write_cmos_sensor(0x05, 0x02);
			GC03102NDMIPI_write_cmos_sensor(0x06, 0xd1); //HB
			GC03102NDMIPI_write_cmos_sensor(0x07, 0x00);
			GC03102NDMIPI_write_cmos_sensor(0x08, 0x22); //VB
			
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x25, 0x00);   //anti-flicker step [11:8]
			GC03102NDMIPI_write_cmos_sensor(0x26, 0x6a);   //anti-flicker step [7:0]
			 
			GC03102NDMIPI_write_cmos_sensor(0x27,0x02); //20fps
			GC03102NDMIPI_write_cmos_sensor(0x28,0x7c);  
			GC03102NDMIPI_write_cmos_sensor(0x29,0x03); //12.5fps
			GC03102NDMIPI_write_cmos_sensor(0x2a,0xba); 
			GC03102NDMIPI_write_cmos_sensor(0x2b,0x06); //7.14fps
			GC03102NDMIPI_write_cmos_sensor(0x2c,0x36);
			GC03102NDMIPI_write_cmos_sensor(0x2d, 0x07);   //exp level 3  5.55fps
			GC03102NDMIPI_write_cmos_sensor(0x2e, 0x74); 
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
			break;

		case AE_FLICKER_MODE_60HZ:
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00); 
			GC03102NDMIPI_write_cmos_sensor(0x05, 0x02); 	
			GC03102NDMIPI_write_cmos_sensor(0x06, 0x60); 
			GC03102NDMIPI_write_cmos_sensor(0x07, 0x00);
			GC03102NDMIPI_write_cmos_sensor(0x08, 0x58);
			
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x25, 0x00);   //anti-flicker step [11:8]
			GC03102NDMIPI_write_cmos_sensor(0x26, 0x60);   //anti-flicker step [7:0]
			
			GC03102NDMIPI_write_cmos_sensor(0x27, 0x02);   //exp level 0  20fps
			GC03102NDMIPI_write_cmos_sensor(0x28, 0xA0); 
			GC03102NDMIPI_write_cmos_sensor(0x29, 0x03);   //exp level 1  13fps
			GC03102NDMIPI_write_cmos_sensor(0x2a, 0xC0); 
			GC03102NDMIPI_write_cmos_sensor(0x2b, 0x06);   //exp level 2  7.5fps
			GC03102NDMIPI_write_cmos_sensor(0x2c, 0x60); 
			GC03102NDMIPI_write_cmos_sensor(0x2d, 0x08);   //exp level 3  5.45fps
			GC03102NDMIPI_write_cmos_sensor(0x2e, 0x40); 
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC03102NDMIPI_set_param_banding */


static BOOL GC03102NDMIPI_set_param_exposure(UINT16 para)
{


	switch (para)
	{
		case AE_EV_COMP_n30:
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x13, 0x20);
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n20:
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x13, 0x28);
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_n10:
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x13, 0x30);
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;					
		
		case AE_EV_COMP_00:		
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x13, 0x38);//35
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_10:			
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x13, 0x40);
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_20:
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x13, 0x48);
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;
		
		case AE_EV_COMP_30:
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x01);
			GC03102NDMIPI_write_cmos_sensor(0x13, 0x50);
			GC03102NDMIPI_write_cmos_sensor(0xfe, 0x00);
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC03102NDMIPI_set_param_exposure */



static UINT32 GC03102NDMIPIYUVSetVideoMode(UINT16 u2FrameRate)    // lanking add
{
  
        GC03102NDMIPI_MPEG4_encode_mode = KAL_TRUE;
     if (u2FrameRate == 30)
   	{
   	
   	    /*********video frame ************/
		
   	}
    else if (u2FrameRate == 15)       
    	{
    	
   	    /*********video frame ************/
		
    	}
    else
   	{
   	
            SENSORDB("Wrong Frame Rate"); 
			
   	}

      return TRUE;

}


static UINT32 GC03102NDMIPIYUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{

#ifdef DEBUG_SENSOR_GC03102NDMIPI
		printk("______%s______ Tflash debug \n",__func__);
		return TRUE;
#endif

    switch (iCmd) {
    case FID_AWB_MODE:
        GC03102NDMIPI_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        GC03102NDMIPI_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        GC03102NDMIPI_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        GC03102NDMIPI_set_param_banding(iPara);
		break;
    case FID_SCENE_MODE:
	 GC03102NDMIPINightMode(iPara);
        break;
    default:
        break;
    }
    return TRUE;
} /* GC03102NDMIPIYUVSensorSetting */


static UINT32 GC03102NDMIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    unsigned long long *feature_data=(unsigned long long *) pFeaturePara;

	
	MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
	MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

	switch (FeatureId)
	{
		case SENSOR_FEATURE_GET_RESOLUTION:
			*pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
			*pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PERIOD:
			*pFeatureReturnPara16++=IMAGE_SENSOR_PV_WIDTH;
			*pFeatureReturnPara16=IMAGE_SENSOR_PV_HEIGHT;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			//*pFeatureReturnPara32 = GC03102NDMIPI_sensor_pclk/10;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_SET_ESHUTTER:
		break;
		case SENSOR_FEATURE_SET_NIGHTMODE:
			GC03102NDMIPINightMode((BOOL) *feature_data);
		break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		break;
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			GC03102NDMIPI_isp_master_clock=*pFeatureData32;
		break;
		case SENSOR_FEATURE_SET_REGISTER:
			GC03102NDMIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
		break;
		case SENSOR_FEATURE_GET_REGISTER:
			pSensorRegData->RegData = GC03102NDMIPI_read_cmos_sensor(pSensorRegData->RegAddr);
		break;
		case SENSOR_FEATURE_GET_CONFIG_PARA:
			memcpy(pSensorConfigData, &GC03102NDMIPISensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
			*pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
		break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:

		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
		case SENSOR_FEATURE_GET_GROUP_INFO:
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
		break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
                        *pFeatureReturnPara32++=0;
                        *pFeatureParaLen=4;	    
		    break; 
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
			// get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
			// if EEPROM does not exist in camera module.
			*pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
			*pFeatureParaLen=4;
		break;
		case SENSOR_FEATURE_CHECK_SENSOR_ID:
			 GC03102NDMIPIGetSensorID(pFeatureData32);
			 break;
		case SENSOR_FEATURE_SET_YUV_CMD:
		       printk("GC03102NDMIPI YUV sensor Setting:%d, %d \n", *pFeatureData32,  *(pFeatureData32+1));
			GC03102NDMIPIYUVSensorSetting((FEATURE_ID)*feature_data, *(feature_data+1));
		break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
		       GC03102NDMIPIYUVSetVideoMode(*feature_data);
		       break; 
	break;
    default:
        break;
	}
return ERROR_NONE;
}	/* GC03102NDMIPIFeatureControl() */


static SENSOR_FUNCTION_STRUCT	SensorFuncGC03102NDMIPIYUV=
{
	GC03102NDMIPIOpen,
	GC03102NDMIPIGetInfo,
	GC03102NDMIPIGetResolution,
	GC03102NDMIPIFeatureControl,
	GC03102NDMIPIControl,
	GC03102NDMIPIClose
};


UINT32 GC03102ND_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncGC03102NDMIPIYUV;
	return ERROR_NONE;
} /* SensorInit() */
