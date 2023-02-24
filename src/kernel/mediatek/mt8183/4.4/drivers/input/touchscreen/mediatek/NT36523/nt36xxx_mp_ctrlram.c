/*
 * Copyright (C) 2020 Novatek, Inc.
 *
 * $Revision: 60130 $
 * $Date: 2020-04-10 17:43:33 +0800$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include "nt36xxx.h"
#include "nt36xxx_mp_ctrlram.h"

#if NVT_TOUCH_MP

#define NORMAL_MODE 0x00
#define TEST_MODE_1 0x21
#define TEST_MODE_2 0x22
#define MP_MODE_CC 0x41
#define FREQ_HOP_DISABLE 0x66
#define FREQ_HOP_ENABLE 0x65

#define SHORT_TEST_CSV_FILE "/data/local/tmp/ShortTest.csv"
#define OPEN_TEST_CSV_FILE "/data/local/tmp/OpenTest.csv"
#define FW_RAWDATA_CSV_FILE "/data/local/tmp/FWMutualTest.csv"
#define FW_CC_CSV_FILE "/data/local/tmp/FWCCTest.csv"
#define NOISE_TEST_CSV_FILE "/data/local/tmp/NoiseTest.csv"
#define PEN_FW_RAW_TEST_CSV_FILE "/data/local/tmp/PenFWRawTest.csv"
#define PEN_NOISE_TEST_CSV_FILE "/data/local/tmp/PenNoiseTest.csv"

#define nvt_mp_seq_printf(m, fmt, args...) do {	\
	seq_printf(m, fmt, ##args);	\
	if (!nvt_mp_test_result_printed)	\
		printk(fmt, ##args);	\
} while (0)

static uint8_t *RecordResult_Short;
static uint8_t *RecordResult_Open;
static uint8_t *RecordResult_FWMutual;
static uint8_t *RecordResult_FW_CC;
static uint8_t *RecordResult_FW_DiffMax;
static uint8_t *RecordResult_FW_DiffMin;
static uint8_t *RecordResult_PenTipX_Raw;
static uint8_t *RecordResult_PenTipY_Raw;
static uint8_t *RecordResult_PenRingX_Raw;
static uint8_t *RecordResult_PenRingY_Raw;
static uint8_t *RecordResult_PenTipX_DiffMax;
static uint8_t *RecordResult_PenTipX_DiffMin;
static uint8_t *RecordResult_PenTipY_DiffMax;
static uint8_t *RecordResult_PenTipY_DiffMin;
static uint8_t *RecordResult_PenRingX_DiffMax;
static uint8_t *RecordResult_PenRingX_DiffMin;
static uint8_t *RecordResult_PenRingY_DiffMax;
static uint8_t *RecordResult_PenRingY_DiffMin;

static int32_t TestResult_Short;
static int32_t TestResult_Open;
static int32_t TestResult_FW_Rawdata;
static int32_t TestResult_FWMutual;
static int32_t TestResult_FW_CC;
static int32_t TestResult_Noise;
static int32_t TestResult_FW_DiffMax;
static int32_t TestResult_FW_DiffMin;
static int32_t TestResult_Pen_FW_Raw;
static int32_t TestResult_PenTipX_Raw;
static int32_t TestResult_PenTipY_Raw;
static int32_t TestResult_PenRingX_Raw;
static int32_t TestResult_PenRingY_Raw;
static int32_t TestResult_Pen_Noise;
static int32_t TestResult_PenTipX_DiffMax;
static int32_t TestResult_PenTipX_DiffMin;
static int32_t TestResult_PenTipY_DiffMax;
static int32_t TestResult_PenTipY_DiffMin;
static int32_t TestResult_PenRingX_DiffMax;
static int32_t TestResult_PenRingX_DiffMin;
static int32_t TestResult_PenRingY_DiffMax;
static int32_t TestResult_PenRingY_DiffMin;

static int32_t *RawData_Short;
static int32_t *RawData_Open;
static int32_t *RawData_Diff;
static int32_t *RawData_Diff_Min;
static int32_t *RawData_Diff_Max;
static int32_t *RawData_FWMutual;
static int32_t *RawData_FW_CC;
static int32_t *RawData_PenTipX_Raw;
static int32_t *RawData_PenTipY_Raw;
static int32_t *RawData_PenRingX_Raw;
static int32_t *RawData_PenRingY_Raw;
static int32_t *RawData_PenTipX_DiffMin;
static int32_t *RawData_PenTipX_DiffMax;
static int32_t *RawData_PenTipY_DiffMin;
static int32_t *RawData_PenTipY_DiffMax;
static int32_t *RawData_PenRingX_DiffMin;
static int32_t *RawData_PenRingX_DiffMax;
static int32_t *RawData_PenRingY_DiffMin;
static int32_t *RawData_PenRingY_DiffMax;

static struct proc_dir_entry *NVT_proc_selftest_entry;
static int8_t nvt_mp_test_result_printed;
static uint8_t fw_ver;

extern void nvt_change_mode(uint8_t mode);
extern uint8_t nvt_get_fw_pipe(void);
extern void nvt_read_mdata(uint32_t xdata_addr, uint32_t xdata_btn_addr);
extern void nvt_get_mdata(int32_t *buf, uint8_t *m_x_num, uint8_t *m_y_num);
extern void nvt_read_get_num_mdata(uint32_t xdata_addr, int32_t *buffer, uint32_t num);
int32_t nvt_mp_parse_dt(struct device_node *root, const char *node_compatible);
int32_t nvt_mp_parse_header(uint32_t index);
int32_t nvt_mp_parse_csv(void);

/*******************************************************
* Description:
*	Novatek touchscreen allocate buffer for mp selftest.
*
* return:
*	Executive outcomes. 0---succeed. -12---Out of memory
*******************************************************/
static int nvt_mp_buffer_init(void)
{
	size_t RecordResult_BufSize = IC_X_CFG_SIZE * IC_Y_CFG_SIZE + IC_KEY_CFG_SIZE;
	size_t RawData_BufSize = (IC_X_CFG_SIZE * IC_Y_CFG_SIZE + IC_KEY_CFG_SIZE) * sizeof(int32_t);
	size_t Pen_RecordResult_BufSize = max(ts->x_num * ts->y_gang_num, ts->x_gang_num * ts->y_num);
	size_t Pen_RawData_BufSize = max(ts->x_num * ts->y_gang_num, ts->x_gang_num * ts->y_num) * sizeof(int32_t);

	RecordResult_Short = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_Short) {
		NVT_ERR("kzalloc for RecordResult_Short failed!\n");
		return -ENOMEM;
	}

	RecordResult_Open = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_Open) {
		NVT_ERR("kzalloc for RecordResult_Open failed!\n");
		return -ENOMEM;
	}

	RecordResult_FWMutual = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FWMutual) {
		NVT_ERR("kzalloc for RecordResult_FWMutual failed!\n");
		return -ENOMEM;
	}

	RecordResult_FW_CC = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FW_CC) {
		NVT_ERR("kzalloc for RecordResult_FW_CC failed!\n");
		return -ENOMEM;
	}

	RecordResult_FW_DiffMax = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FW_DiffMax) {
		NVT_ERR("kzalloc for RecordResult_FW_DiffMax failed!\n");
		return -ENOMEM;
	}

	RecordResult_FW_DiffMin = (uint8_t *)kzalloc(RecordResult_BufSize, GFP_KERNEL);
	if (!RecordResult_FW_DiffMin) {
		NVT_ERR("kzalloc for RecordResult_FW_DiffMin failed!\n");
		return -ENOMEM;
	}

	if (ts->pen_support) {
		RecordResult_PenTipX_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipX_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenTipX_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipY_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipY_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenTipY_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingX_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingX_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenRingX_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingY_Raw = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingY_Raw) {
			NVT_ERR("kzalloc for RecordResult_PenRingY_Raw failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipX_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipX_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenTipX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipX_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipX_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenTipX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipY_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipY_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenTipY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenTipY_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenTipY_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenTipY_DiffMin failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingX_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingX_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenRingX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingX_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingX_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenRingX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingY_DiffMax = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingY_DiffMax) {
			NVT_ERR("kzalloc for RecordResult_PenRingY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RecordResult_PenRingY_DiffMin = (uint8_t *)kzalloc(Pen_RecordResult_BufSize, GFP_KERNEL);
		if (!RecordResult_PenRingY_DiffMin) {
			NVT_ERR("kzalloc for RecordResult_PenRingY_DiffMin failed!\n");
			return -ENOMEM;
		}
	} /* if (ts->pen_support) */

	RawData_Short = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Short) {
		NVT_ERR("kzalloc for RawData_Short failed!\n");
		return -ENOMEM;
	}

	RawData_Open = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Open) {
		NVT_ERR("kzalloc for RawData_Open failed!\n");
		return -ENOMEM;
	}

	RawData_Diff = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Diff) {
		NVT_ERR("kzalloc for RawData_Diff failed!\n");
		return -ENOMEM;
	}

	RawData_Diff_Min = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Diff_Min) {
		NVT_ERR("kzalloc for RawData_Diff_Min failed!\n");
		return -ENOMEM;
	}

	RawData_Diff_Max = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_Diff_Max) {
		NVT_ERR("kzalloc for RawData_Diff_Max failed!\n");
		return -ENOMEM;
	}

	RawData_FWMutual = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_FWMutual) {
		NVT_ERR("kzalloc for RawData_FWMutual failed!\n");
		return -ENOMEM;
	}

	RawData_FW_CC = (int32_t *)kzalloc(RawData_BufSize, GFP_KERNEL);
	if (!RawData_FW_CC) {
		NVT_ERR("kzalloc for RawData_FW_CC failed!\n");
		return -ENOMEM;
	}

	if (ts->pen_support) {
		RawData_PenTipX_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipX_Raw) {
			NVT_ERR("kzalloc for RawData_PenTipX_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipY_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipY_Raw) {
			NVT_ERR("kzalloc for RawData_PenTipY_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingX_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingX_Raw) {
			NVT_ERR("kzalloc for RawData_PenRingX_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingY_Raw = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingY_Raw) {
			NVT_ERR("kzalloc for RawData_PenRingY_Raw failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipX_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipX_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenTipX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipX_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipX_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenTipX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipY_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipY_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenTipY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenTipY_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenTipY_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenTipY_DiffMin failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingX_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingX_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenRingX_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingX_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingX_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenRingX_DiffMin failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingY_DiffMax = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingY_DiffMax) {
			NVT_ERR("kzalloc for RawData_PenRingY_DiffMax failed!\n");
			return -ENOMEM;
		}

		RawData_PenRingY_DiffMin = (int32_t *)kzalloc(Pen_RawData_BufSize, GFP_KERNEL);
		if (!RawData_PenRingY_DiffMin) {
			NVT_ERR("kzalloc for RawData_PenRingY_DiffMin failed!\n");
			return -ENOMEM;
		}
	} /* if (ts->pen_support) */

	return 0;
}

/*******************************************************
* Description:
*	Novatek touchscreen free buffer for mp selftest.
*
* return:
*	n.a.
*******************************************************/
static void nvt_mp_buffer_deinit(void)
{
	kfree(RecordResult_Short);
	RecordResult_Short = NULL;


	kfree(RecordResult_Open);
	RecordResult_Open = NULL;

	kfree(RecordResult_FWMutual);
	RecordResult_FWMutual = NULL;

	kfree(RecordResult_FW_CC);
	RecordResult_FW_CC = NULL;

	kfree(RecordResult_FW_DiffMax);
	RecordResult_FW_DiffMax = NULL;

	kfree(RecordResult_FW_DiffMin);
	RecordResult_FW_DiffMin = NULL;

	if (ts->pen_support) {
		kfree(RecordResult_PenTipX_Raw);
		RecordResult_PenTipX_Raw = NULL;

		kfree(RecordResult_PenTipY_Raw);
		RecordResult_PenTipY_Raw = NULL;

		kfree(RecordResult_PenRingX_Raw);
		RecordResult_PenRingX_Raw = NULL;

		kfree(RecordResult_PenRingY_Raw);
		RecordResult_PenRingY_Raw = NULL;

		kfree(RecordResult_PenTipX_DiffMax);
		RecordResult_PenTipX_DiffMax = NULL;

		kfree(RecordResult_PenTipX_DiffMin);
		RecordResult_PenTipX_DiffMin = NULL;


		kfree(RecordResult_PenTipY_DiffMax);
		RecordResult_PenTipY_DiffMax = NULL;

		kfree(RecordResult_PenTipY_DiffMin);
		RecordResult_PenTipY_DiffMin = NULL;

		kfree(RecordResult_PenRingX_DiffMax);
		RecordResult_PenRingX_DiffMax = NULL;

		kfree(RecordResult_PenRingX_DiffMin);
		RecordResult_PenRingX_DiffMin = NULL;


		kfree(RecordResult_PenRingY_DiffMax);
		RecordResult_PenRingY_DiffMax = NULL;

		kfree(RecordResult_PenRingY_DiffMin);
		RecordResult_PenRingY_DiffMin = NULL;
	} /* if (ts->pen_support) */

	kfree(RawData_Short);
	RawData_Short = NULL;


	kfree(RawData_Open);
	RawData_Open = NULL;


	kfree(RawData_Diff);
	RawData_Diff = NULL;

	kfree(RawData_Diff_Min);
	RawData_Diff_Min = NULL;

	kfree(RawData_Diff_Max);
	RawData_Diff_Max = NULL;

	kfree(RawData_FWMutual);
	RawData_FWMutual = NULL;

	kfree(RawData_FW_CC);
	RawData_FW_CC = NULL;

	if (ts->pen_support) {
		kfree(RawData_PenTipX_Raw);
		RawData_PenTipX_Raw = NULL;

		kfree(RawData_PenTipY_Raw);
		RawData_PenTipY_Raw = NULL;

		kfree(RawData_PenRingX_Raw);
		RawData_PenRingX_Raw = NULL;

		kfree(RawData_PenRingY_Raw);
		RawData_PenRingY_Raw = NULL;

		kfree(RawData_PenTipX_DiffMax);
		RawData_PenTipX_DiffMax = NULL;

		kfree(RawData_PenTipX_DiffMin);
		RawData_PenTipX_DiffMin = NULL;

		kfree(RawData_PenTipY_DiffMax);
		RawData_PenTipY_DiffMax = NULL;

		kfree(RawData_PenTipY_DiffMin);
		RawData_PenTipY_DiffMin = NULL;

		kfree(RawData_PenRingX_DiffMax);
		RawData_PenRingX_DiffMax = NULL;

		kfree(RawData_PenRingX_DiffMin);
		RawData_PenRingX_DiffMin = NULL;

		kfree(RawData_PenRingY_DiffMax);
		RawData_PenRingY_DiffMax = NULL;

		kfree(RawData_PenRingY_DiffMin);
		RawData_PenRingY_DiffMin = NULL;
	} /* if (ts->pen_support) */
}

static void nvt_print_data_log_in_one_line(int32_t *data, int32_t data_num)
{
	char *tmp_log = NULL;
	int32_t i = 0;

	tmp_log = (char *)kzalloc(data_num * 7 + 1, GFP_KERNEL);
	if (!tmp_log) {
		NVT_ERR("kzalloc for tmp_log failed!\n ");
		return;
	}

	for (i = 0; i < data_num; i++) {
		sprintf(tmp_log + i * 7, "%5d, ", data[i]);
	}
	tmp_log[data_num * 7] = '\0';
	printk("%s", tmp_log);
	kfree(tmp_log);
	tmp_log = NULL;

	return;
}

static void nvt_print_result_log_in_one_line(uint8_t *result, int32_t result_num)
{
	char *tmp_log = NULL;
	int32_t i = 0;

	tmp_log = (char *)kzalloc(result_num * 6 + 1, GFP_KERNEL);
	if (!tmp_log) {
		NVT_ERR("kzalloc for tmp_log failed!\n ");
		return;
	}

	for (i = 0; i < result_num; i++) {
		sprintf(tmp_log + i * 6, "0x%02X, ", result[i]);
	}
	tmp_log[result_num * 6] = '\0';
	printk("%s", tmp_log);
	kfree(tmp_log);
	tmp_log = NULL;

	return;
}

/*******************************************************
* Description:
*	Novatek touchscreen self-test criteria print function.
*
* return:
*	n.a.
*******************************************************/
static void nvt_print_lmt_array(int32_t *array, int32_t x_ch, int32_t y_ch)
{
	int32_t j = 0;

	for (j = 0; j < y_ch; j++) {
		nvt_print_data_log_in_one_line(array + j * x_ch, x_ch);
		printk("\n");
	}
#if TOUCH_KEY_NUM > 0
	nvt_print_data_log_in_one_line(array + y_ch * x_ch, Key_Channel);
	printk("\n");
#endif /* #if TOUCH_KEY_NUM > 0 */
}

static void nvt_print_criteria(void)
{
	NVT_LOG("++\n");

	/* ---PS_Config_Lmt_Short_Rawdata--- */
	printk("PS_Config_Lmt_Short_Rawdata_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Short_Rawdata_P, X_Channel, Y_Channel);
	printk("PS_Config_Lmt_Short_Rawdata_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Short_Rawdata_N, X_Channel, Y_Channel);

	/* ---PS_Config_Lmt_Open_Rawdata--- */
	printk("PS_Config_Lmt_Open_Rawdata_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Open_Rawdata_P, X_Channel, Y_Channel);
	printk("PS_Config_Lmt_Open_Rawdata_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_Open_Rawdata_N, X_Channel, Y_Channel);

	/* ---PS_Config_Lmt_FW_Rawdata--- */
	printk("PS_Config_Lmt_FW_Rawdata_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_P, X_Channel, Y_Channel);
	printk("PS_Config_Lmt_FW_Rawdata_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Rawdata_N, X_Channel, Y_Channel);

	/* ---PS_Config_Lmt_FW_CC--- */
	printk("PS_Config_Lmt_FW_CC_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_CC_P, X_Channel, Y_Channel);
	printk("PS_Config_Lmt_FW_CC_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_CC_N, X_Channel, Y_Channel);

	/* ---PS_Config_Lmt_FW_Diff--- */
	printk("PS_Config_Lmt_FW_Diff_P:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Diff_P, X_Channel, Y_Channel);
	printk("PS_Config_Lmt_FW_Diff_N:\n");
	nvt_print_lmt_array(PS_Config_Lmt_FW_Diff_N, X_Channel, Y_Channel);

	if (ts->pen_support) {
		/* ---PS_Config_Lmt_PenTipX_FW_Raw--- */
		printk("PS_Config_Lmt_PenTipX_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Raw_P, ts->x_num, ts->y_gang_num);
		printk("PS_Config_Lmt_PenTipX_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Raw_N, ts->x_num, ts->y_gang_num);

		/* ---PS_Config_Lmt_PenTipY_FW_Raw--- */
		printk("PS_Config_Lmt_PenTipY_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Raw_P, ts->x_gang_num, ts->y_num);
		printk("PS_Config_Lmt_PenTipY_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Raw_N, ts->x_gang_num, ts->y_num);

		/* ---PS_Config_Lmt_PenRingX_FW_Raw--- */
		printk("PS_Config_Lmt_PenRingX_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Raw_P, ts->x_num, ts->y_gang_num);
		printk("PS_Config_Lmt_PenRingX_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Raw_N, ts->x_num, ts->y_gang_num);

		/* ---PS_Config_Lmt_PenRingY_FW_Raw--- */
		printk("PS_Config_Lmt_PenRingY_FW_Raw_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Raw_P, ts->x_gang_num, ts->y_num);
		printk("PS_Config_Lmt_PenRingY_FW_Raw_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Raw_N, ts->x_gang_num, ts->y_num);

		/* ---PS_Config_Lmt_PenTipX_FW_Diff--- */
		printk("PS_Config_Lmt_PenTipX_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Diff_P, ts->x_num, ts->y_gang_num);
		printk("PS_Config_Lmt_PenTipX_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipX_FW_Diff_N, ts->x_num, ts->y_gang_num);

		/* ---PS_Config_Lmt_PenTipY_FW_Diff--- */
		printk("PS_Config_Lmt_PenTipY_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Diff_P, ts->x_gang_num, ts->y_num);
		printk("PS_Config_Lmt_PenTipY_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenTipY_FW_Diff_N, ts->x_gang_num, ts->y_num);

		/* ---PS_Config_Lmt_PenRingX_FW_Diff--- */
		printk("PS_Config_Lmt_PenRingX_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Diff_P, ts->x_num, ts->y_gang_num);
		printk("PS_Config_Lmt_PenRingX_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingX_FW_Diff_N, ts->x_num, ts->y_gang_num);

		/* ---PS_Config_Lmt_PenRingY_FW_Diff--- */
		printk("PS_Config_Lmt_PenRingY_FW_Diff_P:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Diff_P, ts->x_gang_num, ts->y_num);
		printk("PS_Config_Lmt_PenRingY_FW_Diff_N:\n");
		nvt_print_lmt_array(PS_Config_Lmt_PenRingY_FW_Diff_N, ts->x_gang_num, ts->y_num);
	} /* if (ts->pen_support) */

	NVT_LOG("--\n");
}

static int32_t nvt_save_rawdata_to_csv(int32_t *rawdata, uint8_t x_ch, uint8_t y_ch, const char *file_path, uint32_t offset)
{
	int32_t x = 0;
	int32_t y = 0;
	int32_t iArrayIndex = 0;
	struct file *fp = NULL;
	char *fbufp = NULL;
	mm_segment_t org_fs;
	int32_t write_ret = 0;
	uint32_t output_len = 0;
	loff_t pos = 0;
#if TOUCH_KEY_NUM > 0
	int32_t k = 0;
	int32_t keydata_output_offset = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	printk("%s:++\n", __func__);
	fbufp = (char *)kzalloc(16384, GFP_KERNEL);
	if (!fbufp) {
		NVT_ERR("kzalloc for fbufp failed!\n");
		return -ENOMEM;
	}

	for (y = 0; y < y_ch; y++) {
		for (x = 0; x < x_ch; x++) {
			iArrayIndex = y * x_ch + x;
			sprintf(fbufp + iArrayIndex * 7 + y * 2, "%5d, ", rawdata[iArrayIndex]);
		}
		nvt_print_data_log_in_one_line(rawdata + y * x_ch, x_ch);
		printk("\n");
		sprintf(fbufp + (iArrayIndex + 1) * 7 + y * 2, "\r\n");
	}
#if TOUCH_KEY_NUM > 0
	keydata_output_offset = y_ch * x_ch * 7 + y_ch * 2;
	for (k = 0; k < Key_Channel; k++) {
		iArrayIndex = y_ch * x_ch + k;
		sprintf(fbufp + keydata_output_offset + k * 7, "%5d, ", rawdata[iArrayIndex]);
	}
	nvt_print_data_log_in_one_line(rawdata + y_ch * x_ch, Key_Channel);
	printk("\n");
	sprintf(fbufp + y_ch * x_ch * 7 + y_ch * 2 + Key_Channel * 7, "\r\n");
#endif /* #if TOUCH_KEY_NUM > 0 */

	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		NVT_ERR("open %s failed\n", file_path);
		set_fs(org_fs);
		kfree(fbufp);
		fbufp = NULL;

		return -1;
	}

#if TOUCH_KEY_NUM > 0
	output_len = y_ch * x_ch * 7 + y_ch * 2 + Key_Channel * 7 + 2;
#else
	output_len = y_ch * x_ch * 7 + y_ch * 2;
#endif /* #if TOUCH_KEY_NUM > 0 */
	pos = offset;
	write_ret = vfs_write(fp, (char __user *)fbufp, output_len, &pos);
	if (write_ret <= 0) {
		NVT_ERR("write %s failed\n", file_path);
		set_fs(org_fs);
		if (fp) {
			filp_close(fp, NULL);
			fp = NULL;
		}
		kfree(fbufp);
		fbufp = NULL;
		return -1;
	}

	set_fs(org_fs);
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
	kfree(fbufp);
	fbufp = NULL;

	printk("%s:--\n", __func__);

	return 0;
}

static int32_t nvt_polling_hand_shake_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 250;

	for (i = 0; i < retry; i++) {
		/* ---set xdata index to EVENT BUF ADDR--- */
		nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

		/* ---read fw status--- */
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0x00;
		buf[2] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);

		if ((buf[2] == 0xA0) || (buf[2] == 0xA1))
			break;

		usleep_range(10000, 10000);
	}

	if (i >= retry) {
		NVT_ERR("polling hand shake status failed, buf[2]=0x%02X\n", buf[2]);

		/* Read back 5 bytes from offset EVENT_MAP_HOST_CMD for debug check */
		nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0x00;
		buf[2] = 0x00;
		buf[3] = 0x00;
		buf[4] = 0x00;
		buf[5] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 6);
		NVT_ERR("Read back 5 bytes from offset EVENT_MAP_HOST_CMD: 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", buf[1], buf[2], buf[3], buf[4], buf[5]);

		return -1;
	} else {
		return 0;
	}
}


static int8_t nvt_switch_FreqHopEnDis(uint8_t FreqHopEnDis)
{
	uint8_t buf[8] = {0};
	uint8_t retry = 0;
	int8_t ret = 0;

	NVT_LOG("++\n");

	for (retry = 0; retry < 20; retry++) {
		/* ---set xdata index to EVENT BUF ADDR--- */
		nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

		/* ---switch FreqHopEnDis--- */
		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = FreqHopEnDis;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		msleep(35);

		buf[0] = EVENT_MAP_HOST_CMD;
		buf[1] = 0xFF;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;
	}

	if (unlikely(retry == 20)) {
		NVT_ERR("switch FreqHopEnDis 0x%02X failed, buf[1]=0x%02X\n", FreqHopEnDis, buf[1]);
		ret = -1;
	}

	NVT_LOG("--\n");

	return ret;
}

static int32_t nvt_read_baseline(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;
#if TOUCH_KEY_NUM > 0
	int32_t k = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	NVT_LOG("++\n");

	nvt_read_mdata(ts->mmap->BASELINE_ADDR, ts->mmap->BASELINE_BTN_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
		}
	}
#if TOUCH_KEY_NUM > 0
	for (k = 0; k < Key_Channel; k++) {
		iArrayIndex = Y_Channel * X_Channel + k;
		xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	printk("%s:\n", __func__);
	/* Save Rawdata to CSV file */
	if (nvt_save_rawdata_to_csv(xdata, X_Channel, Y_Channel, FW_RAWDATA_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("--\n");

	return 0;
}

static int32_t nvt_read_CC(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;
#if TOUCH_KEY_NUM > 0
	int32_t k = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	NVT_LOG("++\n");

	if (nvt_get_fw_pipe() == 0)
		nvt_read_mdata(ts->mmap->DIFF_PIPE1_ADDR, ts->mmap->DIFF_BTN_PIPE1_ADDR);
	else
		nvt_read_mdata(ts->mmap->DIFF_PIPE0_ADDR, ts->mmap->DIFF_BTN_PIPE0_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
		}
	}
#if TOUCH_KEY_NUM > 0
	for (k = 0; k < Key_Channel; k++) {
		iArrayIndex = Y_Channel * X_Channel + k;
		xdata[iArrayIndex] = (int16_t)xdata[iArrayIndex];
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	printk("%s:\n", __func__);
	/* Save Rawdata to CSV file */
	if (nvt_save_rawdata_to_csv(xdata, X_Channel, Y_Channel, FW_CC_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("--\n");

	return 0;
}

static int32_t nvt_read_pen_baseline(void)
{
	uint32_t csv_output_offset = 0;

	NVT_LOG("++\n");

	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_TIP_X_ADDR, RawData_PenTipX_Raw, ts->x_num * ts->y_gang_num);
	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_TIP_Y_ADDR, RawData_PenTipY_Raw, ts->x_gang_num * ts->y_num);
	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_RING_X_ADDR, RawData_PenRingX_Raw, ts->x_num * ts->y_gang_num);
	nvt_read_get_num_mdata(ts->mmap->PEN_2D_BL_RING_Y_ADDR, RawData_PenRingY_Raw, ts->x_gang_num * ts->y_num);

	/* Save Rawdata to CSV file */
	printk("%s:RawData_PenTipX_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenTipX_Raw, ts->x_num, ts->y_gang_num, PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}
	csv_output_offset += ts->y_gang_num * ts->x_num * 7 + ts->y_gang_num * 2;
	printk("%s:RawData_PenTipY_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenTipY_Raw, ts->x_gang_num, ts->y_num, PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}
	csv_output_offset += ts->y_num * ts->x_gang_num * 7 + ts->y_num * 2;
	printk("%s:RawData_PenRingX_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenRingX_Raw, ts->x_num, ts->y_gang_num, PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}
	csv_output_offset += ts->y_gang_num * ts->x_num * 7 + ts->y_gang_num * 2;
	printk("%s:RawData_PenRingY_Raw\n", __func__);
	if (nvt_save_rawdata_to_csv(RawData_PenRingY_Raw, ts->x_gang_num, ts->y_num, PEN_FW_RAW_TEST_CSV_FILE, csv_output_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("--\n");

	return 0;
}

static void nvt_enable_noise_collect(int32_t frame_num)
{
	uint8_t buf[8] = {0};

	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	/* ---enable noise collect--- */
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x47;
	buf[2] = 0xAA;
	buf[3] = frame_num;
	buf[4] = 0x00;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 5);
}

static int32_t nvt_read_fw_noise(int32_t *xdata)
{
	uint8_t x_num = 0;
	uint8_t y_num = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	int32_t iArrayIndex = 0;
	int32_t frame_num = 0;
	uint32_t rawdata_diff_min_offset = 0;
#if TOUCH_KEY_NUM > 0
	int32_t k = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */
	uint32_t csv_pen_noise_offset = 0;

	NVT_LOG("++\n");

	/* ---Enter Test Mode--- */
	if (nvt_clear_fw_status()) {
		return -EAGAIN;
	}

	frame_num = PS_Config_Diff_Test_Frame / 10;
	if (frame_num <= 0)
		frame_num = 1;
	printk("%s: frame_num=%d\n", __func__, frame_num);
	nvt_enable_noise_collect(frame_num);
	/* need wait PS_Config_Diff_Test_Frame * 8.3ms */
	msleep(frame_num * 83);

	if (nvt_polling_hand_shake_status()) {
		return -EAGAIN;
	}

	if (nvt_get_fw_info()) {
		return -EAGAIN;
	}

	if (nvt_get_fw_pipe() == 0)
		nvt_read_mdata(ts->mmap->DIFF_PIPE0_ADDR, ts->mmap->DIFF_BTN_PIPE0_ADDR);
	else
		nvt_read_mdata(ts->mmap->DIFF_PIPE1_ADDR, ts->mmap->DIFF_BTN_PIPE1_ADDR);

	nvt_get_mdata(xdata, &x_num, &y_num);

	for (y = 0; y < y_num; y++) {
		for (x = 0; x < x_num; x++) {
			iArrayIndex = y * x_num + x;
			RawData_Diff_Max[iArrayIndex] = (int8_t)((xdata[iArrayIndex] >> 8) & 0xFF);
			RawData_Diff_Min[iArrayIndex] = (int8_t)(xdata[iArrayIndex] & 0xFF);
		}
	}
#if TOUCH_KEY_NUM > 0
	for (k = 0; k < Key_Channel; k++) {
		iArrayIndex = Y_Channel * X_Channel + k;
		RawData_Diff_Max[iArrayIndex] = (int8_t)((xdata[iArrayIndex] >> 8) & 0xFF);
		RawData_Diff_Min[iArrayIndex] = (int8_t)(xdata[iArrayIndex] & 0xFF);
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	if (ts->pen_support) {
		/* get pen noise data */
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_TIP_X_ADDR, RawData_PenTipX_DiffMax, ts->x_num * ts->y_gang_num);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_TIP_X_ADDR, RawData_PenTipX_DiffMin, ts->x_num * ts->y_gang_num);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_TIP_Y_ADDR, RawData_PenTipY_DiffMax, ts->x_gang_num * ts->y_num);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_TIP_Y_ADDR, RawData_PenTipY_DiffMin, ts->x_gang_num * ts->y_num);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_RING_X_ADDR, RawData_PenRingX_DiffMax, ts->x_num * ts->y_gang_num);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_RING_X_ADDR, RawData_PenRingX_DiffMin, ts->x_num * ts->y_gang_num);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_DIFF_RING_Y_ADDR, RawData_PenRingY_DiffMax, ts->x_gang_num * ts->y_num);
		nvt_read_get_num_mdata(ts->mmap->PEN_2D_RAW_RING_Y_ADDR, RawData_PenRingY_DiffMin, ts->x_gang_num * ts->y_num);
	} /* if (ts->pen_support) */

	/* ---Leave Test Mode--- */
	nvt_change_mode(NORMAL_MODE);

	printk("%s:RawData_Diff_Max:\n", __func__);
	/* Save Rawdata to CSV file */
	if (nvt_save_rawdata_to_csv(RawData_Diff_Max, X_Channel, Y_Channel, NOISE_TEST_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

#if TOUCH_KEY_NUM > 0
	rawdata_diff_min_offset = Y_Channel * X_Channel * 7 + Y_Channel * 2 + Key_Channel * 7 + 2;
#else
	rawdata_diff_min_offset = Y_Channel * X_Channel * 7 + Y_Channel * 2;
#endif /* #if TOUCH_KEY_NUM > 0 */
	printk("%s:RawData_Diff_Min:\n", __func__);
	/* Save Rawdata to CSV file */
	if (nvt_save_rawdata_to_csv(RawData_Diff_Min, X_Channel, Y_Channel, NOISE_TEST_CSV_FILE, rawdata_diff_min_offset) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	if (ts->pen_support) {
		/* save pen noise to csv */
		printk("%s:RawData_PenTipX_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipX_DiffMax, ts->x_num, ts->y_gang_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->y_gang_num * ts->x_num * 7 + ts->y_gang_num * 2;
		printk("%s:RawData_PenTipX_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipX_DiffMin, ts->x_num, ts->y_gang_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->y_gang_num * ts->x_num * 7 + ts->y_gang_num * 2;
		printk("%s:RawData_PenTipY_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipY_DiffMax, ts->x_gang_num, ts->y_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->y_num * ts->x_gang_num * 7 + ts->y_num * 2;
		printk("%s:RawData_PenTipY_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenTipY_DiffMin, ts->x_gang_num, ts->y_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->y_num * ts->x_gang_num * 7 + ts->y_num * 2;
		printk("%s:RawData_PenRingX_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingX_DiffMax, ts->x_num, ts->y_gang_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->y_gang_num * ts->x_num * 7 + ts->y_gang_num * 2;
		printk("%s:RawData_PenRingX_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingX_DiffMin, ts->x_num, ts->y_gang_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->y_gang_num * ts->x_num * 7 + ts->y_gang_num * 2;
		printk("%s:RawData_PenRingY_DiffMax:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingY_DiffMax, ts->x_gang_num, ts->y_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
		csv_pen_noise_offset += ts->y_num * ts->x_gang_num * 7 + ts->y_num * 2;
		printk("%s:RawData_PenRingY_DiffMin:\n", __func__);
		if (nvt_save_rawdata_to_csv(RawData_PenRingY_DiffMin, ts->x_gang_num, ts->y_num, PEN_NOISE_TEST_CSV_FILE, csv_pen_noise_offset) < 0) {
			NVT_ERR("save rawdata to CSV file failed\n");
			return -EAGAIN;
		}
	} /* if (ts->pen_support) */

	NVT_LOG("--\n");

	return 0;
}

static void nvt_enable_open_test(void)
{
	uint8_t buf[8] = {0};

	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	/* ---enable open test--- */
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x45;
	buf[2] = 0xAA;
	buf[3] = 0x02;
	buf[4] = 0x00;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 5);
}

static void nvt_enable_short_test(void)
{
	uint8_t buf[8] = {0};

	/* ---set xdata index to EVENT BUF ADDR--- */
	nvt_set_page(I2C_FW_Address, ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	/* ---enable short test--- */
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = 0x43;
	buf[2] = 0xAA;
	buf[3] = 0x02;
	buf[4] = 0x00;
	CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 5);
}

static int32_t nvt_read_fw_open(int32_t *xdata)
{
	uint32_t raw_pipe_addr = 0;
	uint8_t *rawdata_buf = NULL;
	uint32_t x = 0;
	uint32_t y = 0;
	uint8_t buf[128] = {0};
#if TOUCH_KEY_NUM > 0
	uint32_t raw_btn_pipe_addr = 0;
	int32_t k = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	NVT_LOG("++\n");

	/* ---Enter Test Mode--- */
	if (nvt_clear_fw_status()) {
		return -EAGAIN;
	}

	nvt_enable_open_test();

	if (nvt_polling_hand_shake_status()) {
		return -EAGAIN;
	}

#if TOUCH_KEY_NUM > 0
	rawdata_buf = (uint8_t *)kzalloc((IC_X_CFG_SIZE * IC_Y_CFG_SIZE + IC_KEY_CFG_SIZE) * 2, GFP_KERNEL);
#else
	rawdata_buf = (uint8_t *)kzalloc(IC_X_CFG_SIZE * IC_Y_CFG_SIZE * 2, GFP_KERNEL);
#endif /* #if TOUCH_KEY_NUM > 0 */
	if (!rawdata_buf) {
		NVT_ERR("kzalloc for rawdata_buf failed!\n");
		return -ENOMEM;
	}

	if (nvt_get_fw_pipe() == 0)
		raw_pipe_addr = ts->mmap->RAW_PIPE0_ADDR;
	else
		raw_pipe_addr = ts->mmap->RAW_PIPE1_ADDR;

	for (y = 0; y < IC_Y_CFG_SIZE; y++) {
		/* ---change xdata index--- */
		nvt_set_page(I2C_FW_Address, raw_pipe_addr + y * IC_X_CFG_SIZE * 2);
		buf[0] = (uint8_t)((raw_pipe_addr + y * IC_X_CFG_SIZE * 2) & 0xFF);
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_X_CFG_SIZE * 2 + 1);
		memcpy(rawdata_buf + y * IC_X_CFG_SIZE * 2, buf + 1, IC_X_CFG_SIZE * 2);
	}
#if TOUCH_KEY_NUM > 0
	if (nvt_get_fw_pipe() == 0)
		raw_btn_pipe_addr = ts->mmap->RAW_BTN_PIPE0_ADDR;
	else
		raw_btn_pipe_addr = ts->mmap->RAW_BTN_PIPE1_ADDR;

	/* ---change xdata index--- */
	nvt_set_page(I2C_FW_Address, raw_btn_pipe_addr);
	buf[0] = (uint8_t)(raw_btn_pipe_addr & 0xFF);
	CTP_I2C_READ(ts->client, I2C_FW_Address, buf, IC_KEY_CFG_SIZE * 2 + 1);
	memcpy(rawdata_buf + IC_Y_CFG_SIZE * IC_X_CFG_SIZE * 2, buf + 1, IC_KEY_CFG_SIZE * 2);
#endif /* #if TOUCH_KEY_NUM > 0 */

	for (y = 0; y < IC_Y_CFG_SIZE; y++) {
		for (x = 0; x < IC_X_CFG_SIZE; x++) {
			if ((AIN_Y[y] != 0xFF) && (AIN_X[x] != 0xFF)) {
				xdata[AIN_Y[y] * X_Channel + AIN_X[x]] = (int16_t)((rawdata_buf[(y * IC_X_CFG_SIZE + x) * 2] + 256 * rawdata_buf[(y * IC_X_CFG_SIZE + x) * 2 + 1]));
			}
		}
	}
#if TOUCH_KEY_NUM > 0
	for (k = 0; k < IC_KEY_CFG_SIZE; k++) {
		if (AIN_KEY[k] != 0xFF)
			xdata[Y_Channel * X_Channel + AIN_KEY[k]] = (int16_t)(rawdata_buf[(IC_Y_CFG_SIZE * IC_X_CFG_SIZE + k) * 2] + 256 * rawdata_buf[(IC_Y_CFG_SIZE * IC_X_CFG_SIZE + k) * 2 + 1]);
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	kfree(rawdata_buf);
	rawdata_buf = NULL;

	/* ---Leave Test Mode--- */
	nvt_change_mode(NORMAL_MODE);


	printk("%s:RawData_Open\n", __func__);
	/* Save RawData to CSV file */
	if (nvt_save_rawdata_to_csv(xdata, X_Channel, Y_Channel, OPEN_TEST_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("--\n");

	return 0;
}

static int32_t nvt_read_fw_short(int32_t *xdata)
{
	uint32_t raw_pipe_addr = 0;
	uint8_t *rawdata_buf = NULL;
	uint32_t x = 0;
	uint32_t y = 0;
	uint8_t buf[128] = {0};
	int32_t iArrayIndex = 0;
#if TOUCH_KEY_NUM > 0
	uint32_t raw_btn_pipe_addr = 0;
	int32_t k = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	NVT_LOG("++\n");

	/* ---Enter Test Mode--- */
	if (nvt_clear_fw_status()) {
		return -EAGAIN;
	}

	nvt_enable_short_test();

	if (nvt_polling_hand_shake_status()) {
		return -EAGAIN;
	}

#if TOUCH_KEY_NUM > 0
	rawdata_buf = (uint8_t *)kzalloc((X_Channel * Y_Channel + Key_Channel) * 2, GFP_KERNEL);
#else
	rawdata_buf = (uint8_t *)kzalloc(X_Channel * Y_Channel * 2, GFP_KERNEL);
#endif /* #if TOUCH_KEY_NUM > 0 */
	if (!rawdata_buf) {
		NVT_ERR("kzalloc for rawdata_buf failed!\n");
		return -ENOMEM;
	}

	if (nvt_get_fw_pipe() == 0)
		raw_pipe_addr = ts->mmap->RAW_PIPE0_ADDR;
	else
		raw_pipe_addr = ts->mmap->RAW_PIPE1_ADDR;

	for (y = 0; y < Y_Channel; y++) {
		/* ---change xdata index--- */
		nvt_set_page(I2C_FW_Address, raw_pipe_addr + y * X_Channel * 2);
		buf[0] = (uint8_t)((raw_pipe_addr + y * X_Channel * 2) & 0xFF);
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, X_Channel * 2 + 1);
		memcpy(rawdata_buf + y * X_Channel * 2, buf + 1, X_Channel * 2);
	}
#if TOUCH_KEY_NUM > 0
	if (nvt_get_fw_pipe() == 0)
		raw_btn_pipe_addr = ts->mmap->RAW_BTN_PIPE0_ADDR;
	else
		raw_btn_pipe_addr = ts->mmap->RAW_BTN_PIPE1_ADDR;

    /* ---change xdata index--- */
	nvt_set_page(I2C_FW_Address, raw_btn_pipe_addr);
	buf[0] = (uint8_t)(raw_btn_pipe_addr & 0xFF);
	CTP_I2C_READ(ts->client, I2C_FW_Address, buf, Key_Channel * 2 + 1);
	memcpy(rawdata_buf + Y_Channel * X_Channel * 2, buf + 1, Key_Channel * 2);
#endif /* #if TOUCH_KEY_NUM > 0 */

	for (y = 0; y < Y_Channel; y++) {
		for (x = 0; x < X_Channel; x++) {
			iArrayIndex = y * X_Channel + x;
			xdata[iArrayIndex] = (int16_t)(rawdata_buf[iArrayIndex * 2] + 256 * rawdata_buf[iArrayIndex * 2 + 1]);
		}
	}
#if TOUCH_KEY_NUM > 0
	for (k = 0; k < Key_Channel; k++) {
		iArrayIndex = Y_Channel * X_Channel + k;
		xdata[iArrayIndex] = (int16_t)(rawdata_buf[iArrayIndex * 2] + 256 * rawdata_buf[iArrayIndex * 2 + 1]);
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	kfree(rawdata_buf);
	rawdata_buf = NULL;

	/* ---Leave Test Mode--- */
	nvt_change_mode(NORMAL_MODE);

	printk("%s:RawData_Short\n", __func__);
	/* Save Rawdata to CSV file */
	if (nvt_save_rawdata_to_csv(xdata, X_Channel, Y_Channel, SHORT_TEST_CSV_FILE, 0) < 0) {
		NVT_ERR("save rawdata to CSV file failed\n");
		return -EAGAIN;
	}

	NVT_LOG("--\n");

	return 0;
}

/*******************************************************
* Description:
*	Novatek touchscreen raw data test for each single point function.
*
* return:
*	Executive outcomes. 0---passed. negative---failed.
*******************************************************/
static int32_t RawDataTest_SinglePoint_Sub(int32_t rawdata[], uint8_t RecordResult[], uint8_t x_ch, uint8_t y_ch, int32_t Rawdata_Limit_Postive[], int32_t Rawdata_Limit_Negative[])
{
	int32_t i = 0;
	int32_t j = 0;
#if TOUCH_KEY_NUM > 0
	int32_t k = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */
	int32_t iArrayIndex = 0;
	bool isPass = true;

	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			iArrayIndex = j * x_ch + i;

			RecordResult[iArrayIndex] = 0x00; /* default value for PASS */

			if (rawdata[iArrayIndex] > Rawdata_Limit_Postive[iArrayIndex])
				RecordResult[iArrayIndex] |= 0x01;

			if (rawdata[iArrayIndex] < Rawdata_Limit_Negative[iArrayIndex])
				RecordResult[iArrayIndex] |= 0x02;
		}
	}
#if TOUCH_KEY_NUM > 0
	for (k = 0; k < Key_Channel; k++) {
		iArrayIndex = y_ch * x_ch + k;

		RecordResult[iArrayIndex] = 0x00; /* default value for PASS */

		if (rawdata[iArrayIndex] > Rawdata_Limit_Postive[iArrayIndex])
			RecordResult[iArrayIndex] |= 0x01;

		if (rawdata[iArrayIndex] < Rawdata_Limit_Negative[iArrayIndex])
			RecordResult[iArrayIndex] |= 0x02;
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	/* ---Check RecordResult--- */
	for (j = 0; j < y_ch; j++) {
		for (i = 0; i < x_ch; i++) {
			if (RecordResult[j * x_ch + i] != 0) {
				isPass = false;
				break;
			}
		}
	}
#if TOUCH_KEY_NUM > 0
	for (k = 0; k < Key_Channel; k++) {
		iArrayIndex = y_ch * x_ch + k;
		if (RecordResult[iArrayIndex] != 0) {
			isPass = false;
			break;
		}
	}
#endif /* #if TOUCH_KEY_NUM > 0 */

	if (isPass == false) {
		return -1; /* FAIL */
	} else {
		return 0; /* PASS */
	}
}

/*******************************************************
* Description:
*	Novatek touchscreen print self-test result function.
*
* return:
*	n.a.
*******************************************************/
void print_selftest_result(struct seq_file *m, int32_t TestResult, uint8_t RecordResult[], int32_t rawdata[], uint8_t x_len, uint8_t y_len)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t iArrayIndex = 0;
#if TOUCH_KEY_NUM > 0
	int32_t k = 0;
#endif /* #if TOUCH_KEY_NUM > 0 */

	switch (TestResult) {
	case 0:
		nvt_mp_seq_printf(m, " PASS!\n");
		break;

	case 1:
		nvt_mp_seq_printf(m, " ERROR! Read Data FAIL!\n");
		break;

	case -1:
		nvt_mp_seq_printf(m, " FAIL!\n");
		nvt_mp_seq_printf(m, "RecordResult:\n");
		for (i = 0; i < y_len; i++) {
			for (j = 0; j < x_len; j++) {
				iArrayIndex = i * x_len + j;
				seq_printf(m, "0x%02X, ", RecordResult[iArrayIndex]);
			}
			if (!nvt_mp_test_result_printed)
				nvt_print_result_log_in_one_line(RecordResult + i * x_len, x_len);
			nvt_mp_seq_printf(m, "\n");
		}
#if TOUCH_KEY_NUM > 0
		for (k = 0; k < Key_Channel; k++) {
			iArrayIndex = y_len * x_len + k;
			seq_printf(m, "0x%02X, ", RecordResult[iArrayIndex]);
		}
		if (!nvt_mp_test_result_printed)
			nvt_print_result_log_in_one_line(RecordResult + y_len * x_len, Key_Channel);
		nvt_mp_seq_printf(m, "\n");
#endif /* #if TOUCH_KEY_NUM > 0 */
		nvt_mp_seq_printf(m, "ReadData:\n");
		for (i = 0; i < y_len; i++) {
			for (j = 0; j < x_len; j++) {
				iArrayIndex = i * x_len + j;
				seq_printf(m, "%5d, ", rawdata[iArrayIndex]);
			}
			if (!nvt_mp_test_result_printed)
				nvt_print_data_log_in_one_line(rawdata + i * x_len, x_len);
			nvt_mp_seq_printf(m, "\n");
		}
#if TOUCH_KEY_NUM > 0
		for (k = 0; k < Key_Channel; k++) {
			iArrayIndex = y_len * x_len + k;
			seq_printf(m, "%5d, ", rawdata[iArrayIndex]);
		}
		if (!nvt_mp_test_result_printed)
			nvt_print_data_log_in_one_line(rawdata + y_len * x_len, Key_Channel);
		nvt_mp_seq_printf(m, "\n");
#endif /* #if TOUCH_KEY_NUM > 0 */
		break;
	}
	nvt_mp_seq_printf(m, "\n");
}

/*******************************************************
* Description:
*	Novatek touchscreen self-test sequence print show
*	function.
*
* return:
*	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t c_show_selftest(struct seq_file *m, void *v)
{
	NVT_LOG("++\n");

	nvt_mp_seq_printf(m, "FW Version: %d\n\n", fw_ver);

	nvt_mp_seq_printf(m, "Short Test");
	print_selftest_result(m, TestResult_Short, RecordResult_Short, RawData_Short, X_Channel, Y_Channel);

	nvt_mp_seq_printf(m, "Open Test");
	print_selftest_result(m, TestResult_Open, RecordResult_Open, RawData_Open, X_Channel, Y_Channel);

	nvt_mp_seq_printf(m, "FW Rawdata Test");
	if ((TestResult_FW_Rawdata == 0) || (TestResult_FW_Rawdata == 1)) {
		 print_selftest_result(m, TestResult_FWMutual, RecordResult_FWMutual, RawData_FWMutual, X_Channel, Y_Channel);
	} else {
		/* TestResult_FW_Rawdata is -1 */
		nvt_mp_seq_printf(m, " FAIL!\n");
		if (TestResult_FWMutual == -1) {
			nvt_mp_seq_printf(m, "FW Mutual");
			print_selftest_result(m, TestResult_FWMutual, RecordResult_FWMutual, RawData_FWMutual, X_Channel, Y_Channel);
		}
		if (TestResult_FW_CC == -1) {
			nvt_mp_seq_printf(m, "FW CC");
			print_selftest_result(m, TestResult_FW_CC, RecordResult_FW_CC, RawData_FW_CC, X_Channel, Y_Channel);
		}
	}

	nvt_mp_seq_printf(m, "Noise Test");
	if ((TestResult_Noise == 0) || (TestResult_Noise == 1)) {
		print_selftest_result(m, TestResult_FW_DiffMax, RecordResult_FW_DiffMax, RawData_Diff_Max, X_Channel, Y_Channel);
	} else {
		/* TestResult_Noise is -1 */
		nvt_mp_seq_printf(m, " FAIL!\n");
		if (TestResult_FW_DiffMax == -1) {
			nvt_mp_seq_printf(m, "FW Diff Max");
			print_selftest_result(m, TestResult_FW_DiffMax, RecordResult_FW_DiffMax, RawData_Diff_Max, X_Channel, Y_Channel);
		}
		if (TestResult_FW_DiffMin == -1) {
			nvt_mp_seq_printf(m, "FW Diff Min");
			print_selftest_result(m, TestResult_FW_DiffMin, RecordResult_FW_DiffMin, RawData_Diff_Min, X_Channel, Y_Channel);
		}
	}

	if (ts->pen_support) {
		nvt_mp_seq_printf(m, "Pen FW Rawdata Test");
		if ((TestResult_Pen_FW_Raw == 0) || (TestResult_Pen_FW_Raw == 1)) {
			print_selftest_result(m, TestResult_Pen_FW_Raw, RecordResult_PenTipX_Raw, RawData_PenTipX_Raw, ts->x_num, ts->y_gang_num);
		} else {
			/* TestResult_Pen_FW_Raw is -1 */
			nvt_mp_seq_printf(m, " FAIL!\n");
			if (TestResult_PenTipX_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Tip X Raw");
				print_selftest_result(m, TestResult_PenTipX_Raw, RecordResult_PenTipX_Raw, RawData_PenTipX_Raw, ts->x_num, ts->y_gang_num);
			}
			if (TestResult_PenTipY_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Tip Y Raw");
				print_selftest_result(m, TestResult_PenTipY_Raw, RecordResult_PenTipY_Raw, RawData_PenTipY_Raw, ts->x_gang_num, ts->y_num);
			}
			if (TestResult_PenRingX_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Ring X Raw");
				print_selftest_result(m, TestResult_PenRingX_Raw, RecordResult_PenRingX_Raw, RawData_PenRingX_Raw, ts->x_num, ts->y_gang_num);
			}
			if (TestResult_PenRingY_Raw == -1) {
				nvt_mp_seq_printf(m, "Pen Ring Y Raw");
				print_selftest_result(m, TestResult_PenRingY_Raw, RecordResult_PenRingY_Raw, RawData_PenRingY_Raw, ts->x_gang_num, ts->y_num);
			}
		}

		nvt_mp_seq_printf(m, "Pen Noise Test");
		if ((TestResult_Pen_Noise == 0) || (TestResult_Pen_Noise == 1)) {
			print_selftest_result(m, TestResult_Pen_Noise, RecordResult_PenTipX_DiffMax, RawData_PenTipX_DiffMax, ts->x_num, ts->y_gang_num);
		} else {
			/* TestResult_Pen_Noise is -1 */
			nvt_mp_seq_printf(m, " FAIL!\n");
			if (TestResult_PenTipX_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Tip X Diff Max");
				print_selftest_result(m, TestResult_PenTipX_DiffMax, RecordResult_PenTipX_DiffMax, RawData_PenTipX_DiffMax, ts->x_num, ts->y_gang_num);
			}
			if (TestResult_PenTipX_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Tip X Diff Min");
				print_selftest_result(m, TestResult_PenTipX_DiffMin, RecordResult_PenTipX_DiffMin, RawData_PenTipX_DiffMin, ts->x_num, ts->y_gang_num);
			}
			if (TestResult_PenTipY_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Tip Y Diff Max");
				print_selftest_result(m, TestResult_PenTipY_DiffMax, RecordResult_PenTipY_DiffMax, RawData_PenTipY_DiffMax, ts->x_gang_num, ts->y_num);
			}
			if (TestResult_PenTipY_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Tip Y Diff Min");
				print_selftest_result(m, TestResult_PenTipY_DiffMin, RecordResult_PenTipY_DiffMin, RawData_PenTipY_DiffMin, ts->x_gang_num, ts->y_num);
			}
			if (TestResult_PenRingX_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Ring X Diff Max");
				print_selftest_result(m, TestResult_PenRingX_DiffMax, RecordResult_PenRingX_DiffMax, RawData_PenRingX_DiffMax, ts->x_num, ts->y_gang_num);
			}
			if (TestResult_PenRingX_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Ring X Diff Min");
				print_selftest_result(m, TestResult_PenRingX_DiffMin, RecordResult_PenRingX_DiffMin, RawData_PenRingX_DiffMin, ts->x_num, ts->y_gang_num);
			}
			if (TestResult_PenRingY_DiffMax == -1) {
				nvt_mp_seq_printf(m, "Pen Ring Y Diff Max");
				print_selftest_result(m, TestResult_PenRingY_DiffMax, RecordResult_PenRingY_DiffMax, RawData_PenRingY_DiffMax, ts->x_gang_num, ts->y_num);
			}
			if (TestResult_PenRingY_DiffMin == -1) {
				nvt_mp_seq_printf(m, "Pen Ring Y Diff Min");
				print_selftest_result(m, TestResult_PenRingY_DiffMin, RecordResult_PenRingY_DiffMin, RawData_PenRingY_DiffMin, ts->x_gang_num, ts->y_num);
			}
		}
	} /* if (ts->pen_support) */

	nvt_mp_test_result_printed = 1;

	NVT_LOG("--\n");

	return 0;
}

/*******************************************************
* Description:
*	Novatek touchscreen self-test sequence print start
*	function.
*
* return:
*	Executive outcomes. 1---call next function.
*	NULL---not call next function and sequence loop
*	stop.
*******************************************************/
static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

/*******************************************************
* Description:
*	Novatek touchscreen self-test sequence print next
*	function.
*
* return:
*	Executive outcomes. NULL---no next and call sequence
*	stop function.
*******************************************************/
static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

/*******************************************************
* Description:
*	Novatek touchscreen self-test sequence print stop
*	function.
*
* return:
*	n.a.
*******************************************************/
static void c_stop(struct seq_file *m, void *v)
{
	return;
}

const struct seq_operations nvt_selftest_seq_ops = {
	.start  = c_start,
	.next   = c_next,
	.stop   = c_stop,
	.show   = c_show_selftest
};

/*******************************************************
* Description:
*	Novatek touchscreen /proc/nvt_selftest open function.
*
* return:
*	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_selftest_open(struct inode *inode, struct file *file)
{
	struct device_node *np = ts->client->dev.of_node;
	unsigned char mpcriteria[32] = {0};	/* novatek-mp-criteria-default */
	int32_t index = 0;
	int32_t found_mp_pid_criteria = 0;

	TestResult_Short = 0;
	TestResult_Open = 0;
	TestResult_FW_Rawdata = 0;
	TestResult_FWMutual = 0;
	TestResult_FW_CC = 0;
	TestResult_Noise = 0;
	TestResult_FW_DiffMax = 0;
	TestResult_FW_DiffMin = 0;
	if (ts->pen_support) {
		TestResult_Pen_FW_Raw = 0;
		TestResult_PenTipX_Raw = 0;
		TestResult_PenTipY_Raw = 0;
		TestResult_PenRingX_Raw = 0;
		TestResult_PenRingY_Raw = 0;
		TestResult_Pen_Noise = 0;
		TestResult_PenTipX_DiffMax = 0;
		TestResult_PenTipX_DiffMin = 0;
		TestResult_PenTipY_DiffMax = 0;
		TestResult_PenTipY_DiffMin = 0;
		TestResult_PenRingX_DiffMax = 0;
		TestResult_PenRingX_DiffMin = 0;
		TestResult_PenRingY_DiffMax = 0;
		TestResult_PenRingY_DiffMin = 0;
	} /* if (ts->pen_support) */

	NVT_LOG("++\n");

	if (mutex_lock_interruptible(&ts->lock)) {
		return -ERESTARTSYS;
	}

	if (ts->TouchIsAwake == 0) {
		NVT_ERR("Operation is prohibited in sleep state.\n");
		mutex_unlock(&ts->lock);
		return -EPERM;
	}

#if NVT_TOUCH_ESD_PROTECT
	nvt_esd_check_enable(false);
#endif /* #if NVT_TOUCH_ESD_PROTECT */

	if (nvt_get_fw_info()) {
		mutex_unlock(&ts->lock);
		NVT_ERR("get fw info failed!\n");
		return -EAGAIN;
	}

	fw_ver = ts->fw_ver;

	/* Parsing criteria from dts */
	if (of_property_read_bool(np, "novatek,mp-support-dt")) {
		/*
		 * Parsing Criteria by Novatek PID
		 * The string rule is "novatek-mp-criteria-<nvt_pid>"
		 * nvt_pid is 2 bytes (show hex).
		 *
		 * Ex. nvt_pid = 500A
		 *     mpcriteria = "novatek-mp-criteria-500A"
		 */
		snprintf(mpcriteria, PAGE_SIZE, "novatek-mp-criteria-%04X", ts->nvt_pid);

		if (nvt_mp_parse_dt(np, mpcriteria)) {
			mutex_unlock(&ts->lock);
			NVT_ERR("mp parse device tree failed!\n");
			return -EINVAL;
		}
	} else if (of_property_read_bool(np, "novatek,mp-support-header")) {
		for (index = 0; index < (sizeof(Novatek_mp_criteria) / sizeof(struct nvt_ts_mp_criteria)); index++) {
			if (ts->nvt_pid == Novatek_mp_criteria[index].PID) {
				NVT_LOG("Load mp criteria from header with pid : %04X\n", ts->nvt_pid);
				nvt_mp_parse_header(index);
				found_mp_pid_criteria = 1;
				break;
			}
		}
		if (!found_mp_pid_criteria) {
			mutex_unlock(&ts->lock);
			NVT_ERR("mp parse from header failed!, no pid matches with %04X\n", ts->nvt_pid);
			return -EINVAL;
		}
	} else if (of_property_read_bool(np, "novatek,mp-support-csv")) {
		NVT_LOG("Load mp criteria from csv");
		if (nvt_mp_parse_csv() < 0)
			NVT_ERR("Get selftest limit failed form CSV!\n");
	} else {
		NVT_LOG("No support for novatek,mp-support-dt or novatek,mp-support-header, use default setting\n");
		/* ---Print Test Criteria--- */
		nvt_print_criteria();
	}

	if (nvt_switch_FreqHopEnDis(FREQ_HOP_DISABLE)) {
		mutex_unlock(&ts->lock);
		NVT_ERR("switch frequency hopping disable failed!\n");
		return -EAGAIN;
	}

	if (nvt_check_fw_reset_state(RESET_STATE_NORMAL_RUN)) {
		mutex_unlock(&ts->lock);
		NVT_ERR("check fw reset state failed!\n");
		return -EAGAIN;
	}

	msleep(100);

	/* ---Enter Test Mode--- */
	if (nvt_clear_fw_status()) {
		mutex_unlock(&ts->lock);
		NVT_ERR("clear fw status failed!\n");
		return -EAGAIN;
	}

	nvt_change_mode(MP_MODE_CC);

	if (nvt_check_fw_status()) {
		mutex_unlock(&ts->lock);
		NVT_ERR("check fw status failed!\n");
		return -EAGAIN;
	}

	/* ---FW Rawdata Test--- */
	if (nvt_read_baseline(RawData_FWMutual) != 0) {
		TestResult_FWMutual = 1;
	} else {
		TestResult_FWMutual = RawDataTest_SinglePoint_Sub(RawData_FWMutual, RecordResult_FWMutual, X_Channel, Y_Channel,
												PS_Config_Lmt_FW_Rawdata_P, PS_Config_Lmt_FW_Rawdata_N);
	}
	if (nvt_read_CC(RawData_FW_CC) != 0) {
		TestResult_FW_CC = 1;
	} else {
		TestResult_FW_CC = RawDataTest_SinglePoint_Sub(RawData_FW_CC, RecordResult_FW_CC, X_Channel, Y_Channel,
											PS_Config_Lmt_FW_CC_P, PS_Config_Lmt_FW_CC_N);
	}

	if ((TestResult_FWMutual == 1) || (TestResult_FW_CC == 1)) {
		TestResult_FW_Rawdata = 1;
	} else {
		if ((TestResult_FWMutual == -1) || (TestResult_FW_CC == -1))
			TestResult_FW_Rawdata = -1;
		else
			TestResult_FW_Rawdata = 0;
	}

	if (ts->pen_support) {
		/* ---Pen FW Rawdata Test--- */
		if (nvt_read_pen_baseline() != 0) {
			TestResult_Pen_FW_Raw = 1;
		} else {
			TestResult_PenTipX_Raw = RawDataTest_SinglePoint_Sub(RawData_PenTipX_Raw, RecordResult_PenTipX_Raw, ts->x_num, ts->y_gang_num,
											PS_Config_Lmt_PenTipX_FW_Raw_P, PS_Config_Lmt_PenTipX_FW_Raw_N);
			TestResult_PenTipY_Raw = RawDataTest_SinglePoint_Sub(RawData_PenTipY_Raw, RecordResult_PenTipY_Raw, ts->x_gang_num, ts->y_num,
											PS_Config_Lmt_PenTipY_FW_Raw_P, PS_Config_Lmt_PenTipY_FW_Raw_N);
			TestResult_PenRingX_Raw = RawDataTest_SinglePoint_Sub(RawData_PenRingX_Raw, RecordResult_PenRingX_Raw, ts->x_num, ts->y_gang_num,
											PS_Config_Lmt_PenRingX_FW_Raw_P, PS_Config_Lmt_PenRingX_FW_Raw_N);
			TestResult_PenRingY_Raw = RawDataTest_SinglePoint_Sub(RawData_PenRingY_Raw, RecordResult_PenRingY_Raw, ts->x_gang_num, ts->y_num,
											PS_Config_Lmt_PenRingY_FW_Raw_P, PS_Config_Lmt_PenRingY_FW_Raw_N);

			if ((TestResult_PenTipX_Raw == -1) || (TestResult_PenTipY_Raw == -1) || (TestResult_PenRingX_Raw == -1) || (TestResult_PenRingY_Raw == -1))
				TestResult_Pen_FW_Raw = -1;
			else
				TestResult_Pen_FW_Raw = 0;
		}
	} /* if (ts->pen_support) */

	/* ---Leave Test Mode--- */
	nvt_change_mode(NORMAL_MODE);

	/* ---Noise Test--- */
	if (nvt_read_fw_noise(RawData_Diff) != 0) {
		TestResult_Noise = 1;	/* 1: ERROR */
		TestResult_FW_DiffMax = 1;
		TestResult_FW_DiffMin = 1;
		if (ts->pen_support) {
			TestResult_Pen_Noise = 1;
			TestResult_PenTipX_DiffMax = 1;
			TestResult_PenTipX_DiffMin = 1;
			TestResult_PenTipY_DiffMax = 1;
			TestResult_PenTipY_DiffMin = 1;
			TestResult_PenRingX_DiffMax = 1;
			TestResult_PenRingX_DiffMin = 1;
			TestResult_PenRingY_DiffMax = 1;
			TestResult_PenRingY_DiffMin = 1;
		} /* if (ts->pen_support) */
	} else {
		TestResult_FW_DiffMax = RawDataTest_SinglePoint_Sub(RawData_Diff_Max, RecordResult_FW_DiffMax, X_Channel, Y_Channel,
											PS_Config_Lmt_FW_Diff_P, PS_Config_Lmt_FW_Diff_N);

		TestResult_FW_DiffMin = RawDataTest_SinglePoint_Sub(RawData_Diff_Min, RecordResult_FW_DiffMin, X_Channel, Y_Channel,
											PS_Config_Lmt_FW_Diff_P, PS_Config_Lmt_FW_Diff_N);

		if ((TestResult_FW_DiffMax == -1) || (TestResult_FW_DiffMin == -1))
			TestResult_Noise = -1;
		else
			TestResult_Noise = 0;

		if (ts->pen_support) {
			TestResult_PenTipX_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenTipX_DiffMax, RecordResult_PenTipX_DiffMax, ts->x_num, ts->y_gang_num,
											PS_Config_Lmt_PenTipX_FW_Diff_P, PS_Config_Lmt_PenTipX_FW_Diff_N);

			TestResult_PenTipX_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenTipX_DiffMin, RecordResult_PenTipX_DiffMin, ts->x_num, ts->y_gang_num,
											PS_Config_Lmt_PenTipX_FW_Diff_P, PS_Config_Lmt_PenTipX_FW_Diff_N);

			TestResult_PenTipY_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenTipY_DiffMax, RecordResult_PenTipY_DiffMax, ts->x_gang_num, ts->y_num,
											PS_Config_Lmt_PenTipY_FW_Diff_P, PS_Config_Lmt_PenTipY_FW_Diff_N);

			TestResult_PenTipY_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenTipY_DiffMin, RecordResult_PenTipY_DiffMin, ts->x_gang_num, ts->y_num,
											PS_Config_Lmt_PenTipY_FW_Diff_P, PS_Config_Lmt_PenTipY_FW_Diff_N);

			TestResult_PenRingX_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenRingX_DiffMax, RecordResult_PenRingX_DiffMax, ts->x_num, ts->y_gang_num,
											PS_Config_Lmt_PenRingX_FW_Diff_P, PS_Config_Lmt_PenRingX_FW_Diff_N);

			TestResult_PenRingX_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenRingX_DiffMin, RecordResult_PenRingX_DiffMin, ts->x_num, ts->y_gang_num,
											PS_Config_Lmt_PenRingX_FW_Diff_P, PS_Config_Lmt_PenRingX_FW_Diff_N);

			TestResult_PenRingY_DiffMax = RawDataTest_SinglePoint_Sub(RawData_PenRingY_DiffMax, RecordResult_PenRingY_DiffMax, ts->x_gang_num, ts->y_num,
											PS_Config_Lmt_PenRingY_FW_Diff_P, PS_Config_Lmt_PenRingY_FW_Diff_N);

			TestResult_PenRingY_DiffMin = RawDataTest_SinglePoint_Sub(RawData_PenRingY_DiffMin, RecordResult_PenRingY_DiffMin, ts->x_gang_num, ts->y_num,
											PS_Config_Lmt_PenRingY_FW_Diff_P, PS_Config_Lmt_PenRingY_FW_Diff_N);

			if ((TestResult_PenTipX_DiffMax == -1) || (TestResult_PenTipX_DiffMin == -1) || (TestResult_PenTipY_DiffMax == -1) || (TestResult_PenTipY_DiffMin == -1) ||
				(TestResult_PenRingX_DiffMax == -1) || (TestResult_PenRingX_DiffMin == -1) || (TestResult_PenRingY_DiffMax == -1) || (TestResult_PenRingY_DiffMin == -1))
				TestResult_Pen_Noise = -1;
			else
				TestResult_Pen_Noise = 0;
		} /* if (ts->pen_support) */
	}

	/* --Short Test--- */
	if (nvt_read_fw_short(RawData_Short) != 0) {
		TestResult_Short = 1; /* 1:ERROR */
	} else {
		/* ---Self Test Check ---  0:PASS, -1:FAIL */
		TestResult_Short = RawDataTest_SinglePoint_Sub(RawData_Short, RecordResult_Short, X_Channel, Y_Channel,
										PS_Config_Lmt_Short_Rawdata_P, PS_Config_Lmt_Short_Rawdata_N);
	}

	/* ---Open Test--- */
	if (nvt_read_fw_open(RawData_Open) != 0) {
		TestResult_Open = 1;    /* 1:ERROR */
	} else {
		/* ---Self Test Check ---  0:PASS, -1:FAIL */
		TestResult_Open = RawDataTest_SinglePoint_Sub(RawData_Open, RecordResult_Open, X_Channel, Y_Channel,
											PS_Config_Lmt_Open_Rawdata_P, PS_Config_Lmt_Open_Rawdata_N);
	}

	/* ---Reset IC--- */
	nvt_bootloader_reset();

	mutex_unlock(&ts->lock);

	NVT_LOG("--\n");

	nvt_mp_test_result_printed = 0;

	return seq_open(file, &nvt_selftest_seq_ops);
}

static const struct file_operations nvt_selftest_fops = {
	.owner = THIS_MODULE,
	.open = nvt_selftest_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

#ifdef CONFIG_OF
/*******************************************************
* Description:
*	Novatek touchscreen parse AIN setting for array type.
*
* return:
*	n.a.
*******************************************************/
int32_t nvt_mp_parse_ain(struct device_node *np, const char *name, uint8_t *array, int32_t size)
{
	struct property *data;
	int32_t len, ret;
	int32_t tmp[50];
	int32_t i;

	data = of_find_property(np, name, &len);
	len /= sizeof(u32);
	if ((!data) || (!len) || (len != size)) {
		NVT_ERR("error find %s. len=%d\n", name, len);
		return -1;
	} else {
		NVT_LOG("%s. len=%d\n", name, len);
		ret = of_property_read_u32_array(np, name, tmp, len);
		if (ret) {
			NVT_ERR("error reading %s. ret=%d\n", name, ret);
			return -1;
		}

		for (i = 0; i < len; i++)
			array[i] = tmp[i];

#if NVT_DEBUG
		printk("[NVT-ts] %s = ", name);
		nvt_print_result_log_in_one_line(array, len);
		printk("\n");
#endif
	}

	return 0;
}

/*******************************************************
* Description:
*	Novatek touchscreen parse criterion for u32 type.
*
* return:
*	n.a.
*******************************************************/
int32_t nvt_mp_parse_u32(struct device_node *np, const char *name, int32_t *para)
{
	int32_t ret;

	ret = of_property_read_u32(np, name, para);
	if (ret) {
		NVT_ERR("error reading %s. ret=%d\n", name, ret);
		return -1;
	} else {
#if NVT_DEBUG
		NVT_LOG("%s=%d\n", name, *para);
#endif
	}

	return 0;
}

/*******************************************************
* Description:
*	Novatek touchscreen parse criterion for array type.
*
* return:
*	n.a.
*******************************************************/
int32_t nvt_mp_parse_array(struct device_node *np, const char *name, int32_t *array,
		int32_t size)
{
	struct property *data;
	int32_t len, ret;
#if NVT_DEBUG
	int32_t j = 0;
#endif

	data = of_find_property(np, name, &len);
	len /= sizeof(u32);
	if ((!data) || (!len) || (len < size)) {
		NVT_ERR("error find %s. len=%d\n", name, len);
		return -1;
	} else {
		NVT_LOG("%s. len=%d\n", name, len);
		ret = of_property_read_u32_array(np, name, array, len);
		if (ret) {
			NVT_ERR("error reading %s. ret=%d\n", name, ret);
			return -1;
		}

#if NVT_DEBUG
		NVT_LOG("%s =\n", name);
		for (j = 0; j < Y_Channel; j++) {
			nvt_print_data_log_in_one_line(array + j * X_Channel, X_Channel);
			printk("\n");
		}
#if TOUCH_KEY_NUM > 0
		nvt_print_data_log_in_one_line(array + Y_Channel * X_Channel, Key_Channel);
		printk("\n");
#endif
#endif
	}

	return 0;
}

/*******************************************************
* Description:
*	Novatek touchscreen parse criterion for pen array type.
*
* return:
*	n.a.
*******************************************************/
int32_t nvt_mp_parse_pen_array(struct device_node *np, const char *name, int32_t *array,
		uint32_t x_num, uint32_t y_num)
{
	struct property *data;
	int32_t len, ret;
#if NVT_DEBUG
	int32_t j = 0;
#endif
	uint32_t size;

	size = x_num * y_num;
	data = of_find_property(np, name, &len);
	len /= sizeof(u32);
	if ((!data) || (!len) || (len < size)) {
		NVT_ERR("error find %s. len=%d\n", name, len);
		return -1;
	} else {
		NVT_LOG("%s. len=%d\n", name, len);
		ret = of_property_read_u32_array(np, name, array, len);
		if (ret) {
			NVT_ERR("error reading %s. ret=%d\n", name, ret);
			return -1;
		}

#if NVT_DEBUG
		NVT_LOG("%s =\n", name);
		for (j = 0; j < y_num; j++) {
			nvt_print_data_log_in_one_line(array + j * x_num, x_num);
			printk("\n");
		}
#endif
	}

	return 0;
}

/*******************************************************
* Description:
*	Novatek touchscreen parse device tree mp function.
*
* return:
*	n.a.
*******************************************************/
int32_t nvt_mp_parse_dt(struct device_node *root, const char *node_compatible)
{
	struct device_node *np = root;
	struct device_node *child = NULL;

	NVT_LOG("Parse mp criteria for node %s\n", node_compatible);

	/* find each MP sub-nodes */
	for_each_child_of_node(root, child) {
		/* find the specified node */
		if (of_device_is_compatible(child, node_compatible)) {
			NVT_LOG("found child node %s\n", node_compatible);
			np = child;
			break;
		}
	}
	if (child == NULL) {
		NVT_ERR("Not found compatible node %s!\n", node_compatible);
		return -1;
	}

	/* MP Config*/
	if (nvt_mp_parse_u32(np, "IC_X_CFG_SIZE", &IC_X_CFG_SIZE))
		return -1;

	if (nvt_mp_parse_u32(np, "IC_Y_CFG_SIZE", &IC_Y_CFG_SIZE))
		return -1;

#if TOUCH_KEY_NUM > 0
	if (nvt_mp_parse_u32(np, "IC_KEY_CFG_SIZE", &IC_KEY_CFG_SIZE))
		return -1;
#endif

	if (nvt_mp_parse_u32(np, "X_Channel", &X_Channel))
		return -1;

	if (nvt_mp_parse_u32(np, "Y_Channel", &Y_Channel))
		return -1;

	if (nvt_mp_parse_ain(np, "AIN_X", AIN_X, IC_X_CFG_SIZE))
		return -1;

	if (nvt_mp_parse_ain(np, "AIN_Y", AIN_Y, IC_Y_CFG_SIZE))
		return -1;

#if TOUCH_KEY_NUM > 0
	if (nvt_mp_parse_ain(np, "AIN_KEY", AIN_KEY, IC_KEY_CFG_SIZE))
		return -1;
#endif

	/* MP Criteria */
	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Short_Rawdata_P", PS_Config_Lmt_Short_Rawdata_P,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Short_Rawdata_N", PS_Config_Lmt_Short_Rawdata_N,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Open_Rawdata_P", PS_Config_Lmt_Open_Rawdata_P,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_Open_Rawdata_N", PS_Config_Lmt_Open_Rawdata_N,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Rawdata_P", PS_Config_Lmt_FW_Rawdata_P,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Rawdata_N", PS_Config_Lmt_FW_Rawdata_N,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_CC_P", PS_Config_Lmt_FW_CC_P,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_CC_N", PS_Config_Lmt_FW_CC_N,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Diff_P", PS_Config_Lmt_FW_Diff_P,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (nvt_mp_parse_array(np, "PS_Config_Lmt_FW_Diff_N", PS_Config_Lmt_FW_Diff_N,
			X_Channel * Y_Channel + Key_Channel))
		return -1;

	if (ts->pen_support) {
		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Raw_P", PS_Config_Lmt_PenTipX_FW_Raw_P,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Raw_N", PS_Config_Lmt_PenTipX_FW_Raw_N,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Raw_P", PS_Config_Lmt_PenTipY_FW_Raw_P,
				ts->x_gang_num, ts->y_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Raw_N", PS_Config_Lmt_PenTipY_FW_Raw_N,
				ts->x_gang_num, ts->y_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Raw_P", PS_Config_Lmt_PenRingX_FW_Raw_P,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Raw_N", PS_Config_Lmt_PenRingX_FW_Raw_N,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingY_FW_Raw_P", PS_Config_Lmt_PenRingY_FW_Raw_P,
				ts->x_gang_num, ts->y_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingY_FW_Raw_N", PS_Config_Lmt_PenRingY_FW_Raw_N,
				ts->x_gang_num, ts->y_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Diff_P", PS_Config_Lmt_PenTipX_FW_Diff_P,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipX_FW_Diff_N", PS_Config_Lmt_PenTipX_FW_Diff_N,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Diff_P", PS_Config_Lmt_PenTipY_FW_Diff_P,
				ts->x_gang_num, ts->y_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenTipY_FW_Diff_N", PS_Config_Lmt_PenTipY_FW_Diff_N,
				ts->x_gang_num, ts->y_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Diff_P", PS_Config_Lmt_PenRingX_FW_Diff_P,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingX_FW_Diff_N", PS_Config_Lmt_PenRingX_FW_Diff_N,
				ts->x_num, ts->y_gang_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingY_FW_Diff_P", PS_Config_Lmt_PenRingY_FW_Diff_P,
				ts->x_gang_num, ts->y_num))
			return -1;

		if (nvt_mp_parse_pen_array(np, "PS_Config_Lmt_PenRingY_FW_Diff_N", PS_Config_Lmt_PenRingY_FW_Diff_N,
				ts->x_gang_num, ts->y_num))
			return -1;
	} /* if (ts->pen_support) */

	if (nvt_mp_parse_u32(np, "PS_Config_Diff_Test_Frame", &PS_Config_Diff_Test_Frame))
		return -1;

	NVT_LOG("Parse mp criteria done!\n");

	return 0;
}
#endif /* #ifdef CONFIG_OF */

/*******************************************************
* Description:
*	Novatek touchscreen parse header mp function.
*
* return:
*	n.a.
*******************************************************/
int32_t nvt_mp_parse_header(uint32_t index)
{
	/* MP Config*/
	IC_X_CFG_SIZE = Novatek_mp_criteria[index].IC_X_CFG_SIZE;
	IC_Y_CFG_SIZE = Novatek_mp_criteria[index].IC_Y_CFG_SIZE;
	IC_KEY_CFG_SIZE = Novatek_mp_criteria[index].IC_KEY_CFG_SIZE;
	X_Channel = Novatek_mp_criteria[index].X_Channel;
	Y_Channel = Novatek_mp_criteria[index].Y_Channel;
	memcpy(&AIN_X, &Novatek_mp_criteria[index].AIN_X, sizeof(AIN_X));
	memcpy(&AIN_Y, &Novatek_mp_criteria[index].AIN_Y, sizeof(AIN_Y));
#if TOUCH_KEY_NUM > 0
	memcpy(&AIN_KEY, &Novatek_mp_criteria[index].AIN_KEY, sizeof(AIN_KEY));
#endif
	memcpy(&PS_Config_Lmt_Short_Rawdata_P, &Novatek_mp_criteria[index].PS_Config_Lmt_Short_Rawdata_P, sizeof(PS_Config_Lmt_Short_Rawdata_P));
	memcpy(&PS_Config_Lmt_Short_Rawdata_N, &Novatek_mp_criteria[index].PS_Config_Lmt_Short_Rawdata_N, sizeof(PS_Config_Lmt_Short_Rawdata_N));
	memcpy(&PS_Config_Lmt_Open_Rawdata_P, &Novatek_mp_criteria[index].PS_Config_Lmt_Open_Rawdata_P, sizeof(PS_Config_Lmt_Open_Rawdata_P));
	memcpy(&PS_Config_Lmt_Open_Rawdata_N, &Novatek_mp_criteria[index].PS_Config_Lmt_Open_Rawdata_N, sizeof(PS_Config_Lmt_Open_Rawdata_N));
	memcpy(&PS_Config_Lmt_FW_Rawdata_P, &Novatek_mp_criteria[index].PS_Config_Lmt_FW_Rawdata_P, sizeof(PS_Config_Lmt_FW_Rawdata_P));
	memcpy(&PS_Config_Lmt_FW_Rawdata_N, &Novatek_mp_criteria[index].PS_Config_Lmt_FW_Rawdata_N, sizeof(PS_Config_Lmt_FW_Rawdata_N));
	memcpy(&PS_Config_Lmt_FW_CC_P, &Novatek_mp_criteria[index].PS_Config_Lmt_FW_CC_P, sizeof(PS_Config_Lmt_FW_CC_P));
	memcpy(&PS_Config_Lmt_FW_CC_N, &Novatek_mp_criteria[index].PS_Config_Lmt_FW_CC_N, sizeof(PS_Config_Lmt_FW_CC_N));
	memcpy(&PS_Config_Lmt_FW_Diff_P, &Novatek_mp_criteria[index].PS_Config_Lmt_FW_Diff_P, sizeof(PS_Config_Lmt_FW_Diff_P));
	memcpy(&PS_Config_Lmt_FW_Diff_N, &Novatek_mp_criteria[index].PS_Config_Lmt_FW_Diff_N, sizeof(PS_Config_Lmt_FW_Diff_N));
	PS_Config_Diff_Test_Frame = Novatek_mp_criteria[index].PS_Config_Diff_Test_Frame;
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen parse csv mp function.

return:
	n.a.
*******************************************************/

static void goto_next_line(char **ptr)
{
	do {
		*ptr = *ptr + 1;
	} while (**ptr != '\n');
	*ptr = *ptr + 1;
}

static void copy_this_line(char *dest, char *src)
{
	char *copy_from;
	char *copy_to;

	copy_from = src;
	copy_to = dest;
	do {
		*copy_to = *copy_from;
		copy_from++;
		copy_to++;
	} while ((*copy_from != '\n') && (*copy_from != '\r'));
	*copy_to = '\0';
}

int32_t update_mp_criteria(char *ptr, char *item, int32_t *array)
{
	int32_t i, j, ret, val, offset;
	char *token, *tok_ptr;
	char data[NVT_DATA_LENGTH] = {0};

	ptr = strstr(ptr, item);
	if (ptr == NULL) {
		NVT_ERR("load %s failed!\n", item);
		return -EINVAL;
	}
	goto_next_line(&ptr);
	printk("%s\n\n", item);
	for (i = 0; i < IC_Y_CFG_SIZE; i++) {
		copy_this_line(data, ptr);
		offset = strlen(data)+2;
		tok_ptr = data;
		j = 0;
		while ((token = strsep(&tok_ptr, ", \t\r\0"))) {
			if (strlen(token) == 0)
				continue;
			ret = kstrtoint(token, 10, &val);
			array[i * IC_X_CFG_SIZE + j] = (int32_t) val;
			printk("%5d, ", array[i * IC_X_CFG_SIZE + j]);
			j++;
		}
		printk("\n");
		ptr = ptr + offset;
	}
	printk("\n\n");
	return 0;
}

int32_t nvt_mp_parse_csv(void)
{
	int32_t retval, read_ret, i;
	struct file *fp = NULL;
	char *fbufp, *ptr;
	mm_segment_t org_fs;
	char file_path[NVT_FILE_NAME_LENGTH] =  { 0 };
	struct kstat stat;
	loff_t pos = 0;
	unsigned int temp_id = 0;
	struct nvt_upgrade_module *info = &nvt_module_list[0];

	temp_id = nt36523_incell_compare_id();
	if (temp_id == VID_NULL) {
		NVT_ERR("lcm_id does not match nt36523 incell IC, exit.\n");
		return -ENODEV;
	}
	NVT_LOG("Panel_ID=%x pid=%04X\n", temp_id, ts->nvt_pid);
	memset(file_path, 0, sizeof(file_path));
	for (i = 0; i < nvt_module_num; i++) {
		info = &nvt_module_list[i];
		if (temp_id == info->id) {
			snprintf(file_path, NVT_FILE_NAME_LENGTH, "%s%s",
				 NVT_MP_CRI_PATH, info->selftest_name);
			NVT_ERR("file_path=%s\n", file_path);
			break;
		}
	}

	NVT_LOG("++\n");
	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file_path, O_RDONLY | O_LARGEFILE, 0);
	if (fp == NULL || IS_ERR(fp)) {
		NVT_ERR("open %s failed\n", file_path);
		retval = -ENOENT;
		goto err_fopen;
	}
	retval = vfs_stat(file_path, &stat);
	if (retval) {
		NVT_ERR("failed to get file stat, retval = %d\n", retval);
		retval = -EINVAL;
		goto err_fopen;
	}
	fbufp = kzalloc(stat.size + 2, GFP_KERNEL);
	if (!fbufp) {
		NVT_ERR("kzalloc %lld bytes failed.\n", stat.size);
		retval = -ENOMEM;
		goto err_kzalloc;
	}
	read_ret = vfs_read(fp, (char __user *)fbufp, stat.size, &pos);
	if (read_ret <= 0) {
		NVT_ERR("retval=%d, read_ret=%d, fbufp=%p, stat.size=%lld\n", retval, read_ret, fbufp, stat.size);
		retval = -EINVAL;
		goto exit_free;
	}

	fbufp[stat.size] = '\n';
	fbufp[stat.size + 1] = '\0';
	ptr = fbufp;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_Short_Rawdata_P", PS_Config_Lmt_Short_Rawdata_P);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_Short_Rawdata_N", PS_Config_Lmt_Short_Rawdata_N);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_Open_Rawdata_P", PS_Config_Lmt_Open_Rawdata_P);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_Open_Rawdata_N", PS_Config_Lmt_Open_Rawdata_N);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_FW_Rawdata_P", PS_Config_Lmt_FW_Rawdata_P);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_FW_Rawdata_N", PS_Config_Lmt_FW_Rawdata_N);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_FW_CC_P", PS_Config_Lmt_FW_CC_P);
	if (retval)
	goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_FW_CC_N", PS_Config_Lmt_FW_CC_N);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_FW_Diff_P", PS_Config_Lmt_FW_Diff_P);
	if (retval)
		goto exit_free;
	retval = update_mp_criteria(ptr, "PS_Config_Lmt_FW_Diff_N", PS_Config_Lmt_FW_Diff_N);

exit_free:
	kfree(fbufp);
	fbufp = NULL;
err_kzalloc:
	if (fp) {
		filp_close(fp, NULL);
		fp = NULL;
	}
err_fopen:
	set_fs(org_fs);
	NVT_LOG("--, retval=%d\n", retval);
	return retval;
}


/*******************************************************
* Description:
*	Novatek touchscreen MP function proc. file node
*	initial function.
*
* return:
*	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_mp_proc_init(void)
{
	NVT_proc_selftest_entry = proc_create("nvt_selftest", 0444, NULL, &nvt_selftest_fops);
	if (NVT_proc_selftest_entry == NULL) {
		NVT_ERR("create /proc/nvt_selftest Failed!\n");
		return -1;
	} else {
		if (nvt_mp_buffer_init()) {
			NVT_ERR("Allocate mp memory failed\n");
			return -1;
		} else {
			NVT_LOG("create /proc/nvt_selftest Succeeded!\n");
		}
		return 0;
	}
}

/*******************************************************
* Description:
*	Novatek touchscreen MP function proc. file node
*	deinitial function.
*
* return:
*	n.a.
*******************************************************/
void nvt_mp_proc_deinit(void)
{
	nvt_mp_buffer_deinit();

	if (NVT_proc_selftest_entry != NULL) {
		remove_proc_entry("nvt_selftest", NULL);
		NVT_proc_selftest_entry = NULL;
		NVT_LOG("Removed /proc/%s\n", "nvt_selftest");
	}
}
#endif /* #if NVT_TOUCH_MP */
