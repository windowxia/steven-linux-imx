// SPDX-License-Identifier: GPL-2.0+
/*
 * NEOISP main driver source code
 *
 * This driver is highly inspired from "Raspberry Pi PiSP Backend (BE) ISP driver", developed by
 * "David Plowman <david.plowman@raspberrypi.com>" and
 * "Nick Hollinghurst <nick.hollinghurst@raspberrypi.com>"
 *
 * Copyright 2023-2024 NXP
 * Author: Aymen Sghaier (aymen.sghaier@nxp.com)
 */

#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/minmax.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>

#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-dma-contig.h>
#include <uapi/linux/nxp_neoisp.h>

#include "neoisp.h"
#include "neoisp_regs.h"
#include "neoisp_ctx.h"

static int neoisp_regfield_alloc(struct device *dev, struct neoisp_dev_s *neoispd)
{
	int idx = 0;
	struct reg_field default_regf = REG_FIELD(0, 0, 31);

	for (idx = 0; idx < NEOISP_FIELD_COUNT; idx++) {
		default_regf.reg = neoisp_fields_a[idx];
		neoispd->regs.fields[idx] =
			devm_regmap_field_alloc(dev, neoispd->regmap, default_regf);
		if (IS_ERR(neoispd->regs.fields[idx]))
			return PTR_ERR(neoispd->regs.fields[idx]);
	}

	return 0;
}

static void neoisp_fill_mp(struct v4l2_format *f, const struct neoisp_fmt_s *fmt)
{
	unsigned int nplanes = f->fmt.pix_mp.num_planes;
	unsigned int i;

	for (i = 0; i < nplanes; i++) {
		struct v4l2_plane_pix_format *p = &f->fmt.pix_mp.plane_fmt[i];
		unsigned int bpl, plane_size;

		bpl = f->fmt.pix_mp.width * ((fmt->bit_depth + 7) >> 3);
		bpl = ALIGN(max(p->bytesperline, bpl), fmt->align);

		plane_size = bpl * f->fmt.pix_mp.height;
		if (nplanes > 1)
			plane_size /= fmt->pl_divisors[i];
		plane_size = max(p->sizeimage, plane_size);

		p->bytesperline = bpl;
		p->sizeimage = plane_size;
	}
}

static inline struct neoisp_fmt_s *neoisp_def_format(enum neoisp_fmt_type_e type)
{
	struct neoisp_fmt_s *fmt = NULL;

	switch (type) {
	case NEOISP_FMT_VIDEO_CAPTURE:
		fmt = (struct neoisp_fmt_s *)&formats_vcap[0];
		break;
	case NEOISP_FMT_VIDEO_OUTPUT:
		fmt = (struct neoisp_fmt_s *)&formats_vout[0];
		break;
	case NEOISP_FMT_META_CAPTURE:
		fmt = (struct neoisp_fmt_s *)&formats_mcap[0];
		break;
	case NEOISP_FMT_META_OUTPUT:
		fmt = (struct neoisp_fmt_s *)&formats_mout[0];
		break;
	default:
		break;
	}
	return fmt;
}

static inline int to_neoisp_fmt_type(enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		if (TYPE_IS_META(type))
			return NEOISP_FMT_META_OUTPUT;
		else
			return NEOISP_FMT_VIDEO_OUTPUT;
	else
		if (TYPE_IS_META(type))
			return NEOISP_FMT_VIDEO_OUTPUT;
		else
			return NEOISP_FMT_VIDEO_CAPTURE;
}

static const struct neoisp_fmt_s *neoisp_find_pixel_format(u32 pixel_format,
		int fmt_type)
{
	__u32 i;

	for (i = 0; i < ARRAY_SIZE(formats_vout); i++) {
		const struct neoisp_fmt_s *fmt = &formats_vout[i];

		if (fmt->fourcc == pixel_format && fmt->type & fmt_type)
			return fmt;
	}

	for (i = 0; i < ARRAY_SIZE(formats_vcap); i++) {
		const struct neoisp_fmt_s *fmt = &formats_vcap[i];

		if (fmt->fourcc == pixel_format && fmt->type & fmt_type)
			return fmt;
	}

	for (i = 0; i < ARRAY_SIZE(formats_mcap); i++) {
		const struct neoisp_fmt_s *fmt = &formats_mcap[i];

		if (fmt->fourcc == pixel_format && fmt->type & fmt_type)
			return fmt;
	}

	for (i = 0; i < ARRAY_SIZE(formats_mout); i++) {
		const struct neoisp_fmt_s *fmt = &formats_mout[i];

		if (fmt->fourcc == pixel_format && fmt->type & fmt_type)
			return fmt;
	}

	return NULL;
}

static int neoisp_node_queue_setup(struct vb2_queue *q, __u32 *nbuffers,
		__u32 *nplanes, __u32 sizes[],
		struct device *alloc_devs[])
{
	struct neoisp_node_s *node = vb2_get_drv_priv(q);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	*nplanes = 1;
	if (NODE_IS_MPLANE(node)) {
		__u32 i;

		*nplanes = node->format.fmt.pix_mp.num_planes;
		for (i = 0; i < *nplanes; i++) {
			__u32 size =
				node->format.fmt.pix_mp.plane_fmt[i].sizeimage;
			if (sizes[i] && sizes[i] < size) {
				dev_err(&neoispd->pdev->dev, "%s: size %u < %u\n",
						__func__, sizes[i], size);
				return -EINVAL;
			}
			sizes[i] = size;
		}
	} else if (NODE_IS_META(node)) {
		sizes[0] = node->format.fmt.meta.buffersize;
		/*
		 * Limit the config node buffer count to the number of internal
		 * buffers allocated.
		 */
		if (node->id == NEOISP_PARAMS_NODE)
			*nbuffers = min_t(__u32, *nbuffers,
					VB2_MAX_FRAME);
	}

	dev_dbg(&neoispd->pdev->dev,
			"Image (or metadata) size %u, nbuffers %u for node %s\n",
			sizes[0], *nbuffers, NODE_NAME(node));

	return 0;
}

static int neoisp_node_buffer_prepare(struct vb2_buffer *vb)
{
	struct neoisp_node_s *node = vb2_get_drv_priv(vb->vb2_queue);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;
	unsigned long size = 0;
	__u32 num_planes = NODE_IS_MPLANE(node) ?
		node->format.fmt.pix_mp.num_planes : 1;
	__u32 i;

	for (i = 0; i < num_planes; i++) {
		size = NODE_IS_MPLANE(node)
			? node->format.fmt.pix_mp.plane_fmt[i].sizeimage
			: node->format.fmt.meta.buffersize;

		if (vb2_plane_size(vb, i) < size) {
			dev_err(&neoispd->pdev->dev,
					"data will not fit into plane %d (%lu < %lu)\n",
					i, vb2_plane_size(vb, i), size);
			return -EINVAL;
		}

		vb2_set_plane_payload(vb, i, size);
	}

	if (node->id == NEOISP_PARAMS_NODE) {
		void *dst = &node->node_group->params[vb->index];
		void *src = vb2_plane_vaddr(vb, 0);

		memcpy(dst, src, sizeof(struct neoisp_meta_params_s));
	}

	return 0;
}

static void send_frame_sync_event(struct neoisp_dev_s *neoispd)
{
	struct v4l2_subdev *sd =  &neoispd->queued_job.node_group->sd;
	__u32 sequence = neoispd->queued_job.node_group->frame_sequence;

	struct v4l2_event ev = {
		.type = V4L2_EVENT_FRAME_SYNC,
		.u.frame_sync.frame_sequence = sequence,
	};

	v4l2_event_queue(sd->devnode, &ev);
}

__u32 *get_vaddr(struct neoisp_buffer_s *buf)
{
	if (buf)
		return vb2_plane_vaddr(&buf->vb.vb2_buf, 0);
	return NULL;
}

static dma_addr_t get_addr(struct neoisp_buffer_s *buf, __u32 num_plane)
{
	if (buf)
		return vb2_dma_contig_plane_dma_addr(&buf->vb.vb2_buf, num_plane);
	return 0;
}

static void neoisp_config_gcm_for_yuv(struct neoisp_dev_s *neoispd)
{
	/**
	 * conversion matrix values comes from:
	 * https://en.wikipedia.org/wiki/YCbCr#ITU-R_BT.601_conversion
	 */
	__s16 yuv_mat[3][3] = {
		{65, 129, 25},
		{-38, -74, 112},
		{112, -94, -18},
	};

	regmap_field_write(neoispd->regs.fields[NEO_GCM_OMAT0_CAM0_IDX],
			NEO_GCM_OMAT0_CAM0_R0C0_SET(yuv_mat[0][0])
			| NEO_GCM_OMAT0_CAM0_R0C1_SET(yuv_mat[0][1]));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OMAT1_CAM0_IDX],
			NEO_GCM_OMAT1_CAM0_R0C2_SET(yuv_mat[0][2]));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OMAT2_CAM0_IDX],
			NEO_GCM_OMAT2_CAM0_R1C0_SET(yuv_mat[1][0])
			| NEO_GCM_OMAT2_CAM0_R1C1_SET(yuv_mat[1][1]));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OMAT3_CAM0_IDX],
			NEO_GCM_OMAT3_CAM0_R1C2_SET(yuv_mat[1][2]));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OMAT4_CAM0_IDX],
			NEO_GCM_OMAT4_CAM0_R2C0_SET(yuv_mat[2][0])
			| NEO_GCM_OMAT4_CAM0_R2C1_SET(yuv_mat[2][1]));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OMAT5_CAM0_IDX],
			NEO_GCM_OMAT5_CAM0_R2C2_SET(yuv_mat[2][2]));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OOFFSET0_CAM0_IDX],
			NEO_GCM_OOFFSET0_CAM0_OFFSET0_SET(256));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OOFFSET1_CAM0_IDX],
			NEO_GCM_OOFFSET1_CAM0_OFFSET1_SET(2048));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_OOFFSET2_CAM0_IDX],
			NEO_GCM_OOFFSET2_CAM0_OFFSET2_SET(2048));
	regmap_field_write(neoispd->regs.fields[NEO_GCM_MAT_CONFG_CAM0_IDX],
			NEO_GCM_MAT_CONFG_CAM0_SIGN_CONFG_SET(1));
}

static void neoisp_config_gcm_for_rgb(struct neoisp_dev_s *neoispd)
{
	/* set default gcm parameters that corresponds to rgb output */
	neoisp_set_gcm(&neoisp_default_params.regs, neoispd);

}

static int neoisp_set_packetizer(struct neoisp_dev_s *neoispd)
{
	struct neoisp_mparam_packetizer_s *pck = &mod_params.pack;
	struct neoisp_node_s *nd = &neoispd->queued_job.node_group->node[NEOISP_FRAME_NODE];
	__u32 pixfmt = nd->format.fmt.pix_mp.pixelformat;

	if (FMT_IS_YUV(pixfmt))
		neoisp_config_gcm_for_yuv(neoispd);
	else
		neoisp_config_gcm_for_rgb(neoispd);

	switch (pixfmt) {
	case V4L2_PIX_FMT_NV12:
		pck->ctrl_cam0_type = 0;
		pck->ch12_ctrl_cam0_subsample = 2;
		pck->ch12_ctrl_cam0_obpp = 6;
		/* set channels orders */
		pck->ctrl_cam0_order0 = 0;
		pck->ctrl_cam0_order1 = 0;
		pck->ctrl_cam0_order2 = 1;
		break;
	case V4L2_PIX_FMT_YUYV:
		pck->ctrl_cam0_type = 1;
		pck->ch12_ctrl_cam0_subsample = 1;
		/* set channels orders */
		pck->ctrl_cam0_order0 = 0;
		pck->ctrl_cam0_order1 = 1;
		pck->ctrl_cam0_order2 = 3;
		break;
	default: /* all other pixel formats */
		pck->ctrl_cam0_type = 1;
		pck->ch12_ctrl_cam0_subsample = 0;
		/* set channels orders */
		pck->ctrl_cam0_order0 = 0;
		pck->ctrl_cam0_order1 = 1;
		pck->ctrl_cam0_order2 = 2;
		break;
	}

	/* packetizer settings */
	regmap_field_write(neoispd->regs.fields[NEO_PACKETIZER_CH0_CTRL_CAM0_IDX],
			NEO_PACKETIZER_CH0_CTRL_CAM0_OBPP_SET(pck->ch0_ctrl_cam0_obpp)
			| NEO_PACKETIZER_CH0_CTRL_CAM0_RSA_SET(pck->ch0_ctrl_cam0_rsa)
			| NEO_PACKETIZER_CH0_CTRL_CAM0_LSA_SET(pck->ch0_ctrl_cam0_lsa));
	regmap_field_write(neoispd->regs.fields[NEO_PACKETIZER_CH12_CTRL_CAM0_IDX],
			NEO_PACKETIZER_CH12_CTRL_CAM0_OBPP_SET(pck->ch12_ctrl_cam0_obpp)
			| NEO_PACKETIZER_CH12_CTRL_CAM0_RSA_SET(pck->ch12_ctrl_cam0_rsa)
			| NEO_PACKETIZER_CH12_CTRL_CAM0_LSA_SET(pck->ch12_ctrl_cam0_lsa)
			| NEO_PACKETIZER_CH12_CTRL_CAM0_SUBSAMPLE_SET(
				pck->ch12_ctrl_cam0_subsample));
	regmap_field_write(neoispd->regs.fields[NEO_PACKETIZER_PACK_CTRL_CAM0_IDX],
			NEO_PACKETIZER_PACK_CTRL_CAM0_TYPE_SET(pck->ctrl_cam0_type)
			| NEO_PACKETIZER_PACK_CTRL_CAM0_ORDER0_SET(pck->ctrl_cam0_order0)
			| NEO_PACKETIZER_PACK_CTRL_CAM0_ORDER1_SET(pck->ctrl_cam0_order1)
			| NEO_PACKETIZER_PACK_CTRL_CAM0_ORDER2_SET(pck->ctrl_cam0_order2)
			| NEO_PACKETIZER_PACK_CTRL_CAM0_A0S_SET(pck->ctrl_cam0_a0s));
	return 0;
}

static void neoisp_update_hdr_decompress(__u32 ibpp)
{
	struct neoisp_reg_params_s *regp = &neoisp_default_params.regs;

	/* 16 bits per pixel is already in default configuration */
	if (ibpp < 16) { /* 8, 10, 12 or 14 */
		regp->decompress_input0.knee_point1 = 1 << ibpp;
		regp->decompress_input0.knee_ratio0 = (1 << (20 - ibpp)) - 1;
	}
}

/*
 *  set pipe conf settings
 */
static int neoisp_set_pipe_conf(struct neoisp_dev_s *neoispd)
{
	struct neoisp_buffer_s *buf = neoispd->queued_job.buf[NEOISP_INPUT0_NODE];
	struct neoisp_buffer_s *buf_out = neoispd->queued_job.buf[NEOISP_FRAME_NODE];
	struct neoisp_node_s *nd = &neoispd->queued_job.node_group->node[NEOISP_FRAME_NODE];
	struct neoisp_mparam_conf_s *cfg = &mod_params.conf;
	__u32 width, height, obpp, ibpp, hoffset, voffset;
	__u32 out_pixfmt = nd->format.fmt.pix_mp.pixelformat;

	width = nd->format.fmt.pix_mp.width;
	height = nd->format.fmt.pix_mp.height;
	obpp = (nd->neoisp_format->bit_depth + 7) / 8;
	nd = &neoispd->queued_job.node_group->node[NEOISP_INPUT0_NODE];
	ibpp = (nd->neoisp_format->bit_depth + 7) / 8;
	cfg->img_conf_cam0_ibpp0 = nd->neoisp_format->ibpp;

	/*
	 * Set Head Color selection
	 */
	switch (nd->format.fmt.pix_mp.pixelformat) {
	case (V4L2_PIX_FMT_SRGGB8):
	case (V4L2_PIX_FMT_SRGGB10):
	case (V4L2_PIX_FMT_SRGGB12):
	case (V4L2_PIX_FMT_SRGGB14):
	case (V4L2_PIX_FMT_SRGGB16):
		hoffset = 0;
		voffset = 0;
		break;
	case (V4L2_PIX_FMT_SGRBG8):
	case (V4L2_PIX_FMT_SGRBG10):
	case (V4L2_PIX_FMT_SGRBG12):
	case (V4L2_PIX_FMT_SGRBG14):
	case (V4L2_PIX_FMT_SGRBG16):
		hoffset = 1;
		voffset = 0;
		break;
	case (V4L2_PIX_FMT_SGBRG8):
	case (V4L2_PIX_FMT_SGBRG10):
	case (V4L2_PIX_FMT_SGBRG12):
	case (V4L2_PIX_FMT_SGBRG14):
	case (V4L2_PIX_FMT_SGBRG16):
		hoffset = 0;
		voffset = 1;
		break;
	case (V4L2_PIX_FMT_SBGGR8):
	case (V4L2_PIX_FMT_SBGGR10):
	case (V4L2_PIX_FMT_SBGGR12):
	case (V4L2_PIX_FMT_SBGGR14):
	case (V4L2_PIX_FMT_SBGGR16):
		hoffset = 1;
		voffset = 1;
		break;
	}
	regmap_field_write(neoispd->regs.fields[NEO_HC_CTRL_CAM0_IDX],
			NEO_HC_CTRL_CAM0_HOFFSET_SET(hoffset)
			| NEO_HC_CTRL_CAM0_VOFFSET_SET(voffset));
	/*
	 * FIXME get conf depending on input image
	 */
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_IMG_CONF_CAM0_IDX],
			NEO_PIPE_CONF_IMG_CONF_CAM0_IBPP0_SET(cfg->img_conf_cam0_ibpp0)
			| NEO_PIPE_CONF_IMG_CONF_CAM0_INALIGN0_SET(cfg->img_conf_cam0_inalign0)
			| NEO_PIPE_CONF_IMG_CONF_CAM0_LPALIGN0_SET(cfg->img_conf_cam0_lpalign0)
			| NEO_PIPE_CONF_IMG_CONF_CAM0_IBPP1_SET(cfg->img_conf_cam0_ibpp1)
			| NEO_PIPE_CONF_IMG_CONF_CAM0_INALIGN1_SET(cfg->img_conf_cam0_inalign1)
			| NEO_PIPE_CONF_IMG_CONF_CAM0_LPALIGN1_SET(cfg->img_conf_cam0_lpalign1));
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_IMG_SIZE_CAM0_IDX],
			NEO_PIPE_CONF_IMG_SIZE_CAM0_WIDTH_SET(width)
			| NEO_PIPE_CONF_IMG_SIZE_CAM0_HEIGHT_SET(height));
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_IMG0_IN_LS_CAM0_IDX],
			NEO_PIPE_CONF_IMG0_IN_LS_CAM0_LS_SET(ibpp * width));
	/* raw image addr from video output input0 node buffer */
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_IMG0_IN_ADDR_CAM0_IDX],
			NEO_PIPE_CONF_IMG_ADDR_CAM0_SET(get_addr(buf, 0)));
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_IMG1_IN_ADDR_CAM0_IDX], 0u);
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_IMG1_IN_LS_CAM0_IDX], 0u);

	/* rgb output image address */
	if (out_pixfmt == V4L2_PIX_FMT_NV12) {
		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH0_ADDR_CAM0_IDX],
				NEO_PIPE_CONF_IMG_ADDR_CAM0_SET(get_addr(buf_out, 0)));
		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH0_LS_CAM0_IDX],
				NEO_PIPE_CONF_IMG0_IN_LS_CAM0_LS_SET(width));

		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH1_ADDR_CAM0_IDX],
				NEO_PIPE_CONF_IMG_ADDR_CAM0_SET(get_addr(buf_out, 1)));
		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH1_LS_CAM0_IDX],
				NEO_PIPE_CONF_IMG0_IN_LS_CAM0_LS_SET(width / 2));
	} else {
		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH0_ADDR_CAM0_IDX], 0u);
		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH0_LS_CAM0_IDX], 0u);

		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH1_ADDR_CAM0_IDX],
				NEO_PIPE_CONF_IMG_ADDR_CAM0_SET(get_addr(buf_out, 0)));
		regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTCH1_LS_CAM0_IDX],
				NEO_PIPE_CONF_IMG0_IN_LS_CAM0_LS_SET(obpp * width));
	}
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_OUTIR_LS_CAM0_IDX], 0u);

	return 0;
}

static void neoisp_reset_hw(struct neoisp_dev_s *neoispd, bool is_hw)
{
	__u32 bit = NEO_PIPE_CONF_SOFT_RESET_SOFT_RESET;
	__u32 val, count = 100;

	if (is_hw)
		bit = NEO_PIPE_CONF_SOFT_RESET_HARD_RESET;

	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_SOFT_RESET_IDX], bit);

	/* wait for auto-clear */
	do {
		regmap_field_read(neoispd->regs.fields[NEO_PIPE_CONF_SOFT_RESET_IDX], &val);
		count--;
	} while ((val & bit) && count);
}

static void neoisp_queue_job(struct neoisp_dev_s *neoispd,
		struct neoisp_node_group_s *node_group,
		struct neoisp_meta_params_s *params)
{
	/* if params provided then do setup */
	if (!IS_ERR_OR_NULL(params))
		neoisp_set_params(neoispd, params);
	neoisp_set_packetizer(neoispd);
	neoisp_set_pipe_conf(neoispd);

	/* kick off the hw */
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_TRIG_CAM0_IDX],
			NEO_PIPE_CONF_TRIG_CAM0_TRIGGER);
	send_frame_sync_event(neoispd);
	dev_dbg(&neoispd->pdev->dev, "isp starting\n");
}

static int neoisp_schedule_internal(struct neoisp_node_group_s *node_group, unsigned long flags)
{
	struct neoisp_meta_params_s *params = NULL;
	struct neoisp_dev_s *neoispd = node_group->neoisp_dev;
	struct neoisp_buffer_s *buf[NEOISP_NODES_COUNT];
	struct neoisp_node_s *node;
	unsigned long flags1;
	int i, params_index;
	__u8 staggered_input = 0; /* FIXME compute the actual flag */
	__u8 rgbir_input = 0; /* FIXME compute the actual flag */

	/*
	 * To schedule a job, we need input0 and params (if not disabled) streaming nodes
	 *  to have a buffer ready,
	 * (Note that streaming_map is protected by hw_lock, which is held.)
	 */
	if ((BIT(NEOISP_INPUT0_NODE) & node_group->streaming_map)
			!= BIT(NEOISP_INPUT0_NODE)) {
		dev_dbg(&neoispd->pdev->dev, "Input0 node not ready, nothing to do\n");
		return 0;
	}
	if ((BIT(NEOISP_FRAME_NODE) & node_group->streaming_map)
			!= BIT(NEOISP_FRAME_NODE)) {
		dev_dbg(&neoispd->pdev->dev, "Frame node not ready, nothing to do\n");
		return 0;
	}

	if (!mod_params.test.disable_params) {
		if ((BIT(NEOISP_PARAMS_NODE) & node_group->streaming_map)
				!= BIT(NEOISP_PARAMS_NODE)) {
			dev_dbg(&neoispd->pdev->dev, "Params is not disabled and not ready\n");
			return 0;
		}

		node = &node_group->node[NEOISP_PARAMS_NODE];
		spin_lock_irqsave(&node->ready_lock, flags1);
		buf[NEOISP_PARAMS_NODE] =
			list_first_entry_or_null(&node->ready_queue, struct neoisp_buffer_s,
					ready_list);
		spin_unlock_irqrestore(&node->ready_lock, flags1);

		/* Exit early if no params buffer and not disabled. */
		if (!buf[NEOISP_PARAMS_NODE])
			return 0;
		params_index = buf[NEOISP_PARAMS_NODE]->vb.vb2_buf.index;
		params = &node_group->params[params_index];
	}

	for (i = 0; i < NEOISP_NODES_COUNT; i++) {
		bool ignore_buffers = false;

		/* Params node is handled outside the loop above. */
		if (i == NEOISP_PARAMS_NODE)
			continue;

		buf[i] = NULL;
		if (!(node_group->streaming_map & BIT(i)))
			continue;
		/*
		 * Check whether buffer will be ignored or not according to pix formats, ...
		 * if staggered input then "input1" buffer will be handled
		 * if rgbir input format is set then "ir" buffer will be handled
		 * ...
		 */
		if ((!rgbir_input && i == NEOISP_IR_NODE) ||
				(!staggered_input && i == NEOISP_INPUT1_NODE)) {
			ignore_buffers = true;
		}

		node = &node_group->node[i];

		spin_lock_irqsave(&node->ready_lock, flags1);
		buf[i] = list_first_entry_or_null(&node->ready_queue,
				struct neoisp_buffer_s,
				ready_list);
		spin_unlock_irqrestore(&node->ready_lock, flags1);
		if (!buf[i] && !ignore_buffers) {
			dev_dbg(&neoispd->pdev->dev, "Nothing to do\n");
			return 0;
		}
	}

	/* Pull a buffer from each V4L2 queue to form the queued job */
	for (i = 0; i < NEOISP_NODES_COUNT; i++) {
		if (buf[i]) {
			node = &node_group->node[i];
			spin_lock_irqsave(&node->ready_lock, flags1);
			list_del(&buf[i]->ready_list);
			spin_unlock_irqrestore(&node->ready_lock,
					flags1);
		}
		neoispd->queued_job.buf[i] = buf[i];
	}

	neoispd->queued_job.node_group = node_group;
	neoispd->hw_busy = 1;
	spin_unlock_irqrestore(&neoispd->hw_lock, flags);

	/*
	 * We can kick the job off without the hw_lock, as this can
	 * never run again until hw_busy is cleared, which will happen
	 * only when the following job has been queued.
	 */
	dev_dbg(&neoispd->pdev->dev, "Have buffers - starting hardware\n");

	neoisp_queue_job(neoispd, node_group, params);

	return 1;
}
/* Try and schedule a job for just a single node group. */
static void neoisp_schedule_one(struct neoisp_node_group_s *node_group)
{
	struct neoisp_dev_s *neoispd = node_group->neoisp_dev;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&neoispd->hw_lock, flags);
	if (neoispd->hw_busy) {
		spin_unlock_irqrestore(&neoispd->hw_lock, flags);
		return;
	}

	/* A non-zero return means the lock was released. */
	ret = neoisp_schedule_internal(node_group, flags);
	if (!ret)
		spin_unlock_irqrestore(&neoispd->hw_lock, flags);
}

static void neoisp_node_buffer_queue(struct vb2_buffer *buf)
{
	struct vb2_v4l2_buffer *vbuf =
		container_of(buf, struct vb2_v4l2_buffer, vb2_buf);
	struct neoisp_buffer_s *buffer =
		container_of(vbuf, struct neoisp_buffer_s, vb);
	struct neoisp_node_s *node = vb2_get_drv_priv(buf->vb2_queue);
	struct neoisp_node_group_s *node_group = node->node_group;
	struct neoisp_dev_s *neoisp = node->node_group->neoisp_dev;
	unsigned long flags;

	dev_dbg(&neoisp->pdev->dev, "%s: for node %s\n", __func__, NODE_NAME(node));
	spin_lock_irqsave(&node->ready_lock, flags);
	list_add_tail(&buffer->ready_list, &node->ready_queue);
	spin_unlock_irqrestore(&node->ready_lock, flags);

	/*
	 * Every time we add a buffer, check if there's now some work for the hw
	 * to do, but only for this client.
	 */
	neoisp_schedule_one(node_group);
}

static int neoisp_node_start_streaming(struct vb2_queue *q, __u32 count)
{
	unsigned long flags;
	struct neoisp_node_s *node = vb2_get_drv_priv(q);
	struct neoisp_node_group_s *node_group = node->node_group;
	struct neoisp_dev_s *neoisp = node_group->neoisp_dev;
	int ret;

	ret = pm_runtime_resume_and_get(&neoisp->pdev->dev);
	if (ret < 0)
		return ret;

	spin_lock_irqsave(&neoisp->hw_lock, flags);
	node->node_group->streaming_map |=  BIT(node->id);
	spin_unlock_irqrestore(&neoisp->hw_lock, flags);

	dev_dbg(&neoisp->pdev->dev, "%s: for node %s (count %u)\n",
			__func__, NODE_NAME(node), count);
	dev_dbg(&neoisp->pdev->dev, "Nodes streaming for this group now 0x%x\n",
			node->node_group->streaming_map);

	/* Maybe we're ready to run. */
	neoisp_schedule_one(node_group);

	return 0;
}

static void neoisp_node_stop_streaming(struct vb2_queue *q)
{
	struct neoisp_node_s *node = vb2_get_drv_priv(q);
	struct neoisp_node_group_s *node_group = node->node_group;
	struct neoisp_dev_s *neoispd = node_group->neoisp_dev;
	struct neoisp_buffer_s *buf;
	unsigned long flags;

	/*
	 * Now this is a bit awkward. In a simple M2M device we could just wait
	 * for all queued jobs to complete, but here there's a risk that a
	 * partial set of buffers was queued and cannot be run. For now, just
	 * cancel all buffers stuck in the "ready queue", then wait for any
	 * running job.
	 * XXX This may return buffers out of order.
	 */
	dev_dbg(&neoispd->pdev->dev, "%s: for node %s\n", __func__, NODE_NAME(node));
	spin_lock_irqsave(&neoispd->hw_lock, flags);
	do {
		unsigned long flags1;

		spin_lock_irqsave(&node->ready_lock, flags1);
		buf = list_first_entry_or_null(&node->ready_queue,
				struct neoisp_buffer_s,
				ready_list);
		if (buf) {
			list_del(&buf->ready_list);
			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		}
		spin_unlock_irqrestore(&node->ready_lock, flags1);
	} while (buf);
	spin_unlock_irqrestore(&neoispd->hw_lock, flags);

	vb2_wait_for_all_buffers(&node->queue);

	spin_lock_irqsave(&neoispd->hw_lock, flags);
	node_group->streaming_map &= ~BIT(node->id);
	spin_unlock_irqrestore(&neoispd->hw_lock, flags);

	pm_runtime_mark_last_busy(&neoispd->pdev->dev);
	pm_runtime_put_autosuspend(&neoispd->pdev->dev);

	dev_dbg(&neoispd->pdev->dev, "Nodes streaming for this group now 0x%x\n",
			node_group->streaming_map);
}

static const struct vb2_ops neoisp_node_queue_ops = {
	.queue_setup = neoisp_node_queue_setup,
	.buf_prepare = neoisp_node_buffer_prepare,
	.buf_queue = neoisp_node_buffer_queue,
	.start_streaming = neoisp_node_start_streaming,
	.stop_streaming = neoisp_node_stop_streaming,
};

static const struct v4l2_file_operations neoisp_fops = {
	.owner          = THIS_MODULE,
	.open           = v4l2_fh_open,
	.release        = vb2_fop_release,
	.poll           = vb2_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap           = vb2_fop_mmap
};

static int neoisp_querycap(struct file *file, void *priv,
		struct v4l2_capability *cap)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	strscpy(cap->driver, NEOISP_NAME, sizeof(cap->driver));
	strscpy(cap->card, NEOISP_NAME, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
			dev_name(&neoispd->pdev->dev));

	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE |
		V4L2_CAP_VIDEO_OUTPUT_MPLANE |
		V4L2_CAP_STREAMING | V4L2_CAP_DEVICE_CAPS |
		V4L2_CAP_META_OUTPUT | V4L2_CAP_META_CAPTURE;
	cap->device_caps = node->vfd.device_caps;

	dev_dbg(&neoispd->pdev->dev, "Caps for node %s: %x and %x (dev %x)\n",
			NODE_NAME(node), cap->capabilities, cap->device_caps,
			node->vfd.device_caps);
	return 0;
}

static int neoisp_enum_fmt(struct file *file, void *priv, struct v4l2_fmtdesc *f)
{
	struct neoisp_node_s *node = video_drvdata(file);

	if (f->type != node->queue.type)
		return -EINVAL;

	f->flags = 0;
	if (NODE_IS_META(node)) {
		if (f->index)
			return -EINVAL;

		if (NODE_IS_OUTPUT(node))
			f->pixelformat = V4L2_META_FMT_NEO_ISP_PARAMS;
		else
			f->pixelformat = V4L2_META_FMT_NEO_ISP_STATS;
		return 0;
	}
	if (NODE_IS_OUTPUT(node)) {
		if (f->index >= ARRAY_SIZE(formats_vout))
			return -EINVAL;

		f->pixelformat = formats_vout[f->index].fourcc;
	} else {
		if (f->index >= ARRAY_SIZE(formats_vcap))
			return -EINVAL;

		f->pixelformat = formats_vcap[f->index].fourcc;
	}

	return 0;
}

static int neoisp_enum_framesizes(struct file *file, void *priv,
		struct v4l2_frmsizeenum *fsize)
{
	const struct neoisp_fmt_s *fmt;

	if (fsize->index)
		return -EINVAL;

	fmt = neoisp_find_pixel_format(fsize->pixel_format,
			NEOISP_FMT_VIDEO_OUTPUT | NEOISP_FMT_VIDEO_CAPTURE);
	if (!fmt)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_STEPWISE;
	fsize->stepwise = neoisp_frmsize_stepwise;

	return 0;
}

static int neoisp_g_fmt_meta(struct file *file, void *priv, struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	if (!NODE_IS_META(node)) {
		dev_err(&neoispd->pdev->dev,
				"Cannot get meta fmt for video node %s\n",
				NODE_NAME(node));
		return -EINVAL;
	}
	*f = node->format;
	dev_dbg(&neoispd->pdev->dev, "Get meta format for node %s\n",
			NODE_NAME(node));
	return 0;
}

static int neoisp_try_fmt_meta_out(struct file *file, void *priv, struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	if (!NODE_IS_META(node) || NODE_IS_CAPTURE(node)) {
		dev_err(&neoispd->pdev->dev,
				"Cannot set capture fmt for meta output node %s\n",
				NODE_NAME(node));
		return -EINVAL;
	}

	f->fmt.meta.dataformat = V4L2_META_FMT_NEO_ISP_PARAMS;
	f->fmt.meta.buffersize = sizeof(struct neoisp_meta_params_s);

	return 0;
}

static int neoisp_try_fmt_meta_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	if (!NODE_IS_META(node) || NODE_IS_OUTPUT(node)) {
		dev_err(&neoispd->pdev->dev,
				"Cannot set capture fmt for meta output node %s\n",
				NODE_NAME(node));
		return -EINVAL;
	}

	f->fmt.meta.dataformat = V4L2_META_FMT_NEO_ISP_STATS;
	if (!f->fmt.meta.buffersize)
		f->fmt.meta.buffersize = sizeof(struct neoisp_meta_stats_s);

	return 0;
}

static int neoisp_s_fmt_meta_out(struct file *file, void *priv, struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;
	int ret = neoisp_try_fmt_meta_out(file, priv, f);

	if (ret < 0)
		return ret;

	node->format = *f;
	node->neoisp_format = &formats_mout[0];

	dev_dbg(&neoispd->pdev->dev,
			"Set output format for meta node %s to %x\n",
			NODE_NAME(node),
			f->fmt.meta.dataformat);
	return 0;
}

static int neoisp_s_fmt_meta_cap(struct file *file, void *priv, struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;
	int ret = neoisp_try_fmt_meta_cap(file, priv, f);

	if (ret < 0)
		return ret;

	node->format = *f;
	node->neoisp_format = &formats_mcap[0];

	dev_dbg(&neoispd->pdev->dev,
			"Set capture format for meta node %s to %x\n",
			NODE_NAME(node),
			f->fmt.meta.dataformat);
	return 0;
}

static int neoisp_g_fmt_vid(struct file *file, void *priv, struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	if (NODE_IS_META(node)) {
		dev_err(&neoispd->pdev->dev,
				"Cannot get video fmt for meta node %s\n",
				NODE_NAME(node));
		return -EINVAL;
	}
	*f = node->format;
	dev_dbg(&neoispd->pdev->dev, "Get video format for node %s\n",
			NODE_NAME(node));
	return 0;
}

static int neoisp_try_fmt(struct v4l2_format *f, struct neoisp_node_s *node)
{
	const struct neoisp_fmt_s *fmt;
	unsigned int is_srgb;
	u32 pixfmt = f->fmt.pix_mp.pixelformat;

	if ((pixfmt == V4L2_META_FMT_NEO_ISP_STATS)
			|| (pixfmt == V4L2_META_FMT_NEO_ISP_PARAMS))
		return 0; /* FIXME do check the buffer size */

	fmt = neoisp_find_pixel_format(pixfmt, NEOISP_FMT_VIDEO_OUTPUT | NEOISP_FMT_VIDEO_CAPTURE);
	if (!fmt) {
		if (NODE_IS_OUTPUT(node))
			fmt = &formats_vout[0];
		else
			fmt = &formats_vcap[0];
	}

	f->fmt.pix_mp.pixelformat = fmt->fourcc;
	f->fmt.pix_mp.num_planes = fmt->num_planes;
	f->fmt.pix_mp.field = V4L2_FIELD_NONE;
	f->fmt.pix_mp.width = max(min(f->fmt.pix_mp.width, 65536u), 64u);
	f->fmt.pix_mp.height = max(min(f->fmt.pix_mp.height, 65536u), 64u);

	if (NODE_IS_OUTPUT(node))
		/* FIXME: we should use V4L2_COLORSPACE_RAW here instead of SRGB */
		f->fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;
	else
		f->fmt.pix_mp.colorspace = V4L2_COLORSPACE_SRGB;

	/* In all cases, we only support the defaults for these: */
	f->fmt.pix_mp.ycbcr_enc = V4L2_MAP_YCBCR_ENC_DEFAULT(f->fmt.pix_mp.colorspace);
	f->fmt.pix_mp.xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(f->fmt.pix_mp.colorspace);
	is_srgb = f->fmt.pix_mp.colorspace == V4L2_COLORSPACE_SRGB;
	f->fmt.pix_mp.quantization =
		V4L2_MAP_QUANTIZATION_DEFAULT(is_srgb, f->fmt.pix_mp.colorspace,
				f->fmt.pix_mp.ycbcr_enc);
	/* Set plane size and bytes/line for each plane. */
	neoisp_fill_mp(f, fmt);

	return 0;
}

static int neoisp_try_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	if (!NODE_IS_CAPTURE(node) || NODE_IS_META(node)) {
		dev_err(&neoispd->pdev->dev,
				"Cannot set capture fmt for output node %s\n",
				NODE_NAME(node));
		return -EINVAL;
	}

	return neoisp_try_fmt(f, node);
}

static int neoisp_s_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	int ret;

	ret = neoisp_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;

	node->format = *f;
	node->neoisp_format =
		neoisp_find_pixel_format(f->fmt.pix_mp.pixelformat, NEOISP_FMT_VIDEO_CAPTURE);

	return 0;
}

static int neoisp_try_fmt_vid_out(struct file *file, void *priv,
		struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	if (!NODE_IS_OUTPUT(node) || NODE_IS_META(node)) {
		dev_err(&neoispd->pdev->dev,
				"Cannot set capture fmt for output node %s\n",
				NODE_NAME(node));
		return -EINVAL;
	}

	return neoisp_try_fmt(f, node);
}

static int neoisp_s_fmt_vid_out(struct file *file, void *priv,
		struct v4l2_format *f)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;
	int ret = neoisp_try_fmt_vid_out(file, priv, f);

	if (ret < 0)
		return ret;

	node->format = *f;
	node->neoisp_format =
		neoisp_find_pixel_format(f->fmt.pix_mp.pixelformat, NEOISP_FMT_VIDEO_OUTPUT);

	dev_dbg(&neoispd->pdev->dev,
			"Set output format for node %s to %x\n",
			NODE_NAME(node),
			f->fmt.pix_mp.pixelformat);
	return 0;
}

static int neoisp_node_streamon(struct file *file, void *priv,
		enum v4l2_buf_type type)
{
	struct neoisp_node_s *node = video_drvdata(file);
	struct neoisp_dev_s *neoispd = node->node_group->neoisp_dev;

	dev_dbg(&neoispd->pdev->dev, "Stream on for node %s\n", NODE_NAME(node));
	/*
	 * Check if this is input0 node to preload default params
	 */
	if (node->id == NEOISP_INPUT0_NODE) {
		neoisp_update_hdr_decompress(node->neoisp_format->bit_depth);
		/* program registers and look-up tables */
		neoisp_set_params(neoispd, &neoisp_default_params);
	}

	INIT_LIST_HEAD(&node->ready_queue);

	/* init frame_sequence */
	node->node_group->frame_sequence = 0;

	/* locking should be handled by the queue->lock? */
	return vb2_streamon(&node->queue, type);
}

static int neoisp_node_streamoff(struct file *file, void *priv,
		enum v4l2_buf_type type)
{
	struct neoisp_node_s *node = video_drvdata(file);

	return vb2_streamoff(&node->queue, type);
}

static const struct v4l2_ioctl_ops neoisp_ioctl_ops = {
	.vidioc_querycap		= neoisp_querycap,

	.vidioc_enum_fmt_vid_cap	= neoisp_enum_fmt,
	.vidioc_enum_fmt_meta_cap	= neoisp_enum_fmt,
	.vidioc_enum_framesizes		= neoisp_enum_framesizes,
	.vidioc_g_fmt_vid_cap_mplane	= neoisp_g_fmt_vid,
	.vidioc_s_fmt_vid_cap_mplane	= neoisp_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap_mplane	= neoisp_try_fmt_vid_cap,
	.vidioc_g_fmt_meta_cap		= neoisp_g_fmt_meta,
	.vidioc_s_fmt_meta_cap		= neoisp_s_fmt_meta_cap,
	.vidioc_try_fmt_meta_cap	= neoisp_try_fmt_meta_cap,

	.vidioc_enum_fmt_vid_out	= neoisp_enum_fmt,
	.vidioc_enum_fmt_meta_out	= neoisp_enum_fmt,
	.vidioc_g_fmt_vid_out_mplane	= neoisp_g_fmt_vid,
	.vidioc_s_fmt_vid_out_mplane	= neoisp_s_fmt_vid_out,
	.vidioc_try_fmt_vid_out_mplane	= neoisp_try_fmt_vid_out,
	.vidioc_g_fmt_meta_out		= neoisp_g_fmt_meta,
	.vidioc_s_fmt_meta_out		= neoisp_s_fmt_meta_out,
	.vidioc_try_fmt_meta_out	= neoisp_try_fmt_meta_out,

	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_prepare_buf		= vb2_ioctl_prepare_buf,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_expbuf			= vb2_ioctl_expbuf,

	.vidioc_streamon		= neoisp_node_streamon,
	.vidioc_streamoff		= neoisp_node_streamoff,

	.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
};

static const struct video_device neoisp_videodev = {
	.name = NEOISP_NAME,
	.vfl_dir = VFL_DIR_M2M,
	.fops = &neoisp_fops,
	.ioctl_ops = &neoisp_ioctl_ops,
	.minor = -1,
	.release = video_device_release_empty,
};

/* Try and schedule a job for next of the node groups. */
static void neoisp_schedule_next(struct neoisp_dev_s *neoispd, bool clear_hw_busy)
{
	unsigned long flags;

	spin_lock_irqsave(&neoispd->hw_lock, flags);

	if (clear_hw_busy)
		neoispd->hw_busy = 0;
	if (neoispd->hw_busy == 0) {
		unsigned int i;

		for (i = 0; i < NEOISP_NODE_GROUPS_COUNT; i++) {
			/*
			 * A non-zero return from neoisp_schedule_internal means
			 * the lock was released.
			 */
			if (neoisp_schedule_internal(&neoispd->node_group[i], flags))
				return;
		}
	}
	spin_unlock_irqrestore(&neoispd->hw_lock, flags);
}

/*
 * extract offset and size in bytes from memory region map
 */
static void neoisp_get_offsize(enum isp_block_map_e map, __u32 *offset, __u32 *size)
{
	*offset = ISP_GET_OFF(map) * sizeof(__u32);
	*size = ISP_GET_WSZ(map) * sizeof(__u32);
}

static void neoisp_get_stats(struct neoisp_dev_s *neoispd, struct neoisp_buffer_s *buf)
{
	struct neoisp_meta_stats_s *dest = (struct neoisp_meta_stats_s *)get_vaddr(buf);
	__u8 *src = (__u8 *)(uintptr_t)neoispd->mmio_tcm;
	__u32 offset, size;

	/* check if stats node */
	if (mod_params.test.disable_stats)
		return;

	if (IS_ERR_OR_NULL(dest) || IS_ERR_OR_NULL(src)) {
		dev_err(&neoispd->pdev->dev, "Error: stats pointer\n");
		return;
	}
	/* get stats from registers */
	regmap_bulk_read(neoispd->regmap,
				NEO_ALIAS_ALIAS_REG0,
				(__u32 *)(uintptr_t)dest,
				sizeof(struct neoisp_reg_stats_s) / sizeof(uint32_t));

	/* get ctemp stats from memory */
	memcpy(&dest->mems.ctemp, src, sizeof(struct neoisp_ctemp_mem_stats_s));

	/* get rgbir stats from memory */
	neoisp_get_offsize(NEO_RGBIR_HIST_MAP, &offset, &size);
	memcpy(&dest->mems.rgbir, &src[offset], size);

	/* get histograms stats from memory */
	neoisp_get_offsize(NEO_HIST_STAT_MAP, &offset, &size);
	memcpy(&dest->mems.hist, &src[offset], size);

	/* get drc local sum stats from memory */
	neoisp_get_offsize(NEO_DRC_LOCAL_SUM_MAP, &offset, &size);
	memcpy(&dest->mems.drc.drc_local_sum, &src[offset], size);

	/* get drc hist roi0 stats from memory */
	neoisp_get_offsize(NEO_DRC_GLOBAL_HIST_ROI0_MAP, &offset, &size);
	memcpy(&dest->mems.drc.drc_global_hist_roi0, &src[offset], size);

	/* get drc hist roi1 stats from memory */
	neoisp_get_offsize(NEO_DRC_GLOBAL_HIST_ROI1_MAP, &offset, &size);
	memcpy(&dest->mems.drc.drc_global_hist_roi1, &src[offset], size);
}

static irqreturn_t neoisp_irq_handler(int irq, void *dev_id)
{
	struct neoisp_dev_s *neoispd = (struct neoisp_dev_s *)dev_id;
	struct neoisp_buffer_s **buf = neoispd->queued_job.buf;
	struct neoisp_node_group_s *node_group = neoispd->queued_job.node_group;
	__u64 ts = ktime_get_ns();
	__u32 irq_status = 0;
	__u32 irq_clear = 0;
	bool done = false;
	int i, ret;

	regmap_field_read(neoispd->regs.fields[NEO_PIPE_CONF_INT_STAT0_IDX], &irq_status);

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_FS1) {
		dev_dbg(&neoispd->pdev->dev, "Neo IRQ FS1 !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_FS1;
		done = false;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_FS2) {
		dev_dbg(&neoispd->pdev->dev, "Neo IRQ FS2 !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_FS2;
		done = false;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_FD1) {
		dev_dbg(&neoispd->pdev->dev, "Neo IRQ FD1 !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_FD1;
		done = false;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_STATD) {
		dev_dbg(&neoispd->pdev->dev, "Neo IRQ STATD !\n");
		neoisp_get_stats(neoispd, buf[NEOISP_STATS_NODE]);
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_STATD;
		done = false;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_DRCD) {
		dev_dbg(&neoispd->pdev->dev, "Neo IRQ DRCD !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_DRCD;
		done = false;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_BUS_ERR_MASK) {
		dev_err(&neoispd->pdev->dev, "Neo IRQ BUS ERR !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_BUS_ERR_MASK;
		done = true;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_TRIG_ERR) {
		dev_err(&neoispd->pdev->dev, "Neo IRQ TRIG ERR !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_TRIG_ERR;
		done = true;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_CSI_TERR) {
		dev_err(&neoispd->pdev->dev, "Neo IRQ TRIG CSI Trigger ERR !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_CSI_TERR;
		done = true;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_S_FD2) {
		dev_dbg(&neoispd->pdev->dev, "Neo IRQ FD2 !\n");
		irq_clear |= NEO_PIPE_CONF_INT_STAT0_S_FD2;
		done = true;
	}

	if (irq_status & NEO_PIPE_CONF_INT_STAT0_BUSY)
		dev_err(&neoispd->pdev->dev, "Neo is busy !\n");

	ret = regmap_field_force_write(neoispd->regs.fields[NEO_PIPE_CONF_INT_STAT0_IDX], irq_clear);
	if (ret) {
		dev_err(&neoispd->pdev->dev,
			"Unable to clear irq ret (%x) it will be disabled to avoid stall\n", ret);
		regmap_field_force_write(neoispd->regs.fields[NEO_PIPE_CONF_INT_EN0_IDX], irq_clear);
	}

	if (done) {
		dev_dbg(&neoispd->pdev->dev, "Neo is entring done irq_clear %x\n", irq_clear);
		for (i = 0; i < NEOISP_NODES_COUNT; i++) {
			if (buf[i]) {
				buf[i]->vb.sequence = node_group->frame_sequence;
				buf[i]->vb.vb2_buf.timestamp = ts;
				vb2_buffer_done(&buf[i]->vb.vb2_buf,
						VB2_BUF_STATE_DONE);
			}
		}
		/* update frame_sequence */
		node_group->frame_sequence++;
		/* check if there's more to do before going to sleep */
		neoisp_schedule_next(neoispd, true);
	}

	return IRQ_HANDLED;
}

static int neoisp_sd_subs_evt(struct v4l2_subdev *sd, struct v4l2_fh *fh,
			       struct v4l2_event_subscription *sub)
{
	if (sub->type != V4L2_EVENT_FRAME_SYNC)
		return -EINVAL;

	return v4l2_event_subscribe(fh, sub, 0, NULL);
}

static const struct v4l2_subdev_core_ops neoisp_sd_core_ops = {
	.subscribe_event = neoisp_sd_subs_evt,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_pad_ops neoisp_sd_pad_ops = {
	.link_validate = v4l2_subdev_link_validate_default,
};

static const struct v4l2_subdev_ops neoisp_sd_ops = {
	.core = &neoisp_sd_core_ops,
	.pad = &neoisp_sd_pad_ops,
};

static int neoisp_init_subdev(struct neoisp_node_group_s *node_group)
{
	struct neoisp_dev_s *neoispd = node_group->neoisp_dev;
	struct v4l2_subdev *sd = &node_group->sd;
	__u32 i;
	int ret;

	v4l2_subdev_init(sd, &neoisp_sd_ops);
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_ISP;
	sd->owner = THIS_MODULE;
	sd->dev = &neoispd->pdev->dev;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE
			| V4L2_SUBDEV_FL_STREAMS
			| V4L2_SUBDEV_FL_HAS_EVENTS;
	strscpy(sd->name, NEOISP_NAME, sizeof(sd->name));

	for (i = 0; i < NEOISP_NODES_COUNT; i++)
		node_group->pad[i].flags =
			NODE_DESC_IS_OUTPUT(&node_desc[i]) ?
			MEDIA_PAD_FL_SINK : MEDIA_PAD_FL_SOURCE;

	ret = media_entity_pads_init(&sd->entity, NEOISP_NODES_COUNT,
			node_group->pad);
	if (ret)
		goto error;

	ret = v4l2_device_register_subdev(&node_group->v4l2_dev, sd);
	if (ret)
		goto error;

	return 0;

error:
	media_entity_cleanup(&sd->entity);
	return ret;
}

static void node_set_default_format(struct neoisp_node_s *node)
{
	if (NODE_IS_META(node) && NODE_IS_OUTPUT(node)) {
		/* params node */
		struct v4l2_format *f = &node->format;

		f->fmt.meta.dataformat = V4L2_META_FMT_NEO_ISP_PARAMS;
		f->fmt.meta.buffersize = sizeof(struct neoisp_meta_params_s);
		f->type = node->buf_type;
	} else if (NODE_IS_META(node) && NODE_IS_CAPTURE(node)) {
		/* stats node */
		struct v4l2_format *f = &node->format;

		f->fmt.meta.dataformat = V4L2_META_FMT_NEO_ISP_STATS;
		f->fmt.meta.buffersize =  sizeof(struct neoisp_meta_stats_s);
		f->type = node->buf_type;
	} else {
		struct v4l2_format f = {0};

		if (NODE_IS_CAPTURE(node))
			f.fmt.pix_mp.pixelformat = formats_vcap[0].fourcc;
		else
			f.fmt.pix_mp.pixelformat = formats_vout[0].fourcc;

		f.fmt.pix_mp.width = NEOISP_DEF_W;
		f.fmt.pix_mp.height = NEOISP_DEF_H;
		f.type = node->buf_type;
		neoisp_try_fmt(&f, node);
		node->format = f;
	}
	node->neoisp_format = neoisp_find_pixel_format(node->format.fmt.pix_mp.pixelformat,
			NEOISP_FMT_VIDEO_OUTPUT | NEOISP_FMT_VIDEO_CAPTURE
			| NEOISP_FMT_META_OUTPUT | NEOISP_FMT_META_CAPTURE);
}

/*
 * Initialise a struct neoisp_node_s and register it as /dev/video<N>
 * to represent one of the neoisp's input or output streams.
 */
static int neoisp_init_node(struct neoisp_node_group_s *node_group, __u32 id)
{
	bool output = NODE_DESC_IS_OUTPUT(&node_desc[id]);
	struct neoisp_node_s *node = &node_group->node[id];
	struct neoisp_dev_s *neoispd = node_group->neoisp_dev;
	struct media_entity *entity = &node->vfd.entity;
	struct video_device *vdev = &node->vfd;
	struct vb2_queue *q = &node->queue;
	int ret;

	node->id = id;
	node->node_group = node_group;
	node->buf_type = node_desc[id].buf_type;

	mutex_init(&node->node_lock);
	mutex_init(&node->queue_lock);
	INIT_LIST_HEAD(&node->ready_queue);
	spin_lock_init(&node->ready_lock);

	node->format.type = node->buf_type;
	node_set_default_format(node);

	q->type = node->buf_type;
	q->io_modes = VB2_MMAP | VB2_DMABUF;
	q->mem_ops = &vb2_dma_contig_memops;
	q->drv_priv = node;
	q->ops = &neoisp_node_queue_ops;
	q->buf_struct_size = sizeof(struct neoisp_buffer_s);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->dev = &neoispd->pdev->dev;
	/* get V4L2 to handle node->queue locking */
	q->lock = &node->queue_lock;

	ret = vb2_queue_init(q);
	if (ret < 0) {
		dev_err(&neoispd->pdev->dev, "vb2_queue_init failed\n");
		return ret;
	}

	*vdev = neoisp_videodev; /* default initialization */
	strscpy(vdev->name, node_desc[id].ent_name, sizeof(vdev->name));
	vdev->v4l2_dev = &node_group->v4l2_dev;
	vdev->vfl_dir = output ? VFL_DIR_TX : VFL_DIR_RX;
	/* get V4L2 to serialise our ioctls */
	vdev->lock = &node->node_lock;
	vdev->queue = &node->queue;
	vdev->device_caps = V4L2_CAP_STREAMING | node_desc[id].caps;

	node->pad.flags = output ? MEDIA_PAD_FL_SOURCE : MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(entity, 1, &node->pad);
	if (ret) {
		dev_err(&neoispd->pdev->dev,
				"Failed to register media pads for %s device node\n",
				NODE_NAME(node));
		goto err_unregister_queue;
	}

	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
	if (ret) {
		dev_err(&neoispd->pdev->dev,
				"Failed to register video %s device node\n",
				NODE_NAME(node));
		goto err_unregister_queue;
	}
	video_set_drvdata(vdev, node);

	if (output)
		ret = media_create_pad_link(entity, 0, &node_group->sd.entity,
				id, MEDIA_LNK_FL_IMMUTABLE |
				MEDIA_LNK_FL_ENABLED);
	else
		ret = media_create_pad_link(&node_group->sd.entity, id, entity,
				0, MEDIA_LNK_FL_IMMUTABLE |
				MEDIA_LNK_FL_ENABLED);
	if (ret)
		goto err_unregister_video_dev;

	dev_dbg(&neoispd->pdev->dev,
			"%s device node registered as /dev/video%d\n",
			NODE_NAME(node), node->vfd.num);

	return 0;

err_unregister_video_dev:
	video_unregister_device(&node->vfd);
err_unregister_queue:
	vb2_queue_release(&node->queue);
	return ret;
}
static int neoisp_init_group(struct neoisp_dev_s *neoispd, __u32 id)
{
	struct neoisp_node_group_s *node_group = &neoispd->node_group[id];
	struct v4l2_device *v4l2_dev;
	struct media_device *mdev;
	__u32 num_registered = 0;
	int ret;

	node_group->id = id;
	node_group->neoisp_dev = neoispd;
	node_group->streaming_map = 0;

	dev_dbg(&neoispd->pdev->dev, "Register nodes for group %u\n", id);

	/* Register v4l2_device and media_device */
	mdev = &node_group->mdev;
	mdev->dev = &neoispd->pdev->dev;
	strscpy(mdev->model, NEOISP_NAME, sizeof(mdev->model));
	snprintf(mdev->bus_info, sizeof(mdev->bus_info),
			"platform:%s", dev_name(&neoispd->pdev->dev));
	media_device_init(mdev);

	v4l2_dev = &node_group->v4l2_dev;
	v4l2_dev->mdev = &node_group->mdev;
	strscpy(v4l2_dev->name, NEOISP_NAME, sizeof(v4l2_dev->name));

	ret = v4l2_device_register(mdev->dev, &node_group->v4l2_dev);
	if (ret)
		goto err_media_dev_cleanup;

	/* Register the NEOISP subdevice. */
	ret = neoisp_init_subdev(node_group);
	if (ret)
		goto err_unregister_v4l2;

	/* Create device video nodes */
	for (; num_registered < NEOISP_NODES_COUNT; num_registered++) {
		ret = neoisp_init_node(node_group, num_registered);
		if (ret)
			goto err_unregister_nodes;
	}

	ret = media_device_register(mdev);
	if (ret)
		goto err_unregister_nodes;

	ret = v4l2_device_register_subdev_nodes(&node_group->v4l2_dev);
	if (ret)
		goto err_unregister_nodes;

	node_group->params =
		dma_alloc_coherent(mdev->dev,
				sizeof(struct neoisp_meta_params_s) * VB2_MAX_FRAME,
				&node_group->params_dma_addr, GFP_KERNEL);
	if (!node_group->params) {
		dev_err(mdev->dev, "Unable to allocate cached params buffers.\n");
		ret = -ENOMEM;
		goto err_unregister_mdev;
	}
	return 0;

err_unregister_mdev:
	media_device_unregister(mdev);
err_unregister_nodes:
	while (num_registered-- > 0) {
		video_unregister_device(&node_group->node[num_registered].vfd);
		vb2_queue_release(&node_group->node[num_registered].queue);
	}
	v4l2_device_unregister_subdev(&node_group->sd);
	media_entity_cleanup(&node_group->sd.entity);
err_unregister_v4l2:
	v4l2_device_unregister(v4l2_dev);
err_media_dev_cleanup:
	media_device_cleanup(mdev);
	return ret;
}

static void neoisp_destroy_node_group(struct neoisp_node_group_s *node_group)
{
	struct neoisp_dev_s *neoispd = node_group->neoisp_dev;
	int i;

	if (node_group->params) {
		dma_free_coherent(&neoispd->pdev->dev,
				sizeof(struct neoisp_meta_params_s) * VB2_MAX_FRAME,
				node_group->params,
				node_group->params_dma_addr);
	}

	dev_dbg(&neoispd->pdev->dev, "Unregister from media controller\n");

	v4l2_device_unregister_subdev(&node_group->sd);
	media_entity_cleanup(&node_group->sd.entity);
	media_device_unregister(&node_group->mdev);

	for (i = NEOISP_NODES_COUNT - 1; i >= 0; i--) {
		video_unregister_device(&node_group->node[i].vfd);
		vb2_queue_release(&node_group->node[i].queue);
	}

	media_device_cleanup(&node_group->mdev);
	v4l2_device_unregister(&node_group->v4l2_dev);
}

static int neoisp_init_ctx(void)
{
	int i = 0;
	__u16 *ptr = neoisp_default_params.mems.gtm.drc_global_tonemap;

	/* Fill default global tonemap lut with 1.0 value (256) */
	for (; i < NEO_DRC_GLOBAL_TONEMAP_SIZE; i++)
		ptr[i] = 1 << 8;

	return 0;
}

static int neoisp_init_hw(struct neoisp_dev_s *neoispd)
{
	int ret;

	neoisp_reset_hw(neoispd, false);
	neoisp_reset_hw(neoispd, true);

	/* disable bus error if eDMA transfer is used */
	regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_REG_XFR_DIS_IDX],
			NEO_PIPE_CONF_REG_XFR_DIS_XFR_ERR_DIS);

	// disable debug
	ret = regmap_field_write(neoispd->regs.fields[NEO_IDBG1_LINE_NUM_IDX],
			NEO_IDBG1_LINE_NUM_LINE_NUM_MASK);
	ret += regmap_field_write(neoispd->regs.fields[NEO_IDBG2_LINE_NUM_IDX],
			NEO_IDBG2_LINE_NUM_LINE_NUM_MASK);

	ret += regmap_field_write(neoispd->regs.fields[NEO_PIPE_CONF_INT_EN0_IDX],
			NEO_PIPE_CONF_INT_EN0_EN_FS1
			| NEO_PIPE_CONF_INT_EN0_EN_FS2
			| NEO_PIPE_CONF_INT_EN0_EN_FD1
			| NEO_PIPE_CONF_INT_EN0_EN_FD2
			| NEO_PIPE_CONF_INT_EN0_EN_STATD
			| NEO_PIPE_CONF_INT_EN0_EN_DRCD
			| NEO_PIPE_CONF_INT_EN0_EN_BUS_ERR_MASK
			| NEO_PIPE_CONF_INT_EN0_EN_CSI_TERR
			| NEO_PIPE_CONF_INT_EN0_EN_TRIG_ERR);

	return ret;
}

static int neoisp_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct neoisp_dev_s *neoisp_dev;
	int num_groups, ret, irq;

	neoisp_dev = devm_kzalloc(dev, sizeof(*neoisp_dev), GFP_KERNEL);
	if (!neoisp_dev)
		return -ENOMEM;
	neoisp_dev->pdev = pdev;

	ret = devm_clk_bulk_get_all(dev, &neoisp_dev->clks);
	if (ret < 0) {
		dev_err(dev, "Unable to get clocks: %d\n", ret);
		return ret;
	}
	neoisp_dev->num_clks = ret;

	/* get regs address */
	neoisp_dev->mmio = devm_platform_get_and_ioremap_resource(pdev, 0, NULL);
	if (IS_ERR(neoisp_dev->mmio))
		return PTR_ERR(neoisp_dev->mmio);
	/* get internal isp memory address */
	neoisp_dev->mmio_tcm = devm_platform_get_and_ioremap_resource(pdev, 1, NULL);

	neoisp_dev->regmap = devm_regmap_init_mmio(dev, neoisp_dev->mmio, &neoisp_regmap_config);
	if (IS_ERR(neoisp_dev->regmap)) {
		dev_err(dev, "regmap init failed\n");
		return PTR_ERR(neoisp_dev->regmap);
	}

	if (neoisp_regfield_alloc(dev, neoisp_dev)) {
		dev_err(dev, "reg field alloc failed\n");
		return -ENODEV;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	platform_set_drvdata(pdev, neoisp_dev);

	pm_runtime_enable(&pdev->dev);
	ret = pm_runtime_resume_and_get(&pdev->dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Unable to resume the device: %d\n", ret);
		goto err_pm;
	}

	ret = devm_request_irq(&pdev->dev, irq, neoisp_irq_handler, IRQF_ONESHOT,
			dev_name(&pdev->dev), neoisp_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to request irq: %d\n", ret);
		goto err_pm;
	}

	/*
	 * Initialise and register devices for each node_group, including media
	 * device
	 */
	for (num_groups = 0; num_groups < NEOISP_NODE_GROUPS_COUNT; num_groups++) {
		ret = neoisp_init_group(neoisp_dev, num_groups);
		if (ret)
			goto disable_nodes_err;
	}

	ret = neoisp_init_hw(neoisp_dev);
	if (ret)
		goto disable_nodes_err;

	ret = neoisp_init_ctx();
	if (ret)
		goto disable_nodes_err;

	dev_info(&pdev->dev, "probe: done (%d)\n", ret);
	return ret;

disable_nodes_err:
	while (num_groups-- > 0)
		neoisp_destroy_node_group(&neoisp_dev->node_group[num_groups]);
err_pm:
	pm_runtime_disable(&pdev->dev);

	dev_err(&pdev->dev, "probe: error %d\n", ret);
	return ret;
}

static int neoisp_remove(struct platform_device *pdev)
{
	struct neoisp_dev_s *neoisp_dev = platform_get_drvdata(pdev);
	int i;

	for (i = NEOISP_NODE_GROUPS_COUNT - 1; i >= 0; i--)
		neoisp_destroy_node_group(&neoisp_dev->node_group[i]);

	pm_runtime_disable(&pdev->dev);

	return 0;
}

static int __maybe_unused neoisp_runtime_suspend(struct device *dev)
{
	struct neoisp_dev_s *neoisp_dev = dev_get_drvdata(dev);

	clk_bulk_disable_unprepare(neoisp_dev->num_clks, neoisp_dev->clks);

	return 0;
}

static int __maybe_unused neoisp_runtime_resume(struct device *dev)
{
	int ret;
	struct neoisp_dev_s *neoisp_dev = dev_get_drvdata(dev);

	ret = clk_bulk_prepare_enable(neoisp_dev->num_clks, neoisp_dev->clks);

	if (ret)
		return ret;

	return 0;
}

static const struct dev_pm_ops neoisp_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(pm_runtime_force_suspend,
			pm_runtime_force_resume)
		SET_RUNTIME_PM_OPS(neoisp_runtime_suspend,
				neoisp_runtime_resume, NULL)
};

static const struct of_device_id neoisp_dt_ids[] = {
	{ .compatible = "nxp,neoisp", .data = NULL },
	{ },
};
MODULE_DEVICE_TABLE(of, neoisp_dt_ids);

static struct platform_driver neoisp_driver = {
	.probe  = neoisp_probe,
	.remove = neoisp_remove,
	.driver = {
		.name = NEOISP_NAME,
		.pm = &neoisp_pm,
		.of_match_table = neoisp_dt_ids,
	},
};

module_platform_driver(neoisp_driver);

MODULE_DESCRIPTION("NXP NEOISP Hardware");
MODULE_AUTHOR("Aymen SGHAIER <Aymen.Sghaier@nxp.com>");
MODULE_LICENSE("GPL");
