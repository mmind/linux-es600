/*
*	spi-dev.c - spi-bus driver, char device interface
*
*	Copyright (C) 2006 Samsung Electronics Co. Ltd.
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define DEBUG
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include "es600-mt5921-spi-dev.h"
#include <asm/uaccess.h>

#include <linux/spi/spi.h>

/*
 * Bridge driver
 */

static struct spi_device *mt5921_spi;

/*static int mt5921_bridge_spi_read(struct mt5921_bridge_data *chip, s16 *data)
{
	int ret;

	ret = spi_read(chip->spi_dev, (u8 *)&chip->rx, sizeof(chip->rx));
	if (ret < 0) {
		dev_err(&chip->spi_dev->dev, "SPI read error\n");
		return ret;
	}

	*data = be16_to_cpu(chip->rx);

	return ret;
}*/

static int __devinit mt5921_bridge_probe(struct spi_device *spi)
{
	struct mt5921_spi_bridge_platform_data *pdata = spi->dev.platform_data;
	int ret;

	dev_info(&spi->dev, "MT5921 SPI API-bridge\n");
	mt5921_spi = spi;

	if (pdata->setup) {
		ret = pdata->setup(spi);
		if (ret) {
			mt5921_spi = NULL;
			return ret;
		}
	}

	return 0;
}

static int __devexit mt5921_bridge_remove(struct spi_device *spi)
{
	struct mt5921_spi_bridge_platform_data *pdata = spi->dev.platform_data;

	if (pdata->teardown)
		pdata->teardown(spi);

	mt5921_spi = NULL;
	return 0;
}

static struct spi_driver mt5921_bridge_driver = {
	.driver = {
		.name = "mt5921_spi_bridge",
		.owner = THIS_MODULE,
	},
	.probe = mt5921_bridge_probe,
	.remove = __devexit_p(mt5921_bridge_remove),
};

/*
 * Glue to mt5921sta_spi API
 */

#define BUFFER_SIZE	65536	//8192 for perf measurement

static struct spi_dev es600_mt5921_spi;

struct spi_dev *spi_dev_get_by_minor(unsigned index)
{
	pr_debug("mt5921-spi-dev: spi_dev_get_by_minor %d\n", index);
	return &es600_mt5921_spi;
}
EXPORT_SYMBOL(spi_dev_get_by_minor);

int spi_master_recv(struct spi_dev *spi_dev, char *rbuf, int count)
{
	struct spi_msg msg;
	int ret;

	pr_debug("mt5921-spi-dev: spi_master_recv %d\n", count);
return -1;
	if (spi_dev->algo->master_xfer) {
		msg.flags = spi_dev->flags;
		msg.len = count;
		msg.wbuf = NULL;
		msg.rbuf = rbuf;

		if (spi_dev->stop_xfer)
			return -EIO;
		dev_dbg(&spi_dev->dev, "master_recv: reading %d bytes.\n", count);
		if (down_trylock(&spi_dev->bus_lock)) {
			printk("%s() grab bus_lock fail!!!\n", __FUNCTION__);
			down(&spi_dev->bus_lock);
		}
		if (spi_dev->stop_xfer) {
			printk("%s() stop transfer.!!!\n", __FUNCTION__);
			up(&spi_dev->bus_lock);
			return -EIO;
		}
		ret = spi_dev->algo->master_xfer(spi_dev, &msg, 1);
		up(&spi_dev->bus_lock);

		dev_dbg(&spi_dev->dev, "master_recv: return:%d (count:%d)\n", ret, count);

		/* if everything went ok (i.e. 1 msg transmitted), return #bytes
		 * transmitted, else error code.
		 */
		return (ret == 1) ? count : ret;
	} else {
		dev_err(&spi_dev->dev, "SPI level transfers not supported\n");
		return -ENOSYS;
	}
}
EXPORT_SYMBOL(spi_master_recv);

int spi_master_send(struct spi_dev *spi_dev, char *wbuf, int count)
{
	int ret;
	struct spi_msg msg;

	pr_debug("mt5921-spi-dev: spi_master_send %d\n", count);
return -1;
	if (spi_dev->algo->master_xfer) {
		msg.flags = spi_dev->flags;
		msg.len = count;
		msg.wbuf = wbuf;
		msg.rbuf = NULL;

		if (spi_dev->stop_xfer)
			return -EIO;
		dev_dbg(&spi_dev->dev, "master_send: writing %d bytes.\n", count);
		if (down_trylock(&spi_dev->bus_lock)) {
			printk("%s() grab bus_lock fail!!!\n", __FUNCTION__);
			down(&spi_dev->bus_lock);
		}
		if (spi_dev->stop_xfer) {
			printk("%s() stop transfer.!!!\n", __FUNCTION__);
			up(&spi_dev->bus_lock);
			return -EIO;
		}
		ret = spi_dev->algo->master_xfer(spi_dev, &msg, 1);
		up(&spi_dev->bus_lock);

		/* if everything went ok (i.e. 1 msg transmitted), return #bytes
		 * transmitted, else error code.
		 */
		return (ret == 1) ? count : ret;
	} else {
		dev_err(&spi_dev->dev, "SPI level transfers not supported\n");
		return -ENOSYS;
	}
}
EXPORT_SYMBOL(spi_master_send);

static ssize_t spidev_read(struct file *file, char __user * buf, size_t count, loff_t * offset)
{
	char *tmp;
	int ret;
	struct spi_dev *spi_dev = (struct spi_dev *)file->private_data;
#ifdef CONFIG_WORD_TRANSIZE
	count = count * 4;
#endif
	if (count > BUFFER_SIZE)
		count = BUFFER_SIZE;

/*	if (spi_dev->flags & SPI_M_DMA_MODE) {
		tmp = dma_alloc_coherent(NULL, BUFFER_SIZE, &spi_dev->dmabuf, GFP_KERNEL | GFP_DMA);
	} else {*/
		tmp = kmalloc(count, GFP_KERNEL);
//	}
	if (tmp == NULL)
		return -ENOMEM;

//	pr_debug("%s: tmp=0x%x  dmabuf=0x%x\n", __FUNCTION__, *tmp, spi_dev->dmabuf);
	pr_debug("spi-dev: spi-%d reading %zd bytes.\n", iminor(file->f_dentry->d_inode), count);

	ret = spi_master_recv(spi_dev, tmp, count);
	if (ret >= 0)
		ret = copy_to_user(buf, tmp, count) ? -EFAULT : ret;
/*	if (spi_dev->flags & SPI_M_DMA_MODE) {
		dma_free_coherent(NULL, BUFFER_SIZE, tmp, spi_dev->dmabuf);
	} else {*/
		kfree(tmp);
//	}
	return ret;
}

static ssize_t spidev_write(struct file *file, const char __user * buf, size_t count, loff_t * offset)
{
	int ret;
	char *tmp;
	struct spi_dev *spi_dev = (struct spi_dev *)file->private_data;
#ifdef CONFIG_WORD_TRANSIZE
	count = count * 4;
#endif
	if (count > BUFFER_SIZE)
		count = BUFFER_SIZE;

/*	if (spi_dev->flags & SPI_M_DMA_MODE) {
		tmp = dma_alloc_coherent(NULL, BUFFER_SIZE, &spi_dev->dmabuf, GFP_KERNEL | GFP_DMA);
	} else {*/
		tmp = kmalloc(count, GFP_KERNEL);
//	}

	if (tmp == NULL)
		return -ENOMEM;
//	pr_debug("%s: tmp=0x%x  dmabuf=0x%x\n", __FUNCTION__, *tmp, spi_dev->dmabuf);

	if (copy_from_user(tmp, buf, count)) {
/*		if (spi_dev->flags & SPI_M_DMA_MODE) {
			dma_free_coherent(NULL, BUFFER_SIZE, tmp, spi_dev->dmabuf);
		} else {*/
			kfree(tmp);
//		}
		return -EFAULT;
	}

	pr_debug("spi-dev: spi-%d writing %zd bytes.\n", iminor(file->f_dentry->d_inode), count);

	ret = spi_master_send(spi_dev, tmp, count);
/*	if (spi_dev->flags & SPI_M_DMA_MODE) {
		dma_free_coherent(NULL, BUFFER_SIZE, tmp, spi_dev->dmabuf);
	} else {*/
		kfree(tmp);
//	}
	return ret;
}

static int spidev_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct spi_dev *spi_dev;

	spi_dev = spi_dev_get_by_minor(minor);
	if (!spi_dev)
		return -ENODEV;

	/* registered with adapter, passed as client to user */
	file->private_data = spi_dev;

	return 0;
}

static int spidev_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	return 0;
}

static struct file_operations spidev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.read = spidev_read,
	.write = spidev_write,
	.open = spidev_open,
	.release = spidev_release,
};

//static int __init spi_dev_init(void)
static int mt5921_bridge_init(void)
{
	int res;

	printk(KERN_INFO "spi /dev entries driver\n");

	/* removed Samsung's SoC-conditional
	 * This is now specific to the 2416-spi
	 */
	res = register_chrdev(SPI_MAJOR, "spi0", &spidev_fops);
	if (res)
		return res;

	return spi_register_driver(&mt5921_bridge_driver);
}

static void mt5921_bridge_exit(void)
{
	spi_unregister_driver(&mt5921_bridge_driver);
	unregister_chrdev(SPI_MAJOR, "spi");
}

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("spi /dev entries driver");
MODULE_LICENSE("GPL");

module_init(mt5921_bridge_init);
module_exit(mt5921_bridge_exit);
