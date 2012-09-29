/* linux/arch/arm/mach-s3c2416/es600-power.c
 *
 * Common devices and init for the Qisda ES600 ebook-reader family
 * Copyright (c) 2011 Heiko Stuebner <heiko@sntech.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include <linux/of_platform.h>

#include <linux/regulator/machine.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/fixed.h>
#include <linux/regulator/gpio-regulator.h>

#include <linux/mmc/host.h>

/* power-off handling */
#include <linux/power_supply.h>
#include <linux/reboot.h>

#include <plat/devs.h>
#include <plat/iic.h>
#include <plat/regs-iic.h>
#include <plat/sdhci.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-serial.h>
#include <plat/cpu.h>

#include <mach/regs-gpio.h>
#include <mach/irqs.h>
#include <mach/map.h>
#include <mach/hardware.h>

#include <asm/mach/map.h>

/* for readl */
#include <linux/io.h>

/* es600 data */
#include <mach/es600.h>


#include <linux/usb/gpio_vbus.h>
#include <linux/platform_data/s3c2443_phy.h>
#include <linux/platform_data/s3c-hsudc.h>
#include <plat/udc.h>
#include <plat/usb-control.h>

#include <linux/s3c_adc_battery.h>
#include <linux/pda_power.h>

#include <video/auo_k190xfb.h>

#include <sound/alc5624.h>

#include <linux/input/auo-pixcir-ts.h>

#include <linux/spi/spi.h>
#include <plat/s3c64xx-spi.h>

/* tmp: to check for error in s3c64xx-spi */
#include <linux/spi/spi_gpio.h>

//usb1.1-tmp
#include <mach/regs-s3c2443-clock.h>
#include <linux/io.h>


#include <linux/platform_data/qisda_h18x.h>


/* FIXME */
static struct map_desc es600_iodesc[] __initdata = {
	/* ISA IO Space map (memory space selected by A24) */

	{
		.virtual	= (u32)S3C24XX_VA_ISA_WORD,
		.pfn		= __phys_to_pfn(S3C2410_CS2),
		.length		= 0x10000,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (u32)S3C24XX_VA_ISA_WORD + 0x10000,
		.pfn		= __phys_to_pfn(S3C2410_CS2 + (1<<24)),
		.length		= SZ_4M,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (u32)S3C24XX_VA_ISA_BYTE,
		.pfn		= __phys_to_pfn(S3C2410_CS2),
		.length		= 0x10000,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (u32)S3C24XX_VA_ISA_BYTE + 0x10000,
		.pfn		= __phys_to_pfn(S3C2410_CS2 + (1<<24)),
		.length		= SZ_4M,
		.type		= MT_DEVICE,
	}
};

#define UCON (S3C2410_UCON_DEFAULT	| \
		S3C2440_UCON_PCLK	| \
		S3C2443_UCON_RXERR_IRQEN)

#define ULCON (S3C2410_LCON_CS8 | S3C2410_LCON_PNONE)

#define UFCON (S3C2410_UFCON_RXTRIG8	| \
		S3C2410_UFCON_FIFOMODE	| \
		S3C2440_UFCON_TXTRIG16)

static struct s3c2410_uartcfg es600_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	},
	/* pins of hwport 2 are used otherwise */
	[2] = {
		.hwport	     = 3,
		.flags	     = 0,
		.ucon	     = UCON,
		.ulcon	     = ULCON,
		.ufcon	     = UFCON,
	}
};

/*
 * tps650240 regulators
 */

/*
 * tps650240-DCDC1 - fixed voltage regulator 3.3V
 */
static struct regulator_consumer_supply es600_dcdc1_supply[] = {
	REGULATOR_SUPPLY("vmmc", "s3c-sdhci.0"),
	REGULATOR_SUPPLY("vmmc", "s3c-sdhci.1"),
	REGULATOR_SUPPLY("vdd", "s3c24xx-adc"), /* Used by CPU's ADC drv */
};

static struct regulator_init_data es600_dcdc1_init = {
	.constraints = {
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.always_on	= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_dcdc1_supply),
	.consumer_supplies	= es600_dcdc1_supply,
};

static struct fixed_voltage_config es600_dcdc1 = {
	.supply_name		= "tps650240-dcdc1",
	.microvolts		= 3300000, /* 3.3V */
	.gpio			= -EINVAL,
	.startup_delay		= 750, /* 750us */
	.enabled_at_boot	= 1,
	.init_data		= &es600_dcdc1_init,
};

static struct platform_device es600_dcdc1_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 0,
	.dev = {
		.platform_data = &es600_dcdc1,
	},
};

/*
 * tps650240-DCDC2 - fixed voltage regulator 1.8V
 */
static struct regulator_consumer_supply es600_dcdc2_supply[] = {
	REGULATOR_SUPPLY("vddsram", NULL),  //FIXME
	REGULATOR_SUPPLY("vddsdram", NULL), //FIXME
};

static struct regulator_init_data es600_dcdc2_init = {
	.constraints = {
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.always_on	= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_dcdc2_supply),
	.consumer_supplies	= es600_dcdc2_supply,
};

static struct fixed_voltage_config es600_dcdc2 = {
	.supply_name		= "tps650240-dcdc2",
	.microvolts		= 1800000, /* 1.8V */
	.gpio			= -EINVAL,
	.startup_delay		= 750, /* 750us */
	.enabled_at_boot	= 1,
	.init_data		= &es600_dcdc2_init,
};

static struct platform_device es600_dcdc2_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 1,
	.dev = {
		.platform_data = &es600_dcdc2,
	},
};


/*
 * tps650240-DCDC3 - voltage regulator switchable between 1.0V and 1.3V
 */
static struct regulator_consumer_supply es600_dcdc3_supply[] = {
	REGULATOR_SUPPLY("vddarm", NULL),
};

static struct regulator_init_data es600_dcdc3_init = {
	.constraints = {
		.min_uV		= 1000000,
		.max_uV		= 1300000,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE,
		.always_on	= 1,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_dcdc3_supply),
	.consumer_supplies	= es600_dcdc3_supply,
};

static struct gpio es600_dcdc3_gpios[] = {
	{ ES600_TPS650240_GPIO_DEFDCDC3, GPIOF_OUT_INIT_HIGH, "tps650240-defdcdc3" },
};

static struct gpio_regulator_state es600_dcdc3_states[] = {
		{ .value = 1000000, .gpios = (0 << 0) },
		{ .value = 1300000, .gpios = (1 << 0) },
};

static struct gpio_regulator_config es600_dcdc3_config = {
	.supply_name		= "tps650240-dcdc3",

	.enable_gpio		= -EINVAL,
	.enabled_at_boot	= 1,
	.startup_delay		= 750, /* 750us */

	.gpios			= es600_dcdc3_gpios,
	.nr_gpios		= ARRAY_SIZE(es600_dcdc3_gpios),

	.states			= es600_dcdc3_states,
	.nr_states		= ARRAY_SIZE(es600_dcdc3_states),

	.type			= REGULATOR_VOLTAGE,
	.init_data		= &es600_dcdc3_init,
};


static struct platform_device es600_dcdc3_regulator = {
	.name = "gpio-regulator",
	.id   = 0,
	.dev  = {
		.platform_data = &es600_dcdc3_config,
	},
};

/*
 * tps650240-LDO 1+2 - USBdev 1.2V and 3.3V
 * The device consists essentially of two separate LDOs which are
 * activated by a single GPIO simultanously.
 * Therefore the voltage listed here is only true for one of the LDOs
 */
static struct regulator_consumer_supply es600_ldo_supply[] = {
	REGULATOR_SUPPLY("vdda", "s3c-hsudc"),
	REGULATOR_SUPPLY("vddi", "s3c-hsudc"),
	REGULATOR_SUPPLY("vddosc", "s3c-hsudc"),
};

static struct regulator_init_data es600_ldo_init = {
	.constraints = {
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_ldo_supply),
	.consumer_supplies	= es600_ldo_supply,
};

static struct fixed_voltage_config es600_ldo = {
	.supply_name		= "tps650240-ldo",
	.microvolts		= 3300000, /* 3.3V */
	.gpio			= ES600_TPS650240_GPIO_ENLDO12,
	.startup_delay		= 10, /* 10us */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &es600_ldo_init,
};

static struct platform_device es600_ldo_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 2,
	.dev = {
		.platform_data = &es600_ldo,
	},
};



/*
 * tps650240-LDO3 for VDDalive - not necessary
 */

/*
 * tps79733 for vdd_rtc - probably not necessary to define.
 */

/*
 * Battery charger regulator bq24075
 */

static struct regulator_consumer_supply es600_bq24075_consumers[] = {
	REGULATOR_SUPPLY("vbus_draw", "gpio-vbus"), /* FIXME: remove when s3c2443-phy is stable */
	REGULATOR_SUPPLY("vbus_draw", "s3c2443-phy"),
	REGULATOR_SUPPLY("ac_draw", "pda-power"),
};

static struct regulator_init_data es600_bq24075_init = {
	.constraints = {
		.max_uA         = 975000, /* 975mA according to asus */
		.valid_ops_mask = REGULATOR_CHANGE_CURRENT|REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = ARRAY_SIZE(es600_bq24075_consumers),
	.consumer_supplies      = es600_bq24075_consumers,
};

static struct gpio es600_bq24075_gpios[] = {
	{ ES600_BQ24075_GPIO_EN1, GPIOF_OUT_INIT_LOW, "bq24075_en1" },
	{ ES600_BQ24075_GPIO_EN2, GPIOF_OUT_INIT_LOW, "bq24075_en2" },
};

static struct gpio_regulator_state es600_bq24075_states[] = {
		{ .value = 000000, .gpios = (1 << 1) | (1 << 0) },
		{ .value = 100000, .gpios = (0 << 1) | (0 << 0) },
		{ .value = 500000, .gpios = (0 << 1) | (1 << 0) },
		{ .value = 975000, .gpios = (1 << 1) | (0 << 0) },
};

static struct gpio_regulator_config es600_bq24075_config = {
	.supply_name = "bq24075",

	.enable_gpio = ES600_BQ24075_GPIO_NCE,
	.enable_high = 0,
	.enabled_at_boot = 0,

	.gpios = es600_bq24075_gpios,
	.nr_gpios = ARRAY_SIZE(es600_bq24075_gpios),

	.states = es600_bq24075_states,
	.nr_states = ARRAY_SIZE(es600_bq24075_states),

	.type = REGULATOR_CURRENT,
	.init_data = &es600_bq24075_init,
};


static struct platform_device es600_bq24075_regulator = {
	.name = "gpio-regulator",
	.id   = 1,
	.dev  = {
		.platform_data = &es600_bq24075_config,
	},
};

/*
 * MIC-9404 - alc5624 on/off switch
 * Seems to supply the dvdd1-pin of the alc5624 with 3.3V (from VDD_D)
 */

static struct regulator_consumer_supply es600_mic94043_supply[] = {
	REGULATOR_SUPPLY("vdd", "alc5624"), /* sound codec, FIXME */
};

static struct regulator_init_data es600_mic94043_init = {
	.constraints = {
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_mic94043_supply),
	.consumer_supplies	= es600_mic94043_supply,
};

static struct fixed_voltage_config es600_mic94043 = {
	.supply_name		= "mic94043",
	.microvolts		= 3300000, /* 3.3V */
	.gpio			= ES600_ALC5624_GPIO_EN,
	.startup_delay		= 185, /* 185us */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &es600_mic94043_init,
};

static struct platform_device es600_mic94043_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 3,
	.dev = {
		.platform_data = &es600_mic94043,
	},
};

/*
 * GMT G9093 regulators for wifi supply
 */

static struct regulator_consumer_supply es600_g9093_1_supply[] = {
	REGULATOR_SUPPLY("vdd33", "libertas_spi"),
	REGULATOR_SUPPLY("vdd33", "mt592x_spi"),
	REGULATOR_SUPPLY("vdd33", NULL),
};

static struct regulator_init_data es600_g9093_1_init = {
	.constraints = {
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_g9093_1_supply),
	.consumer_supplies	= es600_g9093_1_supply,
};

static struct fixed_voltage_config es600_g9093_1 = {
	.supply_name		= "g9093_1",
	.microvolts		= 3300000, /* 3.3V */
	.gpio			= ES600_G9093_GPIO_EN3V3,
	.startup_delay		= 50000, /* 50ms */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &es600_g9093_1_init,
};

static struct platform_device es600_g9093_1_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 4,
	.dev = {
		.platform_data = &es600_g9093_1,
	},
};

static struct regulator_consumer_supply es600_g9093_2_supply[] = {
	REGULATOR_SUPPLY("vdd18", "libertas_spi"),
	REGULATOR_SUPPLY("vdd18", "mt592x_spi"),
	REGULATOR_SUPPLY("vdd18", NULL),
};

static struct regulator_init_data es600_g9093_2_init = {
	.constraints = {
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_g9093_2_supply),
	.consumer_supplies	= es600_g9093_2_supply,
};

static struct fixed_voltage_config es600_g9093_2 = {
	.supply_name		= "g9093_2",
	.microvolts		= 1800000, /* 1.8V */
	.gpio			= ES600_G9093_GPIO_EN1V8,
	.startup_delay		= 5000, /* 50ms */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &es600_g9093_2_init,
};

struct platform_device es600_g9093_2_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 5,
	.dev = {
		.platform_data = &es600_g9093_2,
	},
};

/*
 * LR1106 regulator for wifi pa
 */

static struct regulator_consumer_supply es600_lr1106_supply[] = {
	REGULATOR_SUPPLY("vdd33pa", "libertas_spi"), /* wifi, FIXME */
	REGULATOR_SUPPLY("vdd33pa", "mt592x_spi"), /* wifi, FIXME */
	REGULATOR_SUPPLY("vdd33pa", NULL), /* wifi, FIXME */
};

static struct regulator_init_data es600_lr1106_init = {
	.constraints = {
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_lr1106_supply),
	.consumer_supplies	= es600_lr1106_supply,
};

static struct fixed_voltage_config es600_lr1106 = {
	.supply_name		= "lr1106",
	.microvolts		= 3300000, /* 3.3V */
	.gpio			= ES600_LR1106_GPIO_EN,
	.startup_delay		= 1000, /* 1ms */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &es600_lr1106_init,
};

struct platform_device es600_lr1106_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 6,
	.dev = {
		.platform_data = &es600_lr1106,
	},
};

/*
 * 2n7002k mosfet controlling both the 3.3V and 1.8V power of the K190x
 */

static struct regulator_consumer_supply es600_auok190x_supply[] = {
	REGULATOR_SUPPLY("vdd", "auo_k1900fb"),
	REGULATOR_SUPPLY("vdd", "auo_k1901fb"),
};

static struct regulator_init_data es600_auok190x_init = {
	.constraints = {
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_auok190x_supply),
	.consumer_supplies	= es600_auok190x_supply,
};

static struct fixed_voltage_config es600_auok190x = {
	.supply_name		= "AUO-K190x",
	.microvolts		= 3300000, /* 3.3V */
	.gpio			= ES600_AUOK1900_GPIO_POWER,
	.startup_delay		= 1000, /* 1ms */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &es600_auok190x_init,
};

struct platform_device es600_auok190x_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 7,
	.dev = {
		.platform_data = &es600_auok190x,
	},
};

/*
 * LTC3442 regulator for 3g, not present on all models
 */

static struct regulator_consumer_supply es600_ltc3442_supply[] = {
	REGULATOR_SUPPLY("vdd", "qisda_h18x"),
};

static struct regulator_init_data es600_ltc3442_init = {
	.constraints = {
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.valid_ops_mask	= REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(es600_ltc3442_supply),
	.consumer_supplies	= es600_ltc3442_supply,
};

static struct fixed_voltage_config es600_ltc3442 = {
	.supply_name		= "ltc3442",
	.microvolts		= 3300000, /* 3.3V */
	.gpio			= ES600_LTC3442_GPIO_EN,
	.startup_delay		= 1000, /* 1ms */
	.enable_high		= 1,
	.enabled_at_boot	= 0,
	.init_data		= &es600_ltc3442_init,
};

struct platform_device es600_ltc3442_regulator = {
	.name		= "reg-fixed-voltage",
	.id		= 8,
	.dev = {
		.platform_data = &es600_ltc3442,
	},
};


static struct platform_device *es600_regulators[] __initdata = {
	/* tps650240 pmic */
	&es600_dcdc1_regulator,
	&es600_dcdc2_regulator,
	&es600_dcdc3_regulator,
	&es600_ldo_regulator,

	/* bq24075 - charger */
	&es600_bq24075_regulator,

	/* alc5624 regulator */
	&es600_mic94043_regulator,

	/* wifi regulators */
	&es600_g9093_1_regulator,
	&es600_g9093_2_regulator,
	&es600_lr1106_regulator,

	/* K190x power switch */
	&es600_auok190x_regulator,

	/* 3g regulator */
//	&es600_ltc3442_regulator,
};

/*
 * USB "Transceiver"
 */

static struct resource es600_gpio_vbus_resource = {
	.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE |
	         IORESOURCE_IRQ_LOWEDGE,
	.start = ES600_HSUDC_IRQ_VBUS,
	.end   = ES600_HSUDC_IRQ_VBUS,
};

static struct gpio_vbus_mach_info es600_gpio_vbus_pdata = {
	.gpio_vbus		= ES600_HSUDC_GPIO_VBUS,
};

static struct platform_device es600_gpio_vbus = {
	.name		= "gpio-vbus",
	.id		= -1,
	.num_resources	= 1,
	.resource	= &es600_gpio_vbus_resource,
	.dev		= {
		.platform_data = &es600_gpio_vbus_pdata,
	},
};
/*
static struct s3c2443_phy_mach_info es600_gpio_vbus_pdata = {
	.gpio_vbus		= ES600_HSUDC_GPIO_VBUS,
};

static struct platform_device es600_gpio_vbus = {
	.name		= "s3c2443-phy",
	.id		= -1,
//	.num_resources	= 1,
//	.resource	= &es600_gpio_vbus_resource,
	.dev		= {
		.platform_data = &es600_gpio_vbus_pdata,
	},
};
*/

/*
 * High-speed usb device controller
 */

static void es600_hsudc_gpio_init(void)
{
//FIXME: determine correct init
	s3c_gpio_setpull(ES600_TPS650240_GPIO_ENLDO12, S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(S3C2410_GPG(1), S3C_GPIO_PULL_NONE);
	s3c2410_modify_misccr(S3C2416_MISCCR_SEL_SUSPND, 0);

	/* USB ID-pin handling */
	gpio_request(ES600_HSUDC_GPIO_ID, "usb id");
	gpio_direction_output(ES600_HSUDC_GPIO_ID, 1);
}

static void es600_hsudc_gpio_uninit(void)
{
	/* USB ID-pin handling */
	gpio_set_value(ES600_HSUDC_GPIO_ID, 0);
	gpio_free(ES600_HSUDC_GPIO_ID);

//FIXME: determine correct uninit
	s3c2410_modify_misccr(S3C2416_MISCCR_SEL_SUSPND, 1);
	s3c_gpio_setpull(ES600_TPS650240_GPIO_ENLDO12, S3C_GPIO_PULL_NONE);
}

static struct s3c24xx_hsudc_platdata es600_hsudc_platdata = {
	.epnum = 9,
	.gpio_init = es600_hsudc_gpio_init,
	.gpio_uninit = es600_hsudc_gpio_uninit,
};

/*
 * Battery
 */

static const struct s3c_adc_bat_thresh bat_lut_noac[] = {
	{ .volt = 4108, .cur = 0, .level = 100},
	{ .volt = 4030, .cur = 0, .level = 90},
	{ .volt = 3953, .cur = 0, .level = 80},
	{ .volt = 3881, .cur = 0, .level = 70},
	{ .volt = 3831, .cur = 0, .level = 60},
	{ .volt = 3782, .cur = 0, .level = 50},
	{ .volt = 3736, .cur = 0, .level = 40},
	{ .volt = 3691, .cur = 0, .level = 30},
	{ .volt = 3643, .cur = 0, .level = 20},
	{ .volt = 3590, .cur = 0, .level = 10},
	{ .volt = 3540, .cur = 0, .level = 0},
};

/* always (0092) above noac value
 * the real values are still missing
 */
static const struct s3c_adc_bat_thresh bat_lut_acin[] = {
	{ .volt = 4200, .cur = 0, .level = 100},
	{ .volt = 4122, .cur = 0, .level = 90},
	{ .volt = 4054, .cur = 0, .level = 80},
	{ .volt = 3973, .cur = 0, .level = 70},
	{ .volt = 3923, .cur = 0, .level = 60},
	{ .volt = 3874, .cur = 0, .level = 50},
	{ .volt = 3828, .cur = 0, .level = 40},
	{ .volt = 3783, .cur = 0, .level = 30},
	{ .volt = 3735, .cur = 0, .level = 20},
	{ .volt = 3682, .cur = 0, .level = 10},
	{ .volt = 3632, .cur = 0, .level = 0},
};

static struct s3c_adc_bat_pdata es600_bat_info = {
	.gpio_charge_finished = ES600_BQ24075_GPIO_NCHG,
	.lut_noac = bat_lut_noac,
	.lut_noac_cnt = ARRAY_SIZE(bat_lut_noac),
	.lut_acin = bat_lut_acin,
	.lut_acin_cnt = ARRAY_SIZE(bat_lut_acin),
	.volt_channel = 0,
	.current_channel = 1, //FIXME: according to Asus there is no current level readable
	.volt_mult = 1000,
	.current_mult = 0, //FIXME: pull current vals to 0, as value is not available
	.internal_impedance = 0, //FIXME: pull current vals to 0, as value is not available
	.volt_samples = 5,
};

static struct platform_device es600_battery = {
	.name             = "s3c-adc-battery",
	.id               = -1,
	.dev = {
		.parent = &s3c_device_adc.dev,
		.platform_data = &es600_bat_info,
	},
};

/*
 * pda-power to wrap everything.
 * Uses otg-utils for is_usb_online.
 */


/*
 * Detect if ac adapter is pluggd in.
 * To do this on the Qisda ES600 line the state of the D+ and D- lines is
 * checked. If both are 1 the ac adapter is connected.
 * FIXME: duplicating the internal hsudc regs and reading their status
 * without claiming the resource is probably bad => find a better solution.
 */

static int es600_is_ac_online(void)
{
	u32 sys_status;

//	sys_status = readl(S3C2416_PA_HSUDC + S3C_SSR);
//	printk("sys_status is %d, dp is %d and dm is %d\n", sys_status, sys_status & S3C_SSR_DP, sys_status & S3C_SSR_DM);
//	return gpio_get_value(EGPIO_MAGICIAN_CABLE_STATE_AC);
	return 0;
}

static char *es600_supplicants[] = {
	"main-battery",
};

static struct pda_power_pdata es600_power_supply_info = {
	.is_ac_online    = es600_is_ac_online,
	.supplied_to     = es600_supplicants,
	.num_supplicants = ARRAY_SIZE(es600_supplicants),
};

static struct resource es600_power_resources[] = {
	[0] = {
		.name  = "ac",
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE |
		         IORESOURCE_IRQ_LOWEDGE,
		.start = ES600_HSUDC_IRQ_VBUS,
		.end   = ES600_HSUDC_IRQ_VBUS,
	},
	[1] = {
		.name  = "usb",
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE |
		         IORESOURCE_IRQ_LOWEDGE,
		.start = ES600_HSUDC_IRQ_VBUS,
		.end   = ES600_HSUDC_IRQ_VBUS,
	},
};

static struct platform_device es600_power_supply = {
	.name		= "pda-power",
	.id		= -1,
	.resource	= es600_power_resources,
	.num_resources	= ARRAY_SIZE(es600_power_resources),
	.dev		= {
		.platform_data = &es600_power_supply_info,
	},
};

/*
 * hsmmc - on ES600 it's microSD on hsmmc0 and the internal flash on hsmmc1
 */
static struct s3c_sdhci_platdata es600_hsmmc0_pdata __initdata = {
	.max_width		= 4,
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= ES600_HSMMC0_GPIO_CD,
	.ext_cd_gpio_invert	= 1,
	.host_caps2		= MMC_CAP2_BROKEN_VOLTAGE,
};

static struct s3c_sdhci_platdata es600_hsmmc1_pdata __initdata = {
	.max_width		= 4,
	.cd_type		= S3C_SDHCI_CD_PERMANENT,
	.host_caps2		= MMC_CAP2_BROKEN_VOLTAGE,
};

/*
 * ALC5624
 */

static struct alc5624_platform_data alc5624_data = {
//	.gpio_power = ES600_ALC5624_GPIO_EN,
//	.gpio_reset = ES600_ALC5624_GPIO_RST,
	.mclk = "iis",		/* The MCLK, that is required to make the codec work */
	.spkvdd_mv = 1000,	/* Speaker Vdd in millivolts */
	.hpvdd_mv = 1000,	/* Headphone Vdd in millivolts */
};

/*
 * AUO in-cell touchscreen
 */

static void es600_auots_gpio_init(struct i2c_client *client) {

	gpio_request(ES600_AUOTS_GPIO_RST, "auo-pixcir-reset");
	//1 to activate ts
	gpio_direction_output(ES600_AUOTS_GPIO_RST, 1);

	msleep(400);

	gpio_direction_input(ES600_AUOTS_GPIO_INT);
	s3c_gpio_setpull(ES600_AUOTS_GPIO_INT, S3C_GPIO_PULL_UP);
};

static void es600_auots_gpio_uninit(struct i2c_client *client) {
	s3c_gpio_setpull(ES600_AUOTS_GPIO_INT, S3C_GPIO_PULL_NONE);

	//0 to deactivate ts
	gpio_direction_output(ES600_AUOTS_GPIO_RST, 0);
	gpio_free(ES600_AUOTS_GPIO_RST);
};

static struct auo_pixcir_ts_platdata auo_pixcir_ts_data = {
	.gpio_int	= ES600_AUOTS_GPIO_INT,
	.int_setting	= AUO_PIXCIR_INT_COMP_COORD,
	.init_hw	= es600_auots_gpio_init,
	.exit_hw	= es600_auots_gpio_uninit,
	.x_max		= 800,
	.y_max		= 600,
};

/*
 * Common I2C
 */

static struct s3c2410_platform_i2c es600_i2c0_data __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 400*1000,
	.sda_delay	= S3C2410_IICLC_SDA_DELAY5 | S3C2410_IICLC_FILTER_ON,
};

static struct i2c_board_info es600_i2c0_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("alc5624", 0x18),
		.platform_data = &alc5624_data
	}, {
		I2C_BOARD_INFO("auo_pixcir_ts", 0x5C),
		.platform_data = &auo_pixcir_ts_data,
		.irq = ES600_AUOTS_IRQ_INT
	}, {
		/* referenced in the schematics, not found on current models
		 * probably only on 3G models, as it is placed near it
		 */
		I2C_BOARD_INFO("lm75", 0x49),
	}
};

/*
 * The 3G modem
 */

static struct qisda_h18x_pdata es600_h18x_pdata = {
	.gpio_w_disable = ES600_PCIE_GPIO_W_DISABLE,
};

static struct platform_device es600_device_h18x = {
	.name		  = "qisda_h18x",
	.id		  = -1,
	.dev = {
		.platform_data = &es600_h18x_pdata,
	},
};

static struct s3c2410_hcd_info es600_ohci_info __initdata = {
	.port[0]	= {
		.flags	= S3C_HCDFLG_USED
	},
	.port[1]	= {
		.flags	= S3C_HCDFLG_USED
	},
};



static struct spi_gpio_platform_data es600_spi_gpio_data = {
	.sck	= S3C2410_GPE(13),
	.mosi	= S3C2410_GPE(12),
	.miso	= S3C2410_GPE(11),
	.num_chipselect	= 1,
};

static struct platform_device es600_spi_gpio = {
	.name	= "spi_gpio",
	.id	= 0,
	.dev	= {
		.platform_data	= &es600_spi_gpio_data,
	},
};





static struct platform_device *es600_devices[] __initdata = {
//FIXME: the ohci produces a hang during poweroff, preventing the
//real poweroff and thus draining the battery
//	&s3c_device_ohci,
	&s3c_device_i2c0,
	&s3c_device_adc,
//	&s3c64xx_device_spi0,
	&es600_spi_gpio,

	/* I2S */
	&s3c2443_device_iis,
	&samsung_asoc_dma,

	/* HSMMS */
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,

	&es600_gpio_vbus,

	/* usb device - must come after gpio_vbus */
	&s3c_device_usb_hsudc,

	/* battery */
	&es600_battery,

	/* power supply */
	&es600_power_supply,

	/* 3g */
	&es600_device_h18x,
};

static struct platform_device *es600_idle_indicators[] __initdata = {
	&s3c_device_i2c0,
	&s3c64xx_device_spi0,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
};

/*
 * Poweroff handling.
 * The systems stays running as long as either P_KEEP is 1
 * or the VBUS is powered. Therefore it is not possible
 * to turn of the system while connected to a charger or host.
 */
static void es600_poweroff(void)
{
	int ret;

	/* check if vbus powered and reboot if necessary*/
	if (power_supply_is_system_supplied()) {
		printk(KERN_INFO "ES600: system is supplied by power supply\n");
		printk(KERN_INFO "can't power down, will restart instead");
		kernel_restart(NULL);
	} else {
		printk(KERN_INFO "ES600: power-down");
		ret = gpio_request(ES600_BASE_GPIO_PKEEP, "P_KEEP");
		if (ret)
		{
			printk(KERN_ERR "can't request P_KEEP gpio %d, err: %d\n",
				ES600_BASE_GPIO_PKEEP, ret);
			return;
		}
		ret = gpio_direction_output(ES600_BASE_GPIO_PKEEP, 0);
		if (ret)
		{
			printk(KERN_ERR "can't set P_KEEP gpio %d to low, err: %d\n",
				ES600_BASE_GPIO_PKEEP, ret);
			return;
		}
		gpio_free(ES600_BASE_GPIO_PKEEP);
	}
}

static void __init tmp_init_usbhost(void)
{
	u32 cfg;


printk("trying to enable the usb1.1 phy\n");

	gpio_request(ES600_TPS650240_GPIO_ENLDO12, "usbphy-ldo");
	gpio_direction_output(ES600_TPS650240_GPIO_ENLDO12, 1);

/*
from another developer in linux-arm-kernel:
PHYPWR    0x4C00_0084    0x0
PWRCFG    0x4C00_0060    (1<<4)
URSTCON   0x4C00_0088    (0<<2)|(1<<1)|(1<<0)
URSTCON   0x4C00_0088    (0<<2)|(0<<1)|(0<<0)
PHYCTRL   0x4C00_0080    (0<<3)|(0<<2)|(1<<1)|(1<<0)
UCLKCON   0x4C00_008C    0<<31)|(0<<2)|(1<<1)|(1<<0)
*/

	s3c2410_modify_misccr(S3C2416_MISCCR_SEL_SUSPND, 0);

	cfg = readl(S3C2443_URSTCON);
	cfg |= (S3C2443_URSTCON_HOSTRST | S3C2443_URSTCON_PHYRST);
	writel(cfg, S3C2443_URSTCON);
	mdelay(1);

	cfg = readl(S3C2443_URSTCON);
	cfg &= ~(S3C2443_URSTCON_HOSTRST | S3C2443_URSTCON_PHYRST);
	writel(cfg, S3C2443_URSTCON);

	cfg = readl(S3C2443_PHYCTRL);
	cfg &= ~(S3C2443_PHYCTRL_CLKSEL | S3C2443_PHYCTRL_PLLSEL);
	cfg |= (S3C2443_PHYCTRL_EXTCLK | S3C2443_PHYCTRL_DSPORT);
	writel(cfg, S3C2443_PHYCTRL);

	cfg = readl(S3C2443_PHYPWR);
	cfg &= ~(S3C2443_PHYPWR_FSUSPEND | S3C2443_PHYPWR_PLL_PWRDN |
		S3C2443_PHYPWR_XO_ON | S3C2443_PHYPWR_PLL_REFCLK |
		S3C2443_PHYPWR_ANALOG_PD);
	cfg |= S3C2443_PHYPWR_COMMON_ON;
	writel(cfg, S3C2443_PHYPWR);

	cfg = readl(S3C2443_UCLKCON);
	cfg |= (S3C2443_UCLKCON_DETECT_VBUS | S3C2443_UCLKCON_FUNC_CLKEN |
		S3C2443_UCLKCON_HOST_CLKEN | S3C2443_UCLKCON_TCLKEN);
	writel(cfg, S3C2443_UCLKCON);

}

/* FIXME: move to header */
extern int s3c2443_add_idle_indicators(struct platform_device **pdevs, int num_pdevs);

void __init es600_common_map_io(void)
{
	s3c24xx_init_io(es600_iodesc, ARRAY_SIZE(es600_iodesc));
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(es600_uartcfgs, ARRAY_SIZE(es600_uartcfgs));
}

static void tf06_init(void)
{
	pr_info("Initializing a tf06/sg06/pd06 machine\n");

	auo_pixcir_ts_data.x_max = 800;
	auo_pixcir_ts_data.y_max = 600;
}

static void as09_init(void)
{
	pr_info("Initializing a as09 machine\n");
  
	auo_pixcir_ts_data.x_max = 1024;
	auo_pixcir_ts_data.y_max = 768;
}

static const struct of_dev_auxdata es600_auxdata_lookup[] __initconst = {
//	OF_DEV_AUXDATA("", 0x0, "", NULL),
	{},
};

void __init es600_common_init(void)
{
	int ret;

	/* set platform data for s3c devices */
	s3c_i2c0_set_platdata(&es600_i2c0_data);
	s3c_sdhci0_set_platdata(&es600_hsmmc0_pdata);
	s3c_sdhci1_set_platdata(&es600_hsmmc1_pdata);
	s3c_ohci_set_platdata(&es600_ohci_info);
	s3c24xx_hsudc_set_platdata(&es600_hsudc_platdata);

	/* set tps650240-mode to PWM only */
	ret = gpio_request(ES600_TPS650240_GPIO_MODE, "tps650240-mode");
	if (ret)
		printk(KERN_ERR "can't request tps650240-mode gpio, err %d\n", ret);

/*	ret = gpio_direction_output(ES600_TPS650240_GPIO_MODE, 1);
	if (ret)
		printk(KERN_ERR "can't set tps650240-mode gpio, err %d\n", ret);
*/
	/* register regulators */
	platform_add_devices(es600_regulators, ARRAY_SIZE(es600_regulators));

	/* register poweroff-handler */
	pm_power_off = es600_poweroff;

	i2c_register_board_info(0, es600_i2c0_board_info,
				ARRAY_SIZE(es600_i2c0_board_info));

	if (of_machine_is_compatible("qisda,tf06") ||
	    of_machine_is_compatible("qisda,sg06") ||
	    of_machine_is_compatible("qisda,pd06"))
		tf06_init();
	else if(of_machine_is_compatible("qisda,as09"))
		as09_init();
	else
		pr_warn("no compatible machine found, using default values\n");

	/* add stuff from devicetree */
	of_platform_populate(NULL, of_default_bus_match_table,
			     es600_auxdata_lookup, NULL);

	platform_add_devices(es600_devices, ARRAY_SIZE(es600_devices));
//tmp_init_usbhost();
}

static void __init es600_common_idle(void)
{
	s3c2443_add_idle_indicators(es600_idle_indicators,
				    ARRAY_SIZE(es600_idle_indicators));
}
/* arch_initcall inits the domains, so use subsys_initcall */
subsys_initcall(es600_common_idle);

void __init es600_spi_init(struct spi_board_info *board_info, int num_board_info)
{
	/* claim and init CS and TI-sn74cbtlv3257 gpios */
	gpio_request(ES600_HSSPI_GPIO_MUX, "SPI_MUX");

//	gpio_direction_output(ES600_HSSPI_GPIO_CS0, 1);
	gpio_direction_output(ES600_HSSPI_GPIO_MUX, 0);

	s3c64xx_spi0_set_platdata(NULL, 2, num_board_info);

	spi_register_board_info(board_info, num_board_info);
}

/*
 * The SPI line is shared between both devices. A TI-sn74cbtlv3257 is used to
 * switch between the two SPI targets. (GPH6 == 0: wlan, GPH6 == 1: tcon flash)
 * GPL13 is the normal chip-select of the HSSPI controller.
 * Chip-select concurrency handling is done in the spi-s3c64xx driver.
 * The wlan type differs (marvell 8686 and mtk5921) but the chip-select
 * handling is the same for all variants.
 */
void es600_spi_set_cs(unsigned line, int high)
{
	if (line != 0 && line != 1) {
		printk(KERN_ERR "es600: unsupported spi line %d\n", line);
		return;
	}

	/* set the TI-sn74cbtlv3257 to the correct line */
	gpio_set_value(ES600_HSSPI_GPIO_MUX, line);

	/* set the chip select */
	gpio_set_value(ES600_HSSPI_GPIO_CS0, high);
};
