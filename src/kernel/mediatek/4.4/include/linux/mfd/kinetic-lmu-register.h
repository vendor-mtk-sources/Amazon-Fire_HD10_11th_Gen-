/*
 * KINETIC LMU (Lighting Management Unit) Devices
 *
 * Author: Bruce pu <bruce.xm.pu@enskytech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __KTZ8864A_REGISTER_H__
#define __KTZ8864A_REGISTER_H__

#include <linux/bitops.h>

/* KTZ8864A */
#define KTZ8864A_REG_REV				0x01

#define KTZ8864A_REG_BL_CFG_1			0x02
#define KTZ8864A_OVP_SHIFT			5
#define KTZ8864A_OVP_MASK			(BIT(5) | BIT(6) | BIT(7))
#define KTZ8864A_OVP_17V				0
#define KTZ8864A_OVP_21V				(1 << KTZ8864A_OVP_SHIFT)
#define KTZ8864A_OVP_25V				(2 << KTZ8864A_OVP_SHIFT)
#define KTZ8864A_OVP_29V				(3 << KTZ8864A_OVP_SHIFT)
#define KTZ8864A_OVP_19V				(4 << KTZ8864A_OVP_SHIFT)
#define KTZ8864A_OVP_23V				(5 << KTZ8864A_OVP_SHIFT)
#define KTZ8864A_OVP_27V				(6 << KTZ8864A_OVP_SHIFT)
#define KTZ8864A_OVP_32V				(7 << KTZ8864A_OVP_SHIFT)
#define KTZ8864A_PWM_SHIFT			0
#define KTZ8864A_PWM_MASK			BIT(0)
#define KTZ8864A_PWM_DISABLE			(0 << KTZ8864A_PWM_SHIFT)
#define KTZ8864A_PWM_ENABLE			(1 << KTZ8864A_PWM_SHIFT)

#define KTZ8864A_REG_BL_CFG_2			0x03
#define KTZ8864A_REG_BRT_LSB			0x04
#define KTZ8864A_REG_BRT_MSB			0x05
#define KTZ8864A_REG_BL_EN			0x08

#define KTZ8864A_REG_BIAS_CONFIG_1		0x09
#define KTZ8864A_EXT_EN_MASK			BIT(0)
#define KTZ8864A_EN_VNEG_MASK			BIT(1)
#define KTZ8864A_EN_VPOS_MASK			BIT(2)

#define KTZ8864A_REG_BIAS_CONFIG_2		0x0a
#define KTZ8864A_REG_BIAS_CONFIG_3		0x0b
#define KTZ8864A_REG_VOUT_BOOST			0x0c
#define KTZ8864A_REG_VOUT_POS			0x0d
#define KTZ8864A_REG_VOUT_NEG			0x0e
#define KTZ8864A_VOUT_MASK			0x3F

#define KTZ8864A_REG_FLAGS			0x0F
#define KTZ8864A_BL_OCP_MASK			BIT(0)
#define KTZ8864A_BL_OVP_MASK			BIT(1)
#define KTZ8864A_VNEG_SHORT_MASK			BIT(2)
#define KTZ8864A_VPOS_SHORT_MASK			BIT(3)
#define KTZ8864A_TSD_MASK			BIT(6)
#define KTZ8864A_LED_SHORT_MASK			BIT(7)

#define KTZ8864A_REG_OPTION_1			0x10

#define KTZ8864A_REG_OPTION_2			0x11
#define KTZ8864A_BOOST_CUR_LIMIT_SHIFT		0
#define KTZ8864A_BOOST_CUR_LIMIT_MASK		(BIT(0) | BIT(1))
#define KTZ8864A_BOOST_CUR_LIMIT_1_2_A		(0 << KTZ8864A_BOOST_CUR_LIMIT_SHIFT)
#define KTZ8864A_BOOST_CUR_LIMIT_1_5_A		(1 << KTZ8864A_BOOST_CUR_LIMIT_SHIFT)
#define KTZ8864A_BOOST_CUR_LIMIT_1_8_A		(2 << KTZ8864A_BOOST_CUR_LIMIT_SHIFT)
#define KTZ8864A_BOOST_CUR_LIMIT_2_1_A		(3 << KTZ8864A_BOOST_CUR_LIMIT_SHIFT)


#define KTZ8864A_MAX_REG				0x15

#endif //__KTZ8864A_REGISTER_H__
