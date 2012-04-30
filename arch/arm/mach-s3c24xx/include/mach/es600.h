
/*
 * P_KEEP is wired alongside the vbus to a BAT54C Schottky diode.
 * If either P_KEEP is high or VBUS is connected,
 * the system will stay running.
 */
#define ES600_BASE_GPIO_PKEEP		S3C2410_GPG(0)

/*
 * GPIO pins of the HSMMC0 channel
 */
#define ES600_HSMMC0_GPIO_CD		S3C2410_GPF(1)
#define ES600_HSMMC0_GPIO_POWER		S3C2410_GPD(14)

/*
 * GPIO pins of the USB vbus
 */
#define ES600_HSUDC_GPIO_VBUS		S3C2410_GPG(1)
#define ES600_HSUDC_IRQ_VBUS		IRQ_EINT9
#define ES600_HSUDC_GPIO_ID		S3C2410_GPD(13)

/*
 * GPIO pins of the AUO-pixcir touchscreen
 */
#define ES600_AUOTS_GPIO_INT		S3C2410_GPF(2)
#define ES600_AUOTS_IRQ_INT		IRQ_EINT2
#define ES600_AUOTS_GPIO_RST		S3C2410_GPF(5)

/*
 * GPIO pins for the ALC5624 sound codec
 */
#define ES600_ALC5624_GPIO_EN		S3C2410_GPB(10)
#define ES600_ALC5624_GPIO_RST		S3C2410_GPA(1)
#define ES600_ALC5624_GPIO_HEADSET	S3C2410_GPF(4)

/*
 * PINs of the BQ24075 charger IC.
 * SYSOFF is not connected.
 * NPGOOD is hard-wired to a green LED on AS090.
 */
#define ES600_BQ24075_GPIO_NCE		S3C2410_GPB(5)
#define ES600_BQ24075_GPIO_EN2		S3C2410_GPB(0)
#define ES600_BQ24075_GPIO_EN1		S3C2410_GPB(4)
#define ES600_BQ24075_GPIO_NCHG		S3C2410_GPF(6)
#define ES600_BQ24075_GPIO_SYSOFF	null
#define ES600_BQ24075_GPIO_NPGOOD	null

/*
 * tps650240 pmic
 * ENDCDC1 + ENDCDC2 are enabled when battery connected
 * ENDCDC3 is controlled by PWR_EN pin of the S3C2416
 * ENLDO3 vddalive is controlled by the connected battery
 */
#define ES600_TPS650240_GPIO_ENDCDC1	null
#define ES600_TPS650240_GPIO_ENDCDC2	null
#define ES600_TPS650240_GPIO_ENDCDC3	null
#define ES600_TPS650240_GPIO_DEFDCDC3	S3C2410_GPH(13)
#define ES600_TPS650240_GPIO_MODE	S3C2410_GPD(12)
#define ES600_TPS650240_GPIO_ENLDO12	S3C2410_GPH(14)
#define ES600_TPS650240_GPIO_ENLDO3	null
#define ES600_TPS650240_GPIO_NPWRFAIL	S3C2410_GPG(5)

/*
 * GPIO pins of the AUO-K1900 epd controller
 */
#define ES600_AUOK1900_GPIO_POWER	S3C2410_GPB(3)
#define ES600_AUOK1900_GPIO_NRST	S3C2410_GPD(9)
#define ES600_AUOK1900_GPIO_NBUSY	S3C2410_GPB(2)
#define ES600_AUOK1900_GPIO_NSLEEP	S3C2410_GPB(1)

/*
 * GPIO pins for basic SPI handling
 * CS0 is the normal chip-select pin of the S3C2416
 * MUX is the source select pin of the
 */
#define ES600_HSSPI_GPIO_CS0		S3C2410_GPL(13)
#define ES600_HSSPI_GPIO_MUX		S3C2410_GPH(6)

/*
 * GPIO pins for G9093 regulator
 */
#define ES600_G9093_GPIO_EN3V3		S3C2410_GPK(5)
#define ES600_G9093_GPIO_EN1V8		S3C2410_GPK(6)

/*
 * GPIO enable pin for LR1106 regulator
 */
#define ES600_LR1106_GPIO_EN		S3C2410_GPH(12)

/*
 * GPIO enable pin for LTC3442 regulator (mini-PCIe)
 */
#define ES600_LTC3442_GPIO_EN		S3C2410_GPH(11)

/*
 * External PINs of the mini-PCIe connector
 * W_DISABLE .. Wireless disable (active low)
 * PERST# .. Power good (reset / Fundamental reset)
 */
#define ES600_PCIE_GPIO_W_DISABLE	S3C2410_GPH(8)
#define ES600_PCIE_GPIO_PERST		S3C2410_GPH(9)

/*
 * WIFI
 */
#define ES600_WIFI_GPIO_RST		S3C2410_GPD(8)
#define ES600_WIFI_GPIO_MAC_WAKE	S3C2410_GPG(4)
#define ES600_WIFI_GPIO_SPI_INT		S3C2410_GPF(3)
#define ES600_WIFI_IRQ_SPI_INT		IRQ_EINT3
#define ES600_WIFI_TYPE_MT5921		0x1
#define ES600_WIFI_TYPE_MR8686		0x2

/* from es600-common.c */
extern void es600_set_wifi(int wifi_type);
extern void es600_set_resolution(int res);
extern void es600_common_init(void);

struct spi_board_info;

extern void es600_spi_init(struct spi_board_info *board_info, int num_board_info);
extern void es600_spi_set_cs(unsigned line, int high);

//struct platform_device pandora_vwlan_device;
