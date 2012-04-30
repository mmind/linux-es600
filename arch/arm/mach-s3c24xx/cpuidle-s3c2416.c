/* linux/arch/arm/mach-s3c2416/cpuidle.c
 *
 * based on linux/arch/mach-exynos4/cpuidle.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/cpuidle.h>
#include <linux/cpu_pm.h>
#include <linux/io.h>
#include <linux/cpu.h>

#include <asm/proc-fns.h>

#include <mach/regs-s3c2443-clock.h>
#include <mach/regs-irq.h>

//irqmask
#include <plat/pm.h>

static int s3c2416_enter_idle(struct cpuidle_device *dev,
			      struct cpuidle_driver *drv,
			      int index)
{
	struct timeval before, after;
	unsigned long tmp;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	/* ensure our idle mode is to go to idle */
	tmp = __raw_readl(S3C2443_PWRCFG);
	tmp |= S3C2416_PWRCFG_STANDBYWFI;
	__raw_writel(tmp, S3C2443_PWRCFG);

	cpu_do_idle();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	dev->last_residency = idle_time;
	return index;
}

static int s3c2416_try_enter_stop(struct cpuidle_device *dev,
				  struct cpuidle_driver *drv,
				  int index);

static struct cpuidle_state s3c2416_cpuidle_set[] = {
	[0] = {
		.enter			= s3c2416_enter_idle,
		.exit_latency		= 1,
		.target_residency	= 1,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "IDLE",
		.desc			= "System active, ARM gated",
	},
	[1] = {
		.enter			= s3c2416_try_enter_stop,
		.exit_latency		= 1000,
		.target_residency	= 100000,
		.flags			= CPUIDLE_FLAG_TIME_VALID,
		.name			= "C2",
		.desc			= "clock gating for all clocks",
	},
/*	[2] = {
		.enter			= s3c2416_try_enter_stop,
		.exit_latency		= 1000,
		.target_residency	= 100000,
		.flags			= CPUIDLE_FLAG_TIME_VALID | CPUIDLE_FLAG_CHECK_BM | CPUIDLE_FLAG_DEEP,
		.name			= "DEEP-STOP",
		.desc			= "ARM power down",
	},*/
};

static int allow_enter_stop = 0;

static struct cpuidle_device s3c2416_idle_device;

static struct cpuidle_driver s3c2416_idle_driver = {
	.name		= "s3c2416_idle",
	.owner		= THIS_MODULE,
};

static void s3c2416_set_wakeupmask(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S3C2443_PWRCFG);

	/* disable other power-down modes */
	tmp &= ~(S3C2416_PWRCFG_DEEPSTOP | S3C2416_PWRCFG_STANDBYWFI);

	/* enable wakeup sources regardless of battery state */
	tmp |= S3C2443_PWRCFG_SLEEP;

	/* wakeup from rtc alarm and rtc tick */
	tmp &= ~S3C2443_PWRCFG_RTCOFF;
	tmp &= ~S3C2443_PWRCFG_RTCTICKOFF;

	__raw_writel(tmp, S3C2443_PWRCFG);


	/* OSC must keep running in stop mode to supply PLLs */
/*	tmp = __raw_readl(S3C2443_PWRCFG);
	tmp |= S3C2443_PWRCFG_OSC_STOP;
	__raw_writel(tmp, S3C2443_PWRCFG);*/

	/* must keep the plls running in stop mode to get usefull results */
/*	tmp = __raw_readl(S3C2443_MPLLCON);
	tmp |= S3C2443_PLLCON_STOPON;
	__raw_writel(tmp, S3C2443_MPLLCON);
	tmp = __raw_readl(S3C2443_EPLLCON);
	tmp |= S3C2443_PLLCON_STOPON;
	__raw_writel(tmp, S3C2443_EPLLCON);*/
}

static int s3c2416_enter_stop(struct cpuidle_device *dev,
			      struct cpuidle_driver *drv,
			      int index)
{
	struct timeval before, after;
	int idle_time;

/* we need the rtc device and enable its timer tick, to wake us up */

//	tmp = __raw_readl(S3C2410_

	local_irq_disable();
	do_gettimeofday(&before);

	s3c2416_set_wakeupmask();

	cpu_pm_enter();

	/* set the mode as STOP */
	__raw_writel(S3C2443_PWRMODE_STOP, S3C2443_PWRMODE);
	/* After exit STOP mode we back here */

	cpu_pm_exit();

	/* Reset STOP mode */
	__raw_writel(0, S3C2443_PWRMODE);

//	save_cpu_arch_register();

        /* Setting Central Sequence Register for power down mode */
//        tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
//        tmp &= ~S5P_CENTRAL_LOWPWR_CFG;
//        __raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

//        cpu_suspend(0, idle_finisher);

	/* ensure our idle mode is to go to idle */
/*	tmp = __raw_readl(S3C2443_PWRCFG);
	tmp |= S3C2416_PWRCFG_STANDBYWFI;
	__raw_writel(tmp, S3C2443_PWRCFG);

	cpu_do_idle();
*/
//        restore_cpu_arch_register();

	/*
	 * If PMU failed while entering sleep mode, WFI will be
	 * ignored by PMU and then exiting cpu_do_idle().
	 * S5P_CENTRAL_LOWPWR_CFG bit will not be set automatically
	 * in this situation.
	 */
//       tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
//       if (!(tmp & S5P_CENTRAL_LOWPWR_CFG)) {
//               tmp |= S5P_CENTRAL_LOWPWR_CFG;
//               __raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);
//       }

//only enter stop once, after allow is set
allow_enter_stop = 0;

	/* Clear wakeup state register */
	__raw_writel(1, S3C2443_WKUPSTAT);

	do_gettimeofday(&after);

	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	dev->last_residency = idle_time;
	return index;
}

static int s3c2416_try_enter_stop(struct cpuidle_device *dev,
				  struct cpuidle_driver *drv,
				  int index)
{
	if (allow_enter_stop)
		return s3c2416_enter_stop(dev, drv, index);
	else
		return s3c2416_enter_idle(dev, drv, drv->safe_state_index);
}

/* sysfs */

static ssize_t allow_enter_stop_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", allow_enter_stop);
}

static ssize_t allow_enter_stop_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int allow;

	allow = simple_strtol(buf, NULL, 10);

	if(allow > 0)
		allow_enter_stop = 1;
	else
		allow_enter_stop = 0;

	return count;
}

static DEVICE_ATTR(allow_enter_stop, 0644, allow_enter_stop_show, allow_enter_stop_store);

/* init */

static int __init s3c2416_cpuidle_add(struct device *dev,
				      struct subsys_interface *sif)
{
	struct cpuidle_device *device = &s3c2416_idle_device;
	struct cpuidle_driver *drv = &s3c2416_idle_driver;
	struct device *cpudev = get_cpu_device(0); /* CPU 0 */
	int i, max_cpuidle_state;

	/* Setup cpuidle driver */
	drv->state_count = (sizeof(s3c2416_cpuidle_set) /
				       sizeof(struct cpuidle_state));
	max_cpuidle_state = drv->state_count;
	for (i = 0; i < max_cpuidle_state; i++) {
		memcpy(&drv->states[i], &s3c2416_cpuidle_set[i],
				sizeof(struct cpuidle_state));
	}
	drv->safe_state_index = 0;
	cpuidle_register_driver(&s3c2416_idle_driver);

	device->cpu = 0;
	device->state_count = drv->state_count;

	if (cpuidle_register_device(device)) {
		printk(KERN_ERR "CPUidle register device failed\n,");
		return -EIO;
	}

	device_create_file(cpudev, &dev_attr_allow_enter_stop);

	return 0;
}

static struct subsys_interface s3c2416_cpuidle_interface = {
	.name		= "s3c2416_cpuidle",
	.subsys		= &s3c2416_subsys,
	.add_dev	= s3c2416_cpuidle_add,
};

static int __init s3c2416_cpuidle_init(void)
{
	return subsys_interface_register(&s3c2416_cpuidle_interface);
}
arch_initcall(s3c2416_cpuidle_init);
