/*
 * Driver for MediaTek MT5921 SPI WLAN cards.
 * Copyright 2012 Heiko Stuebner <heiko@sntech.de>
 *
 * based on
 *
 * linux/drivers/net/wireless/libertas/if_spi.c
 *
 * Driver for Marvell SPI WLAN cards.
 *
 * Copyright 2008 Analog Devices Inc.
 *
 * Authors:
 * Andrey Yurovsky <andrey@cozybit.com>
 * Colin McCabe <colin@cozybit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEBUG
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/spi/mt592x_spi.h>
#include <linux/spi/spi.h>

#include "host.h"
//#include "decl.h"
#include "defs.h"
#include "dev.h"
#include "if_spi.h"

struct if_spi_packet {
	struct list_head		list;
	u16				blen;
	u8				buffer[0] __attribute__((aligned(4)));
};

struct if_spi_card {
	struct spi_device		*spi;
	struct mt592x_private		*priv;
	struct mt592x_spi_platform_data *pdata;

	/* The card ID and card revision, as reported by the hardware. */
	u16				card_id;
	u8				card_rev;

	/* The last time that we initiated an SPU operation */
	unsigned long			prev_xfer_time;

	int				use_dummy_writes;
	unsigned long			spu_port_delay;
	unsigned long			spu_reg_delay;

	/* Handles all SPI communication (except for FW load) */
	struct workqueue_struct		*workqueue;
	struct work_struct		packet_work;
	struct work_struct		resume_work;

//	u8				cmd_buffer[IF_SPI_CMD_BUF_SIZE];

	/* A buffer of incoming packets from libertas core.
	 * Since we can't sleep in hw_host_to_card, we have to buffer
	 * them. */
	struct list_head		cmd_packet_list;
	struct list_head		data_packet_list;

	/* Protects cmd_packet_list and data_packet_list */
	spinlock_t			buffer_lock;

	/* True is card suspended */
	u8				suspended;
};

static void free_if_spi_card(struct if_spi_card *card)
{
	struct list_head *cursor, *next;
	struct if_spi_packet *packet;

	list_for_each_safe(cursor, next, &card->cmd_packet_list) {
		packet = container_of(cursor, struct if_spi_packet, list);
		list_del(&packet->list);
		kfree(packet);
	}

	list_for_each_safe(cursor, next, &card->data_packet_list) {
		packet = container_of(cursor, struct if_spi_packet, list);
		list_del(&packet->list);
		kfree(packet);
	}

	spi_set_drvdata(card->spi, NULL);
	kfree(card);
}

/*
 * SPI Interface Unit Routines
 *
 * The SPU sits between the host and the WLAN module.
 * All communication with the firmware is through SPU transactions.
 *
 * First we have to put a SPU register name on the bus. Then we can
 * either read from or write to that register.
 *
 */

static void mt592x_spi_transaction_init(struct if_spi_card *card)
{
	if (!time_after(jiffies, card->prev_xfer_time + 1)) {
	/* FIXME: from libertas; is this also true for the mt592x? */
		/* Unfortunately, the SPU requires a delay between successive
		 * transactions. If our last transaction was more than a jiffy
		 * ago, we have obviously already delayed enough.
		 * If not, we have to busy-wait to be on the safe side. */
		ndelay(400);
//		ndelay(800);
//		mdelay(10);
	}
}

static void mt592x_spi_transaction_finish(struct if_spi_card *card)
{
	card->prev_xfer_time = jiffies;
}

static int mt592x_spi_write(struct if_spi_card *card, u32 reg, u8 *buf, int len)
{
	struct spi_message m;
	struct spi_transfer reg_trans;
	struct spi_transfer data_trans;
	int err = 0;
	__le32 reg_out = cpu_to_le32(reg | IF_SPI_NUM_BYTES(len)
					 | IF_SPI_ACTION_WRITE);

	/* You must give an even number of bytes to the SPU, even if it
	 * doesn't care about the last one.  */
	BUG_ON(len & 0x1);

	spi_message_init(&m);
	memset(&reg_trans, 0, sizeof(reg_trans));
	memset(&data_trans, 0, sizeof(data_trans));

	mt592x_spi_transaction_init(card);

	/* write register index */
	reg_trans.tx_buf = &reg_out;
	reg_trans.len = sizeof(reg_out);
	spi_message_add_tail(&reg_trans, &m);

	/* write data */
	data_trans.tx_buf = buf;
	data_trans.len = len;
	spi_message_add_tail(&data_trans, &m);

	err = spi_sync(card->spi, &m);
	mt592x_spi_transaction_finish(card);

	return err;
}

/*
 * Read a number of bytes from a card register.
 */
static int mt592x_spi_read(struct if_spi_card *card, u32 reg, u8 *buf, int len)
{
	struct spi_message m;
	struct spi_transfer reg_trans;
	struct spi_transfer dummy_trans;
	struct spi_transfer data_trans;
	__le32 reg_out = cpu_to_le32(reg | IF_SPI_NUM_BYTES(sizeof(u32))
					 | IF_SPI_ACTION_READ);
	u32 dummy;
	int err;

	/* You must give an even number of bytes to the SPU, even if it
	 * doesn't care about the last one.  */
	BUG_ON(len & 0x1);

	spi_message_init(&m);
	memset(&reg_trans, 0, sizeof(reg_trans));
	memset(&dummy_trans, 0, sizeof(dummy_trans));
	memset(&data_trans, 0, sizeof(data_trans));

	mt592x_spi_transaction_init(card);

	/* write register index */
	reg_trans.tx_buf = &reg_out;
	reg_trans.len = sizeof(reg_out);
	spi_message_add_tail(&reg_trans, &m);

	/* MT592x expects 4 dummy bytes to be read */
	dummy_trans.rx_buf = &dummy;
	dummy_trans.len = 4;
	spi_message_add_tail(&dummy_trans, &m);

	/* read register data */
	data_trans.rx_buf = buf;
	data_trans.len = len;
	spi_message_add_tail(&data_trans, &m);

	err = spi_sync(card->spi, &m);
	mt592x_spi_transaction_finish(card);

	return err;
}

/*
 * Write 32 bits to a SPI register.
 * The write must be done en-block as 64 bits containing the
 * register in the lower 32 bit and the value in the upper 32 bit.
 * The lower bits are read first.
 */
static int mt592x_spi_write_u32(struct if_spi_card *card, u32 reg, u32 val)
{
	__le32 buff = cpu_to_le32(val);

	return mt592x_spi_write(card, reg, (u8 *)&buff, sizeof(u32));
}

/*
 * Read the 32 bit value of a card register
 */
static int mt592x_spi_read_u32(struct if_spi_card *card, u32 reg, u32 *val)
{
	__le32 buf;
	int err;

	err = mt592x_spi_read(card, reg, (u8 *)&buf, sizeof(u32));
	if (!err)
		*val = le32_to_cpu(buf);

	return err;
}

/*
 * Keep reading 32 bits from an SPI register until you get the correct result.
 *
 * If mask = 0, the correct result is any non-zero number.
 * If mask != 0, the correct result is any number where
 * number & target_mask == target.
 * If result param is set, returns the whole result in it.
 *
 * Returns -ETIMEDOUT if a second passes without the correct result.
 */
static int mt592x_spi_wait_for_u32(struct if_spi_card *card, u32 reg,
				   u32 target_mask, u32 target, u32 *result)
{
	int err;
	unsigned long timeout = jiffies + 5*HZ;
	while (1) {
		u32 val;
		err = mt592x_spi_read_u32(card, reg, &val);
		if (err)
			return err;

		if (target_mask) {
			if ((val & target_mask) == target) {
				if (result)
					*result = val;
				return 0;
			}
		} else {
			if (val) {
				if (result)
					*result = val;
				return 0;
			}
		}

		udelay(100);
		if (time_after(jiffies, timeout)) {
			pr_err("%s: timeout with val=%02x, target_mask=%02x, target=%02x\n",
			       __func__, val, target_mask, target);
			return -ETIMEDOUT;
		}
	}
}

/********************** libertas stuff ***********************/

static int spu_set_interrupt_mode(struct if_spi_card *card,
			   int suppress_host_int,
			   int auto_int)
{
	int err = 0;

	/*
	 * We can suppress a host interrupt by clearing the appropriate
	 * bit in the "host interrupt status mask" register
	 */
/*	if (suppress_host_int) {
		err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_MASK_REG, 0);
		if (err)
			return err;
	} else {
		err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_MASK_REG,
			      IF_SPI_HISM_TX_DOWNLOAD_RDY |
			      IF_SPI_HISM_RX_UPLOAD_RDY |
			      IF_SPI_HISM_CMD_DOWNLOAD_RDY |
			      IF_SPI_HISM_CARDEVENT |
			      IF_SPI_HISM_CMD_UPLOAD_RDY);
		if (err)
			return err;
	}*/

	/*
	 * If auto-interrupts are on, the completion of certain transactions
	 * will trigger an interrupt automatically. If auto-interrupts
	 * are off, we need to set the "Card Interrupt Cause" register to
	 * trigger a card interrupt.
	 */
/*	if (auto_int) {
		err = spu_write_u16(card, IF_SPI_HOST_INT_CTRL_REG,
				IF_SPI_HICT_TX_DOWNLOAD_OVER_AUTO |
				IF_SPI_HICT_RX_UPLOAD_OVER_AUTO |
				IF_SPI_HICT_CMD_DOWNLOAD_OVER_AUTO |
				IF_SPI_HICT_CMD_UPLOAD_OVER_AUTO);
		if (err)
			return err;
	} else {
		err = spu_write_u16(card, IF_SPI_HOST_INT_STATUS_MASK_REG, 0);
		if (err)
			return err;
	}*/
	return err;
}

static int spu_set_bus_mode(struct if_spi_card *card, u32 mode)
{
	int err = 0;
	u32 rval;

	/* set bus mode */
/*	err = spu_write_u32(card, MCR_SPICSR, mode);
	if (err)
		return err;*/

	/* Check that we were able to read back what we just wrote. */
/*	err = spu_read_u32(card, MCR_SPICSR, &rval);
	if (err)
		return err;

	if ((rval) != mode) {
		pr_err("Can't read bus mode register\n");
		return -EIO;
	}*/
	return 0;
}

static int spu_init(struct if_spi_card *card)
{
	struct spi_device *spi = card->spi;
	int err = 0;
	u32 delay;

//	card->use_dummy_writes = 0;
	err = spu_set_bus_mode(card, SPICSR_MODE_SEL_16BIT);
	
/*				IF_SPI_BUS_MODE_SPI_CLOCK_PHASE_RISING |
				IF_SPI_BUS_MODE_DELAY_METHOD_TIMED |
				IF_SPI_BUS_MODE_16_BIT_ADDRESS_16_BIT_DATA);*/
	if (err)
		return err;
/*
	card->spu_port_delay = 1000;
	card->spu_reg_delay = 1000;
	err = spu_read_u32(card, IF_SPI_DELAY_READ_REG, &delay);
	if (err)
		return err;
	card->spu_port_delay = delay & 0x0000ffff;
	card->spu_reg_delay = (delay & 0xffff0000) >> 16;*/

	/* If dummy clock delay mode has been requested, switch to it now */
/*	if (use_dummy_writes) {
		card->use_dummy_writes = 1;
		err = spu_set_bus_mode(card,
				IF_SPI_BUS_MODE_SPI_CLOCK_PHASE_RISING |
				IF_SPI_BUS_MODE_DELAY_METHOD_DUMMY_CLOCK |
				IF_SPI_BUS_MODE_16_BIT_ADDRESS_16_BIT_DATA);
		if (err)
			return err;
	}*/

	dev_dbg(&spi->dev, "Initialized SPU unit. "
		    "spu_port_delay=0x%04lx, spu_reg_delay=0x%04lx\n",
		    card->spu_port_delay, card->spu_reg_delay);
	return err;
}


/*
 * SPI Transfer Thread
 *
 * The SPI worker handles all SPI transfers, so there is no need for a lock.
 */

/* Move a command from the card to the host */
static int if_spi_c2h_cmd(struct if_spi_card *card)
{
	struct mt592x_private *priv = card->priv;
	unsigned long flags;
	int err = 0;
	u16 len;
	u8 i;

	/*
	 * We need a buffer big enough to handle whatever people send to
	 * hw_host_to_card
	 */
/*	BUILD_BUG_ON(IF_SPI_CMD_BUF_SIZE < LBS_CMD_BUFFER_SIZE);
	BUILD_BUG_ON(IF_SPI_CMD_BUF_SIZE < LBS_UPLD_SIZE);*/

	/*
	 * It's just annoying if the buffer size isn't a multiple of 4, because
	 * then we might have len < IF_SPI_CMD_BUF_SIZE but
	 * ALIGN(len, 4) > IF_SPI_CMD_BUF_SIZE
	 */
//	BUILD_BUG_ON(IF_SPI_CMD_BUF_SIZE % 4 != 0);

//	//lbs_deb_enter(LBS_DEB_SPI);

	/* How many bytes are there to read? */
/*	err = spu_read_u16(card, IF_SPI_SCRATCH_2_REG, &len);
	if (err)
		goto out;
	if (!len) {
		netdev_err(priv->dev, "%s: error: card has no data for host\n",
			   __func__);
		err = -EINVAL;
		goto out;
	} else if (len > IF_SPI_CMD_BUF_SIZE) {
		netdev_err(priv->dev,
			   "%s: error: response packet too large: %d bytes, but maximum is %d\n",
			   __func__, len, IF_SPI_CMD_BUF_SIZE);
		err = -EINVAL;
		goto out;
	}*/

	/* Read the data from the WLAN module into our command buffer */
/*	err = spu_read(card, IF_SPI_CMD_RDWRPORT_REG,
				card->cmd_buffer, ALIGN(len, 4));
	if (err)
		goto out;

	spin_lock_irqsave(&priv->driver_lock, flags);
	i = (priv->resp_idx == 0) ? 1 : 0;
	BUG_ON(priv->resp_len[i]);
	priv->resp_len[i] = len;
	memcpy(priv->resp_buf[i], card->cmd_buffer, len);
	lbs_notify_command_response(priv, i);
	spin_unlock_irqrestore(&priv->driver_lock, flags);*/

out:
	if (err)
		netdev_err(priv->dev, "%s: err=%d\n", __func__, err);
	//lbs_deb_leave(LBS_DEB_SPI);
	return err;
}

/* Move data from the card to the host */
static int if_spi_c2h_data(struct if_spi_card *card)
{
	struct mt592x_private *priv = card->priv;
	struct sk_buff *skb;
	char *data;
	u16 len;
	int err = 0;

	//lbs_deb_enter(LBS_DEB_SPI);

	/* How many bytes are there to read? */
/*	err = spu_read_u16(card, IF_SPI_SCRATCH_1_REG, &len);
	if (err)
		goto out;
	if (!len) {
		netdev_err(priv->dev, "%s: error: card has no data for host\n",
			   __func__);
		err = -EINVAL;
		goto out;
	} else if (len > MRVDRV_ETH_RX_PACKET_BUFFER_SIZE) {
		netdev_err(priv->dev,
			   "%s: error: card has %d bytes of data, but our maximum skb size is %zu\n",
			   __func__, len, MRVDRV_ETH_RX_PACKET_BUFFER_SIZE);
		err = -EINVAL;
		goto out;
	}*/

	/* TODO: should we allocate a smaller skb if we have less data? */
/*	skb = dev_alloc_skb(MRVDRV_ETH_RX_PACKET_BUFFER_SIZE);
	if (!skb) {
		err = -ENOBUFS;
		goto out;
	}
	skb_reserve(skb, IPFIELD_ALIGN_OFFSET);
	data = skb_put(skb, len);*/

	/* Read the data from the WLAN module into our skb... */
/*	err = spu_read(card, IF_SPI_DATA_RDWRPORT_REG, data, ALIGN(len, 4));
	if (err)
		goto free_skb;*/

	/* pass the SKB to libertas */
/*	err = lbs_process_rxed_packet(card->priv, skb);
	if (err)
		goto free_skb;*/

	/* success */
	goto out;

//free_skb:
//	dev_kfree_skb(skb);
out:
	if (err)
		netdev_err(priv->dev, "%s: err=%d\n", __func__, err);
	//lbs_deb_leave(LBS_DEB_SPI);
	return err;
}

/* Move data or a command from the host to the card. */
/*static void if_spi_h2c(struct if_spi_card *card,
			struct if_spi_packet *packet, int type)
{
	struct mt592x_private *priv = card->priv;
	int err = 0;
	u16 int_type, port_reg;

	switch (type) {
	case MVMS_DAT:
		int_type = IF_SPI_CIC_TX_DOWNLOAD_OVER;
		port_reg = IF_SPI_DATA_RDWRPORT_REG;
		break;
	case MVMS_CMD:
		int_type = IF_SPI_CIC_CMD_DOWNLOAD_OVER;
		port_reg = IF_SPI_CMD_RDWRPORT_REG;
		break;
	default:
		netdev_err(priv->dev, "can't transfer buffer of type %d\n",
			   type);
		err = -EINVAL;
		goto out;
	}

	err = spu_write(card, port_reg, packet->buffer, packet->blen);
	if (err)
		goto out;

out:
	kfree(packet);

	if (err)
		netdev_err(priv->dev, "%s: error %d\n", __func__, err);
}*/

/* Inform the host about a card event */
/*static void if_spi_e2h(struct if_spi_card *card)
{
	int err = 0;
	u32 cause;
	struct mt592x_private *priv = card->priv;

	err = spu_read_u32(card, IF_SPI_SCRATCH_3_REG, &cause);
	if (err)
		goto out;

	// re-enable the card event interrupt
	spu_write_u16(card, IF_SPI_HOST_INT_STATUS_REG,
			~IF_SPI_HICU_CARD_EVENT);

	// generate a card interrupt
	spu_write_u16(card, IF_SPI_CARD_INT_CAUSE_REG, IF_SPI_CIC_HOST_EVENT);

	lbs_queue_event(priv, cause & 0xff);
out:
	if (err)
		netdev_err(priv->dev, "%s: error %d\n", __func__, err);
}*/

/*static void if_spi_host_to_card_worker(struct work_struct *work)
{
	int err;
	struct if_spi_card *card;
	u16 hiStatus;
	unsigned long flags;
	struct if_spi_packet *packet;
	struct mt592x_private *priv;

	card = container_of(work, struct if_spi_card, packet_work);
	priv = card->priv;

	//lbs_deb_enter(LBS_DEB_SPI);

	
	 * Read the host interrupt status register to see what we
	 * can do.
	 
	err = spu_read_u16(card, IF_SPI_HOST_INT_STATUS_REG,
				&hiStatus);
	if (err) {
		netdev_err(priv->dev, "I/O error\n");
		goto err;
	}

	if (hiStatus & IF_SPI_HIST_CMD_UPLOAD_RDY) {
		err = if_spi_c2h_cmd(card);
		if (err)
			goto err;
	}
	if (hiStatus & IF_SPI_HIST_RX_UPLOAD_RDY) {
		err = if_spi_c2h_data(card);
		if (err)
			goto err;
	}

	
	 * workaround: in PS mode, the card does not set the Command
	 * Download Ready bit, but it sets TX Download Ready.
	 
	if (hiStatus & IF_SPI_HIST_CMD_DOWNLOAD_RDY ||
	   (card->priv->psstate != PS_STATE_FULL_POWER &&
	    (hiStatus & IF_SPI_HIST_TX_DOWNLOAD_RDY))) {
		
		 * This means two things. First of all,
		 * if there was a previous command sent, the card has
		 * successfully received it.
		 * Secondly, it is now ready to download another
		 * command.
		 
		lbs_host_to_card_done(card->priv);

		// Do we have any command packets from the host to send?
		packet = NULL;
		spin_lock_irqsave(&card->buffer_lock, flags);
		if (!list_empty(&card->cmd_packet_list)) {
			packet = (struct if_spi_packet *)(card->
					cmd_packet_list.next);
			list_del(&packet->list);
		}
		spin_unlock_irqrestore(&card->buffer_lock, flags);

		if (packet)
			if_spi_h2c(card, packet, MVMS_CMD);
	}
	if (hiStatus & IF_SPI_HIST_TX_DOWNLOAD_RDY) {
		// Do we have any data packets from the host to send?
		packet = NULL;
		spin_lock_irqsave(&card->buffer_lock, flags);
		if (!list_empty(&card->data_packet_list)) {
			packet = (struct if_spi_packet *)(card->
					data_packet_list.next);
			list_del(&packet->list);
		}
		spin_unlock_irqrestore(&card->buffer_lock, flags);

		if (packet)
			if_spi_h2c(card, packet, MVMS_DAT);
	}
	if (hiStatus & IF_SPI_HIST_CARD_EVENT)
		if_spi_e2h(card);

err:
	if (err)
		netdev_err(priv->dev, "%s: got error %d\n", __func__, err);

	//lbs_deb_leave(LBS_DEB_SPI);
}*/

/*
 * Host to Card
 *
 * Called from Libertas to transfer some data to the WLAN device
 * We can't sleep here.
 */
/*static int if_spi_host_to_card(struct mt592x_private *priv,
				u8 type, u8 *buf, u16 nb)
{
	int err = 0;
	unsigned long flags;
	struct if_spi_card *card = priv->card;
	struct if_spi_packet *packet;
	u16 blen;

	//lbs_deb_enter_args(LBS_DEB_SPI, "type %d, bytes %d", type, nb);

	if (nb == 0) {
		netdev_err(priv->dev, "%s: invalid size requested: %d\n",
			   __func__, nb);
		err = -EINVAL;
		goto out;
	}
	blen = ALIGN(nb, 4);
	packet = kzalloc(sizeof(struct if_spi_packet) + blen, GFP_ATOMIC);
	if (!packet) {
		err = -ENOMEM;
		goto out;
	}
	packet->blen = blen;
	memcpy(packet->buffer, buf, nb);
	memset(packet->buffer + nb, 0, blen - nb);

	switch (type) {
	case MVMS_CMD:
		priv->dnld_sent = DNLD_CMD_SENT;
		spin_lock_irqsave(&card->buffer_lock, flags);
		list_add_tail(&packet->list, &card->cmd_packet_list);
		spin_unlock_irqrestore(&card->buffer_lock, flags);
		break;
	case MVMS_DAT:
		priv->dnld_sent = DNLD_DATA_SENT;
		spin_lock_irqsave(&card->buffer_lock, flags);
		list_add_tail(&packet->list, &card->data_packet_list);
		spin_unlock_irqrestore(&card->buffer_lock, flags);
		break;
	default:
		kfree(packet);
		netdev_err(priv->dev, "can't transfer buffer of type %d\n",
			   type);
		err = -EINVAL;
		break;
	}

	// Queue spi xfer work
	queue_work(card->workqueue, &card->packet_work);
out:
	//lbs_deb_leave_args(LBS_DEB_SPI, "err=%d", err);
	return err;
}*/

/*
 * Host Interrupts
 *
 * Service incoming interrupts from the WLAN device. We can't sleep here, so
 * don't try to talk on the SPI bus, just queue the SPI xfer work.
 */
static irqreturn_t mt592x_spi_host_interrupt(int irq, void *dev_id)
{
	struct if_spi_card *card = dev_id;

printk("mt592x: interrupt\n");
//	queue_work(card->workqueue, &card->packet_work);

	return IRQ_HANDLED;
}

/******* move to generic code later on **************/

/* MT592x also knows the concept of port registers
 * i.e. registers that accept data streams 
 */
static inline int mt592x_reg_is_port_reg(u32 reg)
{
	switch (reg) {
	case MCR_HTDR:
	case MCR_HRDR:
	/* probably some more? */
		return 1;
	default:
		return 0;
	}
}

static int mt592x_spi_get_chip_revision(struct if_spi_card *card,
					u16 *card_id, u8 *card_rev)
{
	int err = 0;
	u32 dev_ctrl;
	err = mt592x_spi_read_u32(card, MCR_CIR, &dev_ctrl);
	if (err)
		return err;

	*card_id = CIR_TO_CARD_ID(dev_ctrl);
	*card_rev = CIR_TO_CARD_REV(dev_ctrl);
	return err;
}

int mt592x_read_eeprom(struct if_spi_card *card, u16 *buf, int len)
{
	struct spi_device *spi = card->spi;
	u32 eeprom_info, val;
	int ret, i, size;

	ret = mt592x_spi_read_u32(card, MCR_ESCR, &eeprom_info);
	if (ret) {
		dev_err(&spi->dev, "could not read eeprom info, %d\n", ret);
		return ret;
	}

	if ((eeprom_info & ESCR_2048BYTE) == ESCR_2048BYTE)
		size = 2048;
	else if ((eeprom_info & ESCR_1024BYTE) == ESCR_1024BYTE)
		size = 1024;
	else if ((eeprom_info & ESCR_512BYTE) == ESCR_512BYTE)
		size = 512;
	else if ((eeprom_info & ESCR_256BYTE) == ESCR_256BYTE)
		size = 256;
	else if ((eeprom_info & ESCR_128BYTE) == ESCR_128BYTE)
		size = 128;
	else
		size = 0;

	if (!size) {
		dev_err(&spi->dev, "wrong eeprom size or no eeprom\n");
		return -EINVAL;
	}

	if (size > len) {
		dev_err(&spi->dev, "%d byte eeprom to large for data structure, max: %d\n", size, len);
		return -EINVAL;
	}

	dev_dbg(&spi->dev, "found a %d byte eeprom\n", size);

	ret = mt592x_spi_wait_for_u32(card, MCR_EADR, EADR_EE_RDY, EADR_EE_RDY, NULL);
	if (ret) {
		dev_err(&spi->dev, "eeprom not ready after 5 seconds, %d\n", ret);
		return -EBUSY;
	}

	/* read result always contains two bytes */
	for (i = 0; i < (size >> 1); i++) {
		ret = mt592x_spi_write_u32(card, MCR_EADR, EADR_EE_READ | EADR_SET_ADDRESS(i));

		/* FIXME: check if  EE_RDY really indicates the readiness of the eeprom data */
		ret = mt592x_spi_wait_for_u32(card, MCR_EADR, EADR_EE_RDY, EADR_EE_RDY, &val);
		if (ret) {
			dev_err(&spi->dev, "couldn't read eeprom element %d , %d\n", i, ret);
			return ret;
		}

		/* FIXME: check which byte ordering is correct */
		val = EADR_CLEAN_DATA(val);
		buf[i] = val;
//		buf[2*i+1] = ((val & 0xff00) >> 8);
//		buf[2*i] = (val & 0xff);
	}

for(i = 0; i < (size >> 1); i++)
  printk("0x%04x\n", buf[i]);

	return 0;
}


/*
 * SPI callbacks
 */

static int if_spi_init_card(struct if_spi_card *card)
{
	struct mt592x_private *priv = card->priv;
	int err, i;
	u32 val;
	const struct firmware *helper = NULL;
	const struct firmware *mainfw = NULL;

	//lbs_deb_enter(LBS_DEB_SPI);

//	dev_dbg(
	err = spu_init(card);
	if (err)
		goto out;

	err = mt592x_spi_get_chip_revision(card, &card->card_id, &card->card_rev);
	if (err)
		goto out;

printk("(chip_id = 0x%04x, chip_rev = 0x%02x) \n", card->card_id, card->card_rev);

mt592x_spi_read_u32(card, MCR_CIR, &val);
printk("CIR: %x\n", val);
mt592x_spi_read_u32(card, MCR_CIR, &val);
printk("CIR: %x\n", val);
mt592x_spi_read_u32(card, MCR_CIR, &val);
printk("CIR: %x\n", val);
	/* poweron sequence modeled after the proprietary driver */

	/* power down everything */ 
	mt592x_spi_write_u32(card, MCR_HLPCR, HLPCR_OSC_OUT_PD | HLPCR_BG_PD | HLPCR_PLL_PD | HLPCR_ADC_BUFFER_PD | HLPCR_INTERNAL_32K_PD | HLPCR_EXTERNAL_32K_PD | HLPCR_RF_SX_PD | HLPCR_PLL_CLOCK_GATED);
        msleep(100);

	/* some resets */
	mt592x_spi_write_u32(card, MCR_HLPCR, HLPCR_HIFMAC_LOGRST | HLPCR_BB_LOGRST | HLPCR_LP_OWN_CLR | HLPCR_PLL_CLOCK_GATED);
        msleep(100);

	/* enable the oscilator */
	mt592x_spi_write_u32(card, MCR_HLPCR, HLPCR_OSC_EN | HLPCR_PLL_CLOCK_GATED);
        msleep(100);

	/* enable more stuff */
	mt592x_spi_write_u32(card, MCR_HLPCR, HLPCR_BG_EN | HLPCR_EXTERNAL_32K_EN | HLPCR_RF_SX_EN | HLPCR_PLL_CLOCK_GATED);
        msleep(100);

	/* enable still more stuff */
	mt592x_spi_write_u32(card, MCR_HLPCR, HLPCR_OSC_OUT_EN | HLPCR_PLL_EN | HLPCR_ADC_BUFFER_EN | HLPCR_PLL_CLOCK_GATED);
        msleep(100);

	/* ungate the pll */
	mt592x_spi_write_u32(card, MCR_HLPCR, 0);
        msleep(100);
return 0;

	/* configure IO pins */
	mt592x_spi_read_u32(card, MCR_IOPCR, &val);
	val |= IOPCR_IO_TR_SW_P_DIR | IOPCR_IO_TR_SW_N_DIR | IOPCR_IO_ANT_SEL_P_DIR | IOPCR_IO_ANT_SEL_N_DIR;
	mt592x_spi_write_u32(card, MCR_IOPCR, val);
        msleep(100);

	mt592x_spi_read_u32(card, MCR_IOUDR, &val);
	val &= ~IOUDR_BT_PRI_PU;
	val |= IOUDR_WE_N_PU | IOUDR_OE_N_PU | IOUDR_CS_N_PU;
	mt592x_spi_write_u32(card, MCR_IOUDR, val);
        msleep(100);


/*
recv: MCR_HISR <3>0x0 <3>0x0 <3>0x0 <3>0x0 <3>
recv: MCR_HSCISR <3>0x0 <3>0x0 <3>0x0 <3>0x0 <3>
recv: BBCR_BPROBECTR <3>0x0 <3>0x0 <3>0x0 <3>0x0 <3>
*/

/*
	err = spu_set_interrupt_mode(card, 0, 1);
	if (err)
		goto out;
*/
out:
/*	if (helper)
		release_firmware(helper);
	if (mainfw)
		release_firmware(mainfw);
*/
	//lbs_deb_leave_args(LBS_DEB_SPI, "err %d\n", err);

	return err;
}

static void if_spi_resume_worker(struct work_struct *work)
{
	struct if_spi_card *card;

	card = container_of(work, struct if_spi_card, resume_work);

	if (card->suspended) {
		if (card->pdata->setup)
			card->pdata->setup(card->spi);

		/* Init card ... */
		if_spi_init_card(card);

		enable_irq(card->spi->irq);

		/* And resume it ... */
//		lbs_resume(card->priv);

		card->suspended = 0;
	}
}

static int __devinit mt592x_spi_probe(struct spi_device *spi)
{
	struct if_spi_card *card;
	struct mt592x_private *priv = NULL;
	struct mt592x_spi_platform_data *pdata = spi->dev.platform_data;
	int err = 0;
	
	struct mt592x_eeprom tmp;
	int len;
	u8 *mac;

	//lbs_deb_enter(LBS_DEB_SPI);

	if (!pdata) {
		err = -EINVAL;
		goto out;
	}

	if (pdata->setup) {
		err = pdata->setup(spi);
		if (err)
			goto out;
	}

	/* Allocate card structure to represent this specific device */
	card = kzalloc(sizeof(struct if_spi_card), GFP_KERNEL);
	if (!card) {
		err = -ENOMEM;
		goto teardown;
	}
	spi_set_drvdata(spi, card);
	card->pdata = pdata;
	card->spi = spi;
	card->prev_xfer_time = jiffies;

	INIT_LIST_HEAD(&card->cmd_packet_list);
	INIT_LIST_HEAD(&card->data_packet_list);
	spin_lock_init(&card->buffer_lock);


	err = request_irq(spi->irq, mt592x_spi_host_interrupt,
			IRQF_TRIGGER_FALLING, "mt592x_spi", card);
	if (err) {
		dev_err(&spi->dev, "can't request irq, %d\n", err);
		goto terminate_workqueue;
	}



	/* Initialize the SPI Interface Unit */

	/* Firmware load */
	err = if_spi_init_card(card);
	if (err)
		goto free_card;

	 mt592x_read_eeprom(card, (u16 *)&tmp, sizeof(tmp));

mac = (u8 *)&tmp.mac_addr;
printk("mac %02x:%02x:%02x:%02x:%02x:%02x\n",
       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

return 0;
	/*
	 * Register our card with libertas.
	 * This will call alloc_etherdev.
	 */
/*	priv = lbs_add_card(card, &spi->dev);
	if (!priv) {
		err = -ENOMEM;
		goto free_card;
	}*/
	card->priv = priv;
	priv->setup_fw_on_resume = 1;
	priv->card = card;
//	priv->hw_host_to_card = if_spi_host_to_card;
	priv->enter_deep_sleep = NULL;
	priv->exit_deep_sleep = NULL;
	priv->reset_deep_sleep_wakeup = NULL;
	priv->fw_ready = 1;

	/* Initialize interrupt handling stuff. */
	card->workqueue = create_workqueue("mt592x_spi");
//	INIT_WORK(&card->packet_work, if_spi_host_to_card_worker);
//	INIT_WORK(&card->resume_work, if_spi_resume_worker);

	/*
	 * Start the card.
	 * This will call register_netdev, and we'll start
	 * getting interrupts...
	 */
/*	err = lbs_start_card(priv);
	if (err)
		goto release_irq;*/

	//lbs_deb_spi("Finished initializing WLAN module.\n");

	/* successful exit */
	goto out;

release_irq:
	free_irq(spi->irq, card);
terminate_workqueue:
	flush_workqueue(card->workqueue);
	destroy_workqueue(card->workqueue);
//	lbs_remove_card(priv); /* will call free_netdev */
free_card:
	free_irq(spi->irq, card);
	free_if_spi_card(card);
teardown:
	if (pdata->teardown)
		pdata->teardown(spi);
out:
	//lbs_deb_leave_args(LBS_DEB_SPI, "err %d\n", err);
	return err;
}

static int __devexit mt592x_spi_remove(struct spi_device *spi)
{
	struct if_spi_card *card = spi_get_drvdata(spi);
	struct mt592x_private *priv = card->priv;

//	cancel_work_sync(&card->resume_work);

//	lbs_stop_card(priv);
//	lbs_remove_card(priv); /* will call free_netdev */

	free_irq(spi->irq, card);
/*	flush_workqueue(card->workqueue);
	destroy_workqueue(card->workqueue);*/
	if (card->pdata->teardown)
		card->pdata->teardown(spi);
	else
		dev_err(&spi->dev, "no teardown callback\n");
	free_if_spi_card(card);
	return 0;
}

static int if_spi_suspend(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);
	struct if_spi_card *card = spi_get_drvdata(spi);

	if (!card->suspended) {
//		lbs_suspend(card->priv);
		flush_workqueue(card->workqueue);
		disable_irq(spi->irq);

		if (card->pdata->teardown)
			card->pdata->teardown(spi);
		card->suspended = 1;
	}

	return 0;
}

static int if_spi_resume(struct device *dev)
{
	struct spi_device *spi = to_spi_device(dev);
	struct if_spi_card *card = spi_get_drvdata(spi);

	/* Schedule delayed work */
	schedule_work(&card->resume_work);

	return 0;
}

static const struct dev_pm_ops if_spi_pm_ops = {
	.suspend	= if_spi_suspend,
	.resume		= if_spi_resume,
};

static struct spi_driver mt592x_spi_driver = {
	.probe	= mt592x_spi_probe,
	.remove = __devexit_p(mt592x_spi_remove),
	.driver = {
		.name	= "mt592x_spi",
		.owner	= THIS_MODULE,
		.pm	= &if_spi_pm_ops,
	},
};

/*
 * Module functions
 */

static int __init if_spi_init_module(void)
{
	return spi_register_driver(&mt592x_spi_driver);
}

static void __exit if_spi_exit_module(void)
{
	spi_unregister_driver(&mt592x_spi_driver);
}

module_init(if_spi_init_module);
module_exit(if_spi_exit_module);

MODULE_DESCRIPTION("MT592x SPI WLAN Driver");
MODULE_AUTHOR("Heiko Stuebner <heiko@sntech.de>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:mt592x_spi");
