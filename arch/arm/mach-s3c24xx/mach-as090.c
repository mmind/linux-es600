/* linux/arch/arm/mach-s3c2416/mach-as090.c
 *
 * Machine definition for the Qisda ebook-reader family
 * Copyright (c) 2011 Heiko Stuebner <heiko@sntech.de>
 *
 * based on mach-smdk2416.c
 *
 * Copyright (c) 2009 Yauhen Kharuzhy <jekhor@gmail.com>,
 *	as part of OpenInkpot project
 * Copyright (c) 2009 Promwad Innovation Company
 *	Yauhen Kharuzhy <yauhen.kharuzhy@promwad.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * FIXME: uses currently the MACHINE_ID of the smdk2416 board
 * FIXME: should probably get a name more adept to the multitude of
 * different boards based on this platform.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/mtd/partitions.h>
#include <linux/gpio.h>
#include <linux/fb.h>
#include <linux/delay.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>

#include <linux/input.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <mach/regs-gpio.h>
#include <mach/regs-lcd.h>
#include <mach/regs-s3c2443-clock.h>

#include <mach/idle.h>
#include <mach/leds-gpio.h>

#include <plat/s3c2416.h>
#include <plat/gpio-cfg.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>

#include <linux/spi/spi.h>
#include <plat/s3c64xx-spi.h>
#include <linux/spi/libertas_spi.h>

#include <plat/regs-fb-v4.h>
#include <plat/fb.h>

#include <plat/pm.h>

#include <mach/es600.h>

#include <video/auo_k190xfb.h>

/*
 * SPI
 */

static struct regulator *vdd33, *vdd18, *vdd33pa;

static int as090_libertas_setup(struct spi_device *spi)
{
	int err;

	vdd33pa = regulator_get(NULL, "vdd33pa");

	vdd33 = regulator_get(NULL, "vdd33");

	vdd18 = regulator_get(NULL, "vdd18");

	regulator_enable(vdd33pa);
	regulator_enable(vdd33);
	regulator_enable(vdd18);

	err = gpio_request(ES600_WIFI_GPIO_RST, "WLAN RST");
	if (err)
		goto err_free_strap;

	err = gpio_direction_output(ES600_WIFI_GPIO_RST, 0);
	if (err)
		goto err_free_strap;
	msleep(100);

	gpio_set_value(ES600_WIFI_GPIO_RST, 1);
	msleep(100);

	spi->bits_per_word = 16;
	spi_setup(spi);

	return 0;

err_free_strap:
	return err;
}

static int as090_libertas_teardown(struct spi_device *spi)
{
	gpio_set_value(ES600_WIFI_GPIO_RST, 0);
	gpio_free(ES600_WIFI_GPIO_RST);

	regulator_disable(vdd33);
	regulator_disable(vdd18);
	regulator_disable(vdd33pa);

	regulator_put(vdd33);
	regulator_put(vdd18);
	regulator_put(vdd33pa);

	return 0;
}

struct libertas_spi_platform_data as090_libertas_pdata = {
	.use_dummy_writes	= 0,
	.setup			= as090_libertas_setup,
	.teardown		= as090_libertas_teardown,
};


struct s3c64xx_spi_csinfo as090_libertas_cs_info = {
	.fb_delay = 0,
	.line = ES600_HSSPI_GPIO_CS0,
};


static struct spi_board_info as090_spi_board_info[] = {
	{
		.modalias		= "libertas_spi",
		.max_speed_hz		= 13000000,
		.bus_num		= 0,
		.irq			= ES600_WIFI_IRQ_SPI_INT,
		.chip_select		= 0,
		.mode			= SPI_MODE_0,
		.controller_data	= &as090_libertas_cs_info,
		.platform_data		= &as090_libertas_pdata,
	},
};

static void __init as090_machine_init(void)
{
//	regulator_use_dummy_regulator();

	es600_common_init();

	es600_spi_init(as090_spi_board_info, ARRAY_SIZE(as090_spi_board_info));

//FIXME: this is normaly done in the i2s init
//but the alc5624 needs the cdclk clock source to talk via i2c
//this should be cleared up and this code be removed later on
	clk_enable(clk_get(NULL, "iis"));
	clk_enable(clk_get(NULL, "i2s-if"));

	/* Configure the I2S pins in correct mode */
	s3c2410_gpio_cfgpin(S3C2410_GPE(2), S3C2410_GPE2_CDCLK);

	s3c_pm_init();
}

static char const *as090_dt_compat[] __initdata = {
	"qisda,as09",
	NULL
};

MACHINE_START(SMDK2416, "AS090")
	/* Maintainer: Heiko Stuebner <heiko@sntech.de> */
	.atag_offset	= 0x100,

	.init_irq	= s3c2416_irq_init,
	.map_io		= es600_common_map_io,
	.init_machine	= as090_machine_init,
	.timer		= &s3c24xx_timer,
	.dt_compat	= as090_dt_compat,
	.restart	= s3c2416_restart,
MACHINE_END
