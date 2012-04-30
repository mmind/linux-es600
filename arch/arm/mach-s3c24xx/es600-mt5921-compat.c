/*
 * Compatiblity functions for the proprietary MT5921 module
 *
 * Copyright (c) 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Memory allocator must be SLAB!
 */

#define DEBUG
#include <linux/export.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/wait.h>
#include <linux/irq.h>
#include <linux/proc_fs.h>
#include <plat/gpio-cfg.h>

void s3c2410_gpio_cfgpin(unsigned int pin, unsigned int function)
{
	pr_debug("mt5921-compat: cfgpin %d to %d unimplemented\n", pin, function);
	/* this seems to be the SPI-Int gpio */
//	s3c_gpio_cfgpin(pin, function);
}
EXPORT_SYMBOL(s3c2410_gpio_cfgpin);

#undef alloc_netdev
struct net_device *alloc_netdev(int sizeof_priv, const char *name,
				void (*setup)(struct net_device *))
{
	pr_debug("mt5921-compat: alloc_netdev %s\n", name);
	return alloc_netdev_mqs(sizeof_priv, name, setup, 1, 1);
}
EXPORT_SYMBOL(alloc_netdev);

void __bug(const char *file, int line)
{
	printk("BUG: failure at %s:%d()!\n", file, line);
	panic("BUG!");
}
EXPORT_SYMBOL(__bug);

void __down_failed(void)
{
	pr_warn("mt5921-compat: unimplemented __down_failed called\n");
}
EXPORT_SYMBOL(__down_failed);

void __up_wakeup(void)
{
	pr_warn("mt5921-compat: unimplemented __up_wakeup called\n");
}
EXPORT_SYMBOL(__up_wakeup);

#undef init_waitqueue_head
void init_waitqueue_head(wait_queue_head_t *q)
{
	pr_debug("mt5921-compat: init_waitqueue_head\n");
	do {
		static struct lock_class_key __key;
		__init_waitqueue_head((q), "mt5921-compat", &__key);
	} while (0);
}
EXPORT_SYMBOL(init_waitqueue_head);

int set_irq_type(unsigned int irq, unsigned int type)
{
	pr_debug("mt5921-compat: set irq %d to type %d\n", irq, type);
	return irq_set_irq_type(irq, type);
}
EXPORT_SYMBOL(set_irq_type);

typedef irqreturn_t (*irq_handler_t)(int, void *);

extern int request_threaded_irq(unsigned int irq, irq_handler_t handler,
				irq_handler_t thread_fn,
				unsigned long flags, const char *name, void *dev);

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
		const char *name, void *dev)
{
	pr_debug("mt5921-compat: request irq %d as %s\n", irq, name);
	return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}
EXPORT_SYMBOL(request_irq);

struct proc_dir_entry *proc_net;
EXPORT_SYMBOL(proc_net);

static int es600_mt5921_compat_init(void)
{
	pr_info("mt5921-compatibility layer\n");

	/* create the proc entry the module wants */
	proc_net = proc_mkdir("mt5921", NULL);

	return 0;
}

module_init(es600_mt5921_compat_init);

MODULE_DESCRIPTION("mt5921 compat driver for es600 ereader family");
MODULE_AUTHOR("Heiko Stuebner <heiko@sntech.de>");
MODULE_LICENSE("GPL");

/*************** to sort ******************/

#undef kthread_create
struct task_struct *kthread_create(int (*threadfn)(void *data),
				   void *data,
				   const char namefmt[], ...)
{
	pr_debug("mt5921-compat: kthread_create\n");
//#define kthread_create(threadfn, data, namefmt, arg...)
//	kthread_create_on_node(threadfn, data, -1, namefmt, ##arg)
/* kthread_create_on_node: */


}
EXPORT_SYMBOL(kthread_create);
