/*
** Copyright (C) 2024 Fengxiao Tech wwww.lcddisplay.co  All rights reserved.
** Kernel DRM driver for fx8inchwxga LCD Panel in DSI interface.
** Driver IC: JD9365
*/

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <video/mipi_display.h>

struct fx8inchwxga_panel_desc {
  const struct drm_display_mode *mode;
  unsigned int lanes;
  unsigned long flags;
  enum mipi_dsi_pixel_format format;
};

struct fx8inchwxga {
  struct drm_panel panel;
  struct mipi_dsi_device *dsi;
  const struct fx8inchwxga_panel_desc *desc;
  struct gpio_desc *reset;
};

static inline struct fx8inchwxga *panel_to_fx8inchwxga(struct drm_panel *panel) {
  return container_of(panel, struct fx8inchwxga, panel);
}

static inline int fx8inchwxga_dsi_write(struct fx8inchwxga *fx8inchwxga,
                                       const void *seq, size_t len) {
  return mipi_dsi_dcs_write_buffer(fx8inchwxga->dsi, seq, len);
}

#define fx8inchwxga_command(fx8inchwxga, seq...)          \
  {                                                     \
    const uint8_t d[] = {seq};                          \
    fx8inchwxga_dsi_write(fx8inchwxga, d, ARRAY_SIZE(d)); \
  }

static void fx8inchwxga_init_sequence(struct fx8inchwxga *fx8inchwxga) {
  fx8inchwxga_command(fx8inchwxga, 0xE0, 0x00);

  fx8inchwxga_command(fx8inchwxga, 0xE1, 0x93);
  fx8inchwxga_command(fx8inchwxga, 0xE2, 0x65);
  fx8inchwxga_command(fx8inchwxga, 0xE3, 0xF8);
  fx8inchwxga_command(fx8inchwxga, 0x80, 0x03);

  fx8inchwxga_command(fx8inchwxga, 0xE0, 0x01);
  fx8inchwxga_command(fx8inchwxga, 0x00, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x01, 0x72);
  fx8inchwxga_command(fx8inchwxga, 0x03, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x04, 0x80);


  fx8inchwxga_command(fx8inchwxga, 0x17, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x18, 0xAF);
  fx8inchwxga_command(fx8inchwxga, 0x19, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x1A, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x1B, 0xAF);
  fx8inchwxga_command(fx8inchwxga, 0x1C, 0x00);

  fx8inchwxga_command(fx8inchwxga, 0x24, 0xFE);

  fx8inchwxga_command(fx8inchwxga, 0x37, 0x19);
  fx8inchwxga_command(fx8inchwxga, 0x38, 0x05);
  fx8inchwxga_command(fx8inchwxga, 0x39, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x3A, 0x01);
  fx8inchwxga_command(fx8inchwxga, 0x3B, 0x01);
  fx8inchwxga_command(fx8inchwxga, 0x3C, 0x70);
  fx8inchwxga_command(fx8inchwxga, 0x3D, 0xFF);
  fx8inchwxga_command(fx8inchwxga, 0x3E, 0xFF);
  fx8inchwxga_command(fx8inchwxga, 0x3F, 0xFF);

  fx8inchwxga_command(fx8inchwxga, 0x40, 0x06);
  fx8inchwxga_command(fx8inchwxga, 0x41, 0xA0);
  fx8inchwxga_command(fx8inchwxga, 0x43, 0x1E);
  fx8inchwxga_command(fx8inchwxga, 0x44, 0x10);
  fx8inchwxga_command(fx8inchwxga, 0x45, 0x28);
  fx8inchwxga_command(fx8inchwxga, 0x4B, 0x04);

  fx8inchwxga_command(fx8inchwxga, 0x55, 0x02);
  fx8inchwxga_command(fx8inchwxga, 0x56, 0x01);
  fx8inchwxga_command(fx8inchwxga, 0x57, 0xA9);

  fx8inchwxga_command(fx8inchwxga, 0x58, 0x0A);
  fx8inchwxga_command(fx8inchwxga, 0x59, 0x0A);
  fx8inchwxga_command(fx8inchwxga, 0x5A, 0x37);
  fx8inchwxga_command(fx8inchwxga, 0x5B, 0x1A);

  fx8inchwxga_command(fx8inchwxga, 0x5D, 0x7F);
  fx8inchwxga_command(fx8inchwxga, 0x5E, 0x6A);
  fx8inchwxga_command(fx8inchwxga, 0x5F, 0x5B);
  fx8inchwxga_command(fx8inchwxga, 0x60, 0x50);
  fx8inchwxga_command(fx8inchwxga, 0x61, 0x4D);
  fx8inchwxga_command(fx8inchwxga, 0x62, 0x3F);
  fx8inchwxga_command(fx8inchwxga, 0x63, 0x44);
  fx8inchwxga_command(fx8inchwxga, 0x64, 0x2E);
  fx8inchwxga_command(fx8inchwxga, 0x65, 0x49);
  fx8inchwxga_command(fx8inchwxga, 0x66, 0x48);
  fx8inchwxga_command(fx8inchwxga, 0x67, 0x48);
  fx8inchwxga_command(fx8inchwxga, 0x68, 0x66);
  fx8inchwxga_command(fx8inchwxga, 0x69, 0x54);
  fx8inchwxga_command(fx8inchwxga, 0x6A, 0x5A);
  fx8inchwxga_command(fx8inchwxga, 0x6B, 0x4C);
  fx8inchwxga_command(fx8inchwxga, 0x6C, 0x44);
  fx8inchwxga_command(fx8inchwxga, 0x6D, 0x37);
  fx8inchwxga_command(fx8inchwxga, 0x6E, 0x23);
  fx8inchwxga_command(fx8inchwxga, 0x6F, 0x10);
  fx8inchwxga_command(fx8inchwxga, 0x70, 0x7F);
  fx8inchwxga_command(fx8inchwxga, 0x71, 0x6A);
  fx8inchwxga_command(fx8inchwxga, 0x72, 0x5B);
  fx8inchwxga_command(fx8inchwxga, 0x73, 0x50);
  fx8inchwxga_command(fx8inchwxga, 0x74, 0x4D);
  fx8inchwxga_command(fx8inchwxga, 0x75, 0x3F);
  fx8inchwxga_command(fx8inchwxga, 0x76, 0x44);
  fx8inchwxga_command(fx8inchwxga, 0x77, 0x2E);
  fx8inchwxga_command(fx8inchwxga, 0x78, 0x49);
  fx8inchwxga_command(fx8inchwxga, 0x79, 0x48);
  fx8inchwxga_command(fx8inchwxga, 0x7A, 0x48);
  fx8inchwxga_command(fx8inchwxga, 0x7B, 0x66);
  fx8inchwxga_command(fx8inchwxga, 0x7C, 0x54);
  fx8inchwxga_command(fx8inchwxga, 0x7D, 0x5A);
  fx8inchwxga_command(fx8inchwxga, 0x7E, 0x4C);
  fx8inchwxga_command(fx8inchwxga, 0x7F, 0x44);
  fx8inchwxga_command(fx8inchwxga, 0x80, 0x37);
  fx8inchwxga_command(fx8inchwxga, 0x81, 0x23);
  fx8inchwxga_command(fx8inchwxga, 0x82, 0x10);

  fx8inchwxga_command(fx8inchwxga, 0xE0, 0x02);
  fx8inchwxga_command(fx8inchwxga, 0x00, 0x4B);
  fx8inchwxga_command(fx8inchwxga, 0x01, 0x4B);
  fx8inchwxga_command(fx8inchwxga, 0x02, 0x49);
  fx8inchwxga_command(fx8inchwxga, 0x03, 0x49);
  fx8inchwxga_command(fx8inchwxga, 0x04, 0x47);
  fx8inchwxga_command(fx8inchwxga, 0x05, 0x47);
  fx8inchwxga_command(fx8inchwxga, 0x06, 0x45);
  fx8inchwxga_command(fx8inchwxga, 0x07, 0x45);
  fx8inchwxga_command(fx8inchwxga, 0x08, 0x41);
  fx8inchwxga_command(fx8inchwxga, 0x09, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x0A, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x0B, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x0C, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x0D, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x0E, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x0F, 0x5F);
  fx8inchwxga_command(fx8inchwxga, 0x10, 0x5F);
  fx8inchwxga_command(fx8inchwxga, 0x11, 0x57);
  fx8inchwxga_command(fx8inchwxga, 0x12, 0x77);
  fx8inchwxga_command(fx8inchwxga, 0x13, 0x35);
  fx8inchwxga_command(fx8inchwxga, 0x14, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x15, 0x1F);

  fx8inchwxga_command(fx8inchwxga, 0x16, 0x4A);
  fx8inchwxga_command(fx8inchwxga, 0x17, 0x4A);
  fx8inchwxga_command(fx8inchwxga, 0x18, 0x48);
  fx8inchwxga_command(fx8inchwxga, 0x19, 0x48);
  fx8inchwxga_command(fx8inchwxga, 0x1A, 0x46);
  fx8inchwxga_command(fx8inchwxga, 0x1B, 0x46);
  fx8inchwxga_command(fx8inchwxga, 0x1C, 0x44);
  fx8inchwxga_command(fx8inchwxga, 0x1D, 0x44);
  fx8inchwxga_command(fx8inchwxga, 0x1E, 0x40);
  fx8inchwxga_command(fx8inchwxga, 0x1F, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x20, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x21, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x22, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x23, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x24, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x25, 0x5F);
  fx8inchwxga_command(fx8inchwxga, 0x26, 0x5F);
  fx8inchwxga_command(fx8inchwxga, 0x27, 0x57);
  fx8inchwxga_command(fx8inchwxga, 0x28, 0x77);
  fx8inchwxga_command(fx8inchwxga, 0x29, 0x35);
  fx8inchwxga_command(fx8inchwxga, 0x2A, 0x1F);
  fx8inchwxga_command(fx8inchwxga, 0x2B, 0x1F);



  fx8inchwxga_command(fx8inchwxga, 0x58, 0x40);
  fx8inchwxga_command(fx8inchwxga, 0x59, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x5A, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x5B, 0x10);
  fx8inchwxga_command(fx8inchwxga, 0x5C, 0x02);
  fx8inchwxga_command(fx8inchwxga, 0x5D, 0x40);
  fx8inchwxga_command(fx8inchwxga, 0x5E, 0x01);
  fx8inchwxga_command(fx8inchwxga, 0x5F, 0x02);
  fx8inchwxga_command(fx8inchwxga, 0x60, 0x30);
  fx8inchwxga_command(fx8inchwxga, 0x61, 0x01);
  fx8inchwxga_command(fx8inchwxga, 0x62, 0x02);
  fx8inchwxga_command(fx8inchwxga, 0x63, 0x03);
  fx8inchwxga_command(fx8inchwxga, 0x64, 0x6B);
  fx8inchwxga_command(fx8inchwxga, 0x65, 0x05);
  fx8inchwxga_command(fx8inchwxga, 0x66, 0x0C);
  fx8inchwxga_command(fx8inchwxga, 0x67, 0x73);
  fx8inchwxga_command(fx8inchwxga, 0x68, 0x06);
  fx8inchwxga_command(fx8inchwxga, 0x69, 0x03);
  fx8inchwxga_command(fx8inchwxga, 0x6A, 0x56);
  fx8inchwxga_command(fx8inchwxga, 0x6B, 0x08);
  fx8inchwxga_command(fx8inchwxga, 0x6C, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x6D, 0x04);
  fx8inchwxga_command(fx8inchwxga, 0x6E, 0x04);
  fx8inchwxga_command(fx8inchwxga, 0x6F, 0x88);
  fx8inchwxga_command(fx8inchwxga, 0x70, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x71, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x72, 0x06);
  fx8inchwxga_command(fx8inchwxga, 0x73, 0x7B);
  fx8inchwxga_command(fx8inchwxga, 0x74, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x75, 0xF8);
  fx8inchwxga_command(fx8inchwxga, 0x76, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x77, 0xD5);
  fx8inchwxga_command(fx8inchwxga, 0x78, 0x2E);
  fx8inchwxga_command(fx8inchwxga, 0x79, 0x12);
  fx8inchwxga_command(fx8inchwxga, 0x7A, 0x03);
  fx8inchwxga_command(fx8inchwxga, 0x7B, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x7C, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x7D, 0x03);
  fx8inchwxga_command(fx8inchwxga, 0x7E, 0x7B);


  fx8inchwxga_command(fx8inchwxga, 0xE0, 0x04);
  fx8inchwxga_command(fx8inchwxga, 0x00, 0x0E);
  fx8inchwxga_command(fx8inchwxga, 0x02, 0xB3);
  fx8inchwxga_command(fx8inchwxga, 0x09, 0x60);
  fx8inchwxga_command(fx8inchwxga, 0x0E, 0x2A);
  fx8inchwxga_command(fx8inchwxga, 0x36, 0x59);


  fx8inchwxga_command(fx8inchwxga, 0xE0, 0x00);
  fx8inchwxga_command(fx8inchwxga, 0x51, 0x80);
  fx8inchwxga_command(fx8inchwxga, 0x53, 0x2C);
  fx8inchwxga_command(fx8inchwxga, 0x55, 0x00);
;
}

static int fx8inchwxga_prepare(struct drm_panel *panel) {
  struct fx8inchwxga *fx8inchwxga = panel_to_fx8inchwxga(panel);
  gpiod_set_value(fx8inchwxga->reset, 0);

  msleep(50);
  gpiod_set_value(fx8inchwxga->reset, 1);
  msleep(150);
  mipi_dsi_dcs_soft_reset(fx8inchwxga->dsi);

  msleep(5);

  fx8inchwxga_init_sequence(fx8inchwxga);

  mipi_dsi_dcs_set_tear_on(fx8inchwxga->dsi, MIPI_DSI_DCS_TEAR_MODE_VBLANK);
  mipi_dsi_dcs_exit_sleep_mode(fx8inchwxga->dsi);
  return 0;
}

static int fx8inchwxga_enable(struct drm_panel *panel) {
  return mipi_dsi_dcs_set_display_on(panel_to_fx8inchwxga(panel)->dsi);
}

static int fx8inchwxga_disable(struct drm_panel *panel) {
  return mipi_dsi_dcs_set_display_off(panel_to_fx8inchwxga(panel)->dsi);
}

static int fx8inchwxga_unprepare(struct drm_panel *panel) {
  struct fx8inchwxga *fx8inchwxga = panel_to_fx8inchwxga(panel);

  mipi_dsi_dcs_enter_sleep_mode(fx8inchwxga->dsi);

  gpiod_set_value(fx8inchwxga->reset, 0);

  return 0;
}

static int fx8inchwxga_get_modes(struct drm_panel *panel,
                                struct drm_connector *connector) {
  struct fx8inchwxga *fx8inchwxga = panel_to_fx8inchwxga(panel);
  const struct drm_display_mode *desc_mode = fx8inchwxga->desc->mode;
  struct drm_display_mode *mode;

  mode = drm_mode_duplicate(connector->dev, desc_mode);
  if (!mode) {
    dev_err(&fx8inchwxga->dsi->dev, "failed to add mode %ux%u@%u\n",
            desc_mode->hdisplay, desc_mode->vdisplay,
            drm_mode_vrefresh(desc_mode));
    return -ENOMEM;
  }

  drm_mode_set_name(mode);
  drm_mode_probed_add(connector, mode);

  connector->display_info.width_mm = desc_mode->width_mm;
  connector->display_info.height_mm = desc_mode->height_mm;

  return 1;
}

static const struct drm_panel_funcs fx8inchwxga_funcs = {
    .disable = fx8inchwxga_disable,
    .unprepare = fx8inchwxga_unprepare,
    .prepare = fx8inchwxga_prepare,
    .enable = fx8inchwxga_enable,
    .get_modes = fx8inchwxga_get_modes,
};

static const struct drm_display_mode fx8inchwxga_mode = {
    .clock = 25000,

    .hdisplay = 800,
    .hsync_start = 800 + /* HFP */ 32,
    .hsync_end = 800 + 32 + /* HSync */ 20,
    .htotal = 800 + 32 + 20 + /* HBP */ 20,

    .vdisplay = 1280,
    .vsync_start = 1280 + /* VFP */ 8,
    .vsync_end = 640 + 8 + /* VSync */ 4,
    .vtotal = 640 + 8 + 4 + /* VBP */ 4,

    .width_mm = 107.6,
    .height_mm = 172.2,

    .type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static const struct fx8inchwxga_panel_desc fx8inchwxga_desc = {
    .mode = &fx8inchwxga_mode,
    .lanes = 4,
    .flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST,
    .format = MIPI_DSI_FMT_RGB888,
};

static int fx8inchwxga_dsi_probe(struct mipi_dsi_device *dsi) {
  struct fx8inchwxga *fx8inchwxga =
      devm_kzalloc(&dsi->dev, sizeof(*fx8inchwxga), GFP_KERNEL);
  if (!fx8inchwxga) return -ENOMEM;

  const struct fx8inchwxga_panel_desc *desc =
      of_device_get_match_data(&dsi->dev);
  dsi->mode_flags = desc->flags;
  dsi->format = desc->format;
  dsi->lanes = desc->lanes;

  fx8inchwxga->reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
  if (IS_ERR(fx8inchwxga->reset)) {
    dev_err(&dsi->dev, "Couldn't get our reset GPIO\n");
    return PTR_ERR(fx8inchwxga->reset);
  }

  drm_panel_init(&fx8inchwxga->panel, &dsi->dev, &fx8inchwxga_funcs,
                 DRM_MODE_CONNECTOR_DSI);

  int ret = drm_panel_of_backlight(&fx8inchwxga->panel);
  if (ret) return ret;

  drm_panel_add(&fx8inchwxga->panel);

  mipi_dsi_set_drvdata(dsi, fx8inchwxga);
  fx8inchwxga->dsi = dsi;
  fx8inchwxga->desc = desc;

  return mipi_dsi_attach(dsi);
}

static int fx8inchwxga_dsi_remove(struct mipi_dsi_device *dsi) {
  struct fx8inchwxga *fx8inchwxga = mipi_dsi_get_drvdata(dsi);

  mipi_dsi_detach(dsi);
  drm_panel_remove(&fx8inchwxga->panel);

  return 0;
}

static const struct of_device_id fx8inchwxga_of_match[] = {
    {.compatible = "wlk,fx8inchwxga", .data = &fx8inchwxga_desc}, {}};
MODULE_DEVICE_TABLE(of, fx8inchwxga_of_match);

static struct mipi_dsi_driver fx8inchwxga_dsi_driver = {
    .probe = fx8inchwxga_dsi_probe,
    .remove = fx8inchwxga_dsi_remove,
    .driver =
        {
            .name = "fx8inchwxga",
            .of_match_table = fx8inchwxga_of_match,
        },
};
module_mipi_dsi_driver(fx8inchwxga_dsi_driver);

MODULE_AUTHOR("frank@lcddisplay.co");
MODULE_DESCRIPTION("fx8inchwxga LCD Panel Driver");
MODULE_LICENSE("GPL");

