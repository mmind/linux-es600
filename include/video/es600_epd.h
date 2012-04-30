/*
 * Platformdata for ES600-AUO-K190X glue driver
 *
 * Copyright (C) 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_VIDEO_ES600_EPD_H_
#define _LINUX_VIDEO_ES600_EPD_H_

/**
 * Platform data for AUO-K190x controllers
 * @driver:	name of framebuffer driver to use
 * @resolution:	AUOK190X_RESOLUTION-constant describing the connected display
 * @rotation:	initial rotation of the display
 * @quirks:	AUOK190X_QUIRK-constants describing possible quirks
 * @fps:	target frame per second for defio
 */
struct es600_epd_pdata {
	char *driver;
	int resolution;
	int rotation;
	int quirks;
	int fps;

	int gpio_nsleep;
	int gpio_nrst;
	int gpio_nbusy;
};

#endif