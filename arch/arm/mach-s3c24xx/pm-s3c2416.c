/* linux/arch/arm/mach-s3c2416/pm.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S3C2416 - PM support (Based on Ben Dooks' S3C2412 PM support)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/device.h>
#include <linux/syscore_ops.h>
#include <linux/io.h>

#include <asm/cacheflush.h>

#include <mach/regs-power.h>
#include <mach/regs-s3c2443-clock.h>

#include <plat/wakeup-mask.h>
#include <plat/cpu.h>
#include <plat/pm.h>

extern void s3c2412_sleep_enter(void);

static int s3c2416_cpu_suspend(unsigned long arg)
{
	/* set the mode as sleep, 2BED represents "Go to BED" */
	__raw_writel(S3C2443_PWRMODE_SLEEP, S3C2443_PWRMODE);

	s3c2412_sleep_enter();

	panic("sleep resumed to originator?");
}

/* mapping of interrupts to parts of the wakeup mask */
static struct samsung_wakeup_mask wake_irqs[] = {
	{ .irq = IRQ_RTC,	.bit = S3C2443_PWRCFG_RTCOFF, },
	{ .irq = IRQ_TICK,	.bit = S3C2443_PWRCFG_RTCTICKOFF, },
};

static void s3c2443_sync_wakemask(void)
{
	struct irq_data *data;
	u32 val;

	samsung_sync_wakemask(S3C2443_PWRCFG,
			      wake_irqs, ARRAY_SIZE(wake_irqs));

	/* BATF is reversed, so do it manually */
	data = irq_get_irq_data(IRQ_BATT_FLT);
	val = __raw_readl(S3C2443_PWRCFG);
	if (irqd_is_wakeup_set(data))
		val |= S3C2443_PWRCFG_BATF_ENABLE;
	else
		val &= ~S3C2443_PWRCFG_BATF_MASK;
	__raw_writel(val, S3C2443_PWRCFG);
}


static void s3c2416_pm_prepare(void)
{
	s3c2443_sync_wakemask();

	/*
	 * write the magic value u-boot uses to check for resume into
	 * the INFORM0 register, and ensure INFORM1 is set to the
	 * correct address to resume from.
	 */
	__raw_writel(0x2BED, S3C2412_INFORM0);
	__raw_writel(virt_to_phys(s3c_cpu_resume), S3C2412_INFORM1);
}

static int s3c2416_pm_add(struct device *dev, struct subsys_interface *sif)
{
	pm_cpu_prep = s3c2416_pm_prepare;
	pm_cpu_sleep = s3c2416_cpu_suspend;

	return 0;
}

static struct subsys_interface s3c2416_pm_interface = {
	.name		= "s3c2416_pm",
	.subsys		= &s3c2416_subsys,
	.add_dev	= s3c2416_pm_add,
};

static __init int s3c2416_pm_init(void)
{
	return subsys_interface_register(&s3c2416_pm_interface);
}

arch_initcall(s3c2416_pm_init);


static void s3c2416_pm_resume(void)
{
	/* unset the return-from-sleep amd inform flags */
	__raw_writel(0x0, S3C2443_PWRMODE);
	__raw_writel(0x0, S3C2412_INFORM0);
	__raw_writel(0x0, S3C2412_INFORM1);
}

struct syscore_ops s3c2416_pm_syscore_ops = {
	.resume		= s3c2416_pm_resume,
};
