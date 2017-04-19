

#ifndef __FIRST_LCM_INIT_CODE_H__
#define __FIRST_LCM_INIT_CODE_H__


#include "lcm_drv.h"

#define REGFLAG_DELAY                                                                   0xFE
#define REGFLAG_END_OF_TABLE                                                            0xFF   // END OF REGISTERS MARKER


struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_initialization_setting_first[] = {

        /*
        Note :

        Data ID will depends on the following rule.
        
                count of parameters > 1 => Data ID = 0x39
                count of parameters = 1 => Data ID = 0x15
                count of parameters = 0 => Data ID = 0x05

        Structure Format :

        {DCS command, count of parameters, {parameter list}}
        {REGFLAG_DELAY, milliseconds of time, {}},

        ...

        Setting ending by predefined flag
        
        {REGFLAG_END_OF_TABLE, 0x00, {}}
        */

        {0xB0, 1, {0x00}},
        {0xC0, 1, {0x15}},
        {0xC4, 1, {0x73}},
        {0xBD, 1, {0x72}},
        {0xBB, 1, {0x02}},
        {0xB0, 1, {0X03}},
        {0xB2, 1, {0xA5}},
        {0xB3, 1, {0x04}},
        {0xB0, 1, {0x06}},
        {0xCA, 1, {0x0B}},
        {0x11, 0, {}},
        {REGFLAG_DELAY, 120, {}},
        {0x29, 0, {}},
        {REGFLAG_DELAY, 120, {}},
        {REGFLAG_END_OF_TABLE, 0x00, {}},
};






#endif
