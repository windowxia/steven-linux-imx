/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright 2023 NXP. */

#ifndef __LINUX_REG_PF5300_H__
#define __LINUX_REG_PF5300_H__

#include <linux/regmap.h>

enum pf5300_chip_type {
	PF5300_TYPE_PF5300 = 0,
	PF5300_TYPE_PF5301 = 1,
	PF5300_TYPE_PF5302 = 2,
	PF5300_TYPE_AMOUNT,
};

enum {
	PF5300_SW1 = 0,
	PF5300_REGULATOR_CNT,
};

enum {
	PF5300_DVS_LEVEL_RUN = 0,
	PF5300_DVS_LEVEL_STANDBY,
	PF5300_DVS_LEVEL_MAX,
};

#define PF5300_SW1_VOLTAGE_NUM	0x100

enum {
	PF5300_REG_DEV_ID	    = 0x00,
	PF5300_REG_REV_ID	    = 0x01,
	PF5300_REG_EMREV	    = 0x02,
	PF5300_REG_PROG_ID	    = 0x03,
	PF5300_REG_CONFIG1	    = 0x04,
	PF5300_REG_INT_STATUS1	    = 0x05,
	PF5300_REG_INT_SENSE1	    = 0x06,
	PF5300_REG_INT_STATUS2	    = 0x07,
	PF5300_REG_INT_SENSE2	    = 0x08,
	PF5300_REG_BIST_STAT1	    = 0x09,
	PF5300_REG_BIST_CTRL	    = 0x0A,
	PF5300_REG_STATE	    = 0x0B,
	PF5300_REG_STATE_CTRL       = 0x0C,
	PF5300_REG_SW1_VOLT         = 0x0D,
	PF5300_REG_SW1_STBY_VOLT    = 0x0E,
	PF5300_REG_SW1_CTRL1        = 0x0F,
	PF5300_REG_SW1_CTRL2        = 0x10,
	PF5300_REG_CLK_CTRL         = 0x11,
	PF5300_REG_SEQ_CTRL1        = 0x12,
	PF5300_REG_SEQ_CTRL2        = 0x13,
	PF5300_REG_RANDOM_CHK       = 0x14,
	PF5300_REG_RANDOM_GEN       = 0x15,
	PF5300_REG_WD_CTRL          = 0x16,
	PF5300_REG_WD_SEED          = 0x17,
	PF5300_REG_WD_ANSWER        = 0x18,
	PF5300_REG_FLT_CNT1         = 0x19,
	PF5300_REG_FLT_CNT2         = 0x1A,
	PF5300_MAX_REGISTER,
};


/* PF5300 SW1_CTRL1 */
#define SW_MODE_OFF			0x00
#define SW_MODE_PWM			0x0c

#define SW1_MODE_MASK			0x0C
#define SW1_STBY_MODE_MASK		0x30

#define SW1_RAMP_MASK			0x03

/* PF5300 SW1_VOLT/SW1_STBY_VOLT MASK */
#define SW1_VOLT_MASK			0xFF
#define SW1_STBY_VOLT_MASK		0xFF

/* PF5300_REG_INT_STATUS1 bits */
#define IRQ_SDWN			0x80
#define IRQ_BG_ERR			0x40
#define IRQ_CRC				0x20
#define IRQ_SW1_DVS_DONE		0x10
#define IRQ_SW1_ILIM			0x08
#define IRQ_VMON_UV			0x04
#define IRQ_VMON_OV			0x02
#define IRQ_VIN_OVLO			0x01

/* PF5300_REG_INT_STATUS2 bits */
#define IRQ_PGOOD_STUCK_AT0		0x80
#define IRQ_PGOOD_STUCK_AT1		0x40
#define IRQ_DVS_ERR			0x20
#define IRQ_FSYNC			0x10
#define IRQ_THERM_155			0x08
#define IRQ_THERM_140			0x04
#define IRQ_THERM_125			0x02
#define IRQ_THERM_110			0x01

#endif /* __LINUX_REG_PF5300_H__ */
