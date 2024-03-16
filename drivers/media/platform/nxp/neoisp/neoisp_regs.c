// SPDX-License-Identifier: GPL-2.0+
/*
 * NEOISP registers array
 *
 * Copyright 2023 NXP
 * Author: Aymen Sghaier (aymen.sghaier@nxp.com)
 *
 */

#include "neoisp_regs.h"

/* array of all fields offsets */
const int neoisp_fields_a[NEOISP_FIELD_COUNT] = {
	NEO_PIPE_CONF_SOFT_RESET,
	NEO_PIPE_CONF_BUS_TXPARAM,
	NEO_PIPE_CONF_REG_XFR_DIS,
	NEO_PIPE_CONF_CSI_CTRL,
	NEO_PIPE_CONF_FRAME_NUM,
	NEO_PIPE_CONF_REG_SHD_CTRL,
	NEO_PIPE_CONF_REG_SHD_CMD,
	NEO_PIPE_CONF_TRIG_CAM0,
	NEO_PIPE_CONF_IMG_CONF_CAM0,
	NEO_PIPE_CONF_IMG_SIZE_CAM0,
	NEO_PIPE_CONF_IMG0_IN_ADDR_CAM0,
	NEO_PIPE_CONF_IMG1_IN_ADDR_CAM0,
	NEO_PIPE_CONF_OUTCH0_ADDR_CAM0,
	NEO_PIPE_CONF_OUTCH1_ADDR_CAM0,
	NEO_PIPE_CONF_OUTIR_ADDR_CAM0,
	NEO_PIPE_CONF_IMG0_IN_LS_CAM0,
	NEO_PIPE_CONF_IMG1_IN_LS_CAM0,
	NEO_PIPE_CONF_OUTCH0_LS_CAM0,
	NEO_PIPE_CONF_OUTCH1_LS_CAM0,
	NEO_PIPE_CONF_OUTIR_LS_CAM0,
	NEO_PIPE_CONF_SKIP_CTRL0,
	NEO_PIPE_CONF_INT_EN0,
	NEO_PIPE_CONF_INT_STAT0,
	NEO_PIPE_CONF_CSI_STAT,
	NEO_HC_CTRL_CAM0,
	NEO_HDR_DECOMPRESS0_CTRL_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_POINT1_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_POINT2_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_POINT3_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_POINT4_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_OFFSET0_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_OFFSET1_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_OFFSET2_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_OFFSET3_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_OFFSET4_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_RATIO01_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_RATIO23_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_RATIO4_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_NPOINT0_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_NPOINT1_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_NPOINT2_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_NPOINT3_CAM0,
	NEO_HDR_DECOMPRESS0_KNEE_NPOINT4_CAM0,
	NEO_HDR_DECOMPRESS1_CTRL_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_POINT1_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_POINT2_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_POINT3_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_POINT4_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_OFFSET0_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_OFFSET1_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_OFFSET2_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_OFFSET3_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_OFFSET4_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_RATIO01_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_RATIO23_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_RATIO4_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_NPOINT0_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_NPOINT1_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_NPOINT2_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_NPOINT3_CAM0,
	NEO_HDR_DECOMPRESS1_KNEE_NPOINT4_CAM0,
	NEO_OB_WB0_CTRL_CAM0,
	NEO_OB_WB0_R_CTRL_CAM0,
	NEO_OB_WB0_GR_CTRL_CAM0,
	NEO_OB_WB0_GB_CTRL_CAM0,
	NEO_OB_WB0_B_CTRL_CAM0,
	NEO_OB_WB1_CTRL_CAM0,
	NEO_OB_WB1_R_CTRL_CAM0,
	NEO_OB_WB1_GR_CTRL_CAM0,
	NEO_OB_WB1_GB_CTRL_CAM0,
	NEO_OB_WB1_B_CTRL_CAM0,
	NEO_OB_WB2_CTRL_CAM0,
	NEO_OB_WB2_R_CTRL_CAM0,
	NEO_OB_WB2_GR_CTRL_CAM0,
	NEO_OB_WB2_GB_CTRL_CAM0,
	NEO_OB_WB2_B_CTRL_CAM0,
	NEO_HDR_MERGE_CTRL_CAM0,
	NEO_HDR_MERGE_GAIN_OFFSET_CAM0,
	NEO_HDR_MERGE_GAIN_SCALE_CAM0,
	NEO_HDR_MERGE_GAIN_SHIFT_CAM0,
	NEO_HDR_MERGE_LUMA_TH_CAM0,
	NEO_HDR_MERGE_LUMA_SCALE_CAM0,
	NEO_HDR_MERGE_DOWNSCALE_CAM0,
	NEO_HDR_MERGE_UPSCALE_CAM0,
	NEO_HDR_MERGE_POST_SCALE_CAM0,
	NEO_HDR_MERGE_S_GAIN_OFFSET_CAM0,
	NEO_HDR_MERGE_S_GAIN_SCALE_CAM0,
	NEO_HDR_MERGE_S_GAIN_SHIFT_CAM0,
	NEO_HDR_MERGE_S_LUMA_TH_CAM0,
	NEO_HDR_MERGE_S_LUMA_SCALE_CAM0,
	NEO_HDR_MERGE_S_DOWNSCALE_CAM0,
	NEO_HDR_MERGE_S_UPSCALE_CAM0,
	NEO_HDR_MERGE_S_POST_SCALE_CAM0,
	NEO_HDR_MERGE_S_LINE_NUM_CAM0,
	NEO_COLOR_TEMP_CTRL_CAM0,
	NEO_COLOR_TEMP_ROI_POS_CAM0,
	NEO_COLOR_TEMP_ROI_SIZE_CAM0,
	NEO_COLOR_TEMP_REDGAIN_CAM0,
	NEO_COLOR_TEMP_BLUEGAIN_CAM0,
	NEO_COLOR_TEMP_POINT1_CAM0,
	NEO_COLOR_TEMP_POINT2_CAM0,
	NEO_COLOR_TEMP_HOFFSET_CAM0,
	NEO_COLOR_TEMP_VOFFSET_CAM0,
	NEO_COLOR_TEMP_POINT1_SLOPE_CAM0,
	NEO_COLOR_TEMP_POINT2_SLOPE_CAM0,
	NEO_COLOR_TEMP_LUMA_TH_CAM0,
	NEO_COLOR_TEMP_CSC_MAT0_CAM0,
	NEO_COLOR_TEMP_CSC_MAT1_CAM0,
	NEO_COLOR_TEMP_CSC_MAT2_CAM0,
	NEO_COLOR_TEMP_CSC_MAT3_CAM0,
	NEO_COLOR_TEMP_CSC_MAT4_CAM0,
	NEO_COLOR_TEMP_R_GR_OFFSET_CAM0,
	NEO_COLOR_TEMP_GB_B_OFFSET_CAM0,
	NEO_COLOR_TEMP_CNT_WHITE_CAM0,
	NEO_COLOR_TEMP_SUMRL_CAM0,
	NEO_COLOR_TEMP_SUMRH_CAM0,
	NEO_COLOR_TEMP_SUMGL_CAM0,
	NEO_COLOR_TEMP_SUMGH_CAM0,
	NEO_COLOR_TEMP_SUMBL_CAM0,
	NEO_COLOR_TEMP_SUMBH_CAM0,
	NEO_COLOR_TEMP_SUMRGL_CAM0,
	NEO_COLOR_TEMP_SUMRGH_CAM0,
	NEO_COLOR_TEMP_SUMBGL_CAM0,
	NEO_COLOR_TEMP_SUMBGH_CAM0,
	NEO_COLOR_TEMP_STAT_BLK_SIZE0,
	NEO_COLOR_TEMP_STAT_CURR_BLK_Y0,
	NEO_COLOR_TEMP_CROI0_POS_CAM0,
	NEO_COLOR_TEMP_CROI0_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI0_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI0_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI0_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI1_POS_CAM0,
	NEO_COLOR_TEMP_CROI1_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI1_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI1_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI1_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI2_POS_CAM0,
	NEO_COLOR_TEMP_CROI2_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI2_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI2_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI2_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI3_POS_CAM0,
	NEO_COLOR_TEMP_CROI3_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI3_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI3_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI3_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI4_POS_CAM0,
	NEO_COLOR_TEMP_CROI4_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI4_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI4_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI4_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI5_POS_CAM0,
	NEO_COLOR_TEMP_CROI5_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI5_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI5_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI5_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI6_POS_CAM0,
	NEO_COLOR_TEMP_CROI6_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI6_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI6_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI6_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI7_POS_CAM0,
	NEO_COLOR_TEMP_CROI7_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI7_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI7_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI7_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI8_POS_CAM0,
	NEO_COLOR_TEMP_CROI8_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI8_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI8_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI8_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_CROI9_POS_CAM0,
	NEO_COLOR_TEMP_CROI9_PIXCNT_CAM0,
	NEO_COLOR_TEMP_CROI9_SUMRED_CAM0,
	NEO_COLOR_TEMP_CROI9_SUMGREEN_CAM0,
	NEO_COLOR_TEMP_CROI9_SUMBLUE_CAM0,
	NEO_COLOR_TEMP_GR_AVG_IN_CAM0,
	NEO_COLOR_TEMP_GB_AVG_IN_CAM0,
	NEO_COLOR_TEMP_GR_GB_CNT_CAM0,
	NEO_COLOR_TEMP_GR_SUM_CAM0,
	NEO_COLOR_TEMP_GB_SUM_CAM0,
	NEO_COLOR_TEMP_GR2_SUM_CAM0,
	NEO_COLOR_TEMP_GB2_SUM_CAM0,
	NEO_COLOR_TEMP_GRGB_SUM_CAM0,
	NEO_RGBIR_CTRL_CAM0,
	NEO_RGBIR_CCM0_CAM0,
	NEO_RGBIR_CCM1_CAM0,
	NEO_RGBIR_CCM2_CAM0,
	NEO_RGBIR_CCM0_TH_CAM0,
	NEO_RGBIR_CCM1_TH_CAM0,
	NEO_RGBIR_CCM2_TH_CAM0,
	NEO_RGBIR_ROI0_POS_CAM0,
	NEO_RGBIR_ROI0_SIZE_CAM0,
	NEO_RGBIR_ROI1_POS_CAM0,
	NEO_RGBIR_ROI1_SIZE_CAM0,
	NEO_RGBIR_HIST0_CTRL_CAM0,
	NEO_RGBIR_HIST0_SCALE_CAM0,
	NEO_RGBIR_HIST1_CTRL_CAM0,
	NEO_RGBIR_HIST1_SCALE_CAM0,
	NEO_STAT_ROI0_POS_CAM0,
	NEO_STAT_ROI0_SIZE_CAM0,
	NEO_STAT_ROI1_POS_CAM0,
	NEO_STAT_ROI1_SIZE_CAM0,

	NEO_STAT_HIST0_CTRL_CAM0,
	NEO_STAT_HIST0_SCALE_CAM0,
	NEO_STAT_HIST1_CTRL_CAM0,
	NEO_STAT_HIST1_SCALE_CAM0,
	NEO_STAT_HIST2_CTRL_CAM0,
	NEO_STAT_HIST2_SCALE_CAM0,
	NEO_STAT_HIST3_CTRL_CAM0,
	NEO_STAT_HIST3_SCALE_CAM0,
	NEO_IR_COMPRESS_CTRL_CAM0,
	NEO_IR_COMPRESS_KNEE_POINT1_CAM0,
	NEO_IR_COMPRESS_KNEE_POINT2_CAM0,
	NEO_IR_COMPRESS_KNEE_POINT3_CAM0,
	NEO_IR_COMPRESS_KNEE_POINT4_CAM0,
	NEO_IR_COMPRESS_KNEE_OFFSET0_CAM0,
	NEO_IR_COMPRESS_KNEE_OFFSET1_CAM0,

	NEO_IR_COMPRESS_KNEE_OFFSET2_CAM0,
	NEO_IR_COMPRESS_KNEE_OFFSET3_CAM0,
	NEO_IR_COMPRESS_KNEE_OFFSET4_CAM0,
	NEO_IR_COMPRESS_KNEE_RATIO01_CAM0,
	NEO_IR_COMPRESS_KNEE_RATIO23_CAM0,
	NEO_IR_COMPRESS_KNEE_RATIO4_CAM0,
	NEO_IR_COMPRESS_KNEE_NPOINT0_CAM0,
	NEO_IR_COMPRESS_KNEE_NPOINT1_CAM0,
	NEO_IR_COMPRESS_KNEE_NPOINT2_CAM0,
	NEO_IR_COMPRESS_KNEE_NPOINT3_CAM0,
	NEO_IR_COMPRESS_KNEE_NPOINT4_CAM0,
	NEO_BNR_CTRL_CAM0,
	NEO_BNR_YPEAK_CAM0,
	NEO_BNR_YEDGE_TH0_CAM0,
	NEO_BNR_YEDGE_SCALE_CAM0,
	NEO_BNR_YEDGES_TH0_CAM0,
	NEO_BNR_YEDGES_SCALE_CAM0,
	NEO_BNR_YEDGEA_TH0_CAM0,
	NEO_BNR_YEDGEA_SCALE_CAM0,
	NEO_BNR_YLUMA_X_TH0_CAM0,
	NEO_BNR_YLUMA_Y_TH_CAM0,
	NEO_BNR_YLUMA_SCALE_CAM0,
	NEO_BNR_YALPHA_GAIN_CAM0,
	NEO_BNR_CPEAK_CAM0,
	NEO_BNR_CEDGE_TH0_CAM0,
	NEO_BNR_CEDGE_SCALE_CAM0,
	NEO_BNR_CEDGES_TH0_CAM0,
	NEO_BNR_CEDGES_SCALE_CAM0,
	NEO_BNR_CEDGEA_TH0_CAM0,
	NEO_BNR_CEDGEA_SCALE_CAM0,
	NEO_BNR_CLUMA_X_TH0_CAM0,
	NEO_BNR_CLUMA_Y_TH_CAM0,
	NEO_BNR_CLUMA_SCALE_CAM0,
	NEO_BNR_CALPHA_GAIN_CAM0,
	NEO_BNR_EDGE_STAT_CAM0,
	NEO_BNR_EDGES_STAT_CAM0,
	NEO_BNR_STRETCH_CAM0,
	NEO_VIGNETTING_CTRL_CAM0,
	NEO_VIGNETTING_BLK_CONF_CAM0,
	NEO_VIGNETTING_BLK_SIZE_CAM0,
	NEO_VIGNETTING_BLK_STEPY_CAM0,
	NEO_VIGNETTING_BLK_STEPX_CAM0,
	NEO_VIGNETTING_BLK_C_LINE_CAM0,
	NEO_VIGNETTING_BLK_C_ROW_CAM0,
	NEO_VIGNETTING_BLK_C_FRACY_CAM0,
	NEO_IDBG1_LINE_NUM,
	NEO_IDBG1_CURR_LINE_NUM,
	NEO_IDBG1_IMA,
	NEO_IDBG1_IMD,
	NEO_IDBG1_DONE_STAT,
	NEO_DEMOSAIC_CTRL_CAM0,
	NEO_DEMOSAIC_ACTIVITY_CTL_CAM0,
	NEO_DEMOSAIC_DYNAMICS_CTL0_CAM0,
	NEO_DEMOSAIC_DYNAMICS_CTL2_CAM0,
	NEO_RGB_TO_YUV_GAIN_CTRL_CAM0,
	NEO_RGB_TO_YUV_MAT0_CAM0,
	NEO_RGB_TO_YUV_MAT1_CAM0,
	NEO_RGB_TO_YUV_MAT2_CAM0,
	NEO_RGB_TO_YUV_MAT3_CAM0,
	NEO_RGB_TO_YUV_MAT4_CAM0,
	NEO_RGB_TO_YUV_MAT5_CAM0,
	NEO_RGB_TO_YUV_OFFSET0_CAM0,
	NEO_RGB_TO_YUV_OFFSET1_CAM0,
	NEO_RGB_TO_YUV_OFFSET2_CAM0,
	NEO_DRC_ROI0_POS_CAM0,
	NEO_DRC_ROI0_SIZE_CAM0,
	NEO_DRC_ROI1_POS_CAM0,
	NEO_DRC_ROI1_SIZE_CAM0,
	NEO_DRC_GROI_SUM_SHIFT_CAM0,
	NEO_DRC_GBL_GAIN_CAM0,
	NEO_DRC_LCL_BLK_SIZE_CAM0,
	NEO_DRC_LCL_STRETCH_CAM0,
	NEO_DRC_LCL_BLK_STEPY_CAM0,
	NEO_DRC_LCL_BLK_STEPX_CAM0,
	NEO_DRC_LCL_SUM_SHIFT_CAM0,
	NEO_DRC_ALPHA_CAM0,
	NEO_DRC_GROI0_SUM_CAM0,
	NEO_DRC_GROI1_SUM_CAM0,
	NEO_DRC_STAT_BLK_Y_CAM0,
	NEO_DRC_CURR_YFRACT_CAM0,
	NEO_NR_CTRL_CAM0,
	NEO_NR_BLEND_SCALE_CAM0,
	NEO_NR_BLEND_TH0_CAM0,
	NEO_NR_EDGECNT_CAM0,
	NEO_DF_CTRL_CAM0,
	NEO_DF_TH_SCALE_CAM0,
	NEO_DF_BLEND_SHIFT_CAM0,
	NEO_DF_BLEND_TH0_CAM0,
	NEO_DF_EDGECNT_CAM0,
	NEO_EE_CTRL_CAM0,
	NEO_EE_CORING_CAM0,
	NEO_EE_CLIP_CAM0,
	NEO_EE_MASKGAIN_CAM0,
	NEO_EE_EDGECNT_CAM0,
	NEO_CCONVMED_CTRL_CAM0,
	NEO_CAS_GAIN_CAM0,
	NEO_CAS_CORR_CAM0,
	NEO_CAS_OFFSET_CAM0,
	NEO_PACKETIZER_CH0_CTRL_CAM0,
	NEO_PACKETIZER_CH12_CTRL_CAM0,
	NEO_PACKETIZER_PACK_CTRL_CAM0,
	NEO_GCM_IMAT0_CAM0,
	NEO_GCM_IMAT1_CAM0,
	NEO_GCM_IMAT2_CAM0,
	NEO_GCM_IMAT3_CAM0,
	NEO_GCM_IMAT4_CAM0,
	NEO_GCM_IMAT5_CAM0,
	NEO_GCM_IOFFSET0_CAM0,
	NEO_GCM_IOFFSET1_CAM0,
	NEO_GCM_IOFFSET2_CAM0,
	NEO_GCM_OMAT0_CAM0,
	NEO_GCM_OMAT1_CAM0,
	NEO_GCM_OMAT2_CAM0,
	NEO_GCM_OMAT3_CAM0,
	NEO_GCM_OMAT4_CAM0,
	NEO_GCM_OMAT5_CAM0,
	NEO_GCM_OOFFSET0_CAM0,
	NEO_GCM_OOFFSET1_CAM0,
	NEO_GCM_OOFFSET2_CAM0,
	NEO_GCM_GAMMA0_CAM0,
	NEO_GCM_GAMMA1_CAM0,
	NEO_GCM_GAMMA2_CAM0,
	NEO_GCM_BLKLVL0_CTRL_CAM0,
	NEO_GCM_BLKLVL1_CTRL_CAM0,
	NEO_GCM_BLKLVL2_CTRL_CAM0,
	NEO_GCM_LOWTH_CTRL01_CAM0,
	NEO_GCM_LOWTH_CTRL2_CAM0,
	NEO_GCM_MAT_CONFG_CAM0,
	NEO_AUTOFOCUS_ROI0_POS_CAM0,
	NEO_AUTOFOCUS_ROI0_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI1_POS_CAM0,
	NEO_AUTOFOCUS_ROI1_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI2_POS_CAM0,
	NEO_AUTOFOCUS_ROI2_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI3_POS_CAM0,
	NEO_AUTOFOCUS_ROI3_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI4_POS_CAM0,
	NEO_AUTOFOCUS_ROI4_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI5_POS_CAM0,
	NEO_AUTOFOCUS_ROI5_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI6_POS_CAM0,
	NEO_AUTOFOCUS_ROI6_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI7_POS_CAM0,
	NEO_AUTOFOCUS_ROI7_SIZE_CAM0,
	NEO_AUTOFOCUS_ROI8_POS_CAM0,
	NEO_AUTOFOCUS_ROI8_SIZE_CAM0,
	NEO_AUTOFOCUS_FIL0_COEFFS0_CAM0,
	NEO_AUTOFOCUS_FIL0_COEFFS1_CAM0,
	NEO_AUTOFOCUS_FIL0_COEFFS2_CAM0,
	NEO_AUTOFOCUS_FIL0_SHIFT_CAM0,
	NEO_AUTOFOCUS_FIL1_COEFFS0_CAM0,
	NEO_AUTOFOCUS_FIL1_COEFFS1_CAM0,
	NEO_AUTOFOCUS_FIL1_COEFFS2_CAM0,
	NEO_AUTOFOCUS_FIL1_SHIFT_CAM0,
	NEO_AUTOFOCUS_ROI0_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI0_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI1_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI1_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI2_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI2_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI3_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI3_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI4_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI4_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI5_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI5_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI6_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI6_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI7_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI7_SUM1_CAM0,
	NEO_AUTOFOCUS_ROI8_SUM0_CAM0,
	NEO_AUTOFOCUS_ROI8_SUM1_CAM0,
	NEO_IDBG2_LINE_NUM,
	NEO_IDBG2_CURR_LINE_NUM,
	NEO_IDBG2_IMA,
	NEO_IDBG2_IMD,
	NEO_IDBG2_DONE_STAT,
	NEO_ALIAS_ALIAS_REG0,
	NEO_ALIAS_ALIAS_REG1,
	NEO_ALIAS_ALIAS_REG2,
	NEO_ALIAS_ALIAS_REG3,
	NEO_ALIAS_ALIAS_REG4,
	NEO_ALIAS_ALIAS_REG5,
	NEO_ALIAS_ALIAS_REG6,
	NEO_ALIAS_ALIAS_REG7,
	NEO_ALIAS_ALIAS_REG8,
	NEO_ALIAS_ALIAS_REG9,
	NEO_ALIAS_ALIAS_REG10,
	NEO_ALIAS_ALIAS_REG11,
	NEO_ALIAS_ALIAS_REG12,
	NEO_ALIAS_ALIAS_REG13,
	NEO_ALIAS_ALIAS_REG14,
	NEO_ALIAS_ALIAS_REG15,
	NEO_ALIAS_ALIAS_REG16,
	NEO_ALIAS_ALIAS_REG17,
	NEO_ALIAS_ALIAS_REG18,
	NEO_ALIAS_ALIAS_REG19,
	NEO_ALIAS_ALIAS_REG20,
	NEO_ALIAS_ALIAS_REG21,
	NEO_ALIAS_ALIAS_REG22,
	NEO_ALIAS_ALIAS_REG23,
	NEO_ALIAS_ALIAS_REG24,
	NEO_ALIAS_ALIAS_REG25,
	NEO_ALIAS_ALIAS_REG26,
	NEO_ALIAS_ALIAS_REG27,
	NEO_ALIAS_ALIAS_REG28,
	NEO_ALIAS_ALIAS_REG29,
	NEO_ALIAS_ALIAS_REG30,
	NEO_ALIAS_ALIAS_REG31,
	NEO_ALIAS_ALIAS_REG32,
	NEO_ALIAS_ALIAS_REG33,
	NEO_ALIAS_ALIAS_REG34,
	NEO_ALIAS_ALIAS_REG35,
	NEO_ALIAS_ALIAS_REG36,
	NEO_ALIAS_ALIAS_REG37,
	NEO_ALIAS_ALIAS_REG38,
	NEO_ALIAS_ALIAS_REG39,
	NEO_ALIAS_ALIAS_REG40,
	NEO_ALIAS_ALIAS_REG41,
	NEO_ALIAS_ALIAS_REG42,
	NEO_ALIAS_ALIAS_REG43,
	NEO_ALIAS_ALIAS_REG44,
	NEO_ALIAS_ALIAS_REG45,
	NEO_ALIAS_ALIAS_REG46,
	NEO_ALIAS_ALIAS_REG47,
	NEO_ALIAS_ALIAS_REG48,
	NEO_ALIAS_ALIAS_REG49,
	NEO_ALIAS_ALIAS_REG50,
	NEO_ALIAS_ALIAS_REG51,
	NEO_ALIAS_ALIAS_REG52,
	NEO_ALIAS_ALIAS_REG53,
	NEO_ALIAS_ALIAS_REG54,
	NEO_ALIAS_ALIAS_REG55,
	NEO_ALIAS_ALIAS_REG56,
	NEO_ALIAS_ALIAS_REG57,
	NEO_ALIAS_ALIAS_REG58,
	NEO_ALIAS_ALIAS_REG59,
	NEO_ALIAS_ALIAS_REG60,
	NEO_ALIAS_ALIAS_REG61,
	NEO_ALIAS_ALIAS_REG62,
	NEO_ALIAS_ALIAS_REG63,
	NEO_ALIAS_ALIAS_REG64,
	NEO_ALIAS_ALIAS_REG65,
	NEO_ALIAS_ALIAS_REG66,
	NEO_ALIAS_ALIAS_REG67,
	NEO_ALIAS_ALIAS_REG68,
	NEO_ALIAS_ALIAS_REG69,
	NEO_ALIAS_ALIAS_REG70,
	NEO_ALIAS_ALIAS_REG71,
	NEO_ALIAS_ALIAS_REG72,
	NEO_ALIAS_ALIAS_REG73,
	NEO_ALIAS_ALIAS_REG74,
	NEO_ALIAS_ALIAS_REG75,
	NEO_ALIAS_ALIAS_REG76,
	NEO_ALIAS_ALIAS_REG77,
	NEO_ALIAS_ALIAS_REG78,
	NEO_ALIAS_ALIAS_REG79,
	NEO_ALIAS_ALIAS_REG80,
	NEO_ALIAS_ALIAS_REG81,
	NEO_ALIAS_ALIAS_REG82,
	NEO_ALIAS_ALIAS_REG83,
};

const struct regmap_config neoisp_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.fast_io = true,
	.cache_type = REGCACHE_NONE,
};

