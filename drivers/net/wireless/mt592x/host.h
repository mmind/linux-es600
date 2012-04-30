/*
 * This file function prototypes, data structure
 * and  definitions for all the host/station commands
 */

#ifndef _MT592X_HOST_H_
#define _MT592X_HOST_H_

#include "types.h"
#include "defs.h"

/* Common Host Interface */
#define MCR_CIR		0x0000	/* CHIP ID Register */
#define MCR_HISR	0x0004	/* HIF Interrupt Status Register */
#define MCR_HSCISR	0x0008	/* HIF Slow Clock Interrupt Status Register */
#define MCR_HIER	0x000C	/* HIF Interrupt Enable Register */
#define MCR_HSCIER	0x0010	/* HIF Slow Clock Interrupt Enable Register */
#define MCR_HCR		0x0014	/* HIF Control Register */
#define MCR_RR		0x001C	/* RSSI Register */
#define MCR_HTDR	0x0020	/* HIF Tx Data Register */
#define MCR_HTSR	0x0024	/* HIF Tx Status Register */
#define MCR_HRDR	0x0028	/* HIF Rx Data Register */
#define MCR_QAR		0x002C	/* Queue Address Register */
#define MCR_HBDR	0x0030	/* HIF Beacon Data Register */
#define MCR_HWTDR	0x0034	/* HIF WLAN Table Data Register */
#define MCR_HWTCR	0x0038	/* HIF WLAN Table Control Register */
#define MCR_HLPCR	0x003C	/* HIF Low Power Control Register */
#define MCR_HFCR	0x0040	/* HIF FIFO Control Register */
#define MCR_HSDR	0x0044	/* HIF Scan Data Register */
#define MCR_HFDR	0x0044	/* FIXME: probably HIF FIFO Data Register */
#define MCR_HITR	0x0048	/* HIF Interrupt threshold Register */
#define MCR_ASR		0x004C	/* ASR Abnormal Status Register */



/* CONFIG */
#define MCR_DBGR	0x0050	/* Debug Register */
#define MCR_ESCR	0x0054	/* EEPROM Software Control Register */
#define MCR_EADR	0x0058	/* EEPROM Address and Data Register */
#define MCR_32KCCR	0x005C	/* 32K Clock Calibration Register */
#define MCR_CCR		0x0060	/* Clock Control Register */
#define MCR_ODCR	0x0064	/* Output Driving Control Register */
#define MCR_IOUDR	0x0068	/* I/O Pull Up/Down Register */
#define MCR_IOPCR	0x006C	/* I/O Pin Control Register */
#define MCR_SDCR	0x0070	/* SDCR: SRAM Driving Control Register */
#define MCR_ACDR0	0x0074	/* AFE Configuration Data 0 Registers */
#define MCR_ACDR1	0x0078	/* AFE Configuration Data 1 Registers */
#define MCR_ACDR2	0x007C	/* AFE Configuration Data 2 Registers */
#define MCR_ACDR3	0x0080	/* AFE Configuration Data 3 Registers */
#define MCR_ACDR4	0x0084	/* AFE Configuration Data 4 Registers */


/* MAC */
#define MCR_SCR		0x0090	/* System Control Register */
#define MCR_LCR		0x0094	/* LED Control Register */
#define MCR_RFICR	0x0098	/* RF Interface Control Register */
#define MCR_RICR	0x0098	/* FIXME */
#define MCR_RFSCR	0x009C	/* RF Synthesizer Configuration Register */
#define MCR_RSCR	0x009C	/* FIXME */
#define MCR_RFSDR	0x00A0	/* RF Synthesizer Data Register */
#define MCR_RSDR	0x00A0	/* FIXME */
#define MCR_ARPR0	0x00A4	/* Auto Rate Parameter Register 0 */
#define MCR_ARPR1	0x00A8	/* Auto Rate Parameter Register 1 */
#define MCR_ARPR2	0x00AC	/* Auto Rate Parameter Register 2 */
#define MCR_MMCR0	0x00B0	/* FIXME */
#define MCR_MMCR1	0x00B4	/* FIXME */
#define MCR_QCCR0	0x00B8	/* Quiet Channel Control Register 0 */
#define MCR_QCCR1	0x00BC	/* Quiet Channel Control Register 1 */
#define MCR_MIBSCR	0x00C0	/* MIB Status Control Register */
#define MCR_MIBSDR	0x00C4	/* MIB Status Data  Register */


/* ARB */
#define MCR_QCR		0x00D0	/* Queue Control Register */
#define MCR_AIFSR	0x00D4	/* Arbitration InterFrame Spacing Register */
#define MCR_MTCLR	0x00D8	/* Maximum Tx Count Limit Register */
#define MCR_ACCWXR0	0x00DC	/* AC Contention Window Max Register 0 */
#define MCR_ACCWXR1	0x00E0	/* AC Contention Window Max Register 1 */
#define MCR_ACCWIR	0x00E4	/* AC Contention Window Min Register */
#define MCR_AC4CWR	0x00E8	/* AC4 Contention Window Register */
#define MCR_BCWR	0x00EC	/* Beacon Contention Window Register */
#define MCR_DRNGR	0x00F0	/* DCF Random Number Generator Register */
#define MCR_MSCR	0x00F4	/* MAC Status Control Register */
#define MCR_ARCR	0x00F8	/* Auto Rate Control Register */
#define MCR_TXCR	0x00FC	/* Tx Control Register */

/* TMAC */
#define MCR_TRANSMITCR	0x0100	/* Transmit Control Register */
#define MCR_ACTXOPLR0	0x0104	/* AC TxOP Limit Register 0 */
#define MCR_ACTXOPLR1	0x0108	/* AC TxOP Limit Register 1 */
#define MCR_CRFR0	0x010C	/* Control Response Frame Regiester 0 */
#define MCR_CRFR1	0x0110	/* Control Response Frame Regiester 1 */
#define MCR_OFPR	0x0114	/* OFDM Frame Power Register */
#define MCR_CFPR	0x0118	/* CCK Frame Power Register */
#define MCR_BTCER0	0x011C	/* Bluetooth Co-existence Regiester 0 */
#define MCR_BTCER1	0x0120	/* Bluetooth Co-existence Regiester 1 */
#define MCR_BTCER2	0x0124	/* Bluetooth Co-existence Regiester 2 */
#define MCR_BTCER3	0x0128	/* Bluetooth Co-existence Regiester 3 */
#define MCR_BRUR0	0x012C	/* BB Ramp UP Register 0 */
#define MCR_BRUR1	0x0130	/* BB Ramp UP Register 1 */
#define MCR_BRDR	0x0134	/* BB Ramp Down Register */
#define MCR_BPDR	0x0138	/* BB Processing Delay Register */
#define MCR_CDTR	0x013C	/* CCK DCF Timeout Register */
#define MCR_ODTR	0x0140	/* OFDM DCF TImeout Register */
#define MCR_RCCR	0x0144	/* RTS/CTS Control Register */

/* RMAC */
#define MCR_RFCR	0x0150	/* Receive Filter Control Register */
#define MCR_SSCR	0x0154	/* SSID Scan Contrl Register */
#define MCR_SDR		0x0158	/* SSID Scan Data Regiester */
#define MCR_CBR0	0x015C	/* Current BSSID Register 0 */
#define MCR_CBR1	0x0160	/* Current BSSID Register 1 */
#define MCR_MUAR0	0x0164	/* Multicast/Unicast Address Register 0 */
#define MCR_MUAR1	0x0168	/* Multicast/Unicast Address Register 1 */
#define MCR_QDR		0x016C	/* Qos Data Register */
#define MCR_NSUR	0x0170	/* NAV Software Update Register */
#define MCR_RMLR	0x0174	/* Rx Max Packet Length Register */
#define MCR_ALCR0	0x0178	/* ALC Control Register 0 */
#define MCR_ALCR1	0x017C	/* ALC Control Register 1 */

/* LP */
#define MCR_LTTR0	0x0180	/* Local TSF Timer Register 0 */
#define MCR_LTTR1	0x0184	/* Local TSF Timer Register 1 */
#define MCR_PPR		0x018C	/* Packet Pattern Register */
#define MCR_LPWSAR	0x0190	/* Lower Power Wake Starting Address Register */
#define MCR_MPTCR	0x0194	/* MAC Periodic Timer Control Register */
#define MCR_ACWR	0x0198	/* ATIM/CFP Window Register */
#define MCR_BSSSAR	0x019C	/* Background SSID Scan Starting Address Reg */
#define MCR_TDR		0x01A0	/* Timer Data Register */
#define MCR_ATCR	0x01A4	/* Auto Transmit Control Register */
#define MCR_TSTR	0x01A8	/* TBTT Starting Time Register */
#define MCR_TTPCR	0x01AC	/* TBTT Periodic Control Register */

#define MCR_CTPCR	0x01B0	/* CFP Time Period Control Register */
#define MCR_LPCR	0x01B4	/* Low Power Control Register */
#define MCR_PIR		0x01B8	/* Packet Index Register */
#define MCR_PMR		0x01BC	/* Pattern Mask Register */
#define MCR_UTTR0	0x01C0	/* Updated TSF Timer Register 0 */
#define MCR_UTTR1	0x01C4	/* Updated TSF Timer Register 1 */
#define MCR_TTAR	0x01C8	/* TSF Timer Adjustment Register */
#define MCR_LPOSAR	0x01CC	/* Low PowerOn Starting Address Register */
#define MCR_MTR0	0x01D0	/* Media Time Register */
#define MCR_MTCR0	0x01D4	/* Media Time Control Register */
#define MCR_TR		0x01D8	/* Timeout Register */
#define MCR_MTR1	0x01E0	/* Media Time Register */
#define MCR_MTCR1	0x01E4	/* Media Time Control Register */
#define MCR_SPCR	0x01E8	/* Service Period Control Register */
#define MCR_BEIR	0x01EC	/* Beacon Earlier Interval Register */


/* BB */
#define BBCR_BIDR	0x0200	/* BB ID Register CR0 */
#define BBCR_BCR	0x0204	/* BB Configuration Register CR1 */
#define BBCR_BISR	0x0208	/* BB Interrupt Status Register CR2 */
#define BBCR_BTSR	0x020C	/* BB TX Service Register */
#define BBCR_BOTBOR0	0x0210	/* BB OFDM TX Back-off Register 0 */
#define BBCR_BOTBOR1	0x0214	/* BB OFDM TX Back-off Register 1 */
#define BBCR_BOTBOR2	0x0218	/* BB OFDM TX Back-off Register 2 */
#define BBCR_BOTFR0	0x021C	/* BB OFDM TX Filter Register 0 */
#define BBCR_BOTFR1	0x0220	/* BB OFDM TX Filter Register 1 */
#define BBCR_BOTFR2	0x0224	/* BB OFDM TX Filter Register 2 */
#define BBCR_BOTFR3	0x0228	/* BB OFDM TX Filter Register 3 */
#define BBCR_BOTFR4	0x022C	/* BB OFDM TX Filter Register 4 */
#define BBCR_BOTFR5	0x0230	/* BB OFDM TX Filter Register 5 */
#define BBCR_BCTFR0	0x0234	/* BB CCK TX Filter Register 0 */
#define BBCR_BCTFR1	0x0238	/* BB CCK TX Filter Register 1 */
#define BBCR_BCTFR2	0x023C	/* BB CCK TX Filter Register 2 */
#define BBCR_BATR0	0x0240	/* BB AGC Table Register 0-34 */
#define BBCR_BATR1	0x0244	/* BB AGC Table Register 1 */
#define BBCR_BATR2	0x0248	/* BB AGC Table Register 2 */
#define BBCR_BATR3	0x024C	/* BB AGC Table Register 3 */
#define BBCR_BATR4	0x0250	/* BB AGC Table Register 4 */
#define BBCR_BATR5	0x0254	/* BB AGC Table Register 5 */
#define BBCR_BATR6	0x0258	/* BB AGC Table Register 6 */
#define BBCR_BATR7	0x025C	/* BB AGC Table Register 7 */
#define BBCR_BATR8	0x0260	/* BB AGC Table Register 8 */
#define BBCR_BATR9	0x0264	/* BB AGC Table Register 9 */
#define BBCR_BATR10	0x0268	/* BB AGC Table Register 10 */
#define BBCR_BATR11	0x026C	/* BB AGC Table Register 11 */
#define BBCR_BATR12	0x0270	/* BB AGC Table Register 12 */
#define BBCR_BATR13	0x0274	/* BB AGC Table Register 13 */
#define BBCR_BATR14	0x0278	/* BB AGC Table Register 14 */
#define BBCR_BATR15	0x027C	/* BB AGC Table Register 15 */
#define BBCR_BATR16	0x0280	/* BB AGC Table Register 16 */
#define BBCR_BATR17	0x0284	/* BB AGC Table Register 17 */
#define BBCR_BATR18	0x0288	/* BB AGC Table Register 18 */
#define BBCR_BATR19	0x028C	/* BB AGC Table Register 19 */
#define BBCR_BATR20	0x0290	/* BB AGC Table Register 20 */
#define BBCR_BATR21	0x0294	/* BB AGC Table Register 21 */
#define BBCR_BATR22	0x0298	/* BB AGC Table Register 22 */
#define BBCR_BATR23	0x029C	/* BB AGC Table Register 23 */
#define BBCR_BATR24	0x02A0	/* BB AGC Table Register 24 */
#define BBCR_BATR25	0x02A4	/* BB AGC Table Register 25 */
#define BBCR_BATR26	0x02A8	/* BB AGC Table Register 26 */
#define BBCR_BATR27	0x02AC	/* BB AGC Table Register 27 */
#define BBCR_BATR28	0x02B0	/* BB AGC Table Register 28 */
#define BBCR_BATR29	0x02B4	/* BB AGC Table Register 29 */
#define BBCR_BATR30	0x02B8	/* BB AGC Table Register 30 */
#define BBCR_BATR31	0x02BC	/* BB AGC Table Register 31 */
#define BBCR_BATR32	0x02C0	/* BB AGC Table Register 32 */
#define BBCR_BATR33	0x02C4	/* BB AGC Table Register 33 */
#define BBCR_BATR34	0x02C8	/* BB AGC Table Register 34 */
#define BBCR_BOTTR0	0x02CC	/* BB OFDM Tracking Table Register 0-11 */
#define BBCR_BOTTR1	0x02D0 /* BB OFDM Tracking Table Register 1 */
#define BBCR_BOTTR2	0x02D4 /* BB OFDM Tracking Table Register 2 */
#define BBCR_BOTTR3	0x02D8 /* BB OFDM Tracking Table Register 3 */
#define BBCR_BOTTR4	0x02DC /* BB OFDM Tracking Table Register 4 */
#define BBCR_BOTTR5	0x02E0 /* BB OFDM Tracking Table Register 5 */
#define BBCR_BOTTR6	0x02E4 /* BB OFDM Tracking Table Register 6 */
#define BBCR_BOTTR7	0x02E8 /* BB OFDM Tracking Table Register 7 */
#define BBCR_BOTTR8	0x02EC /* BB OFDM Tracking Table Register 8 */
#define BBCR_BOTTR9	0x02F0 /* BB OFDM Tracking Table Register 9 */
#define BBCR_BOTTR10	0x02F4 /* BB OFDM Tracking Table Register 10 */
#define BBCR_BOTTR11	0x02F8 /* BB OFDM Tracking Table Register 11 */
#define BBCR_BCSQCR	0x02FC	/* BB CCK SQ Control Register */
#define BBCR_BCCMR0	0x0300	/* BB CCK Control MISC Register 0 */
#define BBCR_BCCMR1	0x0304	/* BB CCK Control MISC Register 1 */
#define BBCR_BCSCR	0x0308	/* BB CCK Scrambler Control Register */
#define BBCR_BCPCR	0x030C	/* BB CCK Preamble Control Register */
#define BBCR_BMACR	0x0310	/* BB MPDU-antenna Control Register */
#define BBCR_BAAW2R	0x0314	/* BB AP Address W2 Register */
#define BBCR_BAAW1R	0x0318	/* BB AP Address W1 Register */
#define BBCR_BAAW0R	0x031C	/* BB AP Address W0 Register */
#define BBCR_BOMCR	0x0320	/* BB OFDM MISC Control Register */
#define BBCR_BOAFTCR	0x0324	/* BB OFDM ACQ Frequency Tracking Control Re */
#define BBCR_BRPI9_8TR	0x0330	/* BB RPI 9_8 Threshold Register(8,9) */
#define BBCR_BRFCCR	0x0334	/* RF Calibration Control Register */
#define BBCR_BRFCFR	0x0338	/* RF Calibration Feedback Register */
#define BBCR_BTXDCCR	0x033C	/* TX DC Compensation Register */
#define BBCR_BRFTXCR	0x0340	/* RF TX Compensation Register */
#define BBCR_BRFRXCR	0x0344	/* RF RX Compensation Register */
#define BBCR_BRRCR	0x0348	/* BB RPI RDD Control Register */
#define BBCR_BRPI1_0TR	0x034C	/* BB RPI 1_0 Threshold Register(0-7) */
#define BBCR_BRPI3_2TR	0x0350	/* BB RPI 3_2 Threshold Register(0-7) */
#define BBCR_BRPI5_4TR	0x0354	/* BB RPI 5_4 Threshold Register(0-7) */
#define BBCR_BRPI7_6TR	0x0358	/* BB RPI 7_6 Threshold Register(0-7) */
#define BBCR_BRCR	0x035C	/* BB RDD Counter Register */
#define BBCR_BRPSR	0x0360	/* BB Radar Pulse Status Register */
#define BBCR_BIRPR	0x0364	/* BB Instant Received Power Register */
#define BBCR_BCRFR	0x0368	/* BB Counter Rx Fail Register */
#define BBCR_BCEPR	0x036C	/* BB Counter_ED_PASS Register */
#define BBCR_BCOOPR	0x0370	/* BB Counter_OFDM_OSD_PASS Register */
#define BBCR_BCOMPR	0x0374	/* BB Counter_OFDM_mdrdy_PASS Register */
#define BBCR_BCCSQPR	0x0378	/* BB Counter_CCK_SQ1_PASS Register */
#define BBCR_BCCCPR	0x037C	/* BB Counter_CCK_CRC16_PASS Register */
#define BBCR_BCCSPR	0x0380	/* BB Counter_CCK_SFD_PASS Register */
#define BBCR_BTMR	0x0384	/* BB Testmode Register */
#define BBCR_BTTCR	0x0388	/* BB Testmode TX Control Register */
#define BBCR_BCLKCR	0x038C	/* BB Clock Control Register */
#define BBCR_BTFDR	0x0390	/* BB TXDAC Force Data Register */
#define BBCR_BRODR	0x0394	/* BB RXADC Output Data Register */
#define BBCR_BRDDCR	0x0398	/* New RDD Control Register */
#define BBCR_BRRSR	0x039C	/* RF RSSI Register */

//4  BB RPI / IPI 0-10 counter LSB Register
//4 BB RPI / IPI 0-10 counter MSB Register
#define BBCR_BRPI0LCR	0x03A0
#define BBCR_BRPI0MCR	0x03A4
//4 The offset is 8

#define BBCR_BPROBECTR	0x03E8	/* FIXME */

/* RF Control Registers */
#define RFCR_0		0x400	/* RF Control Register */
#define RFCR_1		0x404	/* RF Control Register */
#define RFCR_2		0x408	/* RF Control Register */
#define RFCR_3		0x40c	/* RF Control Register */
#define RFCR_4		0x410	/* RF Control Register */
#define RFCR_5		0x414	/* RF Control Register */
#define RFCR_6		0x418	/* RF Control Register */
#define RFCR_7		0x41C	/* RF Control Register */
#define RFCR_8		0x420	/* RF Control Register */
#define RFCR_9		0x424	/* RF Control Register */
#define RFCR_10		0x428	/* RF Control Register */
#define RFCR_11		0x42C	/* RF Control Register */
#define RFCR_12		0x430	/* RF Control Register */
#define RFCR_13		0x434	/* RF Control Register */






/* CIR */
#define CIR_IBSS_MASTER			(1 << 25)
#define CIR_SCAN_BUSY			(1 << 24)
#define CIR_RESET			(1 << 23)
#define CIR_WT_BUSY			(1 << 22)
#define CIR_LP_STATE			(1 << 21)
#define CIR_PLL_READY			(1 << 20)
#define CIR_CARD_REV_MASK		0xf0000
#define CIR_CARD_REV_SHIFT		16
#define CIR_CARD_ID_MASK		0x0ffff

#define CIR_TO_CARD_ID(dc) (dc & CIR_CARD_ID_MASK)
#define CIR_TO_CARD_REV(dc) ((dc & CIR_CARD_REV_MASK) >> CIR_CARD_REV_SHIFT)

#define WLAN_WT_IS_BUSY(_u4Value)	((_u4Value) & CIR_WT_BUSY)



/* HISR */
#define HISR_ABNORMAL_INT		(1 << 31)
#define HISR_RCPI_INT			(1 << 30)
#define HISR_ADMIT_TIME_MET		(1 << 29)
#define HISR_ALC_VIOLATE		(1 << 28)
#define HISR_SCAN_DONE			(1 << 27)
#define HISR_BB_INT			(1 << 26)
#define HISR_QUIET_DONE			(1 << 25)
#define HISR_MSR_DONE			(1 << 24)
#define HISR_APSD_TIMEOUT		(1 << 23)
#define HISR_WATCH_DOG			(1 << 21)
#define HISR_TX_PSPOLL_TIMEOUT		(1 << 20)
#define HISR_HCCA_TXOP_SHORT		(1 << 19)
#define HISR_Q_CFPOLL_NO_DATA		(1 << 18)
#define HISR_NO_MORE_PKT		(1 << 17)
#define HISR_TSF_DRIFT			(1 << 16)
#define HISR_PREDTIM			(1 << 15)
#define HISR_PRETBTT			(1 << 14)
#define HISR_TBTT			(1 << 13)
#define HISR_T3_TIME			(1 << 12)
#define HISR_T2_TIME			(1 << 11)
#define HISR_T1_TIME			(1 << 10)
#define HISR_T0_TIME			(1 << 9)
#define HISR_SLOW_WAKEUP		(1 << 8)
#define HISR_EXT_INT			(1 << 7)
#define HISR_GPIO1			(1 << 6)
#define HISR_GPIO0			(1 << 5)
#define HISR_ATIM_W_TIMEUP		(1 << 4)
#define HISR_BEACON_TR_OK		(1 << 3)
#define HISR_BEACON_T_OK		(1 << 2)
#define HISR_TX_DONE			(1 << 1)
#define HISR_RX_DONE			(1 << 0)

/* HSCISR */
#define HSCISR_BCN_LOSS			(1 << 10)
#define HSCISR_PKT_SEARCHED		(1 << 9)
#define HSCISR_TX_NULL_FAIL		(1 << 8)
#define HSCISR_TX_TRIG_FAIL		(1 << 7)
#define HSCISR_TX_PSPOLL_FAIL		(1 << 6)
#define HSCISR_DRIVER_OWN_BACK		(1 << 5)
#define HSCISR_DTIM_CNT_MATCH		(1 << 4)
#define HSCISR_BMC_TIMEOUT		(1 << 3)
#define HSCISR_RX_BMC_BCN		(1 << 2)
#define HSCISR_RX_UC_BCN		(1 << 1)
#define HSCISR_BCN_TIMEOUT		(1 << 0)

/* HIER */
#define HIER_ABNORMAL_INT		(1 << 31)
#define HIER_LOW_RCPI_INT		(1 << 30)
#define HIER_ADMIT_TIME_MET		(1 << 29)
#define HIER_ALC_VIOLATE		(1 << 28)
#define HIER_SCAN_DONE			(1 << 27)
#define HIER_BB_INT			(1 << 26)
#define HIER_QUIET_DONE			(1 << 25)
#define HIER_MSR_DONE			(1 << 24)
#define HIER_APSD_TIMEOUT		(1 << 23)
#define HIER_WATCH_DOG			(1 << 21)
#define HIER_TX_PSPOLL_TIMEOUT		(1 << 20)
#define HIER_Q_CFPOLL_NO_DATA		(1 << 18)
#define HIER_NO_MORE_PKT		(1 << 17)
#define HIER_TSF_DRIFT			(1 << 16)
#define HIER_PREDTIM			(1 << 15)
#define HIER_PRETBTT			(1 << 14)
#define HIER_TBTT			(1 << 13)
#define HIER_T3_TIME			(1 << 12)
#define HIER_T2_TIME			(1 << 11)
#define HIER_T1_TIME			(1 << 10)
#define HIER_T0_TIME			(1 << 9)
#define HIER_SLOW_WAKEUP		(1 << 8)
#define HIER_EXT_INT			(1 << 7)
#define HIER_GPIO1			(1 << 6)
#define HIER_GPIO0			(1 << 5)
#define HIER_ATIM_W_TIMEUP		(1 << 4)
#define HIER_BEACON_TR_OK		(1 << 3)
#define HIER_BEACON_T_OK		(1 << 2)
#define HIER_TX_DONE			(1 << 1)
#define HIER_RX_DONE			(1 << 0)


/* HSCIER */
#define HSCIER_BCN_LOSS			(1 << 10)
#define HSCIER_PKT_SEARCHED		(1 << 9)
#define HSCIER_TX_NULL_FAIL		(1 << 8)
#define HSCIER_TX_TRIG_FAIL		(1 << 7)
#define HSCIER_TX_PSPOLL_FAIL		(1 << 6)
#define HSCIER_DRIVER_OWN_BACK		(1 << 5)
#define HSCIER_DTIM_CNT_MATCH		(1 << 4)
#define HSCIER_BMC_TIMEOUT		(1 << 3)
#define HSCIER_RX_BMC_BCN		(1 << 2)
#define HSCIER_RX_UC_BCN		(1 << 1)
#define HSCIER_BCN_TIMEOUT		(1 << 0)


#define HIER_DEFAULT                    (HIER_ABNORMAL_INT          | \
                                         HIER_LOW_RCPI_INT          | \
                                         HIER_ADMIT_TIME_MET        | \
                                         HIER_SCAN_DONE             | \
                                         HIER_ALC_VIOLATE           | \
                                         HIER_BB_INT                | \
                                         HIER_APSD_TIMEOUT          | \
                                         HIER_WATCH_DOG             | \
                                         HIER_TX_PSPOLL_TIMEOUT     | \
                                         HIER_TSF_DRIFT             | \
                                         HIER_TBTT                  | \
                                         HIER_SLOW_WAKEUP           | \
                                         HIER_TX_DONE               | \
                                         HIER_RX_DONE)

#define HSCIER_DEFAULT                  (HSCIER_BCN_LOSS            | \
                                         HSCIER_TX_NULL_FAIL        | \
                                         HSCIER_TX_TRIG_FAIL        | \
                                         HSCIER_TX_PSPOLL_FAIL      | \
                                         HSCIER_DTIM_CNT_MATCH      | \
                                         HSCIER_BMC_TIMEOUT         | \
                                         HSCIER_RX_BMC_BCN          | \
                                         HSCIER_RX_UC_BCN           | \
                                         HSCIER_BCN_TIMEOUT)

//3 HCR
//4 0: RX, 1: TX
#define HCR_FUNC_TX_RX			(1 << 31)

//4 0: Normal function
#define HCR_INT_OUT_SELECT		(1 << 19)

//4 0: active low, 1: active high
#define HCR_POLARITY_HIGH		(1 << 18)
#define HCR_POLARITY_LOW		0

//4 802.1x check enable
#define HCR_1X_CK_EN			(1 << 13)
#define HCR_1X_CK_DISABLE		0

//4 0: HISR and HSCISR read clear
//4 1: HISR and HSCISR write 1 clear
#define HCR_INT_ACC_CTRL		(1 << 12)

#define HCR_IPV6_RX_OFFLOAD_EN		(1 << 14)
#define HCR_IP_RX_OFFLOAD_EN		(1 << 11)
#define HCR_TCP_RX_OFFLOAD_EN		(1 << 10)
#define HCR_UDP_RX_OFFLOAD_EN		(1 << 9)
#define HCR_TCPIP_TX_OFFLOAD_EN		(1 << 8)
#define HCR_CSUM_OFFLOAD_MASK		(HCR_TCPIP_TX_OFFLOAD_EN | \
                                         HCR_UDP_RX_OFFLOAD_EN | \
                                         HCR_TCP_RX_OFFLOAD_EN | \
                                         HCR_IP_RX_OFFLOAD_EN | \
                                         HCR_IPV6_RX_OFFLOAD_EN)

#define HCR_RX_DATA_RD_DONE		(1 << 5)



/* RR */
/* Enable moving average */
#define RR_ENABLE_MA			(1 << 31)
#define RR_RCPI_PARM_1_OF_16		0
#define RR_RCPI_PARM_1_OF_8		(1 << 24)
#define RR_RCPI_PARM_1_OF_4		(1 << 25)
#define RR_RCPI_PARM_1_OF_1		(3 << 24)
#define RR_RCPI_MOV_AVE_PARM_MASK	(3 << 24)

#define RR_RCPI_HIGH_THRESHOLD_MASK	0xff0000
#define RR_RCPI_LOW_THRESHOLD_MASK	0x00ff00
#define RR_RCPI_VALUE_MASK		0x0000ff

#define RR_RCPI_HIGH_MAXIMUM		255
#define RR_RCPI_LOW_MINIMUM		0


#define RR_SET_MOV_AVE_PARA(_movAvePara) \
            (((_movAvePara) << 0) & RR_RCPI_MOV_AVE_PARM_MASK)
#define RR_SET_HIGH_RCPI_THRESHOLD(_rcpiHighThreshold) \
            (((_rcpiHighThreshold) << 16) & RR_RCPI_HIGH_THRESHOLD_MASK)
#define RR_SET_LOW_RCPI_THRESHOLD(_rcpiLowThreshold) \
            (((_rcpiLowThreshold) << 8) & RR_RCPI_LOW_THRESHOLD_MASK)
#define RR_GET_RCPI(_rr) \
            ((RCPI)((_rr) & RR_RCPI_VALUE_MASK))

//3 QAR
#define QAR_SEL_AC0                     (ENUM_QUEUE_ID_AC0       << 28)
#define QAR_SEL_AC1                     (ENUM_QUEUE_ID_AC1       << 28)
#define QAR_SEL_AC2                     (ENUM_QUEUE_ID_AC2       << 28)
#define QAR_SEL_AC3                     (ENUM_QUEUE_ID_AC3       << 28)
#define QAR_SEL_TSBEACON                (ENUM_QUEUE_ID_TSB       << 28)
#define QAR_SEL_AC4                     (ENUM_QUEUE_ID_AC4       << 28)
#define QAR_SEL_RX                      (ENUM_QUEUE_ID_RX        << 28)
#define QAR_SEL_SCAN                    (ENUM_QUEUE_ID_SCAN      << 28)
#define QAR_SEL_SCAN_CTRL               (ENUM_QUEUE_ID_SCAN_CTRL << 28)
#define QAR_READ_MODE                   (1 << 26)

/* HWTDR */
#define HWTDR_CLR_TX_VLD		(1 << 8)
#define HWTDR_CLR_RX_VLD		(1 << 11)

#define HWTDR_UPDATE_MODE_0		0
#define HWTDR_UPDATE_MODE_1		1
#define HWTDR_UPDATE_MODE_2		2
#define HWTDR_UPDATE_MODE_3		3

#define HWTDR_UPDATE_MODE_0_SZ		4
#define HWTDR_UPDATE_MODE_1_SZ		8
#define HWTDR_UPDATE_MODE_2_SZ		16
#define HWTDR_UPDATE_MODE_3_SZ		48

#define HWTDR_RCA1_ALL			0
#define HWTDR_RCA1_MUAR_ONLY		1
#define HWTDR_RCA1_BC_ONLY		2
#define HWTDR_RCA1_MUAR_BC		3

//3 HWTCR
#define HWTCR_CLR_TX_VLD                (1 << 9)
#define HWTCR_CLR_RX_VLD                (1 << 8)
#define HWTCR_B_RMODE                   (1 << 5)
#define HWTCR_W_RMODE                   (1 << 4)

#define HWTCR_ENTRY_SEL_0               0
#define HWTCR_ENTRY_SEL_1               0x1
#define HWTCR_ENTRY_SEL_2               0x2
#define HWTCR_ENTRY_SEL_3               0x3
#define HWTCR_ENTRY_SEL_4               0x4
#define HWTCR_ENTRY_SEL_5               0x5
#define HWTCR_ENTRY_SEL_6               0x6
#define HWTCR_ENTRY_SEL_7               0x7
#define HWTCR_ENTRY_SEL_8               0x8
#define HWTCR_ENTRY_SEL_9               0x9
#define HWTCR_ENTRY_SEL_10              0xA
#define HWTCR_ENTRY_SEL_11              0xB
#define HWTCR_ENTRY_SEL_12              0xC

#define MASK_HWTDR_TINDEX               0xf
#define MASK_HWTDR_UPMODE               (3 << 4)

#define HWTDR_MODE_OFFSET               4
#define HWTDR_TV_OFFSET                 8
#define HWTDR_TKV_OFFSET                9
#define HWTDR_T1X_OFFSET               10
#define HWTDR_RV_OFFSET                11
#define HWTDR_RKV_OFFSET               12
#define HWTDR_RCA1_OFFSET              13
#define HWTDR_RCA2_OFFSET              15
#define HWTDR_RCID_OFFSET              16
#define HWTDR_R1X_OFFSET               17
#define HWTDR_Q_OFFSET                 18
#define HWTDR_A_OFFSET                 19
#define HWTDR_IKV_OFFSET               20
#define HWTDR_CIPHER_OFFSET            24
#define HWTDR_KID_OFFSET               28

/* HLPCR */
#define HLPCR_TOG_INT_EN		(1 << 31)
#define HLPCR_RESERVED			(1 << 30)
#define HLPCR_HIFMAC_LOGRST		(1 << 29)
#define HLPCR_HIF_REGRST		(1 << 28)
#define HLPCR_MAC_REGRST		(1 << 27)
#define HLPCR_BB_REGRST			(1 << 26)
#define HLPCR_BB_LOGRST			(1 << 25)
#define HLPCR_CHIP_RESET		(1 << 24)
#define HLPCR_OSC_EN			(1 << 23)
#define HLPCR_OSC_OUT_EN		(1 << 22)
#define HLPCR_BG_EN			(1 << 21)
#define HLPCR_PLL_EN			(1 << 20)
#define HLPCR_ADC_BUFFER_EN		(1 << 19)
#define HLPCR_INTERNAL_32K_EN		(1 << 18)
#define HLPCR_EXTERNAL_32K_EN		(1 << 17)
#define HLPCR_RF_SX_EN			(1 << 16)
#define HLPCR_OSC_PD			(1 << 15)
#define HLPCR_OSC_OUT_PD		(1 << 14)
#define HLPCR_BG_PD			(1 << 13)
#define HLPCR_PLL_PD			(1 << 12)
#define HLPCR_ADC_BUFFER_PD		(1 << 11)
#define HLPCR_INTERNAL_32K_PD		(1 << 10)
#define HLPCR_EXTERNAL_32K_PD		(1 << 9)
#define HLPCR_RF_SX_PD			(1 << 8)
#define HLPCR_DELAY_INT			(1 << 7)
#define HLPCR_GINT_ISENABLE		(1 << 6)
#define HLPCR_DISABLE_GINT		(1 << 5)
#define HLPCR_ENABLE_GINT		(1 << 4)
#define HLPCR_PLL_CLOCK_GATED		(1 << 3)
#define HLPCR_LP_OWN_STATE		(1 << 2)
#define HLPCR_LP_OWN_CLR		(1 << 1)
#define HLPCR_LP_OWN_SET		(1 << 0)

#define HLPCR_PD_ALL			0xff00

/* HFCR - HIF FIFO Control Register */
#define HFCR_SELECT_DATA_FIFO		0
#define HFCR_SELECT_WLAN_FIFO		(1 << 15)
#define HFCR_FIFO_WRITE			(1 << 14)
#define HFCR_FIFO_READ			(1 << 13)
#define HFCR_FIFO_ADDR_MASK		0x1fff

//3 HITR        0x0048
#define HFCR_RX_INT_THRESHOLD           0x3fff



//2 CONFIG
/* DBGR - Debug Register */
#define DBGR_WATCH_DOG_TIMER_ENABLE	(1 << 29)
#define DBGR_DEBUG_ENABLE		(1 << 28)
#define DBGR_SLEEP_TIMEOUT_UNIT_1024_TU	(1 << 15)
#define DBGR_SLEEP_TIMEOUT_COUNT	0x7fff

/* ESCR - EEPROM Software Control Register*/
#define ESCR_EE_TYPE_MASK		(7 << 17)
#define ESCR_EE_TYPE_NONE		0
#define ESCR_2048BYTE			((1 << 19) | (1 << 17))
#define ESCR_1024BYTE			(1 << 19)
#define ESCR_512BYTE			((1 << 18) | (1 << 17))
#define ESCR_256BYTE			(1 << 18) /* originally: (1 << 18) | (1 << 17) */
#define ESCR_128BYTE			(1 << 17)
#define ESCR_SW_ACCESS			(1 << 16)
#define ESCR_EECS			(1 << 3)
#define ESCR_EESK			(1 << 2)
#define ESCR_EEDI			(1 << 1)
#define ESCR_UNKNOWN_BIT0		(1 << 0)

/* EADR - EEPROM Address and Data Register */
#define EADR_EE_READ			(1 << 31)
#define EADR_EE_WRITE			(1 << 30)
#define EADR_EE_RDY			(1 << 29)
#define EADR_EE_CSERR			(1 << 28)
#define EADR_EE_RECALL			(1 << 27)
/* FIXME: 10bit correct for address mask? */
#define EADR_EE_ADDRESS_MASK		0x3ff0000
#define EADR_EE_ADDRESS_OFFSET		16
#define EADR_EE_DATA_MASK		0xffff

#define EADR_SET_ADDRESS(_n)		((_n << EADR_EE_ADDRESS_OFFSET) & EADR_EE_ADDRESS_MASK)
#define EADR_CLEAN_DATA(_n)		(_n & EADR_EE_DATA_MASK)



//3 32KCCR
#define X32KCCR_CAL_START               (1 << 31)
#define X32KCCR_CLK_SEL_RING_OSC        (1 << 30)
#define X32KCCR_RESIDUAL_1000           0xf000000
#define X32KCCR_RESIDUAL_100            0x0f00000
#define X32KCCR_RESIDUAL_10             0x00f0000
#define X32KCCR_SLOW_CLOCK_TSF          0x3f

//3 CCR
#define CCR_BB_CLK_ON                   (1 << 31) | (1 << 30)
#define CCR_BB_CLK_TXRX                 (1 << 31)
#define CCR_BB_CLK_OFF                  0

#define CCR_TXDAC_PWD_MODE_RFTX         0
#define CCR_TXDAC_PWD_MODE_TXPE         (1 << 27)
#define CCR_TXDAC_PWD_MODE_RFTX_TXPE    (1 << 28)
#define CCR_TXDAC_PWD_MODE_ON           (1 << 29)
#define CCR_TXDAC_PWD_MODE_OFF          (1 << 29) | (1 << 27)
#define CCR_TXDAC_PWD_MODE_MASK         (1 << 29) | (1 << 28) | (1 << 27)


#define CCR_TXDAC_CLK_MODE_RFTX         0
#define CCR_TXDAC_CLK_MODE_TXPE         (1 << 24)
#define CCR_TXDAC_CLK_MODE_RFTX_TXPE    (1 << 25)
#define CCR_TXDAC_CLK_MODE_ON           (1 << 26)
#define CCR_TXDAC_CLK_MODE_OFF          (1 << 26) | (1 << 24)

#define CCR_RXDAC_PWD_MODE_RFRX         0
#define CCR_RXDAC_PWD_MODE_RXPE         (1 << 19)
#define CCR_RXDAC_PWD_MODE_RFRX_RXPE    (1 << 20)
#define CCR_RXDAC_PWD_MODE_ON           (1 << 21)
#define CCR_RXDAC_PWD_MODE_OFF          (1 << 21) | (1 << 19)

#define CCR_RXDAC_CLK_MODE_RFRX         0
#define CCR_RXDAC_CLK_MODE_RXPE         (1 << 16)
#define CCR_RXDAC_CLK_MODE_RFRX_RXPE    (1 << 17)
#define CCR_RXDAC_CLK_MODE_ON           (1 << 18)
#define CCR_RXDAC_CLK_MODE_OFF          (1 << 18) | (1 << 16)

#define CCR_WLAN_CLK_CTRL_11J           (1 << 15)
#define CCR_WLAN_CLK_CTRL_11ABG         0

#define CCR_ALCADC_PWD_MODE_BB_ALC      0
#define CCR_ALCADC_PWD_MODE_ON          (1 << 14)
#define CCR_ALCADC_PWD_MODE_OFF         (1 << 13) | (1 << 14)


#define CCR_ALCADC_CLK_MODE_BB_ALC      0
#define CCR_ALCADC_CLK_MODE_ON          (1 << 12)
#define CCR_ALCADC_CLK_MODE_OFF         (1 << 11) | (1 << 12)


#define CCR_EEIF_GC_DIS                 (1 << 8)
#define CCR_EEIF_GC_EN                  0

#define CCR_BUF_LOGIC_GC_DIS            (1 << 7)
#define CCR_BUF_LOGIC_GC_EN             0

#define CCR_LP_LOGIC_GC_DIS             (1 << 6)
#define CCR_LP_LOGIC_GC_EN              0

#define CCR_RMAC_LOGIC_GC_DIS           (1 << 5)
#define CCR_RMAC_LOGIC_GC_EN            0

#define CCR_TMAC_LOGIC_GC_DIS           (1 << 4)
#define CCR_TMAC_LOGIC_GC_EN            0

#define CCR_SEC_LOGIC_GC_DIS            (1 << 3)
#define CCR_SEC_LOGIC_GC_EN             0

#define CCR_ARB_LOGIC_GC_DIS            (1 << 2)
#define CCR_ARB_LOGIC_GC_EN             0

#define CCR_HIF_LOGIC_GC_DIS            (1 << 1)
#define CCR_HIF_LOGIC_GC_EN             0

#define CCR_MCR_GC_DIS                  (1 << 0)
#define CCR_MCR_GC_EN                   0


//3 ODCR
//4 2006/09/11, mikewu, <todo> ODCR

//3 IOUDR
#define IOUDR_BT_PRI_PU                 (1 << 19)
#define IOUDR_WE_N_PU                   (1 << 18)
#define IOUDR_OE_N_PU                   (1 << 17)
#define IOUDR_CS_N_PU                   (1 << 16)
#define IOUDR_BT_PRI_PD                 (1 << 3)
#define IOUDR_WE_N_PD                   (1 << 2)
#define IOUDR_OE_N_PD                   (1 << 1)
#define IOUDR_CS_N_PD                   (1 << 0)

//3 IOPCR
//4 2006/09/11, mikewu, <todo> IOPCR
#define IOPCR_ALL_TRAP_PIN_OUTPUT_EN    0x1f00
/*BITS(8, 12)*/
#define IOPCR_IO_TR_SW_P_DIR            (1 << 12)
#define IOPCR_IO_TR_SW_N_DIR            (1 << 11)
#define IOPCR_IO_ANT_SEL_P_DIR          (1 << 10)
#define IOPCR_IO_ANT_SEL_N_DIR          (1 << 9)
#define IOPCR_UNKNOWN_BIT8		(1 << 8)

//3 ACDR
#define ACDR4_RG_DIVS                   0x3f0000
/*BITS(16, 21)*/

#define ACDR4_RG_DIVS_20M               0x16
#define ACDR4_RG_DIVS_40M               0x0a
#define ACDR4_RG_DIVS_13M               0x26
#define ACDR4_RG_DIVS_26M               0x12
#define ACDR4_RG_DIVS_19_2M             0x17

//3 SCR
#define SCR_GPIO1_POLAR_HIGH            (1 << 29)
#define SCR_GPIO1_POLAR_LOW             0
#define SCR_GPIO1_CHAIN_SEL             (1 << 28)
#define SCR_BTFREQ_SEL                  (1 << 27)

#define SCR_GPIO1_WDATA                 (1 << 26)
#define SCR_GPIO1_ENABLE_OUTPUT_MODE    (1 << 25)
#define SCR_GPIO1_ENABLE_INPUT_MODE     0
#define SCR_GPIO1_RDATA                 (1 << 24)

#define SCR_GPIO0_POLAR_HIGH            (1 << 21)
#define SCR_GPIO0_POLAR_LOW             0
#define SCR_GPIO0_CHAIN_SEL             (1 << 20)

#define SCR_BT_ACT_SEL                  (1 << 19)

#define SCR_GPIO0_WDATA                 (1 << 18)
#define SCR_GPIO0_ENABLE_OUTPUT_MODE    (1 << 17)
#define SCR_GPIO0_ENABLE_INPUT_MODE     0
#define SCR_GPIO0_RDATA                 (1 << 16)

#define SCR_GPIO2_POLAR_HIGH            (1 << 13)
#define SCR_GPIO2_POLAR_LOW             0
#define SCR_GPIO2_CHAIN_SEL             (1 << 12)
#define SCR_GPIO2_WDATA                 (1 << 10)
#define SCR_GPIO2_ENABLE_OUTPUT_MODE    (1 << 9)
#define SCR_GPIO2_ENABLE_INPUT_MODE     0
#define SCR_GPIO2_RDATA                 (1 << 8)

#define SCR_DB_EN_GIO                   (1 << 7)
#define SCR_DB_EN_EINT                  (1 << 6)

#define SCR_EINT_POLAR_HIGH             (1 << 5)
#define SCR_EINT_POLAR_LOW              0

#define SCR_ACK                         (1 << 4)

#define SCR_SENS_EDGE                   0
#define SCR_SENS_LEVEL                  (1 << 3)

#define SCR_MIB_CTR_RD_CLEAR            (1 << 2)
#define SCR_OP_MODE_STA                 0
#define SCR_OP_MODE_ADHOC               (1 << 0)
#define SCR_OP_MODE_MASK                0x3

//3 LCR
#define LCR_LED_MODE_TX_WO_BEACON       (1 << 16)
#define LCR_LED_MODE_TX_BEACON          (1 << 17)
#define LCR_LED_MODE_RX                 (1 << 18)
#define LCR_LED_MODE_RX_EX_RFCR_BEACON  (1 << 19)
#define LCR_LED_MODE_RX_RFCR_BEACON     (1 << 20)
#define LCR_LED_POLARITY                (1 << 24)
#define LCR_LED_OUTPUT                  (1 << 25)


//3 RICR
#define RICR_ANT_SEL_N_SW_MODE          (1 << 24)
#define RICR_ANT_SEL_P_SW_MODE          (1 << 23)
#define RICR_SWANT_SEL_N_HIGH           (1 << 22)
#define RICR_SWANT_SEL_N_LOW            0
#define RICR_SWANT_SEL_P_HIGH           (1 << 21)
#define RICR_SWANT_SEL_P_LOW            0

#define RICR_PA5ENPOLARITY              (1 << 20)

#define RICR_PA2ENPOLARITY              (1 << 19)

//4 Enable PA 2.4G Hz
#define RICR_PA2EN                      (1 << 18)

//4 Enable PA 5G Hz
#define RICR_PA5EN                      (1 << 17)

//4 make rxhp to 1
#define RICR_SW_RXHP                    (1 << 13)


#define RICR_RXHP_MODE_CCK              (1 << 12)
#define RICR_RXHP_MODE_OFDR             0

//4 transmission control by MT5921
#define RICR_MAC_TX_EN                  (1 << 10)

//4 receiving control by MT5921
#define RICR_MAC_RX_EN                  (1 << 9)
#define RICR_MAC_TXRX_MASK              0x600

#define RICR_RF_SW_MODE                 (1 << 8)

#define RICR_SW_TR_SW_N                 (1 << 6)
#define RICR_SW_RXADC_DCCAL_EN          (1 << 5)
#define RICR_SW_RF_TX                   (1 << 4)
#define RICR_SW_RF_RX                   (1 << 3)
#define RICR_SW_TR_SW_P                 (1 << 2)
#define RICR_SW_TX_PE                   (1 << 1)
#define RICR_SW_RX_PE                   (1 << 0)


//3 RSCR
#define RSCR_POLARITY_HIGH              (1 << 30)
#define RSCR_POLARITY_LOW               0

#define RSCR_SHIFT_MODE_LEFT            (1 << 28)
#define RSCR_SHIFT_MODE_RIGHT           0

#define RSCR_SW_RF_MODE                 (1 << 27)

#define RSCR_SW_RFEEDI                  (1 << 26)
#define RSCR_SW_RFSE_N                  (1 << 25)


//3 RSDR
#define RSDR_SYNT_PROG_START            (1 << 31)
#define RSDR_RF_READ                    (1 << 30)
#define RSDR_RF_WRITE                   0

//3 ARPR1
#define ARPR1_FAILCOUNT_UP_LIMIT        0xffff0000
#define ARPR1_FAILCOUNT_DOWN_LIMIT      0x0000ffff


//3 MMCR0
#define MMCR0_BSS                       (1 << 22)
#define MMCR0_BSS_REQ                   (1 << 21)
#define MMCR0_IPI_HISTOGRAM_REQ         (1 << 20)
#define MMCR0_RPI_HISTOGRAM_REQ         (1 << 19)
#define MMCR0_CCA_REQ                   (1 << 18)
#define MMCR0_RADAR_REQ                 (1 << 17)
#define MMCR0_BBRXSTS_REQ               (1 << 16)

//3 QCCR0
#define QCCR0_QUIET_ENABLE              (1 << 31)


//3 MIBSCR
#define MIBSCR_TX_COUNT_INDEX           0
#define MIBSCR_BEACON_TX_COUNT_INDEX    1
#define MIBSCR_FRAME_RETRY_COUNT_INDEX  2
#define MIBSCR_RTS_RETRY_COUNT_INDEX    3
#define MIBSCR_RX_FCS_ERROR_COUNT_INDEX 8
#define MIBSCR_RX_FIFO_FULL_COUNT_INDEX 9
#define MIBSCR_RX_MPDU_COUNT_INDEX      10
#define MIBSCR_CHANNEL_IDLE_COUNT_INDEX 11
#define MIBSCR_CCATIME_INDEX            12
#define MIBSCR_CCA_NAV_TX_TIME_INDEX    13
#define MIBSCR_BEACON_TIMEOUT_COUNT_INDEX    14

#define MIBSCR_INDEX_MASK               0xf
#define MIBSCR_INDEX(counter)           MIBSCR_##counter##_INDEX

#define MIBSCR_TX_COUNT_EN              (1 << 27)
#define MIBSCR_BEACON_TX_COUNT_EN       (1 << 26)
#define MIBSCR_FRAME_RETRY_COUNT_EN     (1 << 25)
#define MIBSCR_RTS_RETRY_COUNT_EN       (1 << 24)
#define MIBSCR_BEACON_TIMEOUT_COUNT_EN  (1 << 22)
#define MIBSCR_RX_FCS_ERROR_COUNT_EN    (1 << 21)
#define MIBSCR_RX_FIFO_FULL_COUNT_EN    (1 << 20)
#define MIBSCR_RX_MPDU_COUNT_EN         (1 << 19)
#define MIBSCR_CHANNEL_IDLE_COUNT_EN    (1 << 18)
#define MIBSCR_CCATIME_EN               (1 << 17)
#define MIBSCR_CCA_NAV_TX_TIME_EN       (1 << 16)
#define MIBSCR_EN(counter)              MIBSCR_##counter##_EN


#define MIBSR_DEFAULT   (MIBSCR_BEACON_TX_COUNT_EN | \
                         MIBSCR_BEACON_TIMEOUT_COUNT_EN |\
                         MIBSCR_RX_FCS_ERROR_COUNT_EN | \
                         MIBSCR_RX_FIFO_FULL_COUNT_EN | \
                         MIBSCR_RX_MPDU_COUNT_EN \
                         )


//3 0x00D0 - QCR
#define QCR_QUE_FLUSH_OFFSET            16
#define QCR_QUE_STOP_OFFSET             8
#define QCR_QUE_START_OFFSET            0

#define QCR_RX_FLUSH                    ((1 << RXQ) << QCR_QUE_FLUSH_OFFSET)
#define QCR_AC4_FLUSH                   ((1 << AC4) << QCR_QUE_FLUSH_OFFSET)
#define QCR_TS_B_FLUSH                  ((1 << TS0) << QCR_QUE_FLUSH_OFFSET)
#define QCR_AC3_FLUSH                   ((1 << AC3) << QCR_QUE_FLUSH_OFFSET)
#define QCR_AC2_FLUSH                   ((1 << AC2) << QCR_QUE_FLUSH_OFFSET)
#define QCR_AC1_FLUSH                   ((1 << AC1) << QCR_QUE_FLUSH_OFFSET)
#define QCR_AC0_FLUSH                   ((1 << AC0) << QCR_QUE_FLUSH_OFFSET)

#define QCR_RX_STOP                     ((1 << RXQ) << QCR_QUE_STOP_OFFSET)
#define QCR_AC4_STOP                    ((1 << AC4) << QCR_QUE_STOP_OFFSET)
#define QCR_TS_B_STOP                   ((1 << TS0) << QCR_QUE_STOP_OFFSET)
#define QCR_AC3_STOP                    ((1 << AC3) << QCR_QUE_STOP_OFFSET)
#define QCR_AC2_STOP                    ((1 << AC2) << QCR_QUE_STOP_OFFSET)
#define QCR_AC1_STOP                    ((1 << AC1) << QCR_QUE_STOP_OFFSET)
#define QCR_AC0_STOP                    ((1 << AC0) << QCR_QUE_STOP_OFFSET)

#define QCR_RX_START                    ((1 << RXQ) << QCR_QUE_START_OFFSET)
#define QCR_AC4_START                   ((1 << AC4) << QCR_QUE_START_OFFSET)
#define QCR_TS_B_START                  ((1 << TS0) << QCR_QUE_START_OFFSET)
#define QCR_AC3_START                   ((1 << AC3) << QCR_QUE_START_OFFSET)
#define QCR_AC2_START                   ((1 << AC2) << QCR_QUE_START_OFFSET)
#define QCR_AC1_START                   ((1 << AC1) << QCR_QUE_START_OFFSET)
#define QCR_AC0_START                   ((1 << AC0) << QCR_QUE_START_OFFSET)

#define QCR_ALL_FLUSH                   (HW_QUE_MASK << QCR_QUE_FLUSH_OFFSET)
#define QCR_ALL_STOP                    (HW_QUE_MASK << QCR_QUE_STOP_OFFSET)
#define QCR_ALL_STOP_FLUSH              (QCR_ALL_STOP | QCR_ALL_FLUSH)
#define QCR_ALL_START                   (HW_QUE_MASK << QCR_QUE_START_OFFSET)

#define QCR_TX_FLUSH                    (HW_QUE_TX_MASK << QCR_QUE_FLUSH_OFFSET)
#define QCR_TX_STOP                     (HW_QUE_TX_MASK << QCR_QUE_STOP_OFFSET)
#define QCR_TX_STOP_FLUSH               (QCR_TX_STOP | QCR_TX_FLUSH)
#define QCR_TX_START                    (HW_QUE_TX_MASK << QCR_QUE_START_OFFSET)


//3 0x00D4 - AIFSR
#define AIFSR_CW_CTRL_ENLARGED          (1 << 16)
#define AIFSR_AIFS_MASK                 0x000f
#define AIFSR_AIFS0_MASK                0x000f
#define AIFSR_AIFS1_MASK                0x00f0
#define AIFSR_AIFS2_MASK                0x0f00
#define AIFSR_AIFS3_MASK                0xf000
#define AIFSR_AIFS0_OFFSET              0
#define AIFSR_AIFS1_OFFSET              4
#define AIFSR_AIFS2_OFFSET              8
#define AIFSR_AIFS3_OFFSET              12

//4 0x00D8  - MTCLR
#define MTCLR_RATE1_MASK                0x0007
#define MTCLR_RATE2_MASK                0x0070
#define MTCLR_RATE3_MASK                0x0700
#define MTCLR_RATE4_MASK                0x7000
#define MTCLR_RTS_MASK                  0x0f00000
#define MTCLR_BR_MASK                   0xf000000
#define MTCLR_RATE1_OFFSET              0
#define MTCLR_RATE2_OFFSET              4
#define MTCLR_RATE3_OFFSET              8
#define MTCLR_RATE4_OFFSET              12
#define MTCLR_RTS_OFFSET                20
#define MTCLR_BR_OFFSET                 24
#define MTCLR_DISABLE_ALL_RATES         0


//3 0x00DC - ACCWXR0
#define ACCWXR0_AC0_CWMAX_OFFSET        0
#define ACCWXR0_AC1_CWMAX_OFFSET        16

//3 0x00E0 - ACCWXR1
#define ACCWXR1_AC2_CWMAX_OFFSET        0
#define ACCWXR1_AC3_CWMAX_OFFSET        16

//3 0x00E4 - ACCWIR
#define ACCWIR_AC_CWMIN_MASK            0xff
#define ACCWIR_AC0_CWMIN_OFFSET         0
#define ACCWIR_AC1_CWMIN_OFFSET         8
#define ACCWIR_AC2_CWMIN_OFFSET         16
#define ACCWIR_AC3_CWMIN_OFFSET         24

//3 0x00E8 - AC4CWR
#define AC4CWR_AC4_CWMAX_OFFSET         0
#define AC4CWR_AC4_CWMIN_MASK           BITS(0,7)
#define AC4CWR_AC4_CWMIN_OFFSET         16
#define AC4CWR_AIFS4_MASK               BITS(0,3)
#define AC4CWR_AIFS4_OFFSET             24


//3 DRNGR
#define DRNGR_FIXED                     (1 << 16)

//3 MSCR
#define MSCR_TXOP_REMAINING_MASK        (1 << 11) | (1 << 10)
#define MSCR_TXOP_REMAINING_EN          (1 << 10)
#define MSCR_TX_LENGTH_EN               (1 << 11)
#define MSCR_TX_SEQCTRL_EN              (1 << 11) | (1 << 10)
#define MSCR_TX_PKT_CS_EN               (1 << 9)
#define MSCR_TX_STATUS_TD_EN            (1 << 8)
#define MSCR_RX_BBP_RXSTS_SEL           (1 << 5)
#define MSCR_RX_TRANSLATION_EN          (1 << 4)
#define MSCR_RX_CS_EN                   (1 << 3)
#define MSCR_RX_STATUS_G2_EN            (1 << 2)
#define MSCR_RX_STATUS_G1_EN            (1 << 1)
#define MSCR_RX_STATUS_G0_EN            (1 << 0)

//3 ARCR
#define ARCR_1M_IDX                     0
#define ARCR_2ML_IDX                    1
#define ARCR_2MS_IDX                    2
#define ARCR_5_5ML_IDX                  3
#define ARCR_5_5MS_IDX                  4
#define ARCR_11ML_IDX                   5
#define ARCR_11MS_IDX                   6
#define ARCR_6M_IDX                     7
#define ARCR_9M_IDX                     8
#define ARCR_12M_IDX                    9
#define ARCR_18M_IDX                    10
#define ARCR_24M_IDX                    11
#define ARCR_36M_IDX                    12
#define ARCR_48M_IDX                    13
#define ARCR_54M_IDX                    14

#define ARCR_WRITE                      (1 << 31)
#define ARCR_RESET                      (1 << 30)
#define ARCR_ENTRY_INDEX_OFFSET         24
#define ARCR_CURR_RATE1_INDEX_MASK      0xf00000
#define ARCR_CURR_RATE1_INDEX_OFFSET    20
#define ARCR_RATE1_INDEX_OFFSET         16
#define ARCR_1M                         (1 << ARCR_1M_IDX)
#define ARCR_2ML                        (1 << ARCR_2ML_IDX)
#define ARCR_2MS                        (1 << ARCR_2MS_IDX)
#define ARCR_5_5ML                      (1 << ARCR_5_5ML_IDX)
#define ARCR_5_5MS                      (1 << ARCR_5_5MS_IDX)
#define ARCR_11ML                       (1 << ARCR_11ML_IDX)
#define ARCR_11MS                       (1 << ARCR_11MS_IDX)
#define ARCR_6M                         (1 << ARCR_6M_IDX)
#define ARCR_9M                         (1 << ARCR_9M_IDX)
#define ARCR_12M                        (1 << ARCR_12M_IDX)
#define ARCR_18M                        (1 << ARCR_18M_IDX)
#define ARCR_24M                        (1 << ARCR_24M_IDX)
#define ARCR_36M                        (1 << ARCR_36M_IDX)
#define ARCR_48M                        (1 << ARCR_48M_IDX)
#define ARCR_54M                        (1 << ARCR_54M_IDX)
#define ARCR_SUPPORT_RATES_DEFAULT      0x7fff /*BITS(0,14)*/


//3 TXCR
#define TXCR_AIFS_AC3                   ((1 << 31) | (1 << 30))
#define TXCR_AIFS_AC2                   (1 << 31)
#define TXCR_AIFS_AC1                   (1 << 30)
#define TXCR_AIFS_AC0                   0

#define TXCR_NULL_TYPE_QOS              (1 << 25)
#define TXCR_NULL_TYPE_NON_QOS          0
#define TXCR_HW_TX_RATE_SEL_BASIC_RATE  (1 << 24)
#define TXCR_QOS_NULL_TID               0xf00000 /*BITS(20, 23)*/
#define TXCR_BIP_EN                     (1 << 19)
#define TXCR_MFP_CCX_MODE               (1 << 18)


#define TXCR_QOS_REPORT_CTRL_QSIZE      (1 << 17)
#define TXCR_PWRMGT_SET                 (1 << 16)
#define TXCR_TS_EXT_MODE_EXTRA          (1 << 15)

#define TXCR_AC_TRIG_ENABLED_MASK       (1 << 15) | (1 << 14) | (1 << 13) | (1 << 12)
#define TXCR_AC3_TRIG_ENABLED           (1 << 15)
#define TXCR_AC2_TRIG_ENABLED           (1 << 14)
#define TXCR_AC1_TRIG_ENABLED           (1 << 13)
#define TXCR_AC0_TRIG_ENABLED           (1 << 12)
#define TXCR_AC_TRIG_FUNCTION_ENABLE    (1 << 11)
#define TXCR_RESERVED                   (1 << 10)
#define TXCR_IBSS_Q_STOP                (1 << 9)
#define TXCR_AC3_DELIVERY_ENABLED       (1 << 7)
#define TXCR_AC2_DELIVERY_ENABLED       (1 << 6)
#define TXCR_AC1_DELIVERY_ENABLED       (1 << 5)
#define TXCR_AC0_DELIVERY_ENABLED       (1 << 4)
#define TXCR_AC_DLVR_ENABLED_MASK       BITS(4, 7)
#define TXCR_AUTO_RATE_ENABLE           (1 << 3)


//2 TMAC

//3 0x0100 - TCR
#define TCR_TX_RESP_ANT_MODE_0          (1 << 25)
#define TCR_TX_RESP_ANT_MODE_1          (1 << 26)
#define TCR_TX_RESP_ANT_MODE_LAST       BITS(25,26)

#define TCR_DURATION_CAL                (1 << 13)
#define TCR_TXOP_BURST_STOP             (1 << 12)

#define TCR_END_PACKET_TYPE_QOSNULL     (1 << 6)
#define TCR_END_PACKET_TYPE_NORESP      0

//3 0x0104 - ACTXOPLR0
#define ACTXOPLR0_AC0_LIMIT_OFFSET      0
#define ACTXOPLR0_AC1_LIMIT_OFFSET      16

#define ACTXOPLR0_AC0_LIMIT_MASK        BITS(0,15)
#define ACTXOPLR0_AC1_LIMIT_MASK        BITS(16,31)


//3 0x0108 - ACTXOPLR1
#define ACTXOPLR1_AC2_LIMIT_OFFSET      0
#define ACTXOPLR1_AC3_LIMIT_OFFSET      16

#define ACTXOPLR1_AC2_LIMIT_MASK        BITS(0,15)
#define ACTXOPLR1_AC3_LIMIT_MASK        BITS(16,31)

//3 OFPR
#define OFPR_48_54_FRAME_POWER_MASK     BITS(24,30)
#define OFPR_24_36_FRAME_POWER_MASK     BITS(16,22)
#define OFPR_12_18_FRAME_POWER_MASK     BITS(8,14)
#define OFPR_6_9_FRAME_POWER_MASK       BITS(0,6)

#define OFPR_48_54_FRAME_POWER_STARTBIT 24
#define OFPR_24_36_FRAME_POWER_STARTBIT 16
#define OFPR_12_18_FRAME_POWER_STARTBIT 8
#define OFPR_6_9_FRAME_POWER_STARTBIT 0


//3 CFPR
#define CFPR_FRAME_POWER_MASK           BITS(0,6)
#define CFPR_FRAME_POWER_STARTBIT       0


/*
 * The definition of PTA function
 *
 *
 */

#define PTA_HW_DISABLE              (0)
#define PTA_HW_ENABLE               (1)
/*BTCER0*/
#define PTA_BTCER0_COEXIST_EN      (1 << 0)
#define PTA_BTCER0_WLAN_ACT_POL    (1 << 1)
#define PTA_BTCER0_BURST_MODE      (1 << 2)
#define PTA_BTCER0_WLAN_ACK        (1 << 3)
#define PTA_BTCER0_TX_MODE         BITS(4,5)
#define PTA_BTCER0_RX_MODE         (1 << 6)
#define PTA_BTCER0_WLAN_AC0        (1 << 8)
#define PTA_BTCER0_WLAN_AC1        (1 << 9)
#define PTA_BTCER0_WLAN_AC2        (1 << 10)
#define PTA_BTCER0_WLAN_AC3        (1 << 11)
#define PTA_BTCER0_WLAN_BCN        (1 << 12)
#define PTA_BTCER0_WLAN_AC4        (1 << 13)
#define PTA_BTCER0_WLAN_RX         (1 << 14)
#define PTA_BTCER0_WLAN_CTRL       (1 << 15)
#define PTA_BTCER0_BCN_TIMEOUT_EN  (1 << 16)
#define PTA_BTCER0_QCFP_TIMEOUT_EN (1 << 17)
#define PTA_BTCER0_SP_TIMEOUT_EN   (1 << 18)
#define PTA_BTCER0_REMAIN_TIME     BITS(24,31)
/*BTCER1*/
#define PTA_BTCER1_BT_2ND_SAMPLE_TIME       BITS(0,4)
#define PTA_BTCER1_BT_1ST_SAMPLE_TIME       BITS(5,7)
#define PTA_BTCER1_1ST_SAMPLE_MODE          BITS(8,9)
#define PTA_BTCER1_2ND_SAMPLE_MODE          BITS(10,11)
#define PTA_BTCER1_BT_PRI_MODE              (1 << 12)
#define PTA_BTCER1_BT_TR_MODE               (1 << 13)
#define PTA_BTCER1_CONCATE_MODE             (1 << 14)
#define PTA_BTCER1_T6_PERIOD                BITS(16,20)
#define PTA_BTCER1_SINGLE_ANT               (1 << 28)
#define PTA_BTCER1_WIRE_MODE                BITS(29,30)

#define PTA_BTCER1_1_WIRE_MODE              0x00000000
#define PTA_BTCER1_2_WIRE_MODE              (1 << 30)
#define PTA_BTCER1_3_WIRE_MODE              (1 << 30) | (1 << 29)
#define PTA_BTCER1_4_WIRE_MODE              PTA_BTCER1_3_WIRE_MODE

/*BTCER2*/
#define PTA_BTCER2_QUOTA_LIMIT              BITS(0,15)
#define PTA_BTCER2_FAIRNESS_PERIOD          BITS(16,31)

/*BTCER3*/
#define PTA_BTCER3_SP_TIMEOUT               BITS(0,7)
#define PTA_BTCER3_BCN_QCFP_TIMEOUT         BITS(8,15)
#define PTA_BTCER3_BT_GUARD_ITVAL           BITS(16,23)
#define PTA_BTCER3_QUOTA_CTRL               (1 << 24)
#define PTA_BTCER3_QUOTA_EN                 (1 << 25)



//4 2006/09/11, mikewu, <todo>BTCER1-3, BRUR0-1, BRDR, BPDR, CDTR, ODTR
#define BPDR_SLOTTIME_MASK              BITS(17,21)
#define BPDR_SLOTTIME_OFFSET            17



/* RCCR */
/* Long Preamble */
#define RCCR_RC_BR_CCK_LONG_1M          0
#define RCCR_RC_BR_CCK_LONG_2M          (1 << 24)
#define RCCR_RC_BR_CCK_LONG_5p5M        (1 << 25)
#define RCCR_RC_BR_CCK_LONG_11M         (1 << 25) | (1 << 24)

/* Short Preamble */
#define RCCR_RC_BR_CCK_SHORT_2M         (1 << 28) | (1 << 24)
#define RCCR_RC_BR_CCK_SHORT_5p5M       (1 << 28) | (1 << 25)
#define RCCR_RC_BR_CCK_SHORT_11M        (1 << 28) | (1 << 25) | (1 << 24)

#define RCCR_RC_BR_OFDM_6M              (1 << 29) | (1 << 27) | (1 << 25) | (1 << 24)
#define RCCR_RC_BR_OFDM_9M              (1 << 29) | (1 << 27) | (1 << 26) | (1 << 25) | (1 << 24)
#define RCCR_RC_BR_OFDM_12M             (1 << 29) | (1 << 27) | (1 << 25)
#define RCCR_RC_BR_OFDM_18M             (1 << 29) | (1 << 27) | (1 << 26) | (1 << 25)
#define RCCR_RC_BR_OFDM_24M             (1 << 29) | (1 << 27) | (1 << 24)
#define RCCR_RC_BR_OFDM_36M             (1 << 29) | (1 << 24) | (1 << 27) | (1 << 26)
#define RCCR_RC_BR_OFDM_48M             (1 << 29) | (1 << 27)
#define RCCR_RC_BR_OFDM_54M             (1 << 29) | (1 << 27) | (1 << 26)
#define RCCR_RC_BR_MASK                 (1 << 29) | (1 << 28) | (1 << 27) | (1 << 26) | (1 << 25) | (1 << 24)
#define RCCR_RC_BR_OFFSET                24

#define RCCR_BR_MASK                     BITS(16,21)
#define RCCR_BR_CCK_2M                  (1 << 20) | (1 << 24)
#define RCCR_BR_CCK_5p5M                (1 << 20) | (1 << 25)
#define RCCR_BR_CCK_11M                 (1 << 20) | (1 << 25) | (1 << 24)

#define RCCR_BR_OFDM_6M                 (1 << 21) | (1 << 19) | (1 << 17) | (1 << 16)
#define RCCR_BR_OFDM_9M                 (1 << 21) | (1 << 19) | (1 << 18) | (1 << 17) | (1 << 16)
#define RCCR_BR_OFDM_12M                (1 << 21) | (1 << 19) | (1 << 17)
#define RCCR_BR_OFDM_18M                (1 << 21) | (1 << 19) | (1 << 18) | (1 << 17)
#define RCCR_BR_OFDM_16M                (1 << 21) | (1 << 19) | (1 << 16)
#define RCCR_BR_OFDM_36M                (1 << 21) | (1 << 16) | (1 << 19) | (1 << 18)
#define RCCR_BR_OFDM_48M                (1 << 21) | (1 << 19)
#define RCCR_BR_OFDM_54M                (1 << 21) | (1 << 19) | (1 << 18)
#define RCCR_CTS_PROTECTION              (1 << 15)
#define RCCR_CTS_MODE_PROPRIETARY        (1 << 14)
#define RCCR_RTS_THRESHOLD_MASK          0xfff
//2 RMAC

//3 RFCR
#define RFCR_RX_SAMEBSSIDPRORESP_CTRL   (1 << 28)
#define RFCR_RX_DIFFBSSIDPRORESP_CTRL   (1 << 27)
#define RFCR_RX_SAMEBSSIDATIM_CTRL      (1 << 26)
#define RFCR_RX_DIFFBSSIDATIM_CTRL      (1 << 25)
#define RFCR_RX_SAMEBSSIDNULL_CTRL      (1 << 24)
#define RFCR_RX_DIFFBSSIDNULL_CTRL      (1 << 23)
#define RFCR_RX_SAMEBSSIDBCN_CTRL       (1 << 22)
#define RFCR_RX_DIFFBSSIDBCN_CTRL       (1 << 21)
#define RFCR_RX_PROREQ_CTRL             (1 << 20)
#define RFCR_DROP_VERSIONNOT0_CTRL      (1 << 19)
#define RFCR_RX_NOACK_CTRL              (1 << 18)
#define RFCR_RX_MGMT_FRAME_CTRL         (1 << 17)
#define RFCR_RX_DATA_FRAME_CTRL         (1 << 16)
#define RFCR_DROP_DIFFBSSIDMGT_CTRL     (1 << 15)
#define RFCR_DROP_ADDR3OWNSA_CTRL       (1 << 14)
#define RFCR_DROP_DIFFBSSIDA3_CTRL      (1 << 13)
#define RFCR_DROP_DIFFBSSIDA2_CTRL      (1 << 12)
#define RFCR_RX_BCFRAME_CTRL            (1 << 11)
#define RFCR_MCTABLE_NOCHK_CTRL         (1 << 10)
#define RFCR_RX_MCFRAME_CTRL            (1 << 9)
#define RFCR_RX_PROMISCUOUSFRAME_CTRL   (1 << 1)
#define RFCR_DROP_FCSERRORFRAME_CTRL    (1 << 0)

//3 SSCR
#define SSCR_MIN_RCPI               BITS(24,31)
#define SSCR_EXT_SCAN_TIME          BITS(16,23)
#define SSCR_MIN_SCAN_TIME          BITS(8,15)
#define SSCR_EXT_TIME_CTRL          (1 << 5)
#define SSCR_SSID_START             (1 << 4)
#define SSCR_SSID_NUMBER            BITS(0,3)


//3 MUAR
#define MUAR1_SEARCH_IDX_MASK       BITS(24,29)
#define MUAR1_READ                  0
#define MUAR1_WRITE                 (1 << 17)
#define MUAR1_ACCESS_START          (1 << 16)
#define MUAR1_ADDR_INDEX_OFFSET     24


//3 NSUR
#define NSUR_NAV_UPDATE             (1 << 31)
#define NSUR_NAV_UPDATE_VALUE_MASK  BITS(0,25)  /* NAV updated value */

//3 ALCR
#define ALCR_ALC_CALCULATION_EN     (1 << 31)
#define ALCR_ALC_TRIGGER            (1 << 30)
#define ALCR_ALC_BUSY               (1 << 28)
#define ALCR_ALC_AR_FACTOR_MASK     ((1 << 25) | (1 << 24))
#define ALCR_AR_PARM_1_OF_32        0
#define ALCR_AR_PARM_1_OF_16        (1 << 24)
#define ALCR_AR_PARM_1_OF_4         (1 << 25)
#define ALCR_AR_PARM_1_OF_1         ((1 << 25) | (1 << 24))

#define ALCR_ALC_MIN_THRESHOLD      BITS(18,23)
#define ALCR_ALC_MAX_THRESHOLD      BITS(12,17)
#define ALCR_ALC_CAL_VALUE_MASK     BITS(6,11)
#define ALCR_ALC_RAW_VALUE_MASK     BITS(0,5)


#define ALCR_SET_MOV_AVE_PARA(_movAvePara) \
                    (((_movAvePara) << 0) & ALCR_ALC_AR_FACTOR_MASK)
#define ALCR_SET_MIN_THRESHOLD(_alcMinThreshold) \
                (((_alcMinThreshold) << 18) & ALCR_ALC_MIN_THRESHOLD)
#define ALCR_SET_MAX_THRESHOLD(_alcMaxThreshold) \
                (((_alcMaxThreshold) << 12) & ALCR_ALC_MAX_THRESHOLD)

#define ALCR_SET_ALC_INIT_VALUE(_alcInitVal) \
                (((_alcInitVal) << 6) & ALCR_ALC_CAL_VALUE_MASK)

#define ALCR_GET_ALC_CAL_VALUE(_alcr) \
                ((ALC_VAL)(((_alcr) & ALCR_ALC_CAL_VALUE_MASK)>>6))
#define ALCR_GET_ALC_RAW_VALUE(_alcr) \
                    ((ALC_VAL)(((_alcr) & ALCR_ALC_RAW_VALUE_MASK)>>0))

//2 LP
//3 LTTR

//3 PIR
#define PIR_BC_PATTERN_SRCH_EN          (1 << 22)
#define PIR_MC_PATTERN_SRCH_EN          (1 << 21)
#define PIR_UC_PATTERN_SRCH_EN          (1 << 20)
#define PIR_BC_MATCHING_OPERATION       (1 << 18)
#define PIR_MC_MATCHING_OPERATION       (1 << 17)
#define PIR_UC_MATCHING_OPERATION       (1 << 16)
#define PIR_IPV6_FRAME_MATCH_CTRL       (1 << 12)
#define PIR_PATTERN_WRITE_MODE          (1 << 7)
#define PIR_PATTERN_INDEX               BITS(0, 4)

//3 PMR
#define PIR_PATTERN_BC_CHECK            (1 << 24)
#define PIR_PATTERN_MC_CHECK            (1 << 23)
#define PIR_PATTERN_UC_CHECK            (1 << 22)
#define PIR_IPV4_IP_CTRL                (1 << 21)
#define PIR_IPV6_ICMP_CTRL              (1 << 20)
#define PIR_GARP_IP_EQUAL_CTRL          (1 << 19)
#define PIR_ARP_PATTERN_CTRL            (1 << 18)
#define PIR_AND_OR_OPERAND              (1 << 17)
#define PIR_NOT_OPERAND                 (1 << 16)
#define PIR_PATTERN_BYTE_MASK           BITS(8, 15)
#define PIR_PATTERN_OFFSET              BITS(0, 7)

//3 PPR

//3 LPWSAR

/* MPTCR */
#define MPTCR_BMC_TIMEOUT_EN		(1 << 31)
#define MPTCR_MORE_TRIG_EN		(1 << 30)
#define MPTCR_TX_DONE_SLEEP_CTRL	(1 << 29)
#define MPTCR_TX_PSPOLL_TIMEOUT_EN	(1 << 28)
#define MPTCR_BCN_CONTENT_CHK_EN	(1 << 27)
#define MPTCR_APSD_TIMEOUT_EN		(1 << 25)
#define MPTCR_BCN_TIMEOUT_EN		(1 << 24)
#define MPTCR_SCAN_EN			(1 << 23)
#define MPTCR_SSID_SRCH			(1 << 22)
#define MPTCR_CFPPERIODTIMEREN		(1 << 21)
#define MPTCR_T3_TIMER_EN		(1 << 20)
#define MPTCR_T2_TIMER_EN		(1 << 19)
#define MPTCR_T1_PERIOD_TIMER_EN	(1 << 18)
#define MPTCR_T0_PERIOD_TIMER_EN	(1 << 17)
#define MPTCR_TBTT_PERIOD_TIMER_EN	(1 << 16)
#define MPTCR_BCN_UC_EN			(1 << 15)
#define MPTCR_TX_NULL_EN		(1 << 14)
#define MPTCR_BCN_BMC_EN		(1 << 13)
#define MPTCR_BCN_PARSE_TIM_EN		(1 << 12)
#define MPTCR_Q_CFP_NO_DATA_EN		(1 << 10)
#define MPTCR_HCCA_TXOP_SHORT_EN	(1 << 9)
#define MPTCR_PREDTIM_TIMEUP_EN		(1 << 8)
#define MPTCR_PRETBTT_TIMEUP_EN		(1 << 7)
#define MPTCR_TBTT_TIMEUP_EN		(1 << 6)
#define MPTCR_GPIO2_TRIGGER		(1 << 5)
#define MPTCR_GPIO1_TRIGGER		(1 << 4)
#define MPTCR_GPIO0_TRIGGER		(1 << 3)
#define MPTCR_RX_BMC_MGT_EN		(1 << 2)
#define MPTCR_PREDTIM_TRIG_EN		(1 << 1)
#define MPTCR_PRETBTT_TRIG_EN		(1 << 0)


/* ACWR */
#define ACWR_ATIM_WINDOW_MASK		0xffff


/* BSSSAR */
#define BSSSAR_CHNL_INDEX_MASK		0xff000000
#define BSSSAR_CHNL_INDEX_SHIFT		24
#define BSSSAR_BAND_INDEX_MASK		0x00ff0000
#define BSSSAR_BAND_INDEX_SHIFT		16

//3 TCR, 0x019C
#define TCR_BCN_FRAME_TYPE_PSPOLL       0
#define TCR_BCN_FRAME_TYPE_QoS_TRIG_FRM (1 << 20)
#define TCR_TIMER_SEL_T0                0
#define TCR_TIMER_SEL_T1                (1 << 0)

//4 T2, T3 can be configured period only
#define TCR_TIMER_SEL_T2                (1 << 1)
#define TCR_TIMER_SEL_T3                (3 << 0)
#define TCR_TIME_VALUE_CTRL_PERIOD      (1 << 2)
#define TCR_TIME_VALUE_CTRL_START_TIME  0

#define TCR_T0_CTRL_TS                  (1 << 5)
#define TCR_T0_CTRL_NO_TRIG_FRAME       (3 << 3)
#define TCR_T0_CTRL_PSPOLL              (1 << 4)
#define TCR_T0_CTRL_QOS_TRIG            (1 << 3)
#define TCR_T0_CTRL_TSF_TIMER           0


#define TCR_T1_CTRL_NO_TRIG_FRAME       (3 << 6)
#define TCR_T1_CTRL_PSPOLL              (1 << 7)
#define TCR_T1_CTRL_QOS_TRIG            (1 << 6)
#define TCR_T1_CTRL_TSF_TIMER           0

#define TCR_T2_CTRL_PSPOLL              (1 << 9)
#define TCR_T2_CTRL_QOS_TRIG            (1 << 8)
#define TCR_T2_CTRL_FREERUN_TIMER       0

#define TCR_T3_CTRL_PSPOLL              (1 << 11)
#define TCR_T3_CTRL_QOS_TRIG            (1 << 10)
#define TCR_T3_CTRL_FREERUN_TIMER       0

#define TCR_T2_COUNT_UNIT_1US           0
#define TCR_T2_COUNT_UNIT_1TU           (1 << 16)

#define TCR_T2_MODE_AUTO_REPEAT         (1 << 17)
#define TCR_T2_MODE_ONE_SHOT            0

#define TCR_T3_COUNT_UNIT_1US           0
#define TCR_T3_COUNT_UNIT_1TU           (1 << 18)

#define TCR_T3_MODE_AUTO_REPEAT         (1 << 19)
#define TCR_T3_MODE_ONE_SHOT            0


//3 TDR, 0x01A0
#define TDR_TIMER_SEL_MASK              BITS(30,31)
#define TDR_TIMER_SEL_T0                (0 << 30)
#define TDR_TIMER_SEL_T1                (1 << 30)
#define TDR_TIMER_SEL_T2                (2 << 30)
#define TDR_TIMER_SEL_T3                (3 << 30)
#define TDR_TIME_VALUE_CTRL_PERIOD      (1 << 29)
#define TDR_TIMEUP_ENABLE               (1 << 27)
#define TDR_TIMER_CTRL_MASK             BITS(24,26)
#define TDR_TIMER_CTRL_TSF              (0 << 24)
#define TDR_TIMER_CTRL_FREE_RUN         (0 << 24)
#define TDR_TIMER_CTRL_AUTO_TRIGGER     (1 << 24)
#define TDR_TIMER_CTRL_AUTO_PS_POLL     (2 << 24)
#define TDR_TIMER_CTRL_SAPSD            (3 << 24)
#define TDR_TIMER_CTRL_TS               (4 << 24)
#define TDR_TIMER_MODE_AUTO_REPEAT      (1 << 23)
#define TDR_TIMER_COUNT_UNIT_TU         (1 << 22)
#define TDR_TIME_VALUE_MASK             BITS(0,21)
#define TDR_TIME_VALUE_MAX_T2_T3        BITS(0,15)
#define TDR_TIME_VALUE_MAX_T0_T1        BITS(0,21)

//3 ATCR, 0x01A4
#define ATCR_PSPOLL_END_SP_EN           (1 << 31)
#define ATCR_TRIGGER_END_SP_EN          (1 << 30)
#define ATCR_PSPOLL_NEW_SP_EN           (1 << 28)
#define ATCR_TRIGGER_NEW_SP_EN          (1 << 27)
#define ATCR_MNGT_PSPOLL_EN             (1 << 26)
#define ATCR_TX_NULL_INTERVAL           BITS(16,25)
#define ATCR_TX_NULL_RESET_CTRL         (1 << 15)
#define ATCR_BCN_POLL_QOS_NULL          (1 << 14)
#define ATCR_TRIGGER_THRESHOLD          BITS(8,13)
#define ATCR_TIMEOUT_COUNT_LIMIT        BITS(4,7)
#define ATCR_BCN_TIMEOUT_COUNT_LIMIT    BITS(0,3)



//3 TSTR
#define TSTR_PRETBTT_INTERVAL_MASK      BITS(24,31)
#define TSTR_GUARD_INTERVAL_MASK        BITS(16,23)
#define TSTR_TBTTSTARTTIME_MASK         BITS(0,15)


//3 TTPCR
#define TTPCR_TBTT_CAL_ENABLE           (1 << 31)
#define TTPCR_DTIM_WAKE_PERIOD_MASK     BITS(28,30)
#define TTPCR_TBTT_WAKE_PERIOD_MASK     BITS(24,27)
#define TTPCR_DTIM_PERIOD_MASK          BITS(16,23)
#define TTPCR_BEACON_PERIOD_MASK        BITS(0,15)


//3 CTPCR

/* LPCR - Low Power Control Register */
#define LPCR_LP_STABLE_TIME_MASK        0x7f000000
#define LPCR_TSF_DRIFT_INTR_EN          (1 << 23)
#define LPCR_TX_LIFE_TIME_MASK          0x007ff000
#define LPCR_AID_MASK                   0x00000fff


//3 LPICR

//3 LPIDR

//3 UTTR0

//3 UTTR1

//3 TTAR


#define TTAR_TSF_TSF_DRIFT_WINDOW       BITS(16,23)
#define TTAR_TSF_TIMER_VALUE_CHANGE     (1 << 0)
#define TTAR_DISABLE_HWTSF_UPDATE       (1 << 1)

//3 LPOSAR

//3 TR
#define TR_BCN_MAX_TIME_LIMIT_MASK          BITS(24,31)
#define TR_BCN_MAX_TIME_LIMIT_VALID         (1 << 23)
#define TR_BCN_MIN_TIME_LIMIT_VALID         (1 << 22)
#define TR_BCN_MIN_TIME_LIMIT_MASK          BITS(16,21)
#define TR_BCN_MIN_MAX_TIME_LIMIT_MASK      BITS(16,31)

#define TR_MAX_TIME_LIMIT_MASK              BITS(8,15)
#define TR_MAX_TIME_LIMIT_VALID             (1 << 7)
#define TR_MIN_TIME_LIMIT_VALID             (1 << 6)
#define TR_MIN_TIME_LIMIT_MASK              BITS(0,5)
#define TR_MIN_MAX_TIME_LIMIT_MASK          BITS(0,15)


//3 MTR0, MTR1
#define MTR_ACQ_ADMIT_TIME(ac, value)   (((value) & BITS(0,15)) << ((ac & 1) << 4))
#define MTR_ACQ_USED_TIME(ac, regVal)   (((regVal) >> ((ac & 1) << 4)) & BITS(0,15))

#define MTR0_AC3_ADMIT_TIME_MASK             BITS(16,31)
#define MTR0_AC2_ADMIT_TIME_MASK             BITS(0,15)
#define MTR1_AC1_ADMIT_TIME_MASK             BITS(16,31)
#define MTR1_AC0_ADMIT_TIME_MASK             BITS(0,15)

//3 MTCR0, MTCR1

#define MTCR_MEDIA_TIME_SHIFT               7
#define MTCR_MEDIA_TIME_MASK                0x1UL
#define MTCR_MEDIA_TIME_UNIT(x)             (((x) & MTCR_MEDIA_TIME_MASK) << MTCR_MEDIA_TIME_SHIFT)

#define MTCR_1_SEC                          0
#define MTCR_64_USEC                        1

#define MTCR_AVERAGE_PERIOD_MASK            BITS(0,6)
#define MTCR_ACQ_AVERAGE_PERIOD(ac, value, unit)    (((((value) & MTCR_AVERAGE_PERIOD_MASK) | MTCR_MEDIA_TIME_UNIT(unit)) << ((ac & 1) << 3)) << 16)
#define MTCR_ACQ_ADMIT_TIME_ENABLE(ac)      (1 << ac & 1)


#define MTCR0_AC3_AVERAGE_PERIOD_UNIT_64_US (1 << 31)
#define MTCR0_AC3_AVERAGE_PERIOD_MASK       BITS(24,30)
#define MTCR0_AC2_AVERAGE_PERIOD_UNIT_64_US (1 << 23)
#define MTCR0_AC2_AVERAGE_PERIOD_MASK       BITS(16,22)
#define MTCR0_AC3_ADMIT_TIME_EN             (1 << 1)
#define MTCR0_AC2_ADMIT_TIME_EN             (1 << 0)

#define MTCR1_AC1_AVERAGE_PERIOD_UNIT_64_US (1 << 31)
#define MTCR1_AC1_AVERAGE_PERIOD_MASK       BITS(24,30)
#define MTCR1_AC0_AVERAGE_PERIOD_UNIT_64_US (1 << 23)
#define MTCR1_AC0_AVERAGE_PERIOD_MASK       BITS(16,22)
#define MTCR1_AC1_ADMIT_TIME_EN             (1 << 1)
#define MTCR1_AC0_ADMIT_TIME_EN             (1 << 0)

//3 SPCR
#define SPCR_NULL_TIMEOUT_LIMIT_MASK        BITS(24,31)
#define SPCR_NULL_MAX_TIME_LIMIT_VALID      (1 << 23)
#define SPCR_NULL_MIN_TIME_LIMIT_VALID      (1 << 22)
#define SPCR_NULL_MIN_TIME_LIMIT_MASK       BITS(16,21)
#define SPCR_NULL_MIN_MAX_TIME_LIMIT_MASK   BITS(16,31)

#define SPCR_TO_COUNTER_RESET_CTRL          (1 << 5)
#define SPCR_BEACON_SP_INVALID_MASK         (1 << 4)
#define SPCR_BMC_SP_INVALID_MASK            (1 << 3)
#define SPCR_QOS_CFPOLL_SP_INVALID_MASK     (1 << 2)
#define SPCR_PSPOLL_SP_INVALID_MASK         (1 << 1)
#define SPCR_TRIGGER_SP_INVALID_MASK        (1 << 0)
#define SPCR_ALL_SP_INVALID_MASK            0x1f

//3 BEIR
#define BEIR_BCN_LOST_THRESHOLD             BITS(16,19)
#define BEIR_BEI_CAL_EN                     (1 << 8)
#define BEIR_BCN_EARLIER_INTERVAL_MASK      0xff


//2 BB

//4 CR2, BBISR
#define BBISR_RADAR_DETECTION           (1 << 1)
#define BBISR_TYPE1_DETECTION           (1 << 2)
#define BBISR_TYPE2_DETECTION           (1 << 3)
#define BBISR_TYPE3_DETECTION           (1 << 4)
#define BBISR_TYPE4_DETECTION           (1 << 5)

#define BBISR_RADAR_DETECTION_EN        (1 << 9)
#define BBISR_TYPE1_DETECTION_EN        (1 << 10)
#define BBISR_TYPE2_DETECTION_EN        (1 << 11)
#define BBISR_TYPE3_DETECTION_EN        (1 << 12)
#define BBISR_TYPE4_DETECTION_EN        (1 << 13)

//4 CR1, BCR
#define BCR_BAND_5G_EN                  (1 << 1)
#define BCR_RX_OFDM_DISABLE             (1 << 2)
#define BCR_RX_CCK_DISABLE              (1 << 3)
#define BCR_RX_ANT_SEL_FIX_1            BITS(4,5)
#define BCR_RX_ANT_SEL_MASK             BITS(4,5)
#define BCR_RX_ANT_SEL_FIX_0            (1 << 5)
#define BCR_RX_ANT_SEL_MPDU_BASED       (1 << 4)
#define BCR_RX_ANT_SEL_AGC_BASED        0

#define BCR_CCA_METHOD_ED_OR_EARLYCS    BITS(9,10)
#define BCR_CCA_METHOD_ED_OR_CS         (1 << 8) | (1 << 10)
#define BCR_CCA_METHOD_ED_AND_EARLYCS   (1 << 10)
#define BCR_CCA_METHOD_ED_AND_CS        BITS(8,9)
#define BCR_CCA_METHOD_EARLYCS          (1 << 9)
#define BCR_CCA_METHOD_ED               (1 << 8)
#define BCR_CCA_METHOD_CS               0
#define BCR_CCA_METHOD_MASK             BITS(8,10)

//4 CR65, BCCMR1
#define BCR_BBCMR1_TXF_JP_CH            (1 << 6)

//2 RF
#define RFCR4_XO_TRIM_MASK              0xfe00000 /*BITS(21, 27) */
#define RFCR4_XO_TRIM_OFFSET            21

#define RFCR4_EREFS                     0x7

#endif
