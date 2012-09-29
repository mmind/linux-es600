/* linux/arch/arm/mach-s3c2416/mach-sg060.c
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
#include <linux/input/gpio_tilt.h>

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
#include <linux/spi/mt592x_spi.h>

#include <plat/regs-fb-v4.h>
#include <plat/fb.h>

#include <plat/pm.h>

#include <mach/es600.h>

#include <video/auo_k190xfb.h>

/*
 * Tilt-sensor.
 */
#define SG060_TILT_GPIO_EN	S3C2410_GPC(3)
#define SG060_TILT_GPIO_SENSOR2	S3C2410_GPD(10)
#define SG060_TILT_GPIO_SENSOR1	S3C2410_GPD(11)

static int sg060_tilt_enable(struct device *dev) {
	int ret;

	ret = gpio_request(SG060_TILT_GPIO_EN, "tilt sensor enable");
	if (ret)
	{
		printk(KERN_ERR "can't request gpio %d\n", SG060_TILT_GPIO_EN);
		return ret;
	}
	
	ret = gpio_direction_output(SG060_TILT_GPIO_EN, 1);
	if (ret)
	{
		printk(KERN_ERR "can't set gpio %d\n", SG060_TILT_GPIO_EN);
		gpio_free(SG060_TILT_GPIO_EN);
		return ret;
	}
	return 0;
};

static void sg060_tilt_disable(struct device *dev) {
	gpio_set_value(SG060_TILT_GPIO_EN, 0);
	gpio_free(SG060_TILT_GPIO_EN);
};

static struct gpio sg060_tilt_gpios[] = {
	{ SG060_TILT_GPIO_SENSOR2, GPIOF_IN, "tilt_sensor2" },
	{ SG060_TILT_GPIO_SENSOR1, GPIOF_IN, "tilt_sensor1" },
};

/* to FIX: here tilt-Orientation 0 is portrait mode
 * while display and touch solution are in landscape mode
 */
static struct gpio_tilt_state sg060_tilt_states[] = {
	{
		.gpios = (0 << 1) | (0 << 0),
		.axes = (int[]) {
			0,
		},
	}, {
		.gpios = (0 << 1) | (1 << 0),
		.axes = (int[]) {
			1, /* 90 degrees */
		},
	}, {
		.gpios = (1 << 1) | (1 << 0),
		.axes = (int[]) {
			2, /* 180 degrees */
		},
	}, {
		.gpios = (1 << 1) | (0 << 0),
		.axes = (int[]) {
			3, /* 270 degrees */
		},
	},
};

static struct gpio_tilt_axis sg060_tilt_axes[] = {
	{
		.axis = ABS_RY,
		.min = 0,
		.max = 3,
		.fuzz = 0,
		.flat = 0,
	},
};

static struct gpio_tilt_platform_data sg060_tilt_pdata= {
	.gpios = sg060_tilt_gpios,
	.nr_gpios = ARRAY_SIZE(sg060_tilt_gpios),

	.axes = sg060_tilt_axes,
	.nr_axes = ARRAY_SIZE(sg060_tilt_axes),

	.states = sg060_tilt_states,
	.nr_states = ARRAY_SIZE(sg060_tilt_states),

	.debounce_interval = 100,

	.poll_interval = 1000,
	.enable = sg060_tilt_enable,
	.disable = sg060_tilt_disable,
};

static struct platform_device sg060_device_tilt = {
	.name = "gpio-tilt-polled",
	.id = -1,
	.dev = {
		.platform_data = &sg060_tilt_pdata,
	},
};

static struct platform_device *sg060_devices[] __initdata = {
	/* inputs */
	&sg060_device_tilt,
};

/*
 * SPI
 */

static struct regulator *vdd33, *vdd18, *vdd33pa;

static int sg060_mt5921_setup(struct spi_device *spi)
{
	int err;

	dev_err(&spi->dev, "setup callback\n");

	vdd33pa = regulator_get(NULL, "vdd33pa");
	if (IS_ERR(vdd33pa)) {
		err = PTR_ERR(vdd33pa);
		dev_err(&spi->dev, "could not get vdd33pa, %d\n", err);
		return err;
	}

	vdd33 = regulator_get(NULL, "vdd33");
	if (IS_ERR(vdd33)) {
		err = PTR_ERR(vdd33);
		dev_err(&spi->dev, "could not get vdd33, %d\n", err);
		return err;
	}

	vdd18 = regulator_get(NULL, "vdd18");
	if (IS_ERR(vdd18)) {
		err = PTR_ERR(vdd18);
		dev_err(&spi->dev, "could not get vdd18, %d\n", err);
		return err;
	}

	err = gpio_request(ES600_WIFI_GPIO_RST, "WLAN RST");
	if (err)
		goto err_free_strap;

	err = gpio_direction_output(ES600_WIFI_GPIO_RST, 0);
	if (err)
		goto err_free_strap;
	msleep(100);

	regulator_enable(vdd18);
	msleep(1);
	regulator_enable(vdd33);
	msleep(1);
	regulator_enable(vdd33pa);

	gpio_set_value(ES600_WIFI_GPIO_RST, 1);
	msleep(100);

	spi->bits_per_word = 8;
	spi_setup(spi);

	return 0;

err_free_strap:
	return err;
}

static int sg060_mt5921_teardown(struct spi_device *spi)
{
	dev_err(&spi->dev, "teardown callback\n");

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

struct mt592x_spi_platform_data sg060_mt5921_pdata = {
	.use_dummy_writes	= 1,
	.setup			= sg060_mt5921_setup,
	.teardown		= sg060_mt5921_teardown,
};


struct s3c64xx_spi_csinfo sg060_mt5921_cs_info = {
	.fb_delay = 0,
	.line = ES600_HSSPI_GPIO_CS0,
};


static struct spi_board_info sg060_spi_board_info[] = {
	{
		.modalias		= "mt592x_spi",
		.max_speed_hz		= 1200000,
		.bus_num		= 0,
		.irq			= ES600_WIFI_IRQ_SPI_INT,
		.chip_select		= 0,
		.mode			= SPI_MODE_3,
//		.controller_data	= &sg060_mt5921_cs_info,
		.controller_data	= ES600_HSSPI_GPIO_CS0,
		.platform_data		= &sg060_mt5921_pdata,
	},
};

static void __init sg060_machine_init(void)
{
//	regulator_use_dummy_regulator();

	es600_common_init();
	platform_add_devices(sg060_devices, ARRAY_SIZE(sg060_devices));

	es600_spi_init(sg060_spi_board_info, ARRAY_SIZE(sg060_spi_board_info));

//FIXME: this is normaly done in the i2s init
//but the alc5624 needs the cdclk clock source to talk via i2c
//this should be cleared up and this code be removed later on
//GPG3 is a key in as090, so this is probably wrong
//	gpio_request(S3C2410_GPG(3), "ALC5624_I2C"); //FIXME: check
//	gpio_direction_output(S3C2410_GPG(3), 1);

	clk_enable(clk_get(NULL, "iis"));
	clk_enable(clk_get(NULL, "i2s-if"));

	/* Configure the I2S pins in correct mode */
	s3c2410_gpio_cfgpin(S3C2410_GPE(2), S3C2410_GPE2_CDCLK);

	s3c_pm_init();
}

static char const *sg060_dt_compat[] __initdata = {
	"qisda,tf06", /* all 2nd gen devices */
	"qisda,sg06", /* Thalia Oyo 1, Sagem Binder */
	"qisda,pd06", /* Pandigital Novell, 7 key model */
	NULL
};

MACHINE_START(SMDK2416, "SG060")
	/* Maintainer: Heiko Stuebner <heiko@sntech.de> */
	.atag_offset	= 0x100,

	.init_irq	= s3c2416_irq_init,
	.map_io		= es600_common_map_io,
	.init_machine	= sg060_machine_init,
	.timer		= &s3c24xx_timer,
	.dt_compat	= sg060_dt_compat,
	.restart	= s3c2416_restart,
MACHINE_END
