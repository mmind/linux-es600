/*
 * Serial driver for Qisda H18[s/d/t] USB WWANs
 *
 * Copyright (c) 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_PLATFORM_DATA_QISDA_H18X_H
#define __LINUX_PLATFORM_DATA_QISDA_H18X_H

/**
 * qisda_h18x_platdata - Platform data for Qisda H18x USB WWANs.
 * @gpio_w_disable: GPIO pin the W_DISABLE pin is connected to
 */
struct qisda_h18x_pdata {
	unsigned int	gpio_w_disable;
};

#endif	/* __LINUX_PLATFORM_DATA_QISDA_H18X_H */
