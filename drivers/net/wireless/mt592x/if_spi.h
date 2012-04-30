/*
 *	linux/drivers/net/wireless/libertas/if_spi.c
 *
 *	Driver for Marvell SPI WLAN cards.
 *
 *	Copyright 2008 Analog Devices Inc.
 *
 *	Authors:
 *	Andrey Yurovsky <andrey@cozybit.com>
 *	Colin McCabe <colin@cozybit.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifndef _MT592X_IF_SPI_H_
#define _MT592X_IF_SPI_H_

/************* SPI Control Status Register *************/
#define MCR_SPICSR			0x0018

#define SPICSR_MAX_TXSTATUS_MASK	0xf00000
#define SPICSR_MAX_TXSTATUS_SHIFT	20
#define SPICSR_MAX_RECEIVE_PKT_MASK	0x0f0000
#define SPICSR_MAX_RECEIVE_PKT_SHIFT	16

/* 00: 8-BIT MODE, 01: 16-BIT MODE
 * 10: 32-BIT MODE, 11: 8-BIT MODE
 */
#define SPICSR_MODE_SEL_MASK		0x600
#define SPICSR_MODE_SEL_32BIT		(1 << 10)
#define SPICSR_MODE_SEL_16BIT		(1 << 9)
#define SPICSR_MODE_SEL_8BIT		0

/* 0: LITTLE ENDIAN, 1: BIG ENDIAN */
#define SPICSR_BIG_ENDIAN		(1 << 8)

/* 0: NORMAL MODE, 1: FUNCTION PATTER GENERATION MODE */
#define SPICSR_FUNC_MODE		(1 << 2)
#define SPICSR_INTOUT_MODE		(1 << 1)
#define SPICSR_DATAOUT_MODE		(1 << 0)


/***************** SPI Interface Unit ******************/
/* Indication of the type of operation */
#define IF_SPI_ACTION_READ		0x0
#define IF_SPI_ACTION_WRITE		(1 << 31)

/* Number of bytes to send or receive.
 * Maximum currently seen is 376 bytes, but it could also be more.
 */
#define IF_SPI_NUM_BYTES(_n)		(_n << 16)

#endif
