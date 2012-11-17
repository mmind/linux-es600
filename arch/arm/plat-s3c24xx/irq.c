/*
 * S3C24XX IRQ handling
 *
 * Copyright (c) 2003-2004 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 * Copyright (c) 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/syscore_ops.h>

#include <linux/irqdomain.h>

#include <asm/irq.h>
#include <asm/mach/irq.h>

#include <plat/regs-irqtype.h>

#include <plat/cpu.h>
#include <plat/pm.h>
#include <plat/irq.h>

#define S3C_IRQTYPE_NONE	0

/* s3c_irq_eint0t4 chip + edge handler */
#define S3C_IRQTYPE_EINT0T4	1

/* s3c_irq_chip + edge handler */
#define S3C_IRQTYPE_EDGE	2

/* s3c_irq_level_chip + level handler + without valid flag */
#define S3C_IRQTYPE_PARENT	3

/* s3c_irqsub_level + level handler */
#define S3C_IRQTYPE_SUBLEVEL	4

/* s3c_irqsub_edge + edge handler */
#define S3C_IRQTYPE_SUBEDGE	5

/* s3c_irqext_chip + edge handler */
#define S3C_IRQTYPE_SUBEINT	6

struct s3c_irq_data {
	unsigned int type;
	unsigned long parent_irq;

	/* data gets filled during init */
	struct s3c_irq_intc *intc;
	unsigned long sub_bits;
	struct s3c_irq_intc *sub_intc;
};

/*
 * Sructure holding the controller data
 * @reg_pending		register holding pending irqs
 * @reg_intpnd		special register intpnd in main intc
 * @reg_mask		mask register
 * @domain		irq_domain of the controller
 * @parent		parent controller for ext and sub irqs
 * @irqs		irq-data, always s3c_irq_data[32]
 */
struct s3c_irq_intc {
	void __iomem		*reg_pending;
	void __iomem		*reg_intpnd;
	void __iomem		*reg_mask;
	struct irq_domain	*domain;
	struct s3c_irq_intc	*parent;
	struct s3c_irq_data	*irqs;
};

static inline void s3c_irq_mask(struct irq_data *data)
{
	struct s3c_irq_intc *intc = data->domain->host_data;
	unsigned long mask;

	mask = __raw_readl(intc->reg_mask);
	mask |= 1UL << data->hwirq;
	__raw_writel(mask, intc->reg_mask);
}

static inline void s3c_irq_unmask(struct irq_data *data)
{
	struct s3c_irq_intc *intc = data->domain->host_data;
	unsigned long mask;

	mask = __raw_readl(intc->reg_mask);
	mask &= ~(1UL << data->hwirq);
	__raw_writel(mask, intc->reg_mask);
}

static inline void s3c_irq_ack(struct irq_data *data)
{
	struct s3c_irq_intc *intc = data->domain->host_data;
	unsigned long bitval = 1UL << data->hwirq;

	__raw_writel(bitval, intc->reg_pending);
	if (intc->reg_intpnd)
		__raw_writel(bitval, intc->reg_intpnd);
}

static inline void s3c_irq_maskack(struct irq_data *data)
{
	s3c_irq_mask(data);
	s3c_irq_ack(data);
}

#ifdef CONFIG_PM
/* state for IRQs over sleep
 *
 * default is to allow for EINT0..EINT15, and IRQ_RTC as wakeup sources
 * set bit to 1 in allow bitfield to enable the wakeup settings on it
 */

unsigned long s3c_irqwake_intallow	= 1L << 30 | 0xfL;
unsigned long s3c_irqwake_eintallow	= 0x0000fff0L;

/* FIXME: must not be static until s3c2412 irqs are included */
int s3c_irq_wake(struct irq_data *data, unsigned int state)
{
	unsigned long irqbit = 1 << data->hwirq;

	if (!(s3c_irqwake_intallow & irqbit))
		return -ENOENT;

	pr_info("wake %s for hwirq %lu\n",
		state ? "enabled" : "disabled", data->hwirq);

	if (!state)
		s3c_irqwake_intmask |= irqbit;
	else
		s3c_irqwake_intmask &= ~irqbit;

	return 0;
}
#endif

struct irq_chip s3c_irq_level_chip = {
	.name		= "s3c-level",
	.irq_ack	= s3c_irq_maskack,
	.irq_mask	= s3c_irq_mask,
	.irq_unmask	= s3c_irq_unmask,
	.irq_set_wake	= s3c_irq_wake
};

struct irq_chip s3c_irq_chip = {
	.name		= "s3c",
	.irq_ack	= s3c_irq_ack,
	.irq_mask	= s3c_irq_mask,
	.irq_unmask	= s3c_irq_unmask,
	.irq_set_wake	= s3c_irq_wake
};

static void s3c_irqext_ack(struct irq_data *data)
{
	struct s3c_irq_intc *intc = data->domain->host_data;
	struct s3c_irq_intc *parent_intc = intc->parent;
	struct s3c_irq_data *irq_data = &intc->irqs[data->hwirq];
	struct s3c_irq_data *parent_data;
	unsigned long req;
	unsigned long mask;
	unsigned int irqno;

	parent_data = &parent_intc->irqs[irq_data->parent_irq];

	mask = __raw_readl(intc->reg_mask);

	__raw_writel(1UL << data->hwirq, intc->reg_pending);

	req = __raw_readl(intc->reg_pending);
	req &= ~mask;

	/* not sure if we should be acking the parent irq... */

	if ((req & parent_data->sub_bits) == 0) {
		irqno = irq_find_mapping(parent_intc->domain,
					 irq_data->parent_irq);
		s3c_irq_ack(irq_get_irq_data(irqno));
	}
}

static int s3c_irqext_type_set(void __iomem *gpcon_reg,
			       void __iomem *extint_reg,
			       unsigned long gpcon_offset,
			       unsigned long extint_offset,
			       unsigned int type)
{
	unsigned long newvalue = 0, value;

	/* Set the GPIO to external interrupt mode */
	value = __raw_readl(gpcon_reg);
	value = (value & ~(3 << gpcon_offset)) | (0x02 << gpcon_offset);
	__raw_writel(value, gpcon_reg);

	/* Set the external interrupt to pointed trigger type */
	switch (type)
	{
		case IRQ_TYPE_NONE:
			pr_warn("No edge setting!\n");
			break;

		case IRQ_TYPE_EDGE_RISING:
			newvalue = S3C2410_EXTINT_RISEEDGE;
			break;

		case IRQ_TYPE_EDGE_FALLING:
			newvalue = S3C2410_EXTINT_FALLEDGE;
			break;

		case IRQ_TYPE_EDGE_BOTH:
			newvalue = S3C2410_EXTINT_BOTHEDGE;
			break;

		case IRQ_TYPE_LEVEL_LOW:
			newvalue = S3C2410_EXTINT_LOWLEV;
			break;

		case IRQ_TYPE_LEVEL_HIGH:
			newvalue = S3C2410_EXTINT_HILEV;
			break;

		default:
			pr_err("No such irq type %d", type);
			return -EINVAL;
	}

	value = __raw_readl(extint_reg);
	value = (value & ~(7 << extint_offset)) | (newvalue << extint_offset);
	__raw_writel(value, extint_reg);

	return 0;
}

/* FIXME: make static when it's out of plat-samsung/irq.h */
int s3c_irqext_type(struct irq_data *data, unsigned int type)
{
	void __iomem *extint_reg;
	void __iomem *gpcon_reg;
	unsigned long gpcon_offset, extint_offset;

	if ((data->hwirq >= 4) && (data->hwirq <= 7)) {
		gpcon_reg = S3C2410_GPFCON;
		extint_reg = S3C24XX_EXTINT0;
		gpcon_offset = (data->hwirq) * 2;
		extint_offset = (data->hwirq) * 4;
	} else if ((data->hwirq >= 8) && (data->hwirq <= 15)) {
		gpcon_reg = S3C2410_GPGCON;
		extint_reg = S3C24XX_EXTINT1;
		gpcon_offset = (data->hwirq - 8) * 2;
		extint_offset = (data->hwirq - 8) * 4;
	} else if ((data->hwirq >= 16) && (data->hwirq <= 23)) {
		gpcon_reg = S3C2410_GPGCON;
		extint_reg = S3C24XX_EXTINT2;
		gpcon_offset = (data->hwirq - 8) * 2;
		extint_offset = (data->hwirq - 16) * 4;
	} else {
		return -EINVAL;
	}

	return s3c_irqext_type_set(gpcon_reg, extint_reg, gpcon_offset,
				   extint_offset, type);
}

static int s3c_irqext0_type(struct irq_data *data, unsigned int type)
{
	void __iomem *extint_reg;
	void __iomem *gpcon_reg;
	unsigned long gpcon_offset, extint_offset;

	if ((data->hwirq >= 0) && (data->hwirq <= 3)) {
		gpcon_reg = S3C2410_GPFCON;
		extint_reg = S3C24XX_EXTINT0;
		gpcon_offset = (data->hwirq) * 2;
		extint_offset = (data->hwirq) * 4;
	} else {
		return -EINVAL;
	}

	return s3c_irqext_type_set(gpcon_reg, extint_reg, gpcon_offset,
				   extint_offset, type);
}

/* FIXME: what is the correct behaviour for mask and unmask?
 * The previous s3c_irqext_mask/unmask functions only set the EINTMASK
 * register, while the other subirqs also mask the parent irq
 */
static struct irq_chip s3c_irqext_chip = {
	.name		= "s3c-ext",
	.irq_mask	= s3c_irq_mask,
	.irq_unmask	= s3c_irq_unmask,
	.irq_ack	= s3c_irqext_ack,
	.irq_set_type	= s3c_irqext_type,
	.irq_set_wake	= s3c_irqext_wake
};

static struct irq_chip s3c_irq_eint0t4 = {
	.name		= "s3c-ext0",
	.irq_ack	= s3c_irq_ack,
	.irq_mask	= s3c_irq_mask,
	.irq_unmask	= s3c_irq_unmask,
	.irq_set_wake	= s3c_irq_wake,
	.irq_set_type	= s3c_irqext0_type,
};

static void s3c_subirq_mask(struct irq_data *data)
{
	struct s3c_irq_intc *intc = data->domain->host_data;
	struct s3c_irq_intc *parent_intc = intc->parent;
	struct s3c_irq_data *irq_data = &intc->irqs[data->hwirq];
	struct s3c_irq_data *parent_data;
	unsigned long submask;
	unsigned int irqno;

	parent_data = &parent_intc->irqs[irq_data->parent_irq];

	submask = __raw_readl(intc->reg_mask);
	submask |= (1UL << data->hwirq);

	/* check to see if we need to mask the parent IRQ */
	if ((submask & parent_data->sub_bits) == parent_data->sub_bits) {
		irqno = irq_find_mapping(parent_intc->domain,
					 irq_data->parent_irq);
		s3c_irq_mask(irq_get_irq_data(irqno));
	}

	/* write back masks */
	__raw_writel(submask, intc->reg_mask);
}

static void s3c_subirq_unmask(struct irq_data *data)
{
	struct s3c_irq_intc *intc = data->domain->host_data;
	struct s3c_irq_intc *parent_intc = intc->parent;
	struct s3c_irq_data *irq_data = &intc->irqs[data->hwirq];
	unsigned long submask;
	unsigned int irqno;

	submask = __raw_readl(intc->reg_mask);
	submask &= ~(1UL << data->hwirq);
	__raw_writel(submask, intc->reg_mask);

	irqno = irq_find_mapping(parent_intc->domain, irq_data->parent_irq);
	s3c_irq_unmask(irq_get_irq_data(irqno));
}

static void s3c_subirq_ack(struct irq_data *data)
{
	struct s3c_irq_intc *intc = data->domain->host_data;
	struct s3c_irq_intc *parent_intc = intc->parent;
	struct s3c_irq_data *irq_data = &intc->irqs[data->hwirq];
	unsigned long bit = 1UL << data->hwirq;
	unsigned int irqno;

	__raw_writel(bit, intc->reg_pending);

	/* only ack parent if we've got all the irqs (seems we must
	 * ack, all and hope that the irq system retriggers ok when
	 * the interrupt goes off again)
	 */

	if (1) {
		irqno = irq_find_mapping(parent_intc->domain,
					 irq_data->parent_irq);
		s3c_irq_ack(irq_get_irq_data(irqno));
	}
}

static inline void s3c_subirq_maskack(struct irq_data *data)
{
	s3c_subirq_mask(data);
	s3c_subirq_ack(data);
}

/* used for UARTs */
static struct irq_chip s3c_irqsub_level = {
	.name		= "s3c-sublevel",
	.irq_mask	= s3c_subirq_mask,
	.irq_unmask	= s3c_subirq_unmask,
	.irq_ack	= s3c_subirq_maskack,
};

/* used for ADC and Touchscreen */
static struct irq_chip s3c_irqsub_edge = {
	.name		= "s3c-subedge",
	.irq_mask	= s3c_subirq_mask,
	.irq_unmask	= s3c_subirq_unmask,
	.irq_ack	= s3c_subirq_ack,
};

static void s3c_irq_demux(unsigned int irq, struct irq_desc *desc)
{
	struct s3c_irq_intc *intc = desc->irq_data.domain->host_data;
	struct s3c_irq_data *irq_data = &intc->irqs[desc->irq_data.hwirq];
	struct s3c_irq_intc *sub_intc = irq_data->sub_intc;
	unsigned int src, msk;

	/* read the current pending interrupts, and the mask
	 * for what it is available
	 */

	src = __raw_readl(sub_intc->reg_pending);
	msk = __raw_readl(sub_intc->reg_mask);

	src &= ~msk;
	src &= irq_data->sub_bits;

	while (src) {
		irq = __ffs(src);
		src &= ~(1 << irq);
		generic_handle_irq(irq_find_mapping(sub_intc->domain, irq));
	}
}

#ifdef CONFIG_FIQ
/**
 * s3c24xx_set_fiq - set the FIQ routing
 * @irq: IRQ number to route to FIQ on processor.
 * @on: Whether to route @irq to the FIQ, or to remove the FIQ routing.
 *
 * Change the state of the IRQ to FIQ routing depending on @irq and @on. If
 * @on is true, the @irq is checked to see if it can be routed and the
 * interrupt controller updated to route the IRQ. If @on is false, the FIQ
 * routing is cleared, regardless of which @irq is specified.
 */
int s3c24xx_set_fiq(unsigned int irq, bool on)
{
	u32 intmod;
	unsigned offs;

	if (on) {
		offs = irq - FIQ_START;
		if (offs > 31)
			return -EINVAL;

		intmod = 1 << offs;
	} else {
		intmod = 0;
	}

	__raw_writel(intmod, S3C2410_INTMOD);
	return 0;
}

EXPORT_SYMBOL_GPL(s3c24xx_set_fiq);
#endif

static int s3c24xx_irq_map(struct irq_domain *h, unsigned int virq,
							irq_hw_number_t hw)
{
	struct s3c_irq_intc *intc = h->host_data;
	struct s3c_irq_data *irq_data = &intc->irqs[hw];
	struct s3c_irq_intc *parent_intc;
	struct s3c_irq_data *parent_irq_data;
	bool attach_to_parent = false;
	unsigned int irqno;

	if (!intc) {
		pr_err("irq-s3c24xx: no controller found for hwirq %lu\n", hw);
		return -EINVAL;
	}

	if (!irq_data) {
		pr_err("irq-s3c24xx: no irq data found for hwirq %lu\n", hw);
		return -EINVAL;
	}

	/* attach controller pointer to irq_data */
	irq_data->intc = intc;

	/* set handler and flags */
	switch (irq_data->type) {
	case S3C_IRQTYPE_NONE:
		return 0;
	case S3C_IRQTYPE_EINT0T4:
		irq_set_chip_and_handler(virq, &s3c_irq_eint0t4,
					 handle_edge_irq);
		set_irq_flags(virq, IRQF_VALID);
		break;
	case S3C_IRQTYPE_EDGE:
		irq_set_chip_and_handler(virq, &s3c_irq_chip,
					 handle_edge_irq);
		set_irq_flags(virq, IRQF_VALID);
		break;
	case S3C_IRQTYPE_PARENT:
		irq_set_chip_and_handler(virq, &s3c_irq_level_chip,
					 handle_level_irq);
		break;
	case S3C_IRQTYPE_SUBEINT:
		irq_set_chip_and_handler(virq, &s3c_irqext_chip,
					 handle_edge_irq);
		set_irq_flags(virq, IRQF_VALID);
		attach_to_parent = true;
		break;
	case S3C_IRQTYPE_SUBLEVEL:
		irq_set_chip_and_handler(virq, &s3c_irqsub_level,
					 handle_level_irq);
		set_irq_flags(virq, IRQF_VALID);
		attach_to_parent = true;
		break;
	case S3C_IRQTYPE_SUBEDGE:
		irq_set_chip_and_handler(virq, &s3c_irqsub_edge,
					 handle_edge_irq);
		set_irq_flags(virq, IRQF_VALID);
		attach_to_parent = true;
		break;
	default:
		pr_err("irq-s3c24xx: unsupported irqtype %d\n", irq_data->type);
		return -EINVAL;
	}

	if (attach_to_parent) {
		parent_intc = intc->parent;
		if (!parent_intc) {
			pr_err("irq-s3c24xx: no parent controller found for hwirq %lu\n",
			       hw);
			goto err;
		}

		parent_irq_data = &parent_intc->irqs[irq_data->parent_irq];
		if (!irq_data) {
			pr_err("irq-s3c24xx: no irq data found for hwirq %lu\n",
			       hw);
			goto err;
		}

		parent_irq_data->sub_intc = intc;
		parent_irq_data->sub_bits |= (1UL << hw);

		/* attach the demuxer to the parent irq */
		irqno = irq_find_mapping(parent_intc->domain,
					 irq_data->parent_irq);
		irq_set_chained_handler(irqno, s3c_irq_demux);
	}

	return 0;

err:
	set_irq_flags(virq, 0);

	/* the only error results from bad mapping data*/
	return -EINVAL;
}

static struct irq_domain_ops s3c24xx_irq_ops = {
	.map = s3c24xx_irq_map,
	.xlate = irq_domain_xlate_twocell,
};

static void s3c24xx_clear_intc(struct s3c_irq_intc *intc)
{
	void __iomem *reg_source;
	unsigned long pend;
	unsigned long last;
	int i;

	/* if intpnd is set, read the next pending irq from there */
	reg_source = intc->reg_intpnd ? intc->reg_intpnd : intc->reg_pending;

	last = 0;
	for (i = 0; i < 4; i++) {
		pend = __raw_readl(reg_source);

		if (pend == 0 || pend == last)
			break;

		__raw_writel(pend, intc->reg_pending);
		if (intc->reg_intpnd)
			__raw_writel(pend, intc->reg_intpnd);

		pr_info("irq: clearing pending status %08x\n", (int)pend);
		last = pend;
	}
}

/* s3c24xx_init_irq
 *
 * Initialise S3C2410 IRQ system
*/

struct s3c_irq_data init_base[32] = {
	{ .type = S3C_IRQTYPE_EINT0T4, }, /* EINT0 */
	{ .type = S3C_IRQTYPE_EINT0T4, }, /* EINT1 */
	{ .type = S3C_IRQTYPE_EINT0T4, }, /* EINT2 */
	{ .type = S3C_IRQTYPE_EINT0T4, }, /* EINT3 */
	{ .type = S3C_IRQTYPE_PARENT, }, /* EINT4to7 */
	{ .type = S3C_IRQTYPE_PARENT, }, /* EINT8to23 */
	{ .type = S3C_IRQTYPE_NONE, }, /* reserved */
	{ .type = S3C_IRQTYPE_EDGE, }, /* nBATT_FLT */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TICK */
	{ .type = S3C_IRQTYPE_EDGE, }, /* WDT */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER0 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER1 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER2 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER3 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* TIMER4 */
	{ .type = S3C_IRQTYPE_PARENT, }, /* UART2 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* LCD */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA0 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA1 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA2 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* DMA3 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* SDI */
	{ .type = S3C_IRQTYPE_EDGE, }, /* SPI0 */
	{ .type = S3C_IRQTYPE_PARENT, }, /* UART1 */
	{ .type = S3C_IRQTYPE_NONE, }, /* reserved */
	{ .type = S3C_IRQTYPE_EDGE, }, /* USBD */
	{ .type = S3C_IRQTYPE_EDGE, }, /* USBH */
	{ .type = S3C_IRQTYPE_EDGE, }, /* IIC */
	{ .type = S3C_IRQTYPE_PARENT, }, /* UART0 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* SPI1 */
	{ .type = S3C_IRQTYPE_EDGE, }, /* RTC */
	{ .type = S3C_IRQTYPE_PARENT, }, /* ADCPARENT */
};

struct s3c_irq_data init_eint[32] = {
	{ .type = S3C_IRQTYPE_NONE, }, /* reserved */
	{ .type = S3C_IRQTYPE_NONE, }, /* reserved */
	{ .type = S3C_IRQTYPE_NONE, }, /* reserved */
	{ .type = S3C_IRQTYPE_NONE, }, /* reserved */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 4 }, /* EINT4 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 4 }, /* EINT5 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 4 }, /* EINT6 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 4 }, /* EINT7 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT8 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT9 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT10 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT11 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT12 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT13 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT14 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT15 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT16 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT17 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT18 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT19 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT20 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT21 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT22 */
	{ .type = S3C_IRQTYPE_SUBEINT, .parent_irq = 5 }, /* EINT23 */
};

struct s3c_irq_data init_subint[32] = {
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 28 }, /* UART0-RX */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 28 }, /* UART0-TX */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 28 }, /* UART0-ERR */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 23 }, /* UART1-RX */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 23 }, /* UART1-TX */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 23 }, /* UART1-ERR */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 15 }, /* UART2-RX */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 15 }, /* UART2-TX */
	{ .type = S3C_IRQTYPE_SUBLEVEL, .parent_irq = 15 }, /* UART2-ERR */
	{ .type = S3C_IRQTYPE_SUBEDGE, .parent_irq = 31 }, /* TC */
	{ .type = S3C_IRQTYPE_SUBEDGE, .parent_irq = 31 }, /* ADC */
};

static struct s3c_irq_intc s3c_intc[3] = {
	[0] = {
		.reg_pending = S3C2410_SRCPND,
		.reg_intpnd = S3C2410_INTPND,
		.reg_mask = S3C2410_INTMSK,
		.irqs = &init_base[0],
	},
	[1] = {
		.reg_pending = S3C2410_EINTPEND,
		.reg_mask = S3C2410_EINTMASK,
		.irqs = &init_eint[0],
	},
	[2] = {
		.reg_pending = S3C2410_SUBSRCPND,
		.reg_mask = S3C2410_INTSUBMSK,
		.irqs = &init_subint[0],
	},
};

void __init s3c24xx_init_irq(void)
{
#ifdef CONFIG_FIQ
	init_FIQ(FIQ_START);
#endif

	/* attach the sub handlers to the main one */
	s3c_intc[1].parent = &s3c_intc[0];
	s3c_intc[2].parent = &s3c_intc[0];

	/* basic interrupt register */
	s3c24xx_clear_intc(&s3c_intc[0]);
	s3c_intc[0].domain = irq_domain_add_legacy(NULL, 32, IRQ_EINT0, 0,
					       &s3c24xx_irq_ops, &s3c_intc[0]);

	/* extint register, irqs begin at bit4 */
	s3c24xx_clear_intc(&s3c_intc[1]);
	s3c_intc[1].domain = irq_domain_add_legacy(NULL, 20, IRQ_EINT4, 4,
					       &s3c24xx_irq_ops, &s3c_intc[1]);

	/* subint register, 29 to fit subints of all SoCs */
	s3c24xx_clear_intc(&s3c_intc[2]);
	s3c_intc[2].domain = irq_domain_add_legacy(NULL, 29, IRQ_S3CUART_RX0, 0,
					       &s3c24xx_irq_ops, &s3c_intc[2]);
}

#ifdef CONFIG_PM
static struct sleep_save irq_save[] = {
	SAVE_ITEM(S3C2410_INTMSK),
	SAVE_ITEM(S3C2410_INTSUBMSK),
};

/* the extint values move between the s3c2410/s3c2440 and the s3c2412
 * so we use an array to hold them, and to calculate the address of
 * the register at run-time
*/

static unsigned long save_extint[3];
static unsigned long save_eintflt[4];
static unsigned long save_eintmask;

static int s3c24xx_irq_suspend(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(save_extint); i++)
		save_extint[i] = __raw_readl(S3C24XX_EXTINT0 + (i*4));

	for (i = 0; i < ARRAY_SIZE(save_eintflt); i++)
		save_eintflt[i] = __raw_readl(S3C24XX_EINFLT0 + (i*4));

	s3c_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
	save_eintmask = __raw_readl(S3C24XX_EINTMASK);

	return 0;
}

static void s3c24xx_irq_resume(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(save_extint); i++)
		__raw_writel(save_extint[i], S3C24XX_EXTINT0 + (i*4));

	for (i = 0; i < ARRAY_SIZE(save_eintflt); i++)
		__raw_writel(save_eintflt[i], S3C24XX_EINFLT0 + (i*4));

	s3c_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));
	__raw_writel(save_eintmask, S3C24XX_EINTMASK);
}

struct syscore_ops s3c24xx_irq_syscore_ops = {
	.suspend	= s3c24xx_irq_suspend,
	.resume		= s3c24xx_irq_resume,
};
#endif

#ifdef CONFIG_CPU_S3C2416
#define INTMSK(start, end) ((1 << ((end) + 1 - (start))) - 1)

static inline void s3c2416_irq_demux(unsigned int irq, unsigned int len)
{
	unsigned int subsrc, submsk;
	unsigned int end;

	/* read the current pending interrupts, and the mask
	 * for what it is available */

	subsrc = __raw_readl(S3C2410_SUBSRCPND);
	submsk = __raw_readl(S3C2410_INTSUBMSK);

	subsrc  &= ~submsk;
	subsrc >>= (irq - S3C2410_IRQSUB(0));
	subsrc  &= (1 << len)-1;

	end = len + irq;

	for (; irq < end && subsrc; irq++) {
		if (subsrc & 1)
			generic_handle_irq(irq);

		subsrc >>= 1;
	}
}

/* WDT/AC97 sub interrupts */

static void s3c2416_irq_demux_wdtac97(unsigned int irq, struct irq_desc *desc)
{
	s3c2416_irq_demux(IRQ_S3C2443_WDT, 4);
}

#define INTMSK_WDTAC97	(1UL << (IRQ_WDT - IRQ_EINT0))
#define SUBMSK_WDTAC97	INTMSK(IRQ_S3C2443_WDT, IRQ_S3C2443_AC97)

static void s3c2416_irq_wdtac97_mask(struct irq_data *data)
{
	s3c_irqsub_mask(data->irq, INTMSK_WDTAC97, SUBMSK_WDTAC97);
}

static void s3c2416_irq_wdtac97_unmask(struct irq_data *data)
{
	s3c_irqsub_unmask(data->irq, INTMSK_WDTAC97);
}

static void s3c2416_irq_wdtac97_ack(struct irq_data *data)
{
	s3c_irqsub_maskack(data->irq, INTMSK_WDTAC97, SUBMSK_WDTAC97);
}

static struct irq_chip s3c2416_irq_wdtac97 = {
	.irq_mask	= s3c2416_irq_wdtac97_mask,
	.irq_unmask	= s3c2416_irq_wdtac97_unmask,
	.irq_ack	= s3c2416_irq_wdtac97_ack,
};

/* LCD sub interrupts */

static void s3c2416_irq_demux_lcd(unsigned int irq, struct irq_desc *desc)
{
	s3c2416_irq_demux(IRQ_S3C2443_LCD1, 4);
}

#define INTMSK_LCD	(1UL << (IRQ_LCD - IRQ_EINT0))
#define SUBMSK_LCD	INTMSK(IRQ_S3C2443_LCD1, IRQ_S3C2443_LCD4)

static void s3c2416_irq_lcd_mask(struct irq_data *data)
{
	s3c_irqsub_mask(data->irq, INTMSK_LCD, SUBMSK_LCD);
}

static void s3c2416_irq_lcd_unmask(struct irq_data *data)
{
	s3c_irqsub_unmask(data->irq, INTMSK_LCD);
}

static void s3c2416_irq_lcd_ack(struct irq_data *data)
{
	s3c_irqsub_maskack(data->irq, INTMSK_LCD, SUBMSK_LCD);
}

static struct irq_chip s3c2416_irq_lcd = {
	.irq_mask	= s3c2416_irq_lcd_mask,
	.irq_unmask	= s3c2416_irq_lcd_unmask,
	.irq_ack	= s3c2416_irq_lcd_ack,
};

/* DMA sub interrupts */

static void s3c2416_irq_demux_dma(unsigned int irq, struct irq_desc *desc)
{
	s3c2416_irq_demux(IRQ_S3C2443_DMA0, 6);
}

#define INTMSK_DMA	(1UL << (IRQ_S3C2443_DMA - IRQ_EINT0))
#define SUBMSK_DMA	INTMSK(IRQ_S3C2443_DMA0, IRQ_S3C2443_DMA5)


static void s3c2416_irq_dma_mask(struct irq_data *data)
{
	s3c_irqsub_mask(data->irq, INTMSK_DMA, SUBMSK_DMA);
}

static void s3c2416_irq_dma_unmask(struct irq_data *data)
{
	s3c_irqsub_unmask(data->irq, INTMSK_DMA);
}

static void s3c2416_irq_dma_ack(struct irq_data *data)
{
	s3c_irqsub_maskack(data->irq, INTMSK_DMA, SUBMSK_DMA);
}

static struct irq_chip s3c2416_irq_dma = {
	.irq_mask	= s3c2416_irq_dma_mask,
	.irq_unmask	= s3c2416_irq_dma_unmask,
	.irq_ack	= s3c2416_irq_dma_ack,
};

/* UART3 sub interrupts */

static void s3c2416_irq_demux_uart3(unsigned int irq, struct irq_desc *desc)
{
	s3c2416_irq_demux(IRQ_S3C2443_RX3, 3);
}

#define INTMSK_UART3	(1UL << (IRQ_S3C2443_UART3 - IRQ_EINT0))
#define SUBMSK_UART3	(0x7 << (IRQ_S3C2443_RX3 - S3C2410_IRQSUB(0)))

static void s3c2416_irq_uart3_mask(struct irq_data *data)
{
	s3c_irqsub_mask(data->irq, INTMSK_UART3, SUBMSK_UART3);
}

static void s3c2416_irq_uart3_unmask(struct irq_data *data)
{
	s3c_irqsub_unmask(data->irq, INTMSK_UART3);
}

static void s3c2416_irq_uart3_ack(struct irq_data *data)
{
	s3c_irqsub_maskack(data->irq, INTMSK_UART3, SUBMSK_UART3);
}

static struct irq_chip s3c2416_irq_uart3 = {
	.irq_mask	= s3c2416_irq_uart3_mask,
	.irq_unmask	= s3c2416_irq_uart3_unmask,
	.irq_ack	= s3c2416_irq_uart3_ack,
};

/* second interrupt register */

static inline void s3c2416_irq_ack_second(struct irq_data *data)
{
	unsigned long bitval = 1UL << (data->irq - IRQ_S3C2416_2D);

	__raw_writel(bitval, S3C2416_SRCPND2);
	__raw_writel(bitval, S3C2416_INTPND2);
}

static void s3c2416_irq_mask_second(struct irq_data *data)
{
	unsigned long bitval = 1UL << (data->irq - IRQ_S3C2416_2D);
	unsigned long mask;

	mask = __raw_readl(S3C2416_INTMSK2);
	mask |= bitval;
	__raw_writel(mask, S3C2416_INTMSK2);
}

static void s3c2416_irq_unmask_second(struct irq_data *data)
{
	unsigned long bitval = 1UL << (data->irq - IRQ_S3C2416_2D);
	unsigned long mask;

	mask = __raw_readl(S3C2416_INTMSK2);
	mask &= ~bitval;
	__raw_writel(mask, S3C2416_INTMSK2);
}

struct irq_chip s3c2416_irq_second = {
	.irq_ack	= s3c2416_irq_ack_second,
	.irq_mask	= s3c2416_irq_mask_second,
	.irq_unmask	= s3c2416_irq_unmask_second,
};


/* IRQ initialisation code */

static int s3c2416_add_sub(unsigned int base,
				   void (*demux)(unsigned int,
						 struct irq_desc *),
				   struct irq_chip *chip,
				   unsigned int start, unsigned int end)
{
	unsigned int irqno;

	irq_set_chip_and_handler(base, &s3c_irq_level_chip, handle_level_irq);
	irq_set_chained_handler(base, demux);

	for (irqno = start; irqno <= end; irqno++) {
		irq_set_chip_and_handler(irqno, chip, handle_level_irq);
		set_irq_flags(irqno, IRQF_VALID);
	}

	return 0;
}

static void s3c2416_irq_add_second(void)
{
	unsigned long pend;
	unsigned long last;
	int irqno;
	int i;

	/* first, clear all interrupts pending... */
	last = 0;
	for (i = 0; i < 4; i++) {
		pend = __raw_readl(S3C2416_INTPND2);

		if (pend == 0 || pend == last)
			break;

		__raw_writel(pend, S3C2416_SRCPND2);
		__raw_writel(pend, S3C2416_INTPND2);
		printk(KERN_INFO "irq: clearing pending status %08x\n",
		       (int)pend);
		last = pend;
	}

	for (irqno = IRQ_S3C2416_2D; irqno <= IRQ_S3C2416_I2S1; irqno++) {
		switch (irqno) {
		case IRQ_S3C2416_RESERVED2:
		case IRQ_S3C2416_RESERVED3:
			/* no IRQ here */
			break;
		default:
			irq_set_chip_and_handler(irqno, &s3c2416_irq_second,
						 handle_edge_irq);
			set_irq_flags(irqno, IRQF_VALID);
		}
	}
}

void __init s3c2416_init_irq(void)
{
	pr_info("S3C2416: IRQ Support\n");

	s3c24xx_init_irq();

	s3c2416_add_sub(IRQ_LCD, s3c2416_irq_demux_lcd, &s3c2416_irq_lcd,
			IRQ_S3C2443_LCD2, IRQ_S3C2443_LCD4);

	s3c2416_add_sub(IRQ_S3C2443_DMA, s3c2416_irq_demux_dma,
			&s3c2416_irq_dma, IRQ_S3C2443_DMA0, IRQ_S3C2443_DMA5);

	s3c2416_add_sub(IRQ_S3C2443_UART3, s3c2416_irq_demux_uart3,
			&s3c2416_irq_uart3,
			IRQ_S3C2443_RX3, IRQ_S3C2443_ERR3);

	s3c2416_add_sub(IRQ_WDT, s3c2416_irq_demux_wdtac97,
			&s3c2416_irq_wdtac97,
			IRQ_S3C2443_WDT, IRQ_S3C2443_AC97);

	s3c2416_irq_add_second();
}

#ifdef CONFIG_PM
static struct sleep_save irq2_save[] = {
	SAVE_ITEM(S3C2416_INTMSK2),
};

int s3c2416_irq_suspend(void)
{
	s3c_pm_do_save(irq2_save, ARRAY_SIZE(irq2_save));

	return 0;
}

void s3c2416_irq_resume(void)
{
	s3c_pm_do_restore(irq2_save, ARRAY_SIZE(irq2_save));
}

struct syscore_ops s3c2416_irq_syscore_ops = {
	.suspend	= s3c2416_irq_suspend,
	.resume		= s3c2416_irq_resume,
};
#endif

#endif
