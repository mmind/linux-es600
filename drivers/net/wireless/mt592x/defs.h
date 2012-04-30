/*
 * This file contains definitions and data structures specific
 * to Marvell 802.11 NIC. It contains the Device Information
 * structure struct lbs_private..
 */
#ifndef _MT592X_DEFS_H_
#define _MT592X_DEFS_H_


/* eeprom structure taken from the alcatel release
 * For the spi version at least signature and mac_addr are correct.
 */
struct mt592x_eeprom
{
    u16 signature;
    u16 sdio_cccr;
    u16 sdio_srw;
    u16 sdio_io_code;
    u16 sdio_sps;
    u16 sdio_ocr_l;
    u16 sdio_ocr_h;
    u16 dbg;
    u16 sdram_cfg;
    u16 hif_ctl;
    u16 cis_start;
    u16 cis_len;
    u16 chksum;
    u16 rsv_d;
    u16 rsv_e;
    u16 reg_domain;
    u16 osc_stable_time;
    u16 xtal_freq_trim;
    u16 version;
    u16 mac_addr[3];
    u16 thermo_usage_band_sel;
    u16 vga_thermo_slope;
    u16 us2GCckTxPwrTable[21];
    u16 us2GOfdm0TxPwrTable[21];
    u16 us2GOfdm1TxPwrTable[21];
    u16 us2GOfdm2TxPwrTable[21];
    u16 us2GOfdm3TxPwrTable[21];
    u16 thermo_info;
    u16 daisy_chain_cfg;
    u16 us2GRcpiOffsetTable[7];
    u16 rsv_8a[21];
    u16 nic_setting_check_sum;
    u16 sdio_cis_content[96];
    u16 probablyunused[256];
};




/* The definition in this ENUM is used to describe the HW QUEUEs including RX/TX
 * queues and also define the corresponding bit field for HW registers.
 */
typedef enum _ENUM_HW_QUE_T {
    AC0 = 0,
    AC1,
    AC2,
    AC3,
    TS0,
    AC4,
    RXQ,
    HW_QUE_NUM
} ENUM_HW_QUE_T;

#define HW_QUE_MASK                         (BIT(AC0) | BIT(AC1) | BIT(AC2) | \
                                             BIT(AC3) | BIT(TS0) | BIT(AC4) | \
                                             BIT(RXQ))

#define HW_QUE_TX_MASK                      (BIT(AC0) | BIT(AC1) | BIT(AC2) | \
                                             BIT(AC3) | BIT(TS0) | BIT(AC4))


/* QUEUE_ID */
typedef enum _ENUM_QUEUE_ID_T
{
    ENUM_QUEUE_ID_AC0 = 0,
    ENUM_QUEUE_ID_AC1,
    ENUM_QUEUE_ID_AC2,
    ENUM_QUEUE_ID_AC3,
    ENUM_QUEUE_ID_TSB,
    ENUM_QUEUE_ID_AC4,
    ENUM_QUEUE_ID_RX,
    ENUM_QUEUE_ID_SCAN = 8,
    ENUM_QUEUE_ID_SCAN_CTRL,
    ENUM_QUEUE_ID_MAX
} ENUM_QUEUE_ID_T, *PENUM_QUEUE_ID_T;

typedef enum _MIB_COUNTER_E {
    TX_COUNT = 0,
    BEACON_TX_COUNT,
    FRAME_RETRY_COUNT,
    RTS_RETRY_COUNT,
    RX_FCS_ERROR_COUNT,
    RX_FIFO_FULL_COUNT,
    RX_MPDU_COUNT,
    CHANNEL_IDLE_COUNT,
    CCATIME,
    CCA_NAV_TX_TIME,
    NUM_MIB_COUNTERS
} MIB_COUNTER_E;

#endif
