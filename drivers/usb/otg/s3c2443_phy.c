/*
 * s3c2443_phy.c - phy driver for S3C2443 and up including GPIO VBUS sensing
 *
 * Copyright (c) 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * based on gpio-vbus.c
 *
 * Copyright (c) 2008 Philipp Zabel <philipp.zabel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/usb.h>
#include <linux/workqueue.h>

#include <linux/regulator/consumer.h>

#include <linux/usb/gadget.h>
#include <linux/platform_data/s3c2443_phy.h>
#include <linux/usb/otg.h>

//for misccr_sel_suspnd
#include <mach/regs-gpio.h>

struct s3c2443_phy_data {
	struct usb_phy		phy;
	struct device          *dev;
	struct regulator       *vbus_draw;
	int			vbus_draw_enabled;
	unsigned		mA;
	struct work_struct	work;
};


/*
 * This driver relies on "both edges" triggering.  VBUS has 100 msec to
 * stabilize, so the peripheral controller driver may need to cope with
 * some bouncing due to current surges (e.g. charging local capacitance)
 * and contact chatter.
 *
 * REVISIT in desperate straits, toggling between rising and falling
 * edges might be workable.
 */
#define VBUS_IRQ_FLAGS \
	( IRQF_SAMPLE_RANDOM | IRQF_SHARED \
	| IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING )


static inline struct s3c2443_phy_data *to_s3c2443_phy_data(struct usb_phy *phy)
{
	return container_of(phy, struct s3c2443_phy_data, phy);
}

/* interface to regulator framework */
static void set_vbus_draw(struct s3c2443_phy_data *s3c2443_phy, unsigned mA)
{
	struct regulator *vbus_draw = s3c2443_phy->vbus_draw;
	int enabled;

	if (!vbus_draw)
		return;

	enabled = s3c2443_phy->vbus_draw_enabled;
	if (mA) {
		regulator_set_current_limit(vbus_draw, 0, 1000 * mA);
		if (!enabled) {
			regulator_enable(vbus_draw);
			s3c2443_phy->vbus_draw_enabled = 1;
		}
	} else {
		if (enabled) {
			regulator_disable(vbus_draw);
			s3c2443_phy->vbus_draw_enabled = 0;
		}
	}
	s3c2443_phy->mA = mA;
}

static int is_vbus_powered(struct s3c2443_phy_mach_info *pdata)
{
	int vbus;

	/* if no vbus gpio is set, we assume it's not powered */
	if (!gpio_is_valid(pdata->gpio_vbus))
		return 0;

	vbus = gpio_get_value(pdata->gpio_vbus);

	if (pdata->gpio_vbus_inverted)
		vbus = !vbus;

	return vbus;
}

static void s3c2443_phy_work(struct work_struct *work)
{
	struct s3c2443_phy_data *s3c2443_phy =
		container_of(work, struct s3c2443_phy_data, work);
	struct s3c2443_phy_mach_info *pdata = s3c2443_phy->dev->platform_data;
	int status;

	if (!s3c2443_phy->phy.otg->gadget)
		return;

	if (is_vbus_powered(pdata)) {
		status = USB_EVENT_VBUS;
		s3c2443_phy->phy.state = OTG_STATE_B_PERIPHERAL;
		s3c2443_phy->phy.last_event = status;
		usb_gadget_vbus_connect(s3c2443_phy->phy.otg->gadget);

		/* drawing a "unit load" is *always* OK, except for OTG */
		set_vbus_draw(s3c2443_phy, 100);

		atomic_notifier_call_chain(&s3c2443_phy->phy.notifier,
					   status, s3c2443_phy->phy.otg->gadget);
	} else {
		set_vbus_draw(s3c2443_phy, 0);

		usb_gadget_vbus_disconnect(s3c2443_phy->phy.otg->gadget);
		status = USB_EVENT_NONE;
		s3c2443_phy->phy.state = OTG_STATE_B_IDLE;
		s3c2443_phy->phy.last_event = status;

		atomic_notifier_call_chain(&s3c2443_phy->phy.notifier,
					   status, s3c2443_phy->phy.otg->gadget);
	}
}

/* VBUS change IRQ handler */
static irqreturn_t s3c2443_phy_vbus_irq(int irq, void *data)
{
	struct platform_device *pdev = data;
	struct s3c2443_phy_mach_info *pdata = pdev->dev.platform_data;
	struct s3c2443_phy_data *s3c2443_phy = platform_get_drvdata(pdev);
	struct usb_otg *otg = s3c2443_phy->phy.otg;

	dev_dbg(&pdev->dev, "VBUS %s (gadget: %s)\n",
		is_vbus_powered(pdata) ? "supplied" : "inactive",
		otg->gadget ? otg->gadget->name : "none");

	if (otg->gadget)
		schedule_work(&s3c2443_phy->work);

	return IRQ_HANDLED;
}

/* OTG transceiver interface */

static int s3c2443_phy_set_host(struct usb_otg *otg, struct usb_bus *host)
{
	printk("s3c2443_phy_set_host\n");
	return 0;
 
}

/* bind/unbind the peripheral controller */
static int s3c2443_phy_set_peripheral(struct usb_otg *otg,
					struct usb_gadget *gadget)
{
	struct s3c2443_phy_data *s3c2443_phy = to_s3c2443_phy_data(otg->phy);
	struct platform_device *pdev = to_platform_device(s3c2443_phy->dev);
	struct s3c2443_phy_mach_info *pdata = s3c2443_phy->dev->platform_data;
	int gpio, irq;

	irq = gpio_to_irq(pdata->gpio_vbus);

	if (!gadget) {
		dev_dbg(&pdev->dev, "unregistering gadget '%s'\n",
			otg->gadget->name);

		set_vbus_draw(s3c2443_phy, 0);

		usb_gadget_vbus_disconnect(otg->gadget);
		otg->phy->state = OTG_STATE_UNDEFINED;

		otg->gadget = NULL;
		return 0;
	}

	otg->gadget = gadget;
	dev_dbg(&pdev->dev, "registered gadget '%s'\n", gadget->name);

	/* initialize connection state */
	s3c2443_phy_vbus_irq(irq, pdev);
	return 0;
}

int s3c2443_phy_reset(struct usb_phy *x)
{
	printk("s3c2443_phy_init\n");
	return 0;
}

void s3c2443_phy_shutdown(struct usb_phy *x)
{
	printk("s3c2443_phy_shutdown\n");
}

/* effective for B devices, ignored for A-peripheral */
static int s3c2443_phy_set_power(struct usb_phy *phy, unsigned mA)
{
	struct s3c2443_phy_data *s3c2443_phy = to_s3c2443_phy_data(phy);

	if (phy->state == OTG_STATE_B_PERIPHERAL)
		set_vbus_draw(s3c2443_phy, mA);
	return 0;
}

/* for non-OTG B devices: set/clear transceiver suspend mode */
static int s3c2443_phy_set_suspend(struct usb_phy *phy, int suspend)
{
	struct s3c2443_phy_data *s3c2443_phy = to_s3c2443_phy_data(phy);
	int ret;

	/* draw max 0 mA from vbus in suspend mode; or the previously
	 * recorded amount of current if not suspended
	 *
	 * NOTE: high powered configs (mA > 100) may draw up to 2.5 mA
	 * if they're wake-enabled ... we don't handle that yet.
	 */
	if (suspend) {
		dev_dbg(s3c2443_phy->dev, "suspending transceiver\n");
		ret = s3c2443_phy_set_power(phy, 0);
		s3c2410_modify_misccr(S3C2416_MISCCR_SEL_SUSPND, 1);
	} else {
		dev_dbg(s3c2443_phy->dev, "resuming transceiver\n");
		s3c2410_modify_misccr(S3C2416_MISCCR_SEL_SUSPND, 0);
		ret = s3c2443_phy_set_power(phy, s3c2443_phy->mA);
	}

	return ret;
}

/* platform driver interface */

static int __init s3c2443_phy_probe(struct platform_device *pdev)
{
	struct s3c2443_phy_mach_info *pdata = pdev->dev.platform_data;
	struct s3c2443_phy_data *s3c2443_phy;
	struct resource *res;
	int err, irq = 0;

	if (!pdata)
		return -EINVAL;

	s3c2443_phy = devm_kzalloc(&pdev->dev, sizeof(struct s3c2443_phy_data),
				   GFP_KERNEL);
	if (!s3c2443_phy)
		return -ENOMEM;

	s3c2443_phy->phy.otg = devm_kzalloc(&pdev->dev, sizeof(struct usb_otg),
					    GFP_KERNEL);
	if (!s3c2443_phy->phy.otg)
		return -ENOMEM;

	platform_set_drvdata(pdev, s3c2443_phy);
	s3c2443_phy->dev = &pdev->dev;
	s3c2443_phy->phy.label = "s3c2443-phy";
	s3c2443_phy->phy.init = s3c2443_phy_reset;
	s3c2443_phy->phy.shutdown = s3c2443_phy_shutdown;
	s3c2443_phy->phy.set_power = s3c2443_phy_set_power;
	s3c2443_phy->phy.set_suspend = s3c2443_phy_set_suspend;
	s3c2443_phy->phy.state = OTG_STATE_UNDEFINED;

	s3c2443_phy->phy.otg->phy = &s3c2443_phy->phy;
	s3c2443_phy->phy.otg->set_host = s3c2443_phy_set_host;
	s3c2443_phy->phy.otg->set_peripheral = s3c2443_phy_set_peripheral;

	if (gpio_is_valid(pdata->gpio_vbus)) {
		err = gpio_request(pdata->gpio_vbus, "vbus_detect");
		if (err) {
			dev_err(&pdev->dev, "can't request vbus gpio %d, err: %d\n",
				pdata->gpio_vbus, err);
			goto err_gpio;
		}
		gpio_direction_input(pdata->gpio_vbus);

/*	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res) {
		irq = res->start;
		res->flags &= IRQF_TRIGGER_MASK;
		res->flags |= IRQF_SAMPLE_RANDOM | IRQF_SHARED;
	} else*/
		irq = gpio_to_irq(pdata->gpio_vbus);

		err = request_irq(irq, s3c2443_phy_vbus_irq, VBUS_IRQ_FLAGS,
				  "vbus_detect", pdev);
		if (err) {
			dev_err(&pdev->dev, "can't request irq %i, err: %d\n",
				irq, err);
			goto err_irq;
		}
	}

	ATOMIC_INIT_NOTIFIER_HEAD(&s3c2443_phy->phy.notifier);

	INIT_WORK(&s3c2443_phy->work, s3c2443_phy_work);

	s3c2443_phy->vbus_draw = regulator_get(&pdev->dev, "vbus_draw");
	if (IS_ERR(s3c2443_phy->vbus_draw)) {
		dev_dbg(&pdev->dev, "can't get vbus_draw regulator, err: %ld\n",
			PTR_ERR(s3c2443_phy->vbus_draw));
		s3c2443_phy->vbus_draw = NULL;
	}

	/* only active when a gadget is registered */
	err = usb_set_transceiver(&s3c2443_phy->phy);
	if (err) {
		dev_err(&pdev->dev, "can't register transceiver, err: %d\n",
			err);
		goto err_otg;
	}

	return 0;
err_otg:
	if (gpio_is_valid(pdata->gpio_vbus))
		free_irq(irq, &pdev->dev);
err_irq:
	gpio_free(pdata->gpio_vbus);
err_gpio:
	platform_set_drvdata(pdev, NULL);
	return err;
}

static int __exit s3c2443_phy_remove(struct platform_device *pdev)
{
	struct s3c2443_phy_data *s3c2443_phy = platform_get_drvdata(pdev);
	struct s3c2443_phy_mach_info *pdata = pdev->dev.platform_data;

	regulator_put(s3c2443_phy->vbus_draw);

	usb_set_transceiver(NULL);

	free_irq(gpio_to_irq(pdata->gpio_vbus), &pdev->dev);
	gpio_free(pdata->gpio_vbus);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

/* NOTE:  the s3c2443-phy device may *NOT* be hotplugged */

static struct platform_driver s3c2443_phy_driver = {
	.driver = {
		.name  = "s3c2443-phy",
		.owner = THIS_MODULE,
	},
	.remove  = __exit_p(s3c2443_phy_remove),
};

static int __init s3c2443_phy_init(void)
{
	return platform_driver_probe(&s3c2443_phy_driver, s3c2443_phy_probe);
}
module_init(s3c2443_phy_init);

static void __exit s3c2443_phy_exit(void)
{
	platform_driver_unregister(&s3c2443_phy_driver);
}
module_exit(s3c2443_phy_exit);

MODULE_ALIAS("platform:s3c2443-phy");

MODULE_DESCRIPTION("simple GPIO controlled OTG transceiver driver");
MODULE_AUTHOR("Heiko Stuebner <heiko@sntech.de>");
MODULE_LICENSE("GPL");
