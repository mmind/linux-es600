/*
 * Stub for MT5921 SPI WLAN cards.
 *
 * Copyright 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The only use of this function is to provide a .gnu.linkonce.this_module
 * section matching the current kernel for the proprietary mt5921 module.
 *
 * Usage:
 * - strip the original section from mt5921sta_spi[-dbg].ko with
 *      objcopy -R.gnu.linkonce.this_module mt5921sta_spi-dbg.ko mt5921sta_spi-dbg.o
 *
 * - build this module and do a manual link with
 *      ld -r -o mt5921sta_spi-dbg.ko mt5921sta_spi-dbg.o mt5921sta_spi.mod.o
 */

#include <linux/module.h>

/*
 * Module functions
 */

static int __init mt5921_init(void)
{
	return 0;
}

static void __exit mt5921_exit(void)
{
}

module_init(mt5921_init);
module_exit(mt5921_exit);

MODULE_DESCRIPTION("MT5921 SPI WLAN Stub");
MODULE_AUTHOR("Heiko Stuebner <heiko@sntech.de>");
MODULE_LICENSE("GPL");
