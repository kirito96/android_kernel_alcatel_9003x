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
#include "kd_camera_feature.h"
#include <linux/regulator/consumer.h>
#include <mt-plat/upmu_common.h>


#include <mach/gpio_const.h>
#include <mt-plat/mt_gpio.h>



/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[kd_camera_hw]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(PFX fmt, ##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)   pr_err(fmt, ##arg)
#define PK_XLOG_INFO(fmt, args...) \
		do {    \
		   pr_debug(PFX fmt, ##arg); \
		} while (0)
#else
#define PK_DBG(a, ...)
#define PK_ERR(a, ...)
#define PK_XLOG_INFO(fmt, args...)
#endif

/* GPIO Pin control*/
struct platform_device *cam_plt_dev = NULL;
struct pinctrl *camctrl = NULL;
struct pinctrl_state *cam0_pnd_h = NULL;
struct pinctrl_state *cam0_pnd_l = NULL;
struct pinctrl_state *cam0_rst_h = NULL;
struct pinctrl_state *cam0_rst_l = NULL;
struct pinctrl_state *cam1_pnd_h = NULL;
struct pinctrl_state *cam1_pnd_l = NULL;
struct pinctrl_state *cam1_rst_h = NULL;
struct pinctrl_state *cam1_rst_l = NULL;
struct pinctrl_state *cam_ldo0_h = NULL;
struct pinctrl_state *cam_ldo0_l = NULL;

int mtkcam_gpio_init(struct platform_device *pdev)
{
	int ret = 0;

	camctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(camctrl)) {
		dev_err(&pdev->dev, "Cannot find camera pinctrl!");
		ret = PTR_ERR(camctrl);
	}
	/*Cam0 Power/Rst Ping initialization */
	cam0_pnd_h = pinctrl_lookup_state(camctrl, "cam0_pnd1");
	if (IS_ERR(cam0_pnd_h)) {
		ret = PTR_ERR(cam0_pnd_h);
		pr_debug("%s : pinctrl err, cam0_pnd_h\n", __func__);
	}

	cam0_pnd_l = pinctrl_lookup_state(camctrl, "cam0_pnd0");
	if (IS_ERR(cam0_pnd_l)) {
		ret = PTR_ERR(cam0_pnd_l);
		pr_debug("%s : pinctrl err, cam0_pnd_l\n", __func__);
	}


	cam0_rst_h = pinctrl_lookup_state(camctrl, "cam0_rst1");
	if (IS_ERR(cam0_rst_h)) {
		ret = PTR_ERR(cam0_rst_h);
		pr_debug("%s : pinctrl err, cam0_rst_h\n", __func__);
	}

	cam0_rst_l = pinctrl_lookup_state(camctrl, "cam0_rst0");
	if (IS_ERR(cam0_rst_l)) {
		ret = PTR_ERR(cam0_rst_l);
		pr_debug("%s : pinctrl err, cam0_rst_l\n", __func__);
	}

	/*Cam1 Power/Rst Ping initialization */
	cam1_pnd_h = pinctrl_lookup_state(camctrl, "cam1_pnd1");
	if (IS_ERR(cam1_pnd_h)) {
		ret = PTR_ERR(cam1_pnd_h);
		pr_debug("%s : pinctrl err, cam1_pnd_h\n", __func__);
	}

	cam1_pnd_l = pinctrl_lookup_state(camctrl, "cam1_pnd0");
	if (IS_ERR(cam1_pnd_l )) {
		ret = PTR_ERR(cam1_pnd_l );
		pr_debug("%s : pinctrl err, cam1_pnd_l\n", __func__);
	}


	cam1_rst_h = pinctrl_lookup_state(camctrl, "cam1_rst1");
	if (IS_ERR(cam1_rst_h)) {
		ret = PTR_ERR(cam1_rst_h);
		pr_debug("%s : pinctrl err, cam1_rst_h\n", __func__);
	}


	cam1_rst_l = pinctrl_lookup_state(camctrl, "cam1_rst0");
	if (IS_ERR(cam1_rst_l)) {
		ret = PTR_ERR(cam1_rst_l);
		pr_debug("%s : pinctrl err, cam1_rst_l\n", __func__);
	}
	/*externel LDO enable */
	cam_ldo0_h = pinctrl_lookup_state(camctrl, "cam_ldo0_1");
	if (IS_ERR(cam_ldo0_h)) {
		ret = PTR_ERR(cam_ldo0_h);
		pr_debug("%s : pinctrl err, cam_ldo0_h\n", __func__);
	}


	cam_ldo0_l = pinctrl_lookup_state(camctrl, "cam_ldo0_0");
	if (IS_ERR(cam_ldo0_l)) {
		ret = PTR_ERR(cam_ldo0_l);
		pr_debug("%s : pinctrl err, cam_ldo0_l\n", __func__);
	}
	return ret;
}

int mtkcam_gpio_set(int PinIdx, int PwrType, int Val)
{
	int ret = 0;

	switch (PwrType) {
	case CAMRST:
		if (PinIdx == 0) {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam0_rst_l);
			else
				pinctrl_select_state(camctrl, cam0_rst_h);
		} else {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam1_rst_l);
			else
				pinctrl_select_state(camctrl, cam1_rst_h);
		}
		break;
	case CAMPDN:
		if (PinIdx == 0) {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam0_pnd_l);
			else
				pinctrl_select_state(camctrl, cam0_pnd_h);
		} else {
			if (Val == 0)
				pinctrl_select_state(camctrl, cam1_pnd_l);
			else
				pinctrl_select_state(camctrl, cam1_pnd_h);
		}

		break;
	case CAMLDO:
		if (Val == 0)
			pinctrl_select_state(camctrl, cam_ldo0_l);
		else
			pinctrl_select_state(camctrl, cam_ldo0_h);
		break;
	default:
		PK_DBG("PwrType(%d) is invalid !!\n", PwrType);
		break;
	};

	PK_DBG("PinIdx(%d) PwrType(%d) val(%d)\n", PinIdx, PwrType, Val);

	return ret;
}


int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On,
		       char *mode_name)
{

	u32 pinSetIdx = 0;	/* default main sensor */

#define IDX_PS_CMRST 0
#define IDX_PS_CMPDN 4
#define IDX_PS_MODE 1
#define IDX_PS_ON   2
#define IDX_PS_OFF  3
#define VOL_2800 2800000
#define VOL_1800 1800000
#define VOL_1500 1500000
#define VOL_1200 1200000
#define VOL_1000 1000000

	if (DUAL_CAMERA_MAIN_SENSOR == SensorIdx)
		pinSetIdx = 0;
	else if (DUAL_CAMERA_SUB_SENSOR == SensorIdx)
		pinSetIdx = 1;
	else if (DUAL_CAMERA_MAIN_2_SENSOR == SensorIdx)
		pinSetIdx = 2;

	/* power ON */
	if (On) {

		printk("[PowerON]pinSetIdx:%d, currSensorName: %s\n", pinSetIdx, currSensorName);
    if(currSensorName && pinSetIdx == 0 && (0 == strcmp(currSensorName, SENSOR_DRVNAME_OV5670_MIPI_RAW))){//LATAE 1ST REAR 
    	printk("zgx_ov5670_power_on_sequence\n");
    	ISP_MCLK1_EN(1);
    	/* First Power Pin low and Reset Pin Low */
    	mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);
    	mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);
        mtkcam_gpio_set(1 - pinSetIdx, CAMPDN,GPIO_OUT_ONE);	
			/* VCAM_A */
			if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);
			
			/* VCAM_IO */
			if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable IO power (VCAM_IO),power id = %d\n",VCAMIO);
				goto _kdCISModulePowerOn_exit_;
			}
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);			
			
			mdelay(5);
/*[BUGFIX]-Mod-BEGIN by TCTSZ.(gaoxiang.zou@tcl.com), PR1193041 12/21/2015*/
			if (TRUE != _hwPowerOn(VCAMD, VOL_1200)) {//VOL_1800->VOL_1200
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n",VCAMD);
				goto _kdCISModulePowerOn_exit_;
			}
/*[BUGFIX]-Mod-END by TCTSZ.(gaoxiang.zou@tcl.com), PR1193041*/
			mdelay(5);	
						
			mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ONE);			
			
		  mdelay(5);
						

    } else if (currSensorName && pinSetIdx == 0 && (0 == strcmp(SENSOR_DRVNAME_SP2508_RAW ,currSensorName))){//EMEA 1ST REAR UP

      printk("zgx_sp2508_power_on_sequence\n");
    	/* First Power Pin low and Reset Pin Low */
    	mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);
    	mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);
      mdelay(5);
			/* VCAM_A */
			if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);
			
			/* VCAM_IO */
			if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable IO power (VCAM_IO),power id = %d\n",VCAMIO);
				goto _kdCISModulePowerOn_exit_;
			}
			mdelay(5);
			
			if (TRUE != _hwPowerOn(VCAMD, VOL_1800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n",VCAMD);
				goto _kdCISModulePowerOn_exit_;
			}	
			mdelay(5);
			ISP_MCLK1_EN(1);
			mdelay(5);						
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);			
			mdelay(100);
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);	
			mdelay(1);			
			mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ONE);						
		  	mdelay(5);    	
    	
    } else if (currSensorName && pinSetIdx == 0 && (0 == strcmp(SENSOR_DRVNAME_GC23552ND_MIPI_RAW ,currSensorName))) {//EMEA 2ND REAR ON
			printk("zgx_gc23552nd_power_on_sequence\n");
				/* First Power Pin high and Reset Pin Low */
				
				mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);
				mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);
	
				mdelay(5);
				/* VCAM_IO */
				if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable IO power (VCAM_IO),power id = %d\n",VCAMIO);
					goto _kdCISModulePowerOn_exit_;
				}
	
	
				mdelay(5);
	
      #if 1
				if (TRUE != _hwPowerOn(VCAMD, VOL_1800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n",VCAMD);
				goto _kdCISModulePowerOn_exit_;
			}	
			#else 
				printk("Special DVDD Power Source !!!\n");
				pmic_set_register_value(PMIC_RG_VIBR_EN, 0);
				pmic_set_register_value(PMIC_RG_VIBR_VOSEL,0x3);
				pmic_set_register_value(PMIC_RG_VIBR_EN, 1);
			#endif
				mdelay(5);
				/* VCAM_A */
				if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
					PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
					goto _kdCISModulePowerOn_exit_;
				}
	
				mdelay(5);
				ISP_MCLK1_EN(1);
				mdelay(5);
		  		mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);
		  		mdelay(5);
		  		mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ONE);
				mdelay(5);
		}	else if (currSensorName && pinSetIdx == 1 && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW ,currSensorName))) {//LATAE 1ST FRONT ON
    	printk("zgx_gc2355_power_on_sequence\n");
			/* First Power Pin high and Reset Pin Low */
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);
			mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);

			mdelay(5);
			/* VCAM_IO */
			if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable IO power (VCAM_IO),power id = %d\n",VCAMIO);
				goto _kdCISModulePowerOn_exit_;
			}


			mdelay(5);

      #if 0
			if (TRUE != _hwPowerOn(VCAMD, VOL_1800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_D),power id = %d\n", VCAMD);
				goto _kdCISModulePowerOn_exit_;
			}
			#else 
			printk("Special DVDD Power Source !!!\n");
			pmic_set_register_value(PMIC_RG_VIBR_EN, 0);
			pmic_set_register_value(PMIC_RG_VIBR_VOSEL,0x3);
			pmic_set_register_value(PMIC_RG_VIBR_EN, 1);
			#endif
			mdelay(5);
			/* VCAM_A */
			if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A),power id = %d\n",VCAMA);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);
      		ISP_MCLK2_EN(1);
      		mdelay(5);
      		mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);
      		mdelay(5);
      		mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ONE);
			mdelay(5);
		} else if (currSensorName && pinSetIdx == 1 && (0 == strcmp(SENSOR_DRVNAME_GC0310_MIPI_YUV ,currSensorName)||0 == strcmp(SENSOR_DRVNAME_GC03102ND_MIPI_YUV ,currSensorName)) ) {//EMEA 1ST FRONT ON
			/* First Power Pin low */
			printk("zgx_gc0310_power_on_sequence\n");		
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);

			/* VCAM_IO */
			if (TRUE != _hwPowerOn(VCAMIO, VOL_1800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable digital power (VCAM_IO), power id = %d\n",VCAMIO);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);
			
			/*VCAM_A */
			if (TRUE != _hwPowerOn(VCAMA, VOL_2800)) {
				PK_DBG("[CAMERA SENSOR] Fail to enable analog power (VCAM_A), power id = %d\n",VCAMA);
				goto _kdCISModulePowerOn_exit_;
			}

			mdelay(5);
      		ISP_MCLK2_EN(1);
      		mdelay(5);
      
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);
			mdelay(5);
      		mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);	
			mdelay(5);
			
		}
	} else {		/* power OFF */
    printk("[PowerOFF]pinSetIdx:%d, currSensorName: %s\n", pinSetIdx, currSensorName);
    if(currSensorName && pinSetIdx == 0 && (0 == strcmp(currSensorName, SENSOR_DRVNAME_OV5670_MIPI_RAW))){//LATAE 1ST REAR DOWN
    	printk("zgx_OV5670_power_off_sequence\n");
    	ISP_MCLK1_EN(0);
    	mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);
        mtkcam_gpio_set(1 - pinSetIdx, CAMPDN,GPIO_OUT_ZERO); 	
    	mdelay(5);
    	
    	if (TRUE != _hwPowerDown(VCAMD)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n", VCAMD);
				goto _kdCISModulePowerOn_exit_;
			}
			mdelay(5);
			/* VCAM_A */
			if (TRUE != _hwPowerDown(VCAMA)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
      		mdelay(5);
      
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(VCAMIO)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);				
		  	mdelay(5);
		  		
    } else if (currSensorName && pinSetIdx == 0 && pinSetIdx == 0 && (0 == strcmp(SENSOR_DRVNAME_SP2508_RAW ,currSensorName))){//EMEA 1ST REAR DOWN
    	printk("zgx_SP2508_power_off_sequence\n");
    	ISP_MCLK1_EN(0);
    	mdelay(5);
    	mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);
    	mdelay(5);
    	
    	if (TRUE != _hwPowerDown(VCAMD)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n", VCAMD);
				goto _kdCISModulePowerOn_exit_;
			}
			
			mdelay(5);
      
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(VCAMIO)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			
			mdelay(5);
			/* VCAM_A */
			if (TRUE != _hwPowerDown(VCAMA)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			mdelay(5);
	
    } else if (currSensorName && pinSetIdx == 0 && (0 == strcmp(SENSOR_DRVNAME_GC23552ND_MIPI_RAW, currSensorName))) {//EMEA 2ND REAR DOWN
    	printk("zgx_gc23552nd_power_off_sequence\n");
    	mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);   	
    	mdelay(5);
    	mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);
    	mdelay(5);
		  ISP_MCLK1_EN(0);
    	mdelay(5);
    	
			/* VCAM_A */
			if (TRUE != _hwPowerDown(VCAMA)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
      	mdelay(5);

      #if 1
    	if (TRUE != _hwPowerDown(VCAMD)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n", VCAMD);
				goto _kdCISModulePowerOn_exit_;
			}
			#else 
			printk("Special DVDD Power Down!!!\n");
			pmic_set_register_value(PMIC_RG_VIBR_EN,0); 	
			#endif
			mdelay(5);
			      
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(VCAMIO)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			mdelay(5);
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);				
		  mdelay(5);
		}	else if (currSensorName && pinSetIdx == 1 && (0 == strcmp(SENSOR_DRVNAME_GC2355_MIPI_RAW, currSensorName))) {//LATAE 1ST FRONT DOWN
			printk("zgx_gc2355_power_off_sequence\n");
			mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);	
			mdelay(5);
			mtkcam_gpio_set(pinSetIdx, CAMRST,GPIO_OUT_ZERO);
			mdelay(5);
			ISP_MCLK2_EN(0);
			mdelay(5);
			
				/* VCAM_A */
				if (TRUE != _hwPowerDown(VCAMA)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
		  mdelay(5);
	
      #if 0
			if (TRUE != _hwPowerDown(VCAMD)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF core power (VCAM_D),power id = %d\n", VCAMD);
					goto _kdCISModulePowerOn_exit_;
				}
			#else 
				printk("Special DVDD Power Down!!!\n");
				pmic_set_register_value(PMIC_RG_VIBR_EN,0); 	
			#endif
				mdelay(5);
					  
				/* VCAM_IO */
				if (TRUE != _hwPowerDown(VCAMIO)) {
					PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
					/* return -EIO; */
					goto _kdCISModulePowerOn_exit_;
				}
				mdelay(5);
				mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);				
			  mdelay(5);
	
			
		} else if (currSensorName && pinSetIdx == 1 && (0 == strcmp(SENSOR_DRVNAME_GC0310_MIPI_YUV ,currSensorName)||0 == strcmp(SENSOR_DRVNAME_GC03102ND_MIPI_YUV ,currSensorName)) ) {//EMEA 1ST FRONT DOWN
		printk("zgx_gc0310_power_off_sequence\n");
    	mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ONE);   	
    	mdelay(5);
    	ISP_MCLK2_EN(0);
    	mdelay(5);
    	
			/* VCAM_A */
			if (TRUE != _hwPowerDown(VCAMA)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF analog power (VCAM_A),power id= (%d)\n",VCAMA);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
      		mdelay(5);
	      
			/* VCAM_IO */
			if (TRUE != _hwPowerDown(VCAMIO)) {
				PK_DBG("[CAMERA SENSOR] Fail to OFF digital power (VCAM_IO),power id = %d\n",VCAMIO);
				/* return -EIO; */
				goto _kdCISModulePowerOn_exit_;
			}
			mdelay(5);
		//	mtkcam_gpio_set(pinSetIdx, CAMPDN,GPIO_OUT_ZERO);
		//	mdelay(5);   	
		}

	}

	return 0;

_kdCISModulePowerOn_exit_:
	return -EIO;

}

EXPORT_SYMBOL(kdCISModulePowerOn);

/* !-- */
/*  */
