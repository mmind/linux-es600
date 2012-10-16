/*
 * Glue driver for AUOK190X-EPDs on Qisda ES600 ereaders
 *
 * Copyright (C) 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * based on
 *
 * am300epd.c
 *
 * Copyright (C) 2008, Jaya Kumar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This driver is written to be used with the AUO-K190X display controllers.
 * on the Qisda ES600/ES900 platform with a Sipix EPD based on a S3C2416 SoC.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <video/auo_k190xfb.h>
#include <video/es600_epd.h>

#include <video/samsung_fimd.h>

#include "auo_k190x.h"

//move to fb header

/* I80IFCONAx */
#define I80IFCONAx_LCD_CS_SETUP(x)		(((x) & 0xf) << 16)
#define I80IFCONAx_LCD_WR_SETUP(x)		(((x) & 0xf) << 12)
#define I80IFCONAx_LCD_WR_ACT(x)		(((x) & 0xf) << 8)
#define I80IFCONAx_LCD_WR_HOLD(x)		(((x) & 0xf) << 4)
#define I80IFCONAx_RSPOL			(1 << 2)
#define I80IFCONAx_SUCCEUP_2443			(1 << 1)
#define I80IFCONAx_I80IFEN			(1 << 0)

/* SIFCCON0 from S3C64xx onward*/
#define SIFCCON0_SYS_RS_CON			(1 << 5)
#define SIFCCON0_SYS_CS0_CON			(1 << 4)
#define SIFCCON0_SYS_CS1_CON			(1 << 3)
#define SIFCCON0_SYS_OE_CON			(1 << 2)
#define SIFCCON0_SYS_WE_CON			(1 << 1)
#define SIFCCON0_SCOMEN				(1 << 0)

/* SIFCCON0 bits s3c2443/s3c2416/s3c2450 */
#define SIFCCON0_SYS_CS1_2443			(1 << 9)
#define SIFCCON0_SYS_CS0_2443			(1 << 8)
#define SIFCCON0_SYS_OE_2443			(1 << 7)
#define SIFCCON0_SYS_WE_2443			(1 << 6)
#define SIFCCON0_SYS_RS_2443			(1 << 1)


/* S3C2416 specific registers */
#define S3C_I80IFCONA0		0x130
#define S3C_I80IFCONA1		0x134
#define S3C_SIFCCON0		0x13C
#define S3C_SIFCCON1		0x140
#define S3C_SIFCCON2		0x144

struct es600_epd {
	struct platform_device		*epd_device;
	void __iomem			*regs;
};

static struct es600_epd *the_controller;

static void es600_epd_queue_work(struct auok190xfb_par *par)
{
	unsigned long delay = HZ / 16;

	queue_delayed_work(system_wq, &par->work, delay);
}

static void es600_epd_work(struct work_struct *work)
{
	struct auok190xfb_par *par =
		container_of(work, struct auok190xfb_par, work.work);

	if(gpio_get_value(par->board->gpio_nbusy))
		wake_up(&par->waitq);
	else
		es600_epd_queue_work(par);
}

static int es600_epd_setup_irq(struct fb_info *info)
{
	struct auok190xfb_par *par = info->par;

	/* the busy GPIO of the AUO-K190x is connected to GPB2
	 * which doesn't support interrupts, so use delayed work.
	 */
	INIT_DELAYED_WORK(&par->work, es600_epd_work);

	queue_delayed_work(system_wq, &par->work, 0);

	return 0;
}

static int es600_epd_wait_for_rdy(struct auok190xfb_par *par)
{
	es600_epd_queue_work(par);

	wait_event_timeout(par->waitq,
			   gpio_get_value(par->board->gpio_nbusy), HZ * 3);

	if (unlikely(!gpio_get_value(par->board->gpio_nbusy))) {
		dev_warn(par->info->device, "controller hang, do recovery\n");
		dump_stack();
		par->recover(par);
	}
	return 0;
}

static int es600_epd_init_regs(struct es600_epd *epd)
{
	int tmp;

	/* init VIDCON0 */
	tmp = __raw_readl(epd->regs + VIDCON0);
	tmp &= ~(VIDCON0_VIDOUT_MASK_2443 | VIDCON0_PNRMODE_MASK_2443 |
		 VIDCON0_CLKSEL_MASK);
	tmp |= VIDCON0_VIDOUT_I80_LDI0_2443 | VIDCON0_PNRMODE_RGB_2443 |
	       VIDCON0_VLCKFREE | VIDCON0_CLKDIR | VIDCON0_CLKSEL_HCLK;
	__raw_writel(tmp, epd->regs + VIDCON0);

	/* FIXME: determince clkval (1) */
	tmp = __raw_readl(epd->regs + VIDCON0);
	tmp &= ~VIDCON0_CLKVAL_F_MASK_2443;
	tmp |= VIDCON0_CLKVAL_F(1);
	__raw_writel(tmp, epd->regs + VIDCON0);

	/* init SYSIFCON */
	tmp = I80IFCONAx_RSPOL | I80IFCONAx_SUCCEUP_2443 | I80IFCONAx_I80IFEN;
	__raw_writel(tmp, epd->regs + S3C_I80IFCONA0);

	return 0;
}

static int es600_epd_init_board(struct auok190xfb_par *par)
{
	struct es600_epd *epd = the_controller;
	int ret;

	/* configure gpios to SFN(2), for the I80 controller */
	s3c_gpio_cfgall_range(S3C2410_GPC(0), 3,
		      S3C_GPIO_SFN(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgall_range(S3C2410_GPC(4), 1,
		      S3C_GPIO_SFN(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgall_range(S3C2410_GPC(8), 16,
		      S3C_GPIO_SFN(2), S3C_GPIO_PULL_NONE);

	/* the handling of the busy gpio is system-specific */
	ret = gpio_request(par->board->gpio_nbusy, "auok1900 n_busy");
	if (ret)
		return ret;

	ret = gpio_direction_input(par->board->gpio_nbusy);
	if (ret) {
		gpio_free(par->board->gpio_nbusy);
		return ret;
	}

	return es600_epd_init_regs(epd);
}

static void es600_epd_cleanup(struct auok190xfb_par *par)
{
	struct es600_epd *epd = the_controller;

	cancel_delayed_work_sync(&par->work);

	__raw_writel(0, epd->regs + S3C_I80IFCONA0);

	gpio_free(par->board->gpio_nbusy);
}

static u16 es600_epd_get_hdb(struct auok190xfb_par *par)
{
	struct es600_epd *epd = the_controller;

	return __raw_readl(epd->regs + S3C_SIFCCON2) & 0xffff;
}

static void es600_epd_set_hdb(struct auok190xfb_par *par, u16 data)
{
	struct es600_epd *epd = the_controller;

	__raw_writel(data & 0xffff, epd->regs + S3C_SIFCCON1);
}

static void es600_epd_set_ctl(struct auok190xfb_par *par, unsigned char bit,
				u8 state)
{
	struct es600_epd *epd = the_controller;
	int reg;

	switch (bit) {
	case AUOK190X_I80_CS:
		if (!state) {
			reg = __raw_readl(epd->regs + S3C_SIFCCON0);
			reg |= SIFCCON0_SCOMEN;
			__raw_writel(reg, epd->regs + S3C_SIFCCON0);

			reg = __raw_readl(epd->regs + S3C_SIFCCON0);
			reg |= SIFCCON0_SYS_CS0_2443;
			__raw_writel(reg, epd->regs + S3C_SIFCCON0);
		} else {
			reg = __raw_readl(epd->regs + S3C_SIFCCON0);
			reg &= ~SIFCCON0_SYS_CS0_2443;
			__raw_writel(reg, epd->regs + S3C_SIFCCON0);

			reg = __raw_readl(epd->regs + S3C_SIFCCON0);
			reg &= ~SIFCCON0_SCOMEN;
			__raw_writel(reg, epd->regs + S3C_SIFCCON0);
		}
		break;
	case AUOK190X_I80_DC:
		reg = __raw_readl(epd->regs + S3C_SIFCCON0);
		if (state)
			reg |= SIFCCON0_SYS_RS_2443; /* param mode */
		else
			reg &= ~SIFCCON0_SYS_RS_2443; /* cmd mode */
		__raw_writel(reg, epd->regs + S3C_SIFCCON0);
		break;
	case AUOK190X_I80_WR:
		reg = __raw_readl(epd->regs + S3C_SIFCCON0);
		if (!state)
			reg |= SIFCCON0_SYS_WE_2443;
		else
			reg &= ~SIFCCON0_SYS_WE_2443;
		__raw_writel(reg, epd->regs + S3C_SIFCCON0);
		break;
	case AUOK190X_I80_OE:
		reg = __raw_readl(epd->regs + S3C_SIFCCON0);
		if (!state)
			reg |= SIFCCON0_SYS_OE_2443;
		else
			reg &= ~SIFCCON0_SYS_OE_2443;
		__raw_writel(reg, epd->regs + S3C_SIFCCON0);
		break;
	}
}

#ifdef CONFIG_PM
static int es600_epd_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct es600_epd *epd = platform_get_drvdata(pdev);

	__raw_writel(0, epd->regs + S3C_I80IFCONA0);

	return 0;
}

static int es600_epd_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct es600_epd *epd = platform_get_drvdata(pdev);

	/* reinit the framebuffer registers */
	return es600_epd_init_regs(epd);
}
#endif

struct dev_pm_ops es600_epd_pm = {
        SET_SYSTEM_SLEEP_PM_OPS(es600_epd_suspend, es600_epd_resume)
};

/*
 * Handlers for alternative sources of platform_data
 */

#ifdef CONFIG_OF
/*
 * Translate OpenFirmware node properties into platform_data
 */
static struct es600_epd_pdata * __devinit
es600_epd_get_devtree_pdata(struct device *dev)
{
	struct device_node *node;
	struct es600_epd_pdata *pdata;
	int error;

	node = dev->of_node;
	if (!node) {
		error = -ENODEV;
		goto err_out;
	}

	pdata = kzalloc(sizeof(struct es600_epd_pdata), GFP_KERNEL);
	if (!pdata) {
		error = -ENOMEM;
		goto err_out;
	}

	pdata->driver = of_get_property(node, "driver", NULL);

	if(of_property_read_u32(node, "resolution", &pdata->resolution))
		pdata->resolution = 0;

	if(of_property_read_u32(node, "quirks", &pdata->quirks))
		pdata->quirks = 0;

	if(of_property_read_u32(node, "fps", &pdata->fps))
		pdata->fps = 1;

	pdata->gpio_nrst = of_get_gpio(node, 0);
	if (!gpio_is_valid(pdata->gpio_nrst)) {
		dev_err(dev, "invalid gpio[0]\n");
		error = -EINVAL;
		goto err_free_pdata;
	}

	pdata->gpio_nbusy = of_get_gpio(node, 1);
	if (!gpio_is_valid(pdata->gpio_nbusy)) {
		dev_err(dev, "invalid gpio[1]\n");
		error = -EINVAL;
		goto err_free_pdata;
	}

	pdata->gpio_nsleep = of_get_gpio(node, 2);
	if (!gpio_is_valid(pdata->gpio_nsleep)) {
		dev_err(dev, "invalid gpio[2]\n");
		error = -EINVAL;
		goto err_free_pdata;
	}

	return pdata;

err_free_pdata:
	kfree(pdata);
err_out:
	return ERR_PTR(error);
}

static struct of_device_id es600_epd_of_match[] = {
	{ .compatible = "es600-epd", },
	{ },
};
MODULE_DEVICE_TABLE(of, es600_epd_of_match);

#else

static inline struct es600_epd_pdata *
es600_epd_get_devtree_pdata(struct device *dev)
{
	return ERR_PTR(-ENODEV);
}

#endif

static int __devinit es600_epd_probe(struct platform_device *pdev)
{
	struct es600_epd_pdata *pdata = pdev->dev.platform_data;
	struct auok190x_board *board;
	struct es600_epd *epd;
	struct resource *res;
	int ret;

	if (!pdata) {
		pdata = es600_epd_get_devtree_pdata(&pdev->dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}

	if (pdev->id != -1)
		return -EINVAL;

	epd = devm_kzalloc(&pdev->dev, sizeof(struct es600_epd), GFP_KERNEL);
	if (!epd)
		return -ENOMEM;

	platform_set_drvdata(pdev, epd);

	/* map registers, res is checked by devm_request_and_ioremap */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	epd->regs = devm_request_and_ioremap(&pdev->dev, res);
	if (!epd->regs)
		return -EBUSY;

	board = devm_kzalloc(&pdev->dev, sizeof(struct auok190x_board), GFP_KERNEL);
	if (!epd)
		return -ENOMEM;

	/* init the board information for auo_k190x */
	board->init		= es600_epd_init_board,
	board->cleanup		= es600_epd_cleanup,
	board->set_hdb		= es600_epd_set_hdb,
	board->get_hdb		= es600_epd_get_hdb,
	board->set_ctl		= es600_epd_set_ctl,
	board->wait_for_rdy	= es600_epd_wait_for_rdy,
	board->setup_irq	= es600_epd_setup_irq,
	board->resolution	= pdata->resolution;
	board->quirks		= pdata->quirks;
	board->fps		= pdata->fps;
	board->gpio_nsleep	= pdata->gpio_nsleep;
	board->gpio_nrst	= pdata->gpio_nrst;
	board->gpio_nbusy	= pdata->gpio_nbusy;

	the_controller = epd;

	/* request the platform independent driver */
	ret = request_module(pdata->driver);
	if (ret) {
		dev_err(&pdev->dev, "could not load module %s\n", pdata->driver);
		return ret;
	}

	epd->epd_device = platform_device_alloc(pdata->driver, -1);
	if (!epd->epd_device) {
		dev_err(&pdev->dev, "failed to allocate platform device\n");
		return -ENOMEM;
	}

	platform_device_add_data(epd->epd_device, board,
					sizeof(struct auok190x_board));

	ret = platform_device_add(epd->epd_device);
	if (ret) {
		platform_device_put(epd->epd_device);
		return ret;
	}

	return 0;
}

static int __devexit es600_epd_remove(struct platform_device *pdev)
{
	struct es600_epd *epd = platform_get_drvdata(pdev);

	platform_device_del(epd->epd_device);
	platform_device_put(epd->epd_device);

	return 0;
}

static struct platform_driver es600_epd_driver = {
	.probe		= es600_epd_probe,
	.remove		= __devexit_p(es600_epd_remove),
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "es600-epd",
		.pm	= &es600_epd_pm,
		.of_match_table = of_match_ptr(es600_epd_of_match),
	},
};
module_platform_driver(es600_epd_driver);

MODULE_DESCRIPTION("AUO-K190X board-driver for the Qisda ES600 ereader family");
MODULE_AUTHOR("Heiko Stuebner <heiko@sntech.de>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:es600-epd");
