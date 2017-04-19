
#include <linux/string.h>
//#include <mach/mt_gpio.h>
//#include <mach/mt_pm_ldo.h>
#include "lcm_drv.h"
//#include <cust_gpio_usage.h>
#include <mach/upmu_hw.h>
#include <mt-plat/mt_gpio_core.h>
#include <mach/gpio_const.h>
#include <mt-plat/mt_gpio.h>
#include "../code/first/cfg.h"

#if defined(BUILD_LK)
#define LCM_PRINT printf
#elif defined(BUILD_UBOOT)
#define LCM_PRINT printf
#else
#define LCM_PRINT printk
#endif

#ifndef U32
#define U32 unsigned int
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(600)
#define FRAME_HEIGHT 										(1024)

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};
extern U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT);

//#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

extern unsigned int pmic_config_interface(unsigned int RegNum, unsigned int val, unsigned int MASK, unsigned int SHIFT);
// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
       

#define GPIO_LCM_RESET_PIN 		(GPIO70 | 0x80000000)
#define GPIO_LCM_PWR_EN_3V3_PIN		(GPIO4  | 0x80000000)
#define GPIO_LCM_ID1_PIN		(GPIO1  | 0x80000000)
#define GPIO_LCM_ID2_PIN                (GPIO24 | 0x80000000)
#define GPIO_LCM_DSI_TE_PIN		(GPIO68 | 0x80000000)
#define GPIO_LCM_BIAS_EN_P_PIN         	(GPIO12 | 0x80000000)
#define GPIO_LCM_BIAS_EN_N_PIN          (GPIO9 | 0x80000000)

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
	{0x28, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{0x10, 0, {}},
	{REGFLAG_DELAY, 120, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}

};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++)
	{
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd)
		{
			case REGFLAG_DELAY :
				MDELAY(table[i].count);
				break;

			case REGFLAG_END_OF_TABLE :
				break;
			default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    // enable tearing-free
    params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
    params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    params->dsi.packet_size=256;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active				= 2;
    params->dsi.vertical_backporch					= 8;
    params->dsi.vertical_frontporch					= 22;
    params->dsi.vertical_active_line				= FRAME_HEIGHT;

    params->dsi.horizontal_sync_active				= 24;
    params->dsi.horizontal_backporch				= 36;
    params->dsi.horizontal_frontporch				= 100;
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
    params->dsi.PLL_CLOCK = 274;		//288, div by 26
    params->dsi.esd_check_enable = 1;
//    params->dsi.noncont_clock = 1;
//    params->dsi.CLK_TRAIL = 4;          //for CLK_TRAIL > 60ns
}

static void lcm_3v3_power_on(void)
{
	mt_set_gpio_mode(GPIO_LCM_PWR_EN_3V3_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_PWR_EN_3V3_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_PWR_EN_3V3_PIN, GPIO_OUT_ONE);
}

static void lcm_3v3_power_off(void)
{
	mt_set_gpio_mode(GPIO_LCM_PWR_EN_3V3_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_PWR_EN_3V3_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_PWR_EN_3V3_PIN, GPIO_OUT_ZERO);
}

static void lcm_1v8_power_on(void)
{
        pmic_config_interface(MT6350_PMIC_RG_VGP3_VOSEL_ADDR, 0x6, MT6350_PMIC_RG_VGP3_STB_SEL_MASK, MT6350_PMIC_RG_VGP3_STB_SEL_SHIFT);
        pmic_config_interface(MT6350_PMIC_RG_VGP3_EN_ADDR, 0x1, MT6350_PMIC_RG_VGP3_EN_MASK, MT6350_PMIC_RG_VGP3_EN_SHIFT);
}

static void lcm_1v8_power_off(void)
{
        pmic_config_interface(MT6350_PMIC_RG_VGP3_VOSEL_ADDR, 0x0, MT6350_PMIC_RG_VGP3_STB_SEL_MASK, MT6350_PMIC_RG_VGP3_STB_SEL_SHIFT);
        pmic_config_interface(MT6350_PMIC_RG_VGP3_EN_ADDR, 0x0, MT6350_PMIC_RG_VGP3_EN_MASK, MT6350_PMIC_RG_VGP3_EN_SHIFT);
}

static void lcm_init(void)
{
	LCM_PRINT("[LCD] hx8279a_tdt_dsi_vdo init \n");

	mt_set_gpio_mode(GPIO_LCM_DSI_TE_PIN, GPIO_MODE_06);
        mt_set_gpio_dir(GPIO_LCM_DSI_TE_PIN, GPIO_DIR_IN);

	mt_set_gpio_mode(GPIO_LCM_BIAS_EN_P_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_BIAS_EN_P_PIN, GPIO_DIR_OUT);

	mt_set_gpio_mode(GPIO_LCM_BIAS_EN_N_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_BIAS_EN_N_PIN, GPIO_DIR_OUT);

	mt_set_gpio_mode(GPIO_LCM_RESET_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCM_RESET_PIN, GPIO_DIR_OUT);

	mt_set_gpio_out(GPIO_LCM_BIAS_EN_P_PIN, GPIO_OUT_ZERO);
	mt_set_gpio_out(GPIO_LCM_BIAS_EN_N_PIN, GPIO_OUT_ZERO);
	mt_set_gpio_out(GPIO_LCM_RESET_PIN, GPIO_OUT_ZERO);

	lcm_3v3_power_off();
	lcm_1v8_power_off();

	MDELAY(10);

	lcm_3v3_power_on();
	lcm_1v8_power_on();
		
	MDELAY(10);

	mt_set_gpio_out(GPIO_LCM_BIAS_EN_P_PIN, GPIO_OUT_ONE);
	mt_set_gpio_out(GPIO_LCM_BIAS_EN_N_PIN, GPIO_OUT_ONE);

	MDELAY(10);

	mt_set_gpio_out(GPIO_LCM_RESET_PIN, GPIO_OUT_ONE);

	MDELAY(10);

	push_table(lcm_initialization_setting_first, sizeof(lcm_initialization_setting_first) / sizeof(struct LCM_setting_table), 1);
}


static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

	mt_set_gpio_mode(GPIO_LCM_BIAS_EN_P_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_BIAS_EN_P_PIN, GPIO_DIR_OUT);

	mt_set_gpio_mode(GPIO_LCM_BIAS_EN_N_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_BIAS_EN_N_PIN, GPIO_DIR_OUT);

        mt_set_gpio_mode(GPIO_LCM_RESET_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_RESET_PIN, GPIO_DIR_OUT);

        mt_set_gpio_out(GPIO_LCM_BIAS_EN_P_PIN, GPIO_OUT_ZERO);
	mt_set_gpio_out(GPIO_LCM_BIAS_EN_N_PIN, GPIO_OUT_ZERO);
        mt_set_gpio_out(GPIO_LCM_RESET_PIN, GPIO_OUT_ZERO);

	lcm_3v3_power_off();
        lcm_1v8_power_off();

	LCM_PRINT("[LCD] lcm_suspend \n");
}


static void lcm_resume(void)
{
	lcm_init();

	LCM_PRINT("[LCD] lcm_suspend \n");
}

static unsigned int lcm_compare_id(void)
{
	unsigned int id1_state = 0;
        unsigned int id2_state = 0;

        mt_set_gpio_mode(GPIO_LCM_ID1_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_ID1_PIN, GPIO_DIR_IN);

        mt_set_gpio_mode(GPIO_LCM_ID2_PIN, GPIO_MODE_00);
        mt_set_gpio_dir(GPIO_LCM_ID2_PIN, GPIO_DIR_IN);

        lcm_1v8_power_on();

        MDELAY(10);

        id1_state = mt_get_gpio_in(GPIO_LCM_ID1_PIN);
        id2_state = mt_get_gpio_in(GPIO_LCM_ID2_PIN);

        if ((id1_state == 0) && (id2_state == 0))
        {
                LCM_PRINT("[LCD]hx8279a_tdt lcd selected!\n");
                lcm_1v8_power_off();
                return 1;
        }
        else
        {
                lcm_1v8_power_off();
                return 0;
        }
}

LCM_DRIVER hx8279a_tdt_dsi_vdo_lcm_drv = 
{
    .name			= "hx8279a_tdt_dsi_vdo",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
};
