/* sound/soc/samsung/qisdaebook.c
 *
 * Copyright 2007,2008 Simtec Electronics
 *
 * Based on sound/soc/pxa/spitz.c
 *	Copyright 2005 Wolfson Microelectronics PLC.
 *	Copyright 2005 Openedhand Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/jack.h>

#include <asm/mach-types.h>
#include <mach/gpio.h>
#include <mach/es600.h>

#include "dma.h"
#include "i2s.h"

/* 3.5 pie jack */
static struct snd_soc_jack jack;

/* 3.5 pie jack detection DAPM pins */
static struct snd_soc_jack_pin jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE | SND_JACK_MECHANICAL |
			SND_JACK_LINEOUT,
	},
};

/* 3.5 pie jack detection gpios */
static struct snd_soc_jack_gpio jack_gpios[] = {
	{
		.gpio = ES600_ALC5624_GPIO_HEADSET,
		.name = "headphone detect",
		.report = SND_JACK_HEADPHONE | SND_JACK_MECHANICAL |
			SND_JACK_LINEOUT,
		.invert = 1,
		.debounce_time = 200,
	},
};

static int qisdaebook_alc5624_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
//	struct s3c_i2sv2_rate_calc div;
	unsigned int clk = 0;
	int ret = 0;

printk("start hw_params\n");
	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 48000:
	case 96000:
		clk = 12288000;
		break;
	case 11025:
	case 22050:
	case 44100:
		clk = 11289600;
		break;
	}

	/* set codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
				  SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: snd_soc_dai_set_fmt for codec_dai failed with %d\n", __func__, ret);
		return ret;
	}

	/* set cpu DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
				  SND_SOC_DAIFMT_NB_NF |
				  SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: snd_soc_dai_set_fmt for cpu_dai failed with %d\n", __func__, ret);
		return ret;
	}

	/* Use PCLK for I2S signal generation */
//	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_RCLKSRC_0,
//					0, SND_SOC_CLOCK_IN);
//	if (ret < 0)
//		return ret;

	/* Gate the RCLK output on PAD */
//	ret = snd_soc_dai_set_sysclk(cpu_dai, SAMSUNG_I2S_CDCLK,
//					0, SND_SOC_CLOCK_IN);
//	if (ret < 0)
//		return ret;

	/* set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, clk, SND_SOC_CLOCK_IN);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: snd_soc_dai_set_sysclk failed with %d\n", __func__, ret);
		return ret;
	}

//SAMSUNG_I2S_DIV_BCLK einzige ID fuer _set_clkdiv
/*	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C2412_DIV_RCLK, div.fs_div);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: snd_soc_dai_set_clkdiv for rclk failed with %d\n", __func__, ret);
		return ret;
	}
*/
/*	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C2412_DIV_PRESCALER, div.clk_div - 1);
	if (ret < 0)
	{
		printk(KERN_ERR "%s: snd_soc_dai_set_fmt for prescaler failed with %d\n", __func__, ret);
		return ret;
	}
*/
printk("stop hw_params\n");
	return 0;
}

static struct snd_soc_ops qisdaebook_alc5624_ops = {
	.hw_params	= qisdaebook_alc5624_hw_params,
};

static const struct snd_soc_dapm_widget alc5624_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Internal Speaker", NULL),
	SND_SOC_DAPM_MIC("Internal Mic", NULL),
};

static const struct snd_soc_dapm_route audio_map[] = {
	{"Headphone Jack",	NULL,	"HPL" },
	{"Headphone Jack",	NULL,	"HPR" },

	{"Internal Speaker",		NULL,	"SPKL"},
	{"Internal Speaker",		NULL,	"SPKR"},
	{"Internal Speaker",		NULL,	"SPKLN"},
	{"Internal Speaker",		NULL,	"SPKRN"},

	{ "Mic Bias1",		NULL,	"Internal Mic" },
	{ "MIC1",		NULL,	"Mic Bias1" },
};

static int qisdaebook_alc5624_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int ret;

	snd_soc_dapm_new_controls(dapm, alc5624_dapm_widgets,
				ARRAY_SIZE(alc5624_dapm_widgets));

	snd_soc_dapm_add_routes(dapm, audio_map, ARRAY_SIZE(audio_map));

	snd_soc_dapm_new_widgets(dapm);

	snd_soc_dapm_nc_pin(dapm, "LINEL");
	snd_soc_dapm_nc_pin(dapm, "LINER");
	snd_soc_dapm_nc_pin(dapm, "PHONEIN");
	snd_soc_dapm_nc_pin(dapm, "MIC2");
	snd_soc_dapm_nc_pin(dapm, "MONO");

	snd_soc_dapm_enable_pin(dapm, "Internal Mic");
	snd_soc_dapm_enable_pin(dapm, "Internal Speaker");
	snd_soc_dapm_disable_pin(dapm, "Headphone Jack");

	snd_soc_dapm_sync(dapm);
return 0;
	/* Headset jack detection */
	ret = snd_soc_jack_new(codec, "Headphone Jack",
			SND_JACK_HEADPHONE | SND_JACK_MECHANICAL | SND_JACK_LINEOUT,
			&jack);
	if (ret)
		return ret;

	ret = snd_soc_jack_add_pins(&jack, ARRAY_SIZE(jack_pins), jack_pins);
	if (ret)
		return ret;

	ret = snd_soc_jack_add_gpios(&jack, ARRAY_SIZE(jack_gpios), jack_gpios);
	if (ret)
		return ret;

	return 0;
}

static struct snd_soc_dai_link qisdaebook_dai = {
	.name		= "alc5624",
	.stream_name	= "ALC5624",
	.platform_name	= "samsung-audio",
	.cpu_dai_name	= "samsung-i2s",
	.codec_dai_name	= "alc5624-hifi",
	.codec_name	= "alc5624-codec.0-0018",
	.init		= qisdaebook_alc5624_init,
	.ops		= &qisdaebook_alc5624_ops,
};

/* qisdaebook audio machine driver */
static struct snd_soc_card snd_soc_machine_qisdaebook = {
	.name		= "QisdaEbook",
	.dai_link	= &qisdaebook_dai,
	.num_links	= 1,
};

static struct platform_device *qisdaebook_snd_device;

static int __init qisdaebook_init(void)
{
	int ret;

//	if (!machine_is_jive())
//		return 0;

	printk("Qisda eBook Audio support\n");

	qisdaebook_snd_device = platform_device_alloc("soc-audio", -1);
	if (!qisdaebook_snd_device)
		return -ENOMEM;

	platform_set_drvdata(qisdaebook_snd_device, &snd_soc_machine_qisdaebook);
	ret = platform_device_add(qisdaebook_snd_device);

	if (ret)
		platform_device_put(qisdaebook_snd_device);

	return ret;
}

static void __exit qisdaebook_exit(void)
{
	platform_device_unregister(qisdaebook_snd_device);
}

module_init(qisdaebook_init);
module_exit(qisdaebook_exit);

MODULE_AUTHOR("Heiko Stuebner <heiko@sntech.de>");
MODULE_DESCRIPTION("ALSA SoC Qisda eBook Audio support");
MODULE_LICENSE("GPL");
