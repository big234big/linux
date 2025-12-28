/*
** Copyright (C) 2024 Fengxiao Tech wwww.lcddisplay.co  All rights reserved.
** Kernel DRM driver for fx8inchwxga LCD Panel in DSI interface.
** Driver IC: JD9365
*/

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright © 2023 Raspberry Pi Ltd
 *
 * Based on panel-raspberrypi-touchscreen by Broadcom
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/media-bus-format.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_graph.h>
#include <linux/pm.h>

#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>

#define FX_DSI_DRIVER_NAME "fx-ts-dsi"

struct fx_panel {
	struct drm_panel base;
	struct mipi_dsi_device *dsi;
	struct i2c_client *i2c;
	const struct drm_display_mode *mode;
	enum drm_panel_orientation orientation;
};

struct fx_panel_data {
	const struct drm_display_mode *mode;
	int lanes;
	unsigned long mode_flags;
};

/* 自定义显示屏时序 - 根据设备树参数转换 */
static const struct drm_display_mode custom_panel_mode = {
	.clock = 72400,            // clock-frequency = <72400000> (kHz单位)
	.hdisplay = 1280,          // hactive = <1280>
	.hsync_start = 1280 + 72,  // hfront-porch = <72>
	.hsync_end = 1280 + 72 + 10, // hsync-len = <10>
	.htotal = 1280 + 72 + 10 + 78, // hback-porch = <78>
	.vdisplay = 800,           // vactive = <800>
	.vsync_start = 800 + 15,   // vfront-porch = <15>
	.vsync_end = 800 + 15 + 5, // vsync-len = <5>
	.vtotal = 800 + 15 + 5 + 18, // vback-porch = <18>
	.vrefresh = 60,            // 计算刷新率：72400000/(1440*838)≈60.1Hz
	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |  // hsync-active<0, vsync-active<0
		 DRM_MODE_FLAG_NCSYNC |  // de-active = <0>
		 DRM_MODE_FLAG_PCSYNC,   // pixelclk-active =<1>
	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct fx_panel_data custom_panel_data = {
	.mode = &custom_panel_mode,
	.lanes = 4,
	.mode_flags = MIPI_DSI_MODE_VIDEO_HSE | MIPI_DSI_MODE_VIDEO | MIPI_DSI_CLOCK_NON_CONTINUOUS,
};

static struct fx_panel *panel_to_ts(struct drm_panel *panel)
{
	return container_of(panel, struct fx_panel, base);
}

static void fx_panel_i2c_write(struct fx_panel *ts, u8 reg, u8 val)
{
	int ret;

	ret = i2c_smbus_write_byte_data(ts->i2c, reg, val);
	if (ret)
		dev_err(&ts->i2c->dev, "I2C write failed: %d\n", ret);
}

static int fx_panel_disable(struct drm_panel *panel)
{
	struct fx_panel *ts = panel_to_ts(panel);

	fx_panel_i2c_write(ts, 0xad, 0x00);

	return 0;
}

static int fx_panel_unprepare(struct drm_panel *panel)
{
	return 0;
}

static int fx_panel_prepare(struct drm_panel *panel)
{
	return 0;
}

static int fx_panel_enable(struct drm_panel *panel)
{
	struct fx_panel *ts = panel_to_ts(panel);

	fx_panel_i2c_write(ts, 0xad, 0x01);

	return 0;
}

static int fx_panel_get_modes(struct drm_panel *panel,
			      struct drm_connector *connector)
{
	static const u32 bus_format = MEDIA_BUS_FMT_RGB888_1X24;
	struct fx_panel *ts = panel_to_ts(panel);
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, ts->mode);
	if (!mode) {
		dev_err(panel->dev, "failed to add mode %ux%u@%u\n",
			ts->mode->hdisplay,
			ts->mode->vdisplay,
			drm_mode_vrefresh(ts->mode));
		return -ENOMEM;
	}

	mode->type = ts->mode->type;
	drm_mode_set_name(mode);
	drm_mode_probed_add(connector, mode);

	connector->display_info.bpc = 8;
	connector->display_info.width_mm = 154;   // 可根据实际屏幕尺寸调整
	connector->display_info.height_mm = 86;   // 可根据实际屏幕尺寸调整
	drm_display_info_set_bus_formats(&connector->display_info,
					 &bus_format, 1);

	/*
	 * TODO: Remove once all drm drivers call
	 * drm_connector_set_orientation_from_panel()
	 */
	drm_connector_set_panel_orientation(connector, ts->orientation);

	return 1;
}

static enum drm_panel_orientation fx_panel_get_orientation(struct drm_panel *panel)
{
	struct fx_panel *ts = panel_to_ts(panel);

	return ts->orientation;
}

static const struct drm_panel_funcs fx_panel_funcs = {
	.disable = fx_panel_disable,
	.unprepare = fx_panel_unprepare,
	.prepare = fx_panel_prepare,
	.enable = fx_panel_enable,
	.get_modes = fx_panel_get_modes,
	.get_orientation = fx_panel_get_orientation,
};

static int fx_panel_bl_update_status(struct backlight_device *bl)
{
	struct fx_panel *ts = bl_get_data(bl);

	fx_panel_i2c_write(ts, 0xab, 0xff - backlight_get_brightness(bl));
	fx_panel_i2c_write(ts, 0xaa, 0x01);

	return 0;
}

static const struct backlight_ops fx_panel_bl_ops = {
	.update_status = fx_panel_bl_update_status,
};

static struct backlight_device *
fx_panel_create_backlight(struct fx_panel *ts)
{
	struct device *dev = ts->base.dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 255,
		.max_brightness = 255,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, ts,
					      &fx_panel_bl_ops, &props);
}

static int fx_panel_probe(struct i2c_client *i2c)
{
	struct device *dev = &i2c->dev;
	struct fx_panel *ts;
	struct device_node *endpoint, *dsi_host_node;
	struct mipi_dsi_host *host;
	struct mipi_dsi_device_info info = {
		.type = FX_DSI_DRIVER_NAME,
		.channel = 0,
		.node = NULL,
	};
	const struct fx_panel_data *_fx_panel_data;
	int ret;

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	_fx_panel_data = &custom_panel_data;  // 直接使用自定义面板数据

	ts->mode = _fx_panel_data->mode;
	if (!ts->mode)
		return -EINVAL;

	i2c_set_clientdata(i2c, ts);

	ts->i2c = i2c;

	// 初始化I2C命令
	fx_panel_i2c_write(ts, 0xc0, 0x01);
	fx_panel_i2c_write(ts, 0xc2, 0x01);
	fx_panel_i2c_write(ts, 0xac, 0x01);

	ret = of_drm_get_panel_orientation(dev->of_node, &ts->orientation);
	if (ret) {
		dev_err(dev, "%pOF: failed to get orientation %d\n", dev->of_node, ret);
		return ret;
	}

	/* Look up the DSI host.  It needs to probe before we do. */
	endpoint = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (!endpoint)
		return -ENODEV;

	dsi_host_node = of_graph_get_remote_port_parent(endpoint);
	if (!dsi_host_node)
		goto error;

	host = of_find_mipi_dsi_host_by_node(dsi_host_node);
	of_node_put(dsi_host_node);
	if (!host) {
		of_node_put(endpoint);
		return -EPROBE_DEFER;
	}

	info.node = of_graph_get_remote_port(endpoint);
	if (!info.node)
		goto error;

	of_node_put(endpoint);

	ts->dsi = devm_mipi_dsi_device_register_full(dev, host, &info);
	if (IS_ERR(ts->dsi)) {
		dev_err(dev, "DSI device registration failed: %ld\n",
			PTR_ERR(ts->dsi));
		return PTR_ERR(ts->dsi);
	}

	drm_panel_init(&ts->base, dev, &fx_panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	ts->base.backlight = fx_panel_create_backlight(ts);
	if (IS_ERR(ts->base.backlight)) {
		ret = PTR_ERR(ts->base.backlight);
		dev_err(dev, "Failed to create backlight: %d\n", ret);
		return ret;
	}

	/* This appears last, as it's what will unblock the DSI host
	 * driver's component bind function.
	 */
	drm_panel_add(&ts->base);

	ts->dsi->mode_flags = _fx_panel_data->mode_flags;
	ts->dsi->format = MIPI_DSI_FMT_RGB888;
	ts->dsi->lanes = _fx_panel_data->lanes;

	ret = devm_mipi_dsi_attach(dev, ts->dsi);

	if (ret)
		dev_err(dev, "failed to attach dsi to host: %d\n", ret);

	return 0;

error:
	of_node_put(endpoint);
	return -ENODEV;
}

static void fx_panel_remove(struct i2c_client *i2c)
{
	struct fx_panel *ts = i2c_get_clientdata(i2c);

	fx_panel_disable(&ts->base);

	drm_panel_remove(&ts->base);
}

static void fx_panel_shutdown(struct i2c_client *i2c)
{
	struct fx_panel *ts = i2c_get_clientdata(i2c);

	fx_panel_disable(&ts->base);
}

static const struct of_device_id fx_panel_of_ids[] = {
	{
		.compatible = "fx,custom-panel",  // 自定义设备树兼容名
		.data = &custom_panel_data,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, fx_panel_of_ids);

static struct i2c_driver fx_panel_driver = {
	.driver = {
		.name = "fx_touchscreen",  // 驱动名称改为fx_touchscreen
		.of_match_table = fx_panel_of_ids,
	},
	.probe = fx_panel_probe,
	.remove = fx_panel_remove,
	.shutdown = fx_panel_shutdown,
};
module_i2c_driver(fx_panel_driver);

MODULE_AUTHOR("Dave Stevenson <dave.stevenson@raspberrypi.com>");
MODULE_DESCRIPTION("FX DSI panel driver");  // 描述改为FX DSI
MODULE_LICENSE("GPL");
MODULE_LICENSE("GPL");


