
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(MTK_PLATFORM)/include
subdir-ccflags-y += -Werror -I$(srctree)/drivers/misc/mediatek/include/mt-plat

ifeq ($(CONFIG_MTK_HAFG_20),y)
	obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter_fg_20.o battery_common_fg_20.o
else
obj-$(CONFIG_MTK_SMART_BATTERY) += battery_meter.o battery_common.o
endif

#[FEATURE]-Add-BEGIN by huichen@tcl.com, 2015/11/30,mini sw pr 983534
ifeq ($(TARGET_BUILD_MMITEST), true)
KBUILD_CFLAGS += -DTARGET_BUILD_MMITEST
endif
#[FEATURE]-Add-END by huichen@tcl.com


ifeq ($(CONFIG_MTK_FAN5402_SUPPORT),y)
	obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
else ifeq ($(CONFIG_MTK_FAN5405_SUPPORT),y)
	obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
else
	ifeq ($(CONFIG_MTK_SWCHR_SUPPORT),y)
			obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
	else
		ifeq ($(CONFIG_MTK_BQ24158_SUPPORT),y)
			obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
		else
			ifeq ($(CONFIG_MTK_BQ24196_SUPPORT),y)
				obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
			else
				ifeq ($(CONFIG_MTK_BQ24296_SUPPORT),y)
					obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
				else
					ifeq ($(CONFIG_MTK_NCP1851_SUPPORT),y)
						obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
					else
						ifeq ($(CONFIG_MTK_NCP1854_SUPPORT),y)
							obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
						else
							ifeq ($(CONFIG_MTK_BQ24160_SUPPORT),y)
								obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
							else
								ifeq ($(CONFIG_MTK_BQ24157_SUPPORT),y)
									obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
								else
									ifeq ($(CONFIG_MTK_BQ24250_SUPPORT),y)
										obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
									else
										ifeq ($(CONFIG_MTK_BQ24261_SUPPORT),y)
											obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
										else
											ifeq ($(CONFIG_MTK_RT9536_SUPPORT),y)
												obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
											else
												ifeq ($(CONFIG_MTK_BQ25896_SUPPORT),y)
													obj-$(CONFIG_MTK_SMART_BATTERY) += switch_charging.o
												else
													obj-$(CONFIG_MTK_SMART_BATTERY) += linear_charging.o
												endif
											endif
										endif
									endif
								endif
							endif
						endif
					endif
				endif
			endif
		endif
	endif
endif
