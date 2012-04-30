/*
 * Serial driver for Qisda H18[s/d/t] USB WWANs
 *
 * Copyright (c) 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * based on
 * Qualcomm Serial USB driver
 *
 * Copyright (c) 2008 QUALCOMM Incorporated.
 * Copyright (c) 2009 Greg Kroah-Hartman <gregkh@suse.de>
 * Copyright (c) 2009 Novell Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 */

#define DEBUG
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/usb/serial.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/platform_data/qisda_h18x.h>
#include <linux/regulator/consumer.h>
#include "usb-wwan.h"




static const struct usb_device_id id_table[] = {
	{USB_DEVICE(0x05c6, 0x9211)},	/* Acer Gobi QDL device */
	{ }				/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver h18x_usb_driver = {
	.name			= "qisda_h18x_usb",
	.probe			= usb_serial_probe,
	.disconnect		= usb_serial_disconnect,
	.id_table		= id_table,
	.suspend		= usb_serial_suspend,
	.resume			= usb_serial_resume,
	.supports_autosuspend	= true,
};

static int h18x_usb_probe(struct usb_serial *serial, const struct usb_device_id *id)
{
	struct usb_wwan_intf_private *data;
	struct usb_host_interface *intf = serial->interface->cur_altsetting;
	int retval = -ENODEV;
	__u8 nintf;
	__u8 ifnum;

	printk("%s", __func__);

	nintf = serial->dev->actconfig->desc.bNumInterfaces;
	printk("Num Interfaces = %d", nintf);
	ifnum = intf->desc.bInterfaceNumber;
	printk("This Interface = %d", ifnum);

	data = kzalloc(sizeof(struct usb_wwan_intf_private),
					 GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	spin_lock_init(&data->susp_lock);

	usb_enable_autosuspend(serial->dev);

	switch (nintf) {
	case 1:
		/* QDL mode */
		/* Gobi 2000 has a single altsetting, older ones have two */
		if (serial->interface->num_altsetting == 2)
			intf = &serial->interface->altsetting[1];
		else if (serial->interface->num_altsetting > 2)
			break;

		if (intf->desc.bNumEndpoints == 2 &&
		    usb_endpoint_is_bulk_in(&intf->endpoint[0].desc) &&
		    usb_endpoint_is_bulk_out(&intf->endpoint[1].desc)) {

			if (serial->interface->num_altsetting == 1) {
				retval = 0; /* Success */
				break;
			}

			retval = usb_set_interface(serial->dev, ifnum, 1);
			if (retval < 0) {
				dev_err(&serial->dev->dev,
					"Could not set interface, error %d\n",
					retval);
				retval = -ENODEV;
				kfree(data);
			}
		}
		break;

	case 3:
	case 4:
		/* Composite mode */
		/* ifnum == 0 is a broadband network adapter */
		if (ifnum == 1) {
			/*
			 * Diagnostics Monitor (serial line 9600 8N1)
			 * Qualcomm DM protocol
			 * use "libqcdm" (ModemManager) for communication
			 */
			printk("Diagnostics Monitor found");
			retval = usb_set_interface(serial->dev, ifnum, 0);
			if (retval < 0) {
				dev_err(&serial->dev->dev,
					"Could not set interface, error %d\n",
					retval);
				retval = -ENODEV;
				kfree(data);
			}
		} else if (ifnum == 2) {
			printk("Modem port found");
			retval = usb_set_interface(serial->dev, ifnum, 0);
			if (retval < 0) {
				dev_err(&serial->dev->dev,
					"Could not set interface, error %d\n",
					retval);
				retval = -ENODEV;
				kfree(data);
			}
		}
		break;

	default:
		dev_err(&serial->dev->dev,
			"unknown number of interfaces: %d\n", nintf);
		kfree(data);
		retval = -ENODEV;
	}

	/* Set serial->private if not returning -ENODEV */
	if (retval != -ENODEV)
		usb_set_serial_data(serial, data);
	return retval;
}

static void h18x_usb_release(struct usb_serial *serial)
{
	struct usb_wwan_intf_private *priv = usb_get_serial_data(serial);

	/* Call usb_wwan release & free the private data allocated in qcprobe */
	usb_wwan_release(serial);
	usb_set_serial_data(serial, NULL);
	kfree(priv);
}

static struct usb_serial_driver h18x_usb_device = {
	.driver = {
		.owner		= THIS_MODULE,
		.name		= "qisda_h18x_usb",
	},
	.description		= "Qisda H18x USB modem",
	.id_table		= id_table,
	.usb_driver		= &h18x_usb_driver,
	.num_ports		= 1,
	.probe			= h18x_usb_probe,
	.open			= usb_wwan_open,
	.close			= usb_wwan_close,
	.write			= usb_wwan_write,
	.write_room		= usb_wwan_write_room,
	.chars_in_buffer	= usb_wwan_chars_in_buffer,
	.attach			= usb_wwan_startup,
	.disconnect		= usb_wwan_disconnect,
	.release		= h18x_usb_release,
#ifdef CONFIG_PM
	.suspend		= usb_wwan_suspend,
	.resume			= usb_wwan_resume,
#endif
};

struct h18x_priv
{
	unsigned int gpio_w_disable;
	struct regulator *regulator;

	struct mutex power_lock;
	bool power;
};

static void h18x_power(struct platform_device *pdev, bool power)
{
	struct h18x_priv *priv = platform_get_drvdata(pdev);

	mutex_lock(&priv->power_lock);

	if (power) {
		regulator_enable(priv->regulator);
		gpio_set_value(priv->gpio_w_disable, 1);
		priv->power = 1;
	} else {
		gpio_set_value(priv->gpio_w_disable, 0);
//		regulator_disable(priv->regulator);
		priv->power = 0;
	}

	mutex_unlock(&priv->power_lock);
}

/*
 * sysfs-attributes
 */

static ssize_t h18x_power_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct h18x_priv *priv = platform_get_drvdata(pdev);

	return sprintf(buf, "%d\n", priv->power);
}

static ssize_t h18x_power_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	unsigned int val;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret)
		return ret;

	if (val > 1)
		val = 1;

	h18x_power(pdev, val);
	return count;
}


static DEVICE_ATTR(power_on, 0644, h18x_power_show, h18x_power_store);

static struct attribute *h18x_sysfs_entries[] = {
	&dev_attr_power_on.attr,
	NULL
};

static struct attribute_group h18x_attr_group = {
	.attrs	= h18x_sysfs_entries,
};


static int __devinit h18x_probe(struct platform_device *pdev)
{
	struct qisda_h18x_pdata *pdata = pdev->dev.platform_data;
	struct h18x_priv *priv;
	int ret;

	priv = kzalloc(sizeof(struct h18x_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->gpio_w_disable = pdata->gpio_w_disable;

	ret = gpio_request(priv->gpio_w_disable, "H18x W_DISABLE");
	if(ret) {
		dev_err(&pdev->dev, "Failed to get w_disable gpio: %d\n", ret);
		goto err_gpio;
	}

	ret = gpio_direction_output(priv->gpio_w_disable, 0);
	if(ret) {
		dev_err(&pdev->dev, "Failed to set w_disable gpio: %d\n", ret);
		goto err_reg;
	}

	priv->regulator = regulator_get(&pdev->dev, "vaux");
	if (IS_ERR(priv->regulator)) {
		ret = PTR_ERR(priv->regulator);
		dev_err(&pdev->dev, "Failed to get regulator: %d\n", ret);
		goto err_reg;
	}

	mutex_init(&priv->power_lock);

	platform_set_drvdata(pdev, priv);

	ret = sysfs_create_group(&pdev->dev.kobj, &h18x_attr_group);
	if (ret)
		goto err_sysfs;

	return 0;

err_sysfs:
	platform_set_drvdata(pdev, NULL);
	regulator_put(priv->regulator);
err_reg:
	gpio_free(priv->gpio_w_disable);
err_gpio:
	kfree(priv);

	return ret;
}

static int __devexit h18x_remove(struct platform_device *pdev)
{
	struct h18x_priv *priv = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &h18x_attr_group);

	h18x_power(pdev, 0);

	platform_set_drvdata(pdev, NULL);
	regulator_put(priv->regulator);
	gpio_free(priv->gpio_w_disable);

	kfree(priv);

	return 0;
}

static struct platform_driver h18x_driver = {
	.probe		= h18x_probe,
	.remove		= __devexit_p(h18x_remove),
	.driver		= {
		.name	= "qisda_h18x",
	},
};

static int __init h18x_init(void)
{
	int ret;

	ret = usb_serial_register(&h18x_usb_device);
	if (ret)
		return ret;

	ret = usb_register(&h18x_usb_driver);
	if (ret)
		goto err_usb;

	ret = platform_driver_register(&h18x_driver);
	if (ret)
		goto err_plat;

	return 0;

err_plat:
	usb_deregister(&h18x_usb_driver);
err_usb:
	usb_serial_deregister(&h18x_usb_device);

	return ret;
}

static void __exit h18x_exit(void)
{
	usb_deregister(&h18x_usb_driver);
	usb_serial_deregister(&h18x_usb_device);

	/* remove platform device last, as it will powerdown the device */
	platform_driver_unregister(&h18x_driver);
}

module_init(h18x_init);
module_exit(h18x_exit);

MODULE_AUTHOR("Heiko Stuebner <heiko@sntech.de>");
MODULE_DESCRIPTION("Qisda H18[s/d/t] serial driver");
MODULE_LICENSE("GPL v2");
