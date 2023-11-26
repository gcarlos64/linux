// SPDX-License-Identifier: GPL-2.0
/*
 * Test cases for the drm_framebuffer functions
 *
 * Copyright (c) 2022 Maíra Canal <mairacanal@riseup.net>
 */

#include <kunit/test.h>

#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_mode.h>
#include <drm/drm_file.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_print.h>

#include "../drm_crtc_internal.h"

#define FB_WIDTH  800
#define FB_HEIGHT 600

#define MIN_WIDTH 4
#define MAX_WIDTH 4096
#define MIN_HEIGHT 4
#define MAX_HEIGHT 4096

#define DRM_MODE_FB_INVALID BIT(2)

struct drm_framebuffer_test {
	int buffer_created;
	struct drm_mode_fb_cmd2 cmd;
	const char *name;
};

static const struct drm_framebuffer_test drm_framebuffer_create_cases[] = {
{ .buffer_created = 1, .name = "ABGR8888 normal sizes",
	.cmd = { .width = 600, .height = 600, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * 600, 0, 0 },
	}
},
{ .buffer_created = 1, .name = "ABGR8888 max sizes",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 1, .name = "ABGR8888 pitch greater than min required",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * MAX_WIDTH + 1, 0, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 pitch less than min required",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * MAX_WIDTH - 1, 0, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Invalid width",
	.cmd = { .width = MAX_WIDTH + 1, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * (MAX_WIDTH + 1), 0, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Invalid buffer handle",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 0, 0, 0 }, .pitches = { 4 * MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 0, .name = "No pixel format",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = 0,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Width 0",
	.cmd = { .width = 0, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Height 0",
	.cmd = { .width = MAX_WIDTH, .height = 0, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .pitches = { 4 * MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Out of bound height * pitch combination",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX - 1, 0, 0 },
		 .pitches = { 4 * MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 1, .name = "ABGR8888 Large buffer offset",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX / 2, 0, 0 },
		 .pitches = { 4 * MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Non-zero buffer offset for unused plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX / 2, UINT_MAX / 2, 0 },
		 .pitches = { 4 * MAX_WIDTH, 0, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Invalid flag",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX / 2, 0, 0 },
		 .pitches = { 4 * MAX_WIDTH, 0, 0 }, .flags = DRM_MODE_FB_INVALID,
	}
},
{ .buffer_created = 1, .name = "ABGR8888 Set DRM_MODE_FB_MODIFIERS without modifiers",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX / 2, 0, 0 },
		 .pitches = { 4 * MAX_WIDTH, 0, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
	}
},
{ .buffer_created = 1, .name = "ABGR8888 Valid buffer modifier",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX / 2, 0, 0 },
		 .pitches = { 4 * MAX_WIDTH, 0, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { AFBC_FORMAT_MOD_YTR, 0, 0 },
	}
},
{ .buffer_created = 0,
	.name = "ABGR8888 Invalid buffer modifier(DRM_FORMAT_MOD_SAMSUNG_64_32_TILE)",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX / 2, 0, 0 },
		 .pitches = { 4 * MAX_WIDTH, 0, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, 0, 0 },
	}
},
{ .buffer_created = 1, .name = "ABGR8888 Extra pitches without DRM_MODE_FB_MODIFIERS",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .offsets = { UINT_MAX / 2, 0, 0 },
		 .pitches = { 4 * MAX_WIDTH, 4 * MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 0, .name = "ABGR8888 Extra pitches with DRM_MODE_FB_MODIFIERS",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_ABGR8888,
		 .handles = { 1, 0, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .pitches = { 4 * MAX_WIDTH, 4 * MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 1, .name = "NV12 Normal sizes",
	.cmd = { .width = 600, .height = 600, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .pitches = { 600, 600, 0 },
	}
},
{ .buffer_created = 1, .name = "NV12 Max sizes",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .pitches = { MAX_WIDTH, MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 0, .name = "NV12 Invalid pitch",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .pitches = { MAX_WIDTH, MAX_WIDTH - 1, 0 },
	}
},
{ .buffer_created = 0, .name = "NV12 Invalid modifier/missing DRM_MODE_FB_MODIFIERS flag",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .modifier = { DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, 0, 0 },
		 .pitches = { MAX_WIDTH, MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 0, .name = "NV12 different  modifier per-plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, 0, 0 },
		 .pitches = { MAX_WIDTH, MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 1, .name = "NV12 with DRM_FORMAT_MOD_SAMSUNG_64_32_TILE",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { DRM_FORMAT_MOD_SAMSUNG_64_32_TILE,
			 DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, 0 },
		 .pitches = { MAX_WIDTH, MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 0, .name = "NV12 Valid modifiers without DRM_MODE_FB_MODIFIERS",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .modifier = { DRM_FORMAT_MOD_SAMSUNG_64_32_TILE,
						       DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, 0 },
		 .pitches = { MAX_WIDTH, MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 0, .name = "NV12 Modifier for inexistent plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { DRM_FORMAT_MOD_SAMSUNG_64_32_TILE, DRM_FORMAT_MOD_SAMSUNG_64_32_TILE,
			       DRM_FORMAT_MOD_SAMSUNG_64_32_TILE },
		 .pitches = { MAX_WIDTH, MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 0, .name = "NV12 Handle for inexistent plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 1 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .pitches = { MAX_WIDTH, MAX_WIDTH, 0 },
	}
},
{ .buffer_created = 1, .name = "NV12 Handle for inexistent plane without DRM_MODE_FB_MODIFIERS",
	.cmd = { .width = 600, .height = 600, .pixel_format = DRM_FORMAT_NV12,
		 .handles = { 1, 1, 1 }, .pitches = { 600, 600, 600 },
	}
},
{ .buffer_created = 1, .name = "YVU420 DRM_MODE_FB_MODIFIERS set without modifier",
	.cmd = { .width = 600, .height = 600, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .pitches = { 600, 300, 300 },
	}
},
{ .buffer_created = 1, .name = "YVU420 Normal sizes",
	.cmd = { .width = 600, .height = 600, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .pitches = { 600, 300, 300 },
	}
},
{ .buffer_created = 1, .name = "YVU420 Max sizes",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2),
						      DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 0, .name = "YVU420 Invalid pitch",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2) - 1,
						      DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 1, .name = "YVU420 Different pitches",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2) + 1,
						      DIV_ROUND_UP(MAX_WIDTH, 2) + 7 },
	}
},
{ .buffer_created = 1, .name = "YVU420 Different buffer offsets/pitches",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .offsets = { MAX_WIDTH, MAX_WIDTH  +
			 MAX_WIDTH * MAX_HEIGHT, MAX_WIDTH  + 2 * MAX_WIDTH * MAX_HEIGHT },
		 .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2) + 1,
			 DIV_ROUND_UP(MAX_WIDTH, 2) + 7 },
	}
},
{ .buffer_created = 0,
	.name = "YVU420 Modifier set just for plane 0, without DRM_MODE_FB_MODIFIERS",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .modifier = { AFBC_FORMAT_MOD_SPARSE, 0, 0 },
		 .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2), DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 0,
	.name = "YVU420 Modifier set just for planes 0, 1, without DRM_MODE_FB_MODIFIERS",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 },
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, AFBC_FORMAT_MOD_SPARSE, 0 },
		 .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2), DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 0,
	.name = "YVU420 Modifier set just for plane 0, 1, with DRM_MODE_FB_MODIFIERS",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, AFBC_FORMAT_MOD_SPARSE, 0 },
		 .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2), DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 1, .name = "YVU420 Valid modifier",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, AFBC_FORMAT_MOD_SPARSE,
			 AFBC_FORMAT_MOD_SPARSE },
		 .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2), DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 0, .name = "YVU420 Different modifiers per plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, AFBC_FORMAT_MOD_SPARSE | AFBC_FORMAT_MOD_YTR,
			       AFBC_FORMAT_MOD_SPARSE },
		 .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2), DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 0, .name = "YVU420 Modifier for inexistent plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YVU420,
		 .handles = { 1, 1, 1 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, AFBC_FORMAT_MOD_SPARSE,
			 AFBC_FORMAT_MOD_SPARSE, AFBC_FORMAT_MOD_SPARSE },
		 .pitches = { MAX_WIDTH, DIV_ROUND_UP(MAX_WIDTH, 2), DIV_ROUND_UP(MAX_WIDTH, 2) },
	}
},
{ .buffer_created = 0, .name = "YUV420_10BIT Invalid modifier(DRM_FORMAT_MOD_LINEAR)",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_YUV420_10BIT,
		 .handles = { 1, 0, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .modifier = { DRM_FORMAT_MOD_LINEAR, 0, 0 },
		 .pitches = { MAX_WIDTH, 0, 0 },
	}
},
{ .buffer_created = 1, .name = "X0L2 Normal sizes",
	.cmd = { .width = 600, .height = 600, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 0, 0 }, .pitches = { 1200, 0, 0 }
	}
},
{ .buffer_created = 1, .name = "X0L2 Max sizes",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 0, 0 }, .pitches = { 2 * MAX_WIDTH, 0, 0 }
	}
},
{ .buffer_created = 0, .name = "X0L2 Invalid pitch",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 0, 0 }, .pitches = { 2 * MAX_WIDTH - 1, 0, 0 }
	}
},
{ .buffer_created = 1, .name = "X0L2 Pitch greater than minimum required",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 0, 0 }, .pitches = { 2 * MAX_WIDTH + 1, 0, 0 }
	}
},
{ .buffer_created = 0, .name = "X0L2 Handle for inexistent plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 1, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
		 .pitches = { 2 * MAX_WIDTH + 1, 0, 0 }
	}
},
{ .buffer_created = 1,
	.name = "X0L2 Offset for inexistent plane, without DRM_MODE_FB_MODIFIERS set",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 0, 0 }, .offsets = { 0, 0, 3 },
		 .pitches = { 2 * MAX_WIDTH + 1, 0, 0 }
	}
},
{ .buffer_created = 0, .name = "X0L2 Modifier without DRM_MODE_FB_MODIFIERS set",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 0, 0 }, .pitches = { 2 * MAX_WIDTH + 1, 0, 0 },
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, 0, 0 },
	}
},
{ .buffer_created = 1, .name = "X0L2 Valid modifier",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT, .pixel_format = DRM_FORMAT_X0L2,
		 .handles = { 1, 0, 0 }, .pitches = { 2 * MAX_WIDTH + 1, 0, 0 },
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, 0, 0 }, .flags = DRM_MODE_FB_MODIFIERS,
	}
},
{ .buffer_created = 0, .name = "X0L2 Modifier for inexistent plane",
	.cmd = { .width = MAX_WIDTH, .height = MAX_HEIGHT,
		 .pixel_format = DRM_FORMAT_X0L2, .handles = { 1, 0, 0 },
		 .pitches = { 2 * MAX_WIDTH + 1, 0, 0 },
		 .modifier = { AFBC_FORMAT_MOD_SPARSE, AFBC_FORMAT_MOD_SPARSE, 0 },
		 .flags = DRM_MODE_FB_MODIFIERS,
	}
},
};

/*
 * This struct is intended to provide a way to mocked functions communicate
 * with the outer test when it can't be achieved by using its return value. In
 * this way, the functions that receive the mocked drm_device, for example, can
 * grab a reference to @private and actually return something to be used on some
 * expectation. Also having the @test member allows doing expectations from
 * inside mocked functions.
 */
struct drm_mock {
	struct drm_device dev;
	struct drm_file file_priv;
	struct kunit *test;
	void *private;
};

static struct drm_framebuffer *fb_create_mock(struct drm_device *dev,
					      struct drm_file *file_priv,
					      const struct drm_mode_fb_cmd2 *mode_cmd)
{
	struct drm_mock *mock = container_of(dev, typeof(*mock), dev);
	int *buffer_created = mock->private;
	*buffer_created = 1;
	return ERR_PTR(-EINVAL);
}

static struct drm_mode_config_funcs mock_config_funcs = {
	.fb_create = fb_create_mock,
};

static int drm_framebuffer_test_init(struct kunit *test)
{
	struct drm_mock *mock;
	struct drm_device *dev;
	struct drm_file *file_priv;
	struct drm_driver *driver;

	mock = kunit_kzalloc(test, sizeof(*mock), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mock);
	dev = &mock->dev;
	file_priv = &mock->file_priv;

	driver = kunit_kzalloc(test, sizeof(*dev->driver), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, driver);
	driver->driver_features = DRIVER_MODESET;
	dev->driver = driver;

	idr_init_base(&dev->mode_config.object_idr, 1);
	drm_modeset_lock_init(&dev->mode_config.connection_mutex);
	mutex_init(&dev->mode_config.mutex);
	mutex_init(&dev->mode_config.fb_lock);
	INIT_LIST_HEAD(&dev->mode_config.fb_list);
	INIT_LIST_HEAD(&dev->mode_config.crtc_list);
	INIT_LIST_HEAD(&dev->mode_config.plane_list);
	INIT_LIST_HEAD(&dev->mode_config.privobj_list);
	dev->mode_config.num_fb = 0;
	dev->mode_config.min_width = MIN_WIDTH;
	dev->mode_config.max_width = MAX_WIDTH;
	dev->mode_config.min_height = MIN_HEIGHT;
	dev->mode_config.max_height = MAX_HEIGHT;
	dev->mode_config.funcs = &mock_config_funcs;

	mutex_init(&file_priv->fbs_lock);
	INIT_LIST_HEAD(&file_priv->fbs);

	test->priv = mock;
	return 0;
}

static void drm_framebuffer_test_exit(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	struct drm_file *file_priv = &mock->file_priv;

	mutex_destroy(&dev->mode_config.fb_lock);
	mutex_destroy(&dev->mode_config.mutex);
	mutex_destroy(&file_priv->fbs_lock);
}

static void drm_test_framebuffer_create(struct kunit *test)
{
	const struct drm_framebuffer_test *params = test->param_value;
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	int buffer_created = 0;

	mock->private = &buffer_created;
	drm_internal_framebuffer_create(dev, &params->cmd, NULL);
	KUNIT_EXPECT_EQ(test, params->buffer_created, buffer_created);
}

static void drm_framebuffer_test_to_desc(const struct drm_framebuffer_test *t, char *desc)
{
	strscpy(desc, t->name, KUNIT_PARAM_DESC_SIZE);
}

KUNIT_ARRAY_PARAM(drm_framebuffer_create, drm_framebuffer_create_cases,
		  drm_framebuffer_test_to_desc);

/*
 * This test is very similar to drm_test_framebuffer_create, except that it
 * set mock->mode_config.fb_modifiers_not_supported member to 1, covering
 * the case of trying to create a framebuffer with modifiers without the
 * device really supporting it.
 */
static void drm_test_framebuffer_modifiers_not_supported(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	int buffer_created = 0;

	/* A valid cmd with modifier */
	struct drm_mode_fb_cmd2 cmd = {
		.width = MAX_WIDTH, .height = MAX_HEIGHT,
		.pixel_format = DRM_FORMAT_ABGR8888, .handles = { 1, 0, 0 },
		.offsets = { UINT_MAX / 2, 0, 0 }, .pitches = { 4 * MAX_WIDTH, 0, 0 },
		.flags = DRM_MODE_FB_MODIFIERS,
	};

	mock->private = &buffer_created;
	dev->mode_config.fb_modifiers_not_supported = 1;

	drm_internal_framebuffer_create(dev, &cmd, NULL);
	KUNIT_EXPECT_EQ(test, 0, buffer_created);
}

/* Parameters for testing drm_framebuffer_check_src_coords function */
struct check_src_coords_case {
	const char *name; /* Description of the parameter case */
	const int expect; /* Expected return value by the function */

	/* Deltas to be applied on source */
	const uint32_t dsrc_x;
	const uint32_t dsrc_y;
	const uint32_t dsrc_w;
	const uint32_t dsrc_h;
};

static const struct check_src_coords_case check_src_coords_cases[] = {
	{ .name = "Success: source fits into fb",
	  .expect = 0,
	},
	{ .name = "Fail: overflowing fb with x-axis coordinate",
	  .expect = -ENOSPC, .dsrc_x = 1,
	},
	{ .name = "Fail: overflowing fb with y-axis coordinate",
	  .expect = -ENOSPC, .dsrc_y = 1,
	},
	{ .name = "Fail: overflowing fb with source width",
	  .expect = -ENOSPC, .dsrc_w = 1,
	},
	{ .name = "Fail: overflowing fb with source height",
	  .expect = -ENOSPC, .dsrc_h = 1,
	},
};

static void drm_test_framebuffer_check_src_coords(struct kunit *test)
{
	const struct check_src_coords_case *params = test->param_value;
	const uint32_t src_x = 0 + params->dsrc_x;
	const uint32_t src_y = 0 + params->dsrc_y;
	const uint32_t src_w = (FB_WIDTH << 16) + params->dsrc_w;
	const uint32_t src_h = (FB_HEIGHT << 16) + params->dsrc_h;
	const struct drm_framebuffer fb = {
		.width  = FB_WIDTH,
		.height = FB_HEIGHT,
	};
	int ret;

	ret = drm_framebuffer_check_src_coords(src_x, src_y, src_w, src_h, &fb);
	KUNIT_EXPECT_EQ(test, ret, params->expect);
}

static void check_src_coords_test_to_desc(const struct check_src_coords_case *t,
					  char *desc)
{
	strscpy(desc, t->name, KUNIT_PARAM_DESC_SIZE);
}

KUNIT_ARRAY_PARAM(check_src_coords, check_src_coords_cases,
		  check_src_coords_test_to_desc);

static void drm_test_framebuffer_cleanup(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	struct list_head *fb_list = &dev->mode_config.fb_list;
	struct drm_framebuffer fb1 = { .dev = dev };
	struct drm_framebuffer fb2 = { .dev = dev };

	/* This must result on [fb_list] -> fb1 -> fb2 */
	list_add_tail(&fb1.head, fb_list);
	list_add_tail(&fb2.head, fb_list);
	dev->mode_config.num_fb = 2;

	KUNIT_ASSERT_PTR_EQ(test, fb_list->prev, &fb2.head);
	KUNIT_ASSERT_PTR_EQ(test, fb_list->next, &fb1.head);
	KUNIT_ASSERT_PTR_EQ(test, fb1.head.prev, fb_list);
	KUNIT_ASSERT_PTR_EQ(test, fb1.head.next, &fb2.head);
	KUNIT_ASSERT_PTR_EQ(test, fb2.head.prev, &fb1.head);
	KUNIT_ASSERT_PTR_EQ(test, fb2.head.next, fb_list);

	drm_framebuffer_cleanup(&fb1);

	/* Now [fb_list] -> fb2 */
	KUNIT_ASSERT_PTR_EQ(test, fb_list->prev, &fb2.head);
	KUNIT_ASSERT_PTR_EQ(test, fb_list->next, &fb2.head);
	KUNIT_ASSERT_PTR_EQ(test, fb2.head.prev, fb_list);
	KUNIT_ASSERT_PTR_EQ(test, fb2.head.next, fb_list);
	KUNIT_ASSERT_EQ(test, dev->mode_config.num_fb, 1);

	drm_framebuffer_cleanup(&fb2);

	/* Now fb_list is empty */
	KUNIT_ASSERT_TRUE(test, list_empty(fb_list));
	KUNIT_ASSERT_EQ(test, dev->mode_config.num_fb, 0);
}

static void drm_test_framebuffer_lookup(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	struct drm_framebuffer fb1 = { };
	struct drm_framebuffer *fb2;
	uint32_t id = 0;
	int ret;

	ret = drm_mode_object_add(dev, &fb1.base, DRM_MODE_OBJECT_FB);
	KUNIT_ASSERT_EQ(test, ret, 0);
	id = fb1.base.id;

	/* Looking for fb1 */
	fb2 = drm_framebuffer_lookup(dev, NULL, id);
	KUNIT_EXPECT_PTR_EQ(test, fb2, &fb1);

	/* Looking for an inexistent framebuffer */
	fb2 = drm_framebuffer_lookup(dev, NULL, id + 1);
	KUNIT_EXPECT_NULL(test, fb2);
}

static void drm_test_framebuffer_init(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	struct drm_device wrong_drm = { };
	struct drm_format_info format = { };
	struct drm_framebuffer fb1 = { .dev = dev, .format = &format };
	struct drm_framebuffer *fb2;
	struct drm_framebuffer_funcs funcs = { };
	int ret;

	/* Fails if fb->dev doesn't point to the drm_device passed on first arg */
	fb1.dev = &wrong_drm;
	ret = drm_framebuffer_init(dev, &fb1, &funcs);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	fb1.dev = dev;

	/* Fails if fb.format isn't set */
	fb1.format = NULL;
	ret = drm_framebuffer_init(dev, &fb1, &funcs);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	fb1.format = &format;

	ret = drm_framebuffer_init(dev, &fb1, &funcs);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/*
	 * Check if fb->funcs is actually set to the drm_framebuffer_funcs
	 * passed to it
	 */
	KUNIT_EXPECT_PTR_EQ(test, fb1.funcs, &funcs);

	/* The fb->comm must be set to the current running process */
	KUNIT_EXPECT_STREQ(test, fb1.comm, current->comm);

	/* The fb->base must be successfully initialized */
	KUNIT_EXPECT_EQ(test, fb1.base.id, 1);
	KUNIT_EXPECT_EQ(test, fb1.base.type, DRM_MODE_OBJECT_FB);
	KUNIT_EXPECT_EQ(test, kref_read(&fb1.base.refcount), 1);
	KUNIT_EXPECT_PTR_EQ(test, fb1.base.free_cb, &drm_framebuffer_free);

	/* Checks if the fb is really published and findable */
	fb2 = drm_framebuffer_lookup(dev, NULL, fb1.base.id);
	KUNIT_EXPECT_PTR_EQ(test, fb2, &fb1);

	/* There must be just that one fb initialized */
	KUNIT_EXPECT_EQ(test, dev->mode_config.num_fb, 1);
	KUNIT_EXPECT_PTR_EQ(test, dev->mode_config.fb_list.prev, &fb1.head);
	KUNIT_EXPECT_PTR_EQ(test, dev->mode_config.fb_list.next, &fb1.head);
}

static void destroy_free_mock(struct drm_framebuffer *fb)
{
	struct drm_mock *mock = container_of(fb->dev, typeof(*mock), dev);
	int *buffer_freed = mock->private;
	*buffer_freed = 1;
}

static struct drm_framebuffer_funcs framebuffer_funcs_free_mock = {
	.destroy = destroy_free_mock,
};

static void drm_test_framebuffer_free(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	struct drm_mode_object *obj;
	struct drm_framebuffer fb = {
		.dev = dev,
		.funcs = &framebuffer_funcs_free_mock,
	};
	int buffer_freed = 0;
	int id, ret;

	mock->private = &buffer_freed;

	/*
	 * Case where the fb isn't registered. Just test if
	 * drm_framebuffer_free calls fb->funcs->destroy
	 */
	drm_framebuffer_free(&fb.base.refcount);
	KUNIT_EXPECT_EQ(test, buffer_freed, 1);

	buffer_freed = 0;

	ret = drm_mode_object_add(dev, &fb.base, DRM_MODE_OBJECT_FB);
	KUNIT_ASSERT_EQ(test, ret, 0);
	id = fb.base.id;

	/* Now, test with the fb registered, that must end unregistered */
	drm_framebuffer_free(&fb.base.refcount);
	KUNIT_EXPECT_EQ(test, fb.base.id, 0);
	KUNIT_EXPECT_EQ(test, buffer_freed, 1);

	/* Test if the old id of the fb was really removed from the idr pool */
	obj = drm_mode_object_find(dev, NULL, id, DRM_MODE_OBJECT_FB);
	KUNIT_EXPECT_PTR_EQ(test, obj, NULL);
}

static struct drm_framebuffer *
fb_create_addfb2_mock(struct drm_device *dev, struct drm_file *file_priv,
		      const struct drm_mode_fb_cmd2 *cmd)
{
	struct drm_mock *mock = container_of(dev, typeof(*mock), dev);
	struct drm_framebuffer *fb;
	struct kunit *test = mock->test;

	fb = kunit_kzalloc(test, sizeof(*fb), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fb);

	fb->base.id = 1;

	mock->private = fb;
	return fb;
}

static struct drm_mode_config_funcs config_funcs_addfb2_mock = {
	.fb_create = fb_create_addfb2_mock,
};

static void drm_test_framebuffer_addfb2(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	struct drm_file *file_priv = &mock->file_priv;
	struct drm_framebuffer *fb = NULL;
	int ret;

	/* A valid cmd */
	struct drm_mode_fb_cmd2 cmd = {
		.width = 600, .height = 600,
		.pixel_format = DRM_FORMAT_ABGR8888,
		.handles = { 1, 0, 0 }, .pitches = { 4 * 600, 0, 0 },
	};

	mock->test = test;
	dev->mode_config.funcs = &config_funcs_addfb2_mock;

	/* Must fail due to missing DRIVER_MODESET support */
	ret = drm_mode_addfb2(dev, &cmd, file_priv);
	KUNIT_EXPECT_EQ(test, ret, -EOPNOTSUPP);
	KUNIT_ASSERT_PTR_EQ(test, mock->private, NULL);

	/* Set DRIVER_MODESET support */
	dev->driver_features = dev->driver->driver_features;

	/*
	 * Set an invalid cmd to trigger a fail on the
	 * drm_internal_framebuffer_create function
	 */
	cmd.width = 0;
	ret = drm_mode_addfb2(dev, &cmd, file_priv);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_ASSERT_PTR_EQ(test, mock->private, NULL);
	cmd.width = 600; /* restore to a valid value */

	ret = drm_mode_addfb2(dev, &cmd, file_priv);
	KUNIT_EXPECT_EQ(test, ret, 0);

	/* The fb_create_addfb2_mock set fb id to 1 */
	KUNIT_EXPECT_EQ(test, cmd.fb_id, 1);

	fb = mock->private;

	/* The fb must be properly added to the file_priv->fbs list */
	KUNIT_EXPECT_PTR_EQ(test, file_priv->fbs.prev, &fb->filp_head);
	KUNIT_EXPECT_PTR_EQ(test, file_priv->fbs.next, &fb->filp_head);

	/* There must be just one fb on the list */
	KUNIT_EXPECT_PTR_EQ(test, fb->filp_head.prev, &file_priv->fbs);
	KUNIT_EXPECT_PTR_EQ(test, fb->filp_head.next, &file_priv->fbs);
}

static void drm_framebuffer_fb_release_remove_mock(struct kref *kref)
{
	struct drm_framebuffer *fb = container_of(kref, typeof(*fb), base.refcount);
	struct drm_mock *mock = container_of(fb->dev, typeof(*mock), dev);
	bool *obj_released = mock->private;

	obj_released[fb->base.id - 1] = true;
}

static int crtc_set_config_fb_release_mock(struct drm_mode_set *set,
					   struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_crtc *crtc = set->crtc;
	struct drm_mock *mock = container_of(crtc->dev, typeof(*mock), dev);
	bool *obj_released = mock->private;

	obj_released[crtc->base.id - 1] = true;
	obj_released[crtc->primary->base.id - 1] = true;
	return 0;
}

static int disable_plane_fb_release_mock(struct drm_plane *plane,
					 struct drm_modeset_acquire_ctx *ctx)
{
	struct drm_mock *mock = container_of(plane->dev, typeof(*mock), dev);
	bool *obj_released = mock->private;

	obj_released[plane->base.id - 1] = true;
	return 0;
}

#define NUM_OBJS 5

/*
 * The drm_fb_release function implicitly calls at some point the
 * drm_framebuffer_remove, which actually removes framebuffers
 * based on the driver supporting or not the atomic API. To simplify
 * this test, let it rely on legacy removing and leave the atomic remove
 * to be tested in another test case. By doing that, we can also test
 * the legacy_remove_fb function entirely.
 */
static void drm_test_fb_release(struct kunit *test)
{
	struct drm_mock *mock = test->priv;
	struct drm_device *dev = &mock->dev;
	struct drm_file *file_priv = &mock->file_priv;
	struct drm_plane_funcs plane_funcs = {
		.disable_plane = disable_plane_fb_release_mock
	};
	struct drm_crtc_funcs crtc_funcs = {
		.set_config = crtc_set_config_fb_release_mock
	};
	struct drm_framebuffer *fb1, *fb2;
	struct drm_plane *plane1, *plane2;
	struct drm_crtc *crtc;
	bool *obj_released;

	/*
	 * obj_released[i] where "i" is the obj.base.id - 1. Note that the
	 * "released" word means different things for each kind of obj, which
	 * in case of a framebuffer, means that it was freed, while for the
	 * crtc and plane, means that it was deactivated.
	 */
	obj_released = kunit_kzalloc(test, NUM_OBJS * sizeof(bool), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, obj_released);
	mock->private = obj_released;

	fb1 = kunit_kzalloc(test, sizeof(*fb1), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fb1);
	list_add(&fb1->filp_head, &file_priv->fbs);
	kref_init(&fb1->base.refcount);
	fb1->dev = dev;
	fb1->base.free_cb = drm_framebuffer_fb_release_remove_mock;
	fb1->base.id = 1;

	fb2 = kunit_kzalloc(test, sizeof(*fb2), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fb2);
	list_add(&fb2->filp_head, &file_priv->fbs);
	kref_init(&fb2->base.refcount);
	fb2->dev = dev;
	fb2->base.free_cb = drm_framebuffer_fb_release_remove_mock;
	fb2->base.id = 2;

	plane1 = kunit_kzalloc(test, sizeof(*plane1), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, plane1);
	list_add(&plane1->head, &dev->mode_config.plane_list);
	drm_modeset_lock_init(&plane1->mutex);
	plane1->dev = dev;
	plane1->funcs = &plane_funcs;
	plane1->base.id = 3;

	plane2 = kunit_kzalloc(test, sizeof(*plane2), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, plane2);
	list_add(&plane2->head, &dev->mode_config.plane_list);
	drm_modeset_lock_init(&plane2->mutex);
	plane2->dev = dev;
	plane2->funcs = &plane_funcs;
	plane2->base.id = 4;

	crtc = kunit_kzalloc(test, sizeof(*crtc), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, crtc);
	list_add(&crtc->head, &dev->mode_config.crtc_list);
	drm_modeset_lock_init(&crtc->mutex);
	crtc->dev = dev;
	crtc->funcs = &crtc_funcs;
	crtc->base.id = 5;

	/*
	 * Attach fb2 to some planes to stress the case where we have more than
	 * one reference to the fb. plane1 is attached to crtc as primary plane
	 * and plane2 will represent any non-primary plane, allowing to cover
	 * all codepaths on legacy_remove_fb
	 */
	crtc->primary = plane1;
	plane1->crtc = crtc;
	plane1->fb = fb2;
	plane2->fb = fb2;
	/* Each plane holds one reference to fb */
	drm_framebuffer_get(fb2);
	drm_framebuffer_get(fb2);

	drm_fb_release(file_priv);

	KUNIT_EXPECT_TRUE(test, list_empty(&file_priv->fbs));

	/* Every object from this test should be released */
	for (int i = 0; i < 5; i++)
		KUNIT_EXPECT_EQ(test, obj_released[i], 1);

	KUNIT_EXPECT_FALSE(test, kref_read(&fb1->base.refcount));
	KUNIT_EXPECT_FALSE(test, kref_read(&fb2->base.refcount));

	KUNIT_EXPECT_PTR_EQ(test, plane1->crtc, NULL);
	KUNIT_EXPECT_PTR_EQ(test, plane1->fb, NULL);
	KUNIT_EXPECT_PTR_EQ(test, plane1->old_fb, NULL);
	KUNIT_EXPECT_PTR_EQ(test, plane2->crtc, NULL);
	KUNIT_EXPECT_PTR_EQ(test, plane2->fb, NULL);
	KUNIT_EXPECT_PTR_EQ(test, plane2->old_fb, NULL);
}

static struct kunit_case drm_framebuffer_tests[] = {
	KUNIT_CASE(drm_test_fb_release),
	KUNIT_CASE(drm_test_framebuffer_addfb2),
	KUNIT_CASE(drm_test_framebuffer_cleanup),
	KUNIT_CASE(drm_test_framebuffer_free),
	KUNIT_CASE(drm_test_framebuffer_init),
	KUNIT_CASE(drm_test_framebuffer_lookup),
	KUNIT_CASE(drm_test_framebuffer_modifiers_not_supported),
	KUNIT_CASE_PARAM(drm_test_framebuffer_check_src_coords, check_src_coords_gen_params),
	KUNIT_CASE_PARAM(drm_test_framebuffer_create, drm_framebuffer_create_gen_params),
	{ }
};

static struct kunit_suite drm_framebuffer_test_suite = {
	.name = "drm_framebuffer",
	.init = drm_framebuffer_test_init,
	.exit = drm_framebuffer_test_exit,
	.test_cases = drm_framebuffer_tests,
};

kunit_test_suite(drm_framebuffer_test_suite);

MODULE_LICENSE("GPL");
