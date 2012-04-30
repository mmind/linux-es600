/*
 * alc5624.c  --  ALC5624 ALSA Soc Audio driver
 *
 * Copyright 2011 Eduardo José Tagle <ejtagle@tutopia.com>
 * Based on rt5624.c , Copyright 2008 Realtek Microelectronics flove <flove@realtek.com>
 *  >Fixed several errors on mixer topology
 *  >Removed nonexistant inputs
 *  >Added proper power saving modes and bias control
 *  >Silent initialization (no more pops!)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
/* #define DEBUG */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h> 
#include <sound/initval.h>
#include <sound/alc5624.h>

#include "alc5624.h"


/* Streams that can be running */
#define ALC5624_STREAM_PLAYBACK 1
#define ALC5624_STREAM_RECORD   2
#define ALC5624_STREAM_ALL      3


/* codec private data */
struct alc5624_priv {
	struct regulator	*vdd;
	struct clk		*mclk;		/* the master clock */
	unsigned int		spkvdd_mv;	/* Speaker Vdd in millivolts */
	unsigned int		hpvdd_mv;	/* Headphone Vdd in millivolts */
	
	enum snd_soc_control_type control_type;
	void			*control_data;
	
	u8 			id;		/* device ID */
	unsigned int		sysclk;
	int 			streams_running;/* Streams running ALC5624_STREAM_* */
	int 			using_pll;	/* If using PLL */
	
	/* original R/W functions */
	unsigned int (*bus_hw_read)(struct snd_soc_codec *codec, unsigned int reg);
	int (*bus_hw_write)(void*,const char*, int);  /* codec->control_data(->struct i2c_client), pdata, datalen */
	void *bus_control_data;			/* bus control_data to use when calling the original bus fns */
};


static struct reg_default alc5624_reg_default[] = {
	{ALC5624_SPK_OUT_VOL				, 0x4040 }, /* Unmute left and right channels, enable 0 cross detector, 0db volume */
	{ALC5624_HP_OUT_VOL					, 0x4040 }, /* Unmute left and right channels, enable 0 cross detector, 0db volume */
	{ALC5624_PHONEIN_MONO_OUT_VOL		, 0xDFDF }, /* Phone input muted, Mono output muted */
	{ALC5624_LINE_IN_VOL				, 0xFF1F }, /* Mute Line In volume */
	{ALC5624_STEREO_DAC_VOL				, 0x6808 }, /* Mute volume output to Mono and Speaker */
	{ALC5624_MIC_VOL					, 0x0808 }, /* Mic volume = 0db */
	{ALC5624_MIC_ROUTING_CTRL			, 0xF0F0 }, /* Mute mic volume to Headphone, Speaker and Mono mixers, Differential mode enabled */
	{ALC5624_ADC_REC_GAIN				, 0xF58B },
	{ALC5624_ADC_REC_MIXER				, 0x3F3F }, /* Mic1 as recording sources */
	{ALC5624_OUTPUT_MIXER_CTRL			, 0x6B00 }, /* SPKL from HP mixer, Class D amplifier, SPKR from HP mixer, HPL from headphone mixer, HPR from headphone mixer, Mono from VMid */
	{ALC5624_MIC_CTRL					, 0x0F02 }, /* 1.8uA short current det, Bias volt =0.9Avdd, +40db gain boost */
	{ALC5624_PD_CTRL_STAT				, 0x0000 }, /* No powerdown */
	{ALC5624_MAIN_SDP_CTRL				, 0x8000 }, /* Slave interfase */
	{ALC5624_PWR_MANAG_ADD1				, 0x003C }, /* Nothing powered down, except I2S interface, and main references */
	{ALC5624_PWR_MANAG_ADD2				, 0x07FF }, /* Nothing powered down, except class AB amplifier */
	{ALC5624_PWR_MANAG_ADD3				, 0x4CFF }, /* Nothing powered down, except class AB/D amplifiers*/
	{ALC5624_GEN_CTRL_REG1				, 0x00E8 }, /* 1v as Vmid for all amps (overrided later using platform data) */
	{ALC5624_GEN_CTRL_REG2				, 0x0000 }, /* Class AB differential mode */
	{ALC5624_PLL_CTRL					, 0x0000 },
	{ALC5624_GPIO_PIN_CONFIG			, 0x2E3E }, /* All GPIOs as input */
	{ALC5624_GPIO_PIN_POLARITY			, 0x2E3E }, /* All GPIOs high active */
	{ALC5624_GPIO_PIN_STICKY			, 0x0000 }, /* No sticky ops */
	{ALC5624_GPIO_PIN_WAKEUP			, 0x0000 }, /* No wakeups */
	{ALC5624_GPIO_PIN_SHARING			, 0x0000 },
	{ALC5624_GPIO_OUT_CTRL				, 0x0000 },
	{ALC5624_MISC_CTRL					, 0x8000 }, /* Slow Vref, Strong amp, No AVC, thermal shutdown disabled - overridable by platform data */
	{ALC5624_STEREO_DAC_CLK_CTRL1		, 0x3075 },
	{ALC5624_STEREO_DAC_CLK_CTRL2		, 0x1010 },
	{ALC5624_PSEUDO_SPATIAL_CTRL		, 0x0053 }, /* Disable */
	{VIRTUAL_HPL_MIXER					, 0x0001 },
	{VIRTUAL_HPR_MIXER					, 0x0001 },
	{VIRTUAL_IDX_EQ_BAND0_COEFF			, 0x1b1b },
	{VIRTUAL_IDX_EQ_BAND0_GAIN 			, 0xf510 },
	{VIRTUAL_IDX_EQ_BAND1_COEFF0		, 0xc10f },
	{VIRTUAL_IDX_EQ_BAND1_COEFF1		, 0x1ef6 },
	{VIRTUAL_IDX_EQ_BAND1_GAIN 			, 0xf65f },
	{VIRTUAL_IDX_EQ_BAND2_COEFF0 		, 0xc159 },
	{VIRTUAL_IDX_EQ_BAND2_COEFF1 		, 0x1eb3 },
	{VIRTUAL_IDX_EQ_BAND2_GAIN 			, 0xf510 },
	{VIRTUAL_IDX_EQ_BAND3_COEFF0 		, 0xc386 },
	{VIRTUAL_IDX_EQ_BAND3_COEFF1 		, 0x1cd0 },
	{VIRTUAL_IDX_EQ_BAND3_GAIN 			, 0x0adc },
	{VIRTUAL_IDX_EQ_BAND4_COEFF 		, 0x0436 },
	{VIRTUAL_IDX_EQ_BAND4_GAIN 			, 0x2298 },
	{VIRTUAL_IDX_EQ_CTRL_STAT 			, 0x0000 }, /* Disable Equalizer */
	{VIRTUAL_IDX_EQ_INPUT_VOL 			, 0x0000 }, /* 0db */
	{VIRTUAL_IDX_EQ_OUTPUT_VOL 			, 0x0001 }, /* 0db */
	{VIRTUAL_IDX_AUTO_VOL_CTRL0 		, 0x0050 },
	{VIRTUAL_IDX_AUTO_VOL_CTRL1 		, 0x2710 },
	{VIRTUAL_IDX_AUTO_VOL_CTRL2 		, 0x0BB8 },
	{VIRTUAL_IDX_AUTO_VOL_CTRL3 		, 0x01F4 },
	{VIRTUAL_IDX_AUTO_VOL_CTRL4 		, 0x0190 },
	{VIRTUAL_IDX_AUTO_VOL_CTRL5 		, 0x0200 },
	{VIRTUAL_IDX_DIG_INTERNAL 			, 0x9000 }, /* Strong drive */
	{VIRTUAL_IDX_CLASS_D_TEMP_SENSOR 	, 0x5555 }, /* 95deg max */
	{VIRTUAL_IDX_AD_DA_MIXER_INTERNAL	, 0xE184 },
};

/*
 * ALC5624 Controls
 */
 
/* from -46.5 to 0 dB in 1.5 dB steps - 32 steps */
static DECLARE_TLV_DB_SCALE(spkhpmonooutvol_tlv, -4650, 150, 0); 

/* from -34.5 to 12 dB in 1.5 dB steps - 32 steps */
static DECLARE_TLV_DB_SCALE(daclinmicphonevol_tlv, -3450, 150, 0); 

/* from -16.5 to 30 dB in 1.5 dB steps - 32 steps */
static DECLARE_TLV_DB_SCALE(capvol_tlv, -1650, 150, 0); 

/* Available boosts for Mics */
static const unsigned int micboost_tlv[] = {
	TLV_DB_RANGE_HEAD(4),
	0, 0, TLV_DB_SCALE_ITEM(0, 0, 0),
	1, 1, TLV_DB_SCALE_ITEM(2000, 0, 0),
	2, 2, TLV_DB_SCALE_ITEM(3000, 0, 0),
	3, 3, TLV_DB_SCALE_ITEM(4000, 0, 0),
};


/* Before modifying control names, please think it twice... Names are very important
   for DAPM to deduce topology and propagate power events */

static const char *alc5624_amp_type_sel[] = {"Class AB","Class D"};
static const struct soc_enum alc5624_amp_type_enum[] = {
SOC_ENUM_SINGLE(ALC5624_OUTPUT_MIXER_CTRL,12, 2, alc5624_amp_type_sel), /*Speaker AMP sel 7 */
};

static const struct snd_kcontrol_new alc5624_snd_controls[] = {
SOC_DOUBLE_TLV("Speaker Playback Volume", 	ALC5624_SPK_OUT_VOL, 8, 0, 31, 1, spkhpmonooutvol_tlv),	
SOC_DOUBLE("Speaker Playback Switch", 		ALC5624_SPK_OUT_VOL, 15, 7, 1, 1),
SOC_DOUBLE_TLV("Headphone Playback Volume", ALC5624_HP_OUT_VOL, 8, 0, 31, 1, spkhpmonooutvol_tlv),
SOC_DOUBLE("Headphone Playback Switch", 	ALC5624_HP_OUT_VOL, 15, 7, 1, 1),
SOC_SINGLE_TLV("Mono Playback Volume", 		ALC5624_PHONEIN_MONO_OUT_VOL, 0, 31, 1, spkhpmonooutvol_tlv),
SOC_SINGLE("Mono Playback Switch", 			ALC5624_PHONEIN_MONO_OUT_VOL, 7, 1, 1),
SOC_SINGLE_TLV("PhoneIn Playback Volume", 	ALC5624_PHONEIN_MONO_OUT_VOL, 8, 31, 1, daclinmicphonevol_tlv),
SOC_DOUBLE_TLV("PCM Playback Volume", 		ALC5624_STEREO_DAC_VOL, 8, 0, 31, 1, daclinmicphonevol_tlv),
SOC_DOUBLE("PCM Playback Switch", 			ALC5624_STEREO_DAC_VOL, 15, 7, 1, 1),
SOC_DOUBLE_TLV("LineIn Capture Volume", 	ALC5624_LINE_IN_VOL, 8, 0, 31, 1, daclinmicphonevol_tlv),
SOC_SINGLE_TLV("Mic 1 Capture Volume",		ALC5624_MIC_VOL, 8, 31, 1, daclinmicphonevol_tlv),
SOC_SINGLE_TLV("Mic 2 Capture Volume", 		ALC5624_MIC_VOL, 0, 31, 1, daclinmicphonevol_tlv),
SOC_SINGLE_TLV("Mic 1 Boost Volume",		ALC5624_MIC_CTRL, 10, 3, 0, micboost_tlv),
SOC_SINGLE_TLV("Mic 2 Boost Volume",		ALC5624_MIC_CTRL, 8, 3, 0, micboost_tlv),
SOC_ENUM("Speaker Amp Type",				alc5624_amp_type_enum[0]),
SOC_DOUBLE_TLV("Rec Capture Volume", 		ALC5624_ADC_REC_GAIN, 7, 0, 31, 0, capvol_tlv),
	};

/*
 * DAPM controls
 */

/* Left Headphone Mixers */
static const struct snd_kcontrol_new alc5624_hpl_mixer_controls[] = {
SOC_DAPM_SINGLE("LI2HP Playback Switch", VIRTUAL_HPL_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("PHO2HP Playback Switch", VIRTUAL_HPL_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("MIC12HP Playback Switch", VIRTUAL_HPL_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("MIC22HP Playback Switch", VIRTUAL_HPL_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("DAC2HP Playback Switch", VIRTUAL_HPL_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("ADCL2HP Playback Switch", ALC5624_ADC_REC_GAIN, 15, 1, 1),
};

/* Right Headphone Mixers */
static const struct snd_kcontrol_new alc5624_hpr_mixer_controls[] = {
SOC_DAPM_SINGLE("LI2HP Playback Switch", VIRTUAL_HPR_MIXER, 4, 1, 0),
SOC_DAPM_SINGLE("PHO2HP Playback Switch", VIRTUAL_HPR_MIXER, 3, 1, 0),
SOC_DAPM_SINGLE("MIC12HP Playback Switch", VIRTUAL_HPR_MIXER, 2, 1, 0),
SOC_DAPM_SINGLE("MIC22HP Playback Switch", VIRTUAL_HPR_MIXER, 1, 1, 0),
SOC_DAPM_SINGLE("DAC2HP Playback Switch", VIRTUAL_HPR_MIXER, 0, 1, 0),
SOC_DAPM_SINGLE("ADCR2HP Playback Switch", ALC5624_ADC_REC_GAIN, 14, 1, 1),
};

//Left Record Mixer
static const struct snd_kcontrol_new alc5624_captureL_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", ALC5624_ADC_REC_MIXER, 14, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", ALC5624_ADC_REC_MIXER, 13, 1, 1),
SOC_DAPM_SINGLE("LineInL Capture Switch",ALC5624_ADC_REC_MIXER,12, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", ALC5624_ADC_REC_MIXER, 11, 1, 1),
SOC_DAPM_SINGLE("HPMixerL Capture Switch", ALC5624_ADC_REC_MIXER,10, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",ALC5624_ADC_REC_MIXER,9, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",ALC5624_ADC_REC_MIXER,8, 1, 1),
};

//Right Record Mixer
static const struct snd_kcontrol_new alc5624_captureR_mixer_controls[] = {
SOC_DAPM_SINGLE("Mic1 Capture Switch", ALC5624_ADC_REC_MIXER, 6, 1, 1),
SOC_DAPM_SINGLE("Mic2 Capture Switch", ALC5624_ADC_REC_MIXER, 5, 1, 1),
SOC_DAPM_SINGLE("LineInR Capture Switch",ALC5624_ADC_REC_MIXER,4, 1, 1),
SOC_DAPM_SINGLE("Phone Capture Switch", ALC5624_ADC_REC_MIXER, 3, 1, 1),
SOC_DAPM_SINGLE("HPMixerR Capture Switch", ALC5624_ADC_REC_MIXER,2, 1, 1),
SOC_DAPM_SINGLE("SPKMixer Capture Switch",ALC5624_ADC_REC_MIXER,1, 1, 1),
SOC_DAPM_SINGLE("MonoMixer Capture Switch",ALC5624_ADC_REC_MIXER,0, 1, 1),
};

/* Speaker Mixer */
static const struct snd_kcontrol_new alc5624_speaker_mixer_controls[] = {
SOC_DAPM_SINGLE("LI2SPK Playback Switch", ALC5624_LINE_IN_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("PHO2SPK Playback Switch", ALC5624_PHONEIN_MONO_OUT_VOL, 14, 1, 1),
SOC_DAPM_SINGLE("MIC12SPK Playback Switch", ALC5624_MIC_ROUTING_CTRL, 14, 1, 1),
SOC_DAPM_SINGLE("MIC22SPK Playback Switch", ALC5624_MIC_ROUTING_CTRL, 6, 1, 1),
SOC_DAPM_SINGLE("DAC2SPK Playback Switch", ALC5624_STEREO_DAC_VOL, 14, 1, 1),
};

/* Mono Mixer */
static const struct snd_kcontrol_new alc5624_mono_mixer_controls[] = {
SOC_DAPM_SINGLE("LI2MONO Playback Switch", ALC5624_LINE_IN_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("MIC12MONO Playback Switch", ALC5624_MIC_ROUTING_CTRL, 13, 1, 1),
SOC_DAPM_SINGLE("MIC22MONO Playback Switch", ALC5624_MIC_ROUTING_CTRL, 5, 1, 1),
SOC_DAPM_SINGLE("DAC2MONO Playback Switch", ALC5624_STEREO_DAC_VOL, 13, 1, 1),
SOC_DAPM_SINGLE("ADC2MONO_L Playback Switch", ALC5624_ADC_REC_GAIN, 13, 1, 1),
SOC_DAPM_SINGLE("ADC2MONO_R Playback Switch", ALC5624_ADC_REC_GAIN, 12, 1, 1),
};

/* mono output mux */
static const char *alc5624_mono_pga[] = {"VMID","HP Mixer","SPK Mixer","MONO Mixer"};
static const struct soc_enum alc5624_mono_mux_enum[] = {
SOC_ENUM_SINGLE(ALC5624_OUTPUT_MIXER_CTRL,6,  4, alc5624_mono_pga) /* mono input sel 4 */
};
static const struct snd_kcontrol_new alc5624_mono_mux_controls =
SOC_DAPM_ENUM("Route", alc5624_mono_mux_enum[0]);


/* speaker left output mux */
static const char *alc5624_spkl_pga[] = {"VMID","HPL Mixer","SPK Mixer","MONO Mixer"};
static const struct soc_enum alc5624_hp_spkl_mux_enum[] = {
SOC_ENUM_SINGLE(ALC5624_OUTPUT_MIXER_CTRL,14, 4, alc5624_spkl_pga) /* spk left input sel 0 */	
};
static const struct snd_kcontrol_new alc5624_hp_spkl_mux_controls =
SOC_DAPM_ENUM("Route", alc5624_hp_spkl_mux_enum[0]);

/* speaker right output mux */
static const char *alc5624_spkr_pga[] = {"VMID","HPR Mixer","SPK Mixer","MONO Mixer"};
static const struct soc_enum alc5624_hp_spkr_mux_enum[] = {
SOC_ENUM_SINGLE(ALC5624_OUTPUT_MIXER_CTRL,11, 4, alc5624_spkr_pga) /* spk right input sel 1 */	
};
static const struct snd_kcontrol_new alc5624_hp_spkr_mux_controls =
SOC_DAPM_ENUM("Route", alc5624_hp_spkr_mux_enum[0]);

/* headphone left output mux */
static const char *alc5624_hpl_pga[]  = {"VMID","HPL Mixer"};
static const struct soc_enum alc5624_hpl_out_mux_enum[] = {
SOC_ENUM_SINGLE(ALC5624_OUTPUT_MIXER_CTRL,9,  2, alc5624_hpl_pga) /* hp left input sel 2 */	
};
static const struct snd_kcontrol_new alc5624_hpl_out_mux_controls =
SOC_DAPM_ENUM("Route", alc5624_hpl_out_mux_enum[0]);

/* headphone right output mux */
static const char *alc5624_hpr_pga[]  = {"VMID","HPR Mixer"};
static const struct soc_enum alc5624_hpr_out_mux_enum[] = {
SOC_ENUM_SINGLE(ALC5624_OUTPUT_MIXER_CTRL,8,  2, alc5624_hpr_pga) /* hp right input sel 3 */	
};
static const struct snd_kcontrol_new alc5624_hpr_out_mux_controls =
SOC_DAPM_ENUM("Route", alc5624_hpr_out_mux_enum[0]);

/* We have to create a fake left and right HP mixers because
 * the codec only has a single control that is shared by both channels.
 * This makes it impossible to determine the audio path using the current
 * register map, thus we add a new (virtual) register to help determine the
 * audio route within the device.
 */
static int mixer_event (struct snd_soc_dapm_widget *w, 
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	
	u16 l, r, lineIn,mic1,mic2, phone, pcm;
	dev_dbg(codec->dev,"%s()\n", __FUNCTION__);
	
	l = snd_soc_read(codec, VIRTUAL_HPL_MIXER);
	r = snd_soc_read(codec, VIRTUAL_HPR_MIXER);
	lineIn = snd_soc_read(codec, ALC5624_LINE_IN_VOL);
	mic1 = snd_soc_read(codec, ALC5624_MIC_ROUTING_CTRL);
	mic2 = snd_soc_read(codec, ALC5624_MIC_ROUTING_CTRL);
	phone = snd_soc_read(codec,ALC5624_PHONEIN_MONO_OUT_VOL);
	pcm = snd_soc_read(codec, ALC5624_STEREO_DAC_VOL);

	if (event & SND_SOC_DAPM_PRE_REG)
		return 0;
		
	if (l & 0x1 || r & 0x1)
		snd_soc_write(codec, ALC5624_STEREO_DAC_VOL, pcm & 0x7fff);
	else
		snd_soc_write(codec, ALC5624_STEREO_DAC_VOL, pcm | 0x8000);

	if (l & 0x2 || r & 0x2)
		snd_soc_write(codec, ALC5624_MIC_ROUTING_CTRL, mic2 & 0xff7f);
	else
		snd_soc_write(codec, ALC5624_MIC_ROUTING_CTRL, mic2 | 0x0080);

	if (l & 0x4 || r & 0x4)
		snd_soc_write(codec, ALC5624_MIC_ROUTING_CTRL, mic1 & 0x7fff);
	else
		snd_soc_write(codec, ALC5624_MIC_ROUTING_CTRL, mic1 | 0x8000);

	if (l & 0x8 || r & 0x8)
		snd_soc_write(codec, ALC5624_PHONEIN_MONO_OUT_VOL, phone & 0x7fff);
	else
		snd_soc_write(codec, ALC5624_PHONEIN_MONO_OUT_VOL, phone | 0x8000);

	if (l & 0x10 || r & 0x10)
		snd_soc_write(codec, ALC5624_LINE_IN_VOL, lineIn & 0x7fff);
	else
		snd_soc_write(codec, ALC5624_LINE_IN_VOL, lineIn | 0x8000);

	return 0;
}


static const struct snd_soc_dapm_widget alc5624_dapm_widgets[] = {
SND_SOC_DAPM_MUX("Mono Out Mux", SND_SOC_NOPM, 0, 0,
	&alc5624_mono_mux_controls),
SND_SOC_DAPM_MUX("Left Speaker Out Mux", SND_SOC_NOPM, 0, 0,
	&alc5624_hp_spkl_mux_controls),
SND_SOC_DAPM_MUX("Right Speaker Out Mux", SND_SOC_NOPM, 0, 0,
	&alc5624_hp_spkr_mux_controls),
SND_SOC_DAPM_MUX("Left Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&alc5624_hpl_out_mux_controls),
SND_SOC_DAPM_MUX("Right Headphone Out Mux", SND_SOC_NOPM, 0, 0,
	&alc5624_hpr_out_mux_controls),
SND_SOC_DAPM_MIXER_E("Left HP Mixer",ALC5624_PWR_MANAG_ADD2, 5, 0,
	&alc5624_hpl_mixer_controls[0], ARRAY_SIZE(alc5624_hpl_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER_E("Right HP Mixer",ALC5624_PWR_MANAG_ADD2, 4, 0,
	&alc5624_hpr_mixer_controls[0], ARRAY_SIZE(alc5624_hpr_mixer_controls),
	mixer_event, SND_SOC_DAPM_POST_REG),
SND_SOC_DAPM_MIXER("Mono Mixer", ALC5624_PWR_MANAG_ADD2, 2, 0,
	&alc5624_mono_mixer_controls[0], ARRAY_SIZE(alc5624_mono_mixer_controls)),
SND_SOC_DAPM_MIXER("Speaker Mixer", ALC5624_PWR_MANAG_ADD2,3,0,
	&alc5624_speaker_mixer_controls[0],
	ARRAY_SIZE(alc5624_speaker_mixer_controls)),	
SND_SOC_DAPM_MIXER("Left Record Mixer", ALC5624_PWR_MANAG_ADD2,1,0,
	&alc5624_captureL_mixer_controls[0],
	ARRAY_SIZE(alc5624_captureL_mixer_controls)),	
SND_SOC_DAPM_MIXER("Right Record Mixer", ALC5624_PWR_MANAG_ADD2,0,0,
	&alc5624_captureR_mixer_controls[0],
	ARRAY_SIZE(alc5624_captureR_mixer_controls)),				
SND_SOC_DAPM_DAC("Left DAC", "Left HiFi Playback", ALC5624_PWR_MANAG_ADD2,9, 0),
SND_SOC_DAPM_DAC("Right DAC", "Right HiFi Playback", ALC5624_PWR_MANAG_ADD2, 8, 0),	
SND_SOC_DAPM_MIXER("I2S Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("Line Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_MIXER("HP Mixer", SND_SOC_NOPM, 0, 0, NULL, 0),
SND_SOC_DAPM_ADC("Left ADC", "Left HiFi Capture", ALC5624_PWR_MANAG_ADD2, 7, 0),
SND_SOC_DAPM_ADC("Right ADC", "Right HiFi Capture", ALC5624_PWR_MANAG_ADD2, 6, 0),

SND_SOC_DAPM_PGA("Mono Out", ALC5624_PWR_MANAG_ADD3, 14, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Speaker N", ALC5624_PWR_MANAG_ADD3, 13, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Speaker N", ALC5624_PWR_MANAG_ADD3, 12, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Headphone", ALC5624_PWR_MANAG_ADD3, 11, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Headphone", ALC5624_PWR_MANAG_ADD3, 10, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Speaker", ALC5624_PWR_MANAG_ADD3, 9, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Speaker", ALC5624_PWR_MANAG_ADD3, 8, 0, NULL, 0),
SND_SOC_DAPM_PGA("Left Line In", ALC5624_PWR_MANAG_ADD3, 7, 0, NULL, 0),
SND_SOC_DAPM_PGA("Right Line In", ALC5624_PWR_MANAG_ADD3, 6, 0, NULL, 0),
SND_SOC_DAPM_PGA("Phone In PGA", ALC5624_PWR_MANAG_ADD3, 5, 0, NULL, 0),
SND_SOC_DAPM_PGA("Phone In Mixer", ALC5624_PWR_MANAG_ADD3, 4, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 PGA", ALC5624_PWR_MANAG_ADD3, 3, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 PGA", ALC5624_PWR_MANAG_ADD3, 2, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 1 Pre Amp", ALC5624_PWR_MANAG_ADD3, 1, 0, NULL, 0),
SND_SOC_DAPM_PGA("Mic 2 Pre Amp", ALC5624_PWR_MANAG_ADD3, 0, 0, NULL, 0),

SND_SOC_DAPM_MICBIAS("Mic Bias1", ALC5624_PWR_MANAG_ADD1, 3, 0),
SND_SOC_DAPM_MICBIAS("Mic Bias2", ALC5624_PWR_MANAG_ADD1, 2, 0),
SND_SOC_DAPM_OUTPUT("MONO"),
SND_SOC_DAPM_OUTPUT("HPL"),
SND_SOC_DAPM_OUTPUT("HPR"),
SND_SOC_DAPM_OUTPUT("SPKL"),
SND_SOC_DAPM_OUTPUT("SPKR"),
SND_SOC_DAPM_OUTPUT("SPKLN"),
SND_SOC_DAPM_OUTPUT("SPKRN"),
SND_SOC_DAPM_INPUT("LINEL"),
SND_SOC_DAPM_INPUT("LINER"),
SND_SOC_DAPM_INPUT("PHONEIN"),
SND_SOC_DAPM_INPUT("MIC1"),
SND_SOC_DAPM_INPUT("MIC2"),
SND_SOC_DAPM_VMID("VMID"),
};

static const struct snd_soc_dapm_route audio_map[] = {
	/* virtual mixer - mixes left & right channels for spk and mono mixers */
	{"I2S Mixer", NULL, "Left DAC"},
	{"I2S Mixer", NULL, "Right DAC"},
	{"Line Mixer", NULL, "Left Line In"},
	{"Line Mixer", NULL, "Right Line In"},	
	{"HP Mixer", NULL, "Left HP Mixer"},
	{"HP Mixer", NULL, "Right HP Mixer"},

	/* left HP mixer */
	{"Left HP Mixer", "LI2HP Playback Switch", "LINEL"},
	{"Left HP Mixer", "PHO2HP Playback Switch","PHONEIN"},
	{"Left HP Mixer", "MIC12HP Playback Switch","MIC1"},
	{"Left HP Mixer", "MIC22HP Playback Switch","MIC2"},
	{"Left HP Mixer", "DAC2HP Playback Switch","Left DAC"},
	{"Left HP Mixer", "ADCL2HP Playback Switch","Left Record Mixer"},
	
	/* right HP mixer */
	{"Right HP Mixer", "LI2HP Playback Switch", "LINER"},
	{"Right HP Mixer", "PHO2HP Playback Switch","PHONEIN"},
	{"Right HP Mixer", "MIC12HP Playback Switch","MIC1"},
	{"Right HP Mixer", "MIC22HP Playback Switch","MIC2"},
	{"Right HP Mixer", "DAC2HP Playback Switch","Right DAC"},
	{"Right HP Mixer", "ADCR2HP Playback Switch","Right Record Mixer"},
	
	/* speaker mixer */
	{"Speaker Mixer", "LI2SPK Playback Switch","Line Mixer"},
	{"Speaker Mixer", "PHO2SPK Playback Switch","PHONEIN"},
	{"Speaker Mixer", "MIC12SPK Playback Switch","MIC1"},
	{"Speaker Mixer", "MIC22SPK Playback Switch","MIC2"},
	{"Speaker Mixer", "DAC2SPK Playback Switch","I2S Mixer"},

	/* mono mixer */
	{"Mono Mixer", "LI2MONO Playback Switch","Line Mixer"},
	{"Mono Mixer", "MIC12MONO Playback Switch","MIC1"},
	{"Mono Mixer", "MIC22MONO Playback Switch","MIC2"},
	{"Mono Mixer", "DAC2MONO Playback Switch","I2S Mixer"},
	{"Mono Mixer", "ADC2MONO_L Playback Switch","Left Record Mixer"},
	{"Mono Mixer", "ADC2MONO_R Playback Switch","Right Record Mixer"},
	
	/*Left record mixer */
	{"Left Record Mixer", "Mic1 Capture Switch","Mic 1 Pre Amp"},
	{"Left Record Mixer", "Mic2 Capture Switch","Mic 2 Pre Amp"},
	{"Left Record Mixer", "LineInL Capture Switch","LINEL"},
	{"Left Record Mixer", "Phone Capture Switch","PHONEIN"},
	{"Left Record Mixer", "HPMixerL Capture Switch","Left HP Mixer"},
	{"Left Record Mixer", "SPKMixer Capture Switch","Speaker Mixer"},
	{"Left Record Mixer", "MonoMixer Capture Switch","Mono Mixer"},
	
	/*Right record mixer */
	{"Right Record Mixer", "Mic1 Capture Switch","Mic 1 Pre Amp"},
	{"Right Record Mixer", "Mic2 Capture Switch","Mic 2 Pre Amp"},
	{"Right Record Mixer", "LineInR Capture Switch","LINER"},
	{"Right Record Mixer", "Phone Capture Switch","PHONEIN"},
	{"Right Record Mixer", "HPMixerR Capture Switch","Right HP Mixer"},
	{"Right Record Mixer", "SPKMixer Capture Switch","Speaker Mixer"},
	{"Right Record Mixer", "MonoMixer Capture Switch","Mono Mixer"},	

	/* headphone left mux */
	{"Left Headphone Out Mux", "HPL Mixer", "Left HP Mixer"},

	/* headphone right mux */
	{"Right Headphone Out Mux", "HPR Mixer", "Right HP Mixer"},

	/* speaker left mux */
	{"Left Speaker Out Mux", "HPL Mixer", "Left HP Mixer"},
	{"Left Speaker Out Mux", "SPK Mixer", "Speaker Mixer"},
	{"Left Speaker Out Mux", "MONO Mixer", "Mono Mixer"},

	/* speaker right mux */
	{"Right Speaker Out Mux", "HPR Mixer", "Right HP Mixer"},
	{"Right Speaker Out Mux", "SPK Mixer", "Speaker Mixer"},
	{"Right Speaker Out Mux", "MONO Mixer", "Mono Mixer"},

	/* mono mux */
	{"Mono Out Mux", "HP Mixer", "HP Mixer"},
	{"Mono Out Mux", "SPK Mixer", "Speaker Mixer"},
	{"Mono Out Mux", "MONO Mixer", "Mono Mixer"},
	
	/* output pga */
	{"HPL", NULL, "Left Headphone"},
	{"Left Headphone", NULL, "Left Headphone Out Mux"},
	{"HPR", NULL, "Right Headphone"},
	{"Right Headphone", NULL, "Right Headphone Out Mux"},
	{"SPKL", NULL, "Left Speaker"},
	{"SPKLN", NULL, "Left Speaker N"},
	{"Left Speaker", NULL, "Left Speaker Out Mux"},
	{"Left Speaker N", NULL, "Left Speaker Out Mux"},
	{"SPKR", NULL, "Right Speaker"},
	{"SPKRN", NULL, "Right Speaker N"},	
	{"Right Speaker", NULL, "Right Speaker Out Mux"},
	{"Right Speaker N", NULL, "Right Speaker Out Mux"},
	{"MONO", NULL, "Mono Out"},
	{"Mono Out", NULL, "Mono Out Mux"},

	/* input pga */
	{"Left Line In", NULL, "LINEL"},
	{"Right Line In", NULL, "LINER"},
	{"Phone In PGA", NULL, "PHONEIN"},
	{"Phone In Mixer", NULL, "PHONEIN"},
	{"Mic 1 Pre Amp", NULL, "MIC1"},
	{"Mic 2 Pre Amp", NULL, "MIC2"},	
	{"Mic 1 PGA", NULL, "Mic 1 Pre Amp"},
	{"Mic 2 PGA", NULL, "Mic 2 Pre Amp"},

	/* left ADC */
	{"Left ADC", NULL, "Left Record Mixer"},

	/* right ADC */
	{"Right ADC", NULL, "Right Record Mixer"}
};

static int alc5624_dai_startup(struct snd_pcm_substream *substream,
			      struct snd_soc_dai *dai)
{
	int is_play = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;
	struct snd_soc_codec *codec = dai->codec;
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	
	enum snd_soc_bias_level level = codec->dapm.bias_level;
	
	dev_dbg(codec->dev, "%s(): is_play:%d, bias level:%d\n", __FUNCTION__,is_play, level);
	
	
	/* Power up the required parts */
	if (is_play) {

		/* If codec is fully powered up, do the programming inmediately -
		   Otherwise, it will be deferred until we actually transition
 		   to full power up */
		if (level == SND_SOC_BIAS_ON) {
		
			/* Enable class AB power amplifier and thermal detect */
			snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD2,0xC000,0xC000); 
			
			/* Power up speaker amplifier, Headphone out,Mono out, and Stereo DAC */
			snd_soc_update_bits(codec,ALC5624_PD_CTRL_STAT,0xC200,0x0000);
		}
	
		/* Remember we are playing */
		alc5624->streams_running |= ALC5624_STREAM_PLAYBACK;
		
	} else {

		/* If codec is fully powered up, do the programming inmediately -
		   Otherwise, it will be deferred until we actually transition
 		   to full power up */
		if (level == SND_SOC_BIAS_ON) {
	
			/* Power up Stereo ADC */
			snd_soc_update_bits(codec,ALC5624_PD_CTRL_STAT,0x0100,0x0000);
		}

		/* Remember we are recording */
		alc5624->streams_running |= ALC5624_STREAM_RECORD;
		
	}

	return 0;
}

static void alc5624_dai_shutdown(struct snd_pcm_substream *substream,
			       struct snd_soc_dai *dai)
{
	int is_play = substream->stream == SNDRV_PCM_STREAM_PLAYBACK;
	struct snd_soc_codec *codec = dai->codec;
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);

	enum snd_soc_bias_level level = codec->dapm.bias_level;
	
	dev_dbg(codec->dev, "%s(): is_play:%d, bias level: %d\n", __FUNCTION__,is_play, level);	
	
	if (is_play) {
	
		/* If codec is not off, do the programming inmediately -
		   Otherwise, it will be deferred until we actually power
		   it up (because we don't have an active clock while off
		   and we need it to access the registers) */
		if (level != SND_SOC_BIAS_OFF) {
	
			/* Disable class AB power amplifier and thermal detect */
			snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD2,0xC000,0x0000); 
		
			/* Power down speaker amplifier, Headphone out,Mono out, and Stereo DAC */
			snd_soc_update_bits(codec,ALC5624_PD_CTRL_STAT,0xC200,0xC200);
		}

		/* Remember we are not playing anymore */
		alc5624->streams_running &= ~ALC5624_STREAM_PLAYBACK;

	} else {
	
		/* If codec is not off, do the programming inmediately -
		   Otherwise, it will be deferred until we actually power
		   it up (because we don't have an active clock while off
		   and we need it to access the registers) */
		if (level != SND_SOC_BIAS_OFF) {

			/* Power down Stereo ADC */
			snd_soc_update_bits(codec,ALC5624_PD_CTRL_STAT,0x0100,0x0100);
		}

		/* Remember we are not recording anymore */
		alc5624->streams_running &= ~ALC5624_STREAM_RECORD;

	}
}

/* Greatest common divisor */
static unsigned int gcd(unsigned int u, unsigned int v)
{
	int shift;

	/* GCD(0,x) := x */
	if (u == 0 || v == 0)
		return u | v;

	/* Let shift := lg K, where K is the greatest power of 2
	   dividing both u and v. */
	for (shift = 0; ((u | v) & 1) == 0; ++shift) {
		u >>= 1;
		v >>= 1;
	}

	while ((u & 1) == 0)
		u >>= 1;

	/* From here on, u is always odd. */
	do {
		while ((v & 1) == 0)  /* Loop X */
			v >>= 1;

		/* Now u and v are both odd, so diff(u, v) is even.
		   Let u = min(u, v), v = diff(u, v)/2. */
		if (u < v) {
			v -= u;
		} else {
		
			unsigned int diff = u - v;
			u = v;
			v = diff;
			}
			v >>= 1;
	} while (v != 0);

	return u << shift;
}

static int alc5624_set_dai_pll(struct snd_soc_dai *codec_dai, int pll_id,
		int source, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	unsigned int rin, rout, cd, m, finkhz, msel, ksel, psel, fvcodifsel;
	unsigned int fvcosel;
	
	dev_dbg(codec->dev, "%s(): freq_in:%d, freq_out:%d\n", __FUNCTION__,freq_in,freq_out);

	/* Codec sys-clock from MCLK */
	snd_soc_update_bits(codec,ALC5624_GEN_CTRL_REG1,0x8000, 0x0000);
	
	/* disable PLL power */
	snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD2,0x1000, 0x0000);	
	
	/* We are not using the PLL */
	alc5624->using_pll = 0;
	
	/* If input and output frequency are the same, or no freq specified, disable PLL */
	if ((!freq_in || !freq_out) || freq_in == freq_out) {
		dev_dbg(codec->dev, "%s(): disabling PLL\n", __FUNCTION__);
		return 0;
	}
	
	/* Now, given input and output frequencies, we must find the 
	   values for the PLL */
	
	/* 
	  According to datasheet:
	  	  Fout = (MCLK * (N+2)) / ((M + 2) * (K + 2) * (P+1)) 
		  
	  Where:
		K < 8
	    M < 16
	    N < 256
	    P < 2
	 */
	
	/* First, get the maximum common divider between input and 
	   output frequencies */
	   
	/* Get the greatest common divisor */
	cd = gcd(freq_in,freq_out);
	
	/* Reduce frequencies using it */
	rin = freq_in / cd;
	rout = freq_out / cd;
	
	/* To synthetize fout from fin, we must multiply fin by rout, then 
	   divide by rin. but, in the process, we must try to make sure the 
	   Fvco to stay between 90 ... 100Mhz.
	   
	   Input frequency is first divided by (M+2)*(P+1), then multiplied by N
	   (that is the Fvco), then divided by (K+2) 
	   
	   rout = (N+2)
	   rin  = ((M + 2) * (K + 2) * (P+1))
	   */
	   
	/* Adjust rout and rin to place them in range, if needed */
	if (rout < 2 || rin < 2) {
		rout <<= 1;
		rin <<= 1;
		cd >>= 1;
	}
	   
	/* Check that rout can be achieved */
	if (rout < 2 || rout > 257) {
		dev_err(codec->dev, "N:%d outside of bounds of PLL (1 < N < 258), rin:%d, cd:%d\n", rout,rin,cd);
		return -EINVAL;
	}
	
	/* Get the input frequency in khz, even if losing precision. We will only
	   use it to calculate the approximate VCO frequency and avoid overflows
	   during calculations */
	finkhz = freq_in;
	   
	/* By default, no m selected */
	msel = 0;
	ksel = 0;
	psel = 0;
	fvcodifsel = 0;
	fvcosel = 0;
	   
	/* Iterate through all possible values for M, checking if we can reach
	   the requested ratio */
	for (m = 1; m < 18; m++) {
		unsigned int kdp,fvco,fvcodif;	
		
		/* Check if we can divide without remainder rin by this m value. 
		   This would mean this m value is usable */
		if ((rin % m) != 0) 
			continue;
		
		/* Calculate K * P */
		kdp = rin / m;
		
		/* Filter out impossible K values ... */
		if (kdp > 18 || kdp < 2 || ((kdp & 1) != 0 && kdp > 9)) 
			continue;
		
		/* Calculate PLL Fvco assuming P = 1 */
		fvco = finkhz * rout / m;
		
		/* Calculate distance to optimum fvco */
		fvcodif = (fvco > 95000) ? (fvco - 95000) : (95000 - fvco);
		
		/* If the fvcodif is less than the previously selected one, or no
		   previous selected one... */
		if (!msel || fvcodif < fvcodifsel) {
		
			/* Keep it. Assumes P = 1 */
			msel = m;
			ksel = kdp;
			psel = 1;
			fvcodifsel = fvcodif;
			fvcosel = fvco;
		}
		
		/* If kdp is even, then we can try another configuration with P = 2.
			This means halving the Fvco */
		if (!(kdp & 1) && kdp > 3) {
		
			/* Scale frequency down */
			fvco >>= 1;

			/* Calculate distance to optimum fvco */
			fvcodif = (fvco > 95000) ? (fvco - 95000) : (95000 - fvco);
		
			/* If the fvcodif is less than the previously selected one, or no
				previous selected one... */
			if (!msel || fvcodif < fvcodifsel) {
		
				/* Keep it - P = 2*/
				msel = m;
				ksel = kdp >> 1;
				psel = 2;
				fvcodifsel = fvcodif;
				fvcosel = fvco;
			}
		}
	}

	/* Well, check if there was a valid config */
	if (!msel) {
		dev_err(codec->dev, "No valid M,K,P coefficients gives the needed divider %d for PLL\n", rin);
		return -EINVAL;
	}
	   
	/* Well. we got it. */
	dev_dbg(codec->dev, "Using M:%d, N:%d, K:%d, P:%d coefficients for PLL (div:%d=%d), fvco:%d khz\n", msel,rout,ksel,psel,rin,msel*ksel*psel,fvcosel);

	/* Set the PLL predivider */
	snd_soc_update_bits(codec,ALC5624_GEN_CTRL_REG2,0x0001,(ksel > 1) ? 0x0001 : 0x0000);
	
	/* set PLL parameter */
 	snd_soc_write(codec,ALC5624_PLL_CTRL,
		( (msel - 2) & 0xF) |			/* M coefficient */
		(((ksel - 2) & 0x7) << 4) |		/* K coefficient */
		((msel == 1) ? 0x80 : 0x00) | 	/* M bypass */
		(((rout - 2) & 0xFF) << 8)		/* N coefficient */
		);

	/* enable PLL power */
	snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD2, 0x1000,0x1000);	
	
	/* Codec sys-clock from PLL */
	snd_soc_update_bits(codec,ALC5624_GEN_CTRL_REG1,0x8000,0x8000);

	/* We are using the PLL */
	alc5624->using_pll = 1;

	/* done */
	return 0;
}

/*
 * Clock after PLL and dividers
 */
static int alc5624_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir) 
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	dev_dbg(codec->dev, "%s(): clk_id = %d, freq = %d\n", __FUNCTION__, clk_id, freq);

	if (freq == 0) {
		return -EINVAL;
	}
	
	alc5624->sysclk = freq;
	return 0;
}

static int alc5624_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = 0;
	dev_dbg(codec->dev, "%s(): format: 0x%08x\n", __FUNCTION__, fmt);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		iface = 0x0000;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		iface = 0x8000;
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface |= 0x0000;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iface |= 0x0001;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface |= 0x0002;
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface |= 0x0003;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		iface |= 0x0043;
		break;
	default:
		return -EINVAL;
	}

	/* clock inversion */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iface |= 0x0000;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x1040;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |= 0x1000;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface |= 0x0040;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_update_bits(codec,ALC5624_MAIN_SDP_CTRL,0x9043,iface);
	return 0;
}

struct _coeff_div {
	u32 mclk;
	u32 rate;
	u16 regvalue1;
	u16 regvalue2;
};


/* codec hifi mclk (after PLL) clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
	/* 8k */
	{ 8192000,  8000, 0x3272,0x1212},
	{12288000,  8000, 0x5272,0x2222},
	{24576000,  8000, 0x5324,0x2436},

	/* 11.025k */
	{11289600, 11025, 0x3272,0x1212},
	{16934400, 11025, 0x5272,0x2222},
	{22579200, 11025, 0x7214,0x1436},

	/* 16k */
	{12288000, 16000, 0x2272,0x2020},
	{16384000, 16000, 0x3272,0x1212},
	{24576000, 16000, 0x5272,0x2222},

	/* 22.05k */	
	{11289600, 22050, 0x3172,0x1010},
	{16934400, 22050, 0x2272,0x2020},
	{22579200, 22050, 0x3204,0x3036},

	/* 32k */
	{12288000, 32000, 0x2172,0x2121},
	{16384000, 32000, 0x3172,0x1010},
	{24576000, 32000, 0x2272,0x2020},

	/* 44.1k */
	{11289600, 44100, 0x3072,0x0000},
	{22579200, 44100, 0x3172,0x1010},

	/* 48k */
	{12288000, 48000, 0x3072,0x0000},
	{24576000, 48000, 0x3172,0x1010},
};


static int get_coeff(int mclk, int rate)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(coeff_div); i++) {
		if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk)
			return i;
	}
	return -EINVAL;
}


static int alc5624_pcm_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	
	u16 iface=snd_soc_read(codec,ALC5624_MAIN_SDP_CTRL)&0xfff3;
	int coeff = get_coeff(alc5624->sysclk, params_rate(params));

	dev_dbg(codec->dev, "%s(): format: 0x%08x\n", __FUNCTION__,params_format(params));	
	
	/* bit size */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		iface |= 0x0000;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		iface |= 0x0004;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		iface |= 0x0008;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		iface |= 0x000c;
		break;
	default:
		return -EINVAL;
	}

	/* set iface & srate */
	snd_soc_write(codec, ALC5624_MAIN_SDP_CTRL, iface);
	if (coeff >= 0)
	{
		snd_soc_write(codec, ALC5624_STEREO_DAC_CLK_CTRL1,coeff_div[coeff].regvalue1);
		snd_soc_write(codec, ALC5624_STEREO_DAC_CLK_CTRL2,coeff_div[coeff].regvalue2);
	}
	return 0;
}

static int alc5624_digital_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	dev_dbg(codec->dev, "%s(): mute: %d\n", __FUNCTION__,mute);

	if (mute) {
		snd_soc_update_bits(codec, ALC5624_MISC_CTRL, (0x3<<5), (0x3<<5));
	} else {
		snd_soc_update_bits(codec, ALC5624_MISC_CTRL, (0x3<<5), 0);
	}
	return 0;
}

/* write to the alc5624 register space */
static int alc5624_hw_write(void* control_data,const char* data_in_s,int len)
{
	struct alc5624_priv *alc5624 = control_data;
	u8* data_in = (u8*)data_in_s;
	
	/* If it is a real register, call the original bus access function */
	if (data_in[0] <= ALC5624_VENDOR_ID2)
		return alc5624->bus_hw_write(alc5624->bus_control_data,data_in_s,len);
	
	/* If dealing with one of the virtual mixers, discard the value, as there
	   is no real hw register. This value will also be stored in the register
	   cache, and that value will be used instead of the actual value of the
	   nonexistant register, when read */
	if (data_in[0] == VIRTUAL_HPL_MIXER ||
		data_in[0] == VIRTUAL_HPR_MIXER)
		return 0;
	
	/* Dealing with one of the indexed registers. Perform the access */
	if (data_in[0] >= VIRTUAL_IDX_BASE &&
		data_in[0] < REGISTER_COUNT) {
		
		u8 data[3];
		int ret;
		
		/* Access the indexed register */
		data[0] = ALC5624_INDEX_ADDRESS;
		data[1] = 0; /* hi */
		data[2] = data_in[0]-VIRTUAL_IDX_BASE; /* lo */
		if ((ret = alc5624->bus_hw_write(alc5624->bus_control_data,data,3)) < 0)
			return ret;

		/* Set its value and return */			
		data[0] = ALC5624_INDEX_DATA;
		data[1] = data_in[1];
		data[2] = data_in[2];
		return alc5624->bus_hw_write(alc5624->bus_control_data,data,3);
	}
	
	/* Register does not exist */
	return -EIO;
}


/* Sync reg_cache with the hardware - But skip the RESET address */
static void alc5624_sync_cache(struct snd_soc_codec *codec)
{
	/*struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);*/
	int i, step = codec->driver->reg_cache_step, size = codec->driver->reg_cache_size;
	u16 *cache = codec->reg_cache;
	u8 data[3];

	/* Already synchronized, no need to resync again */
	if (!codec->cache_sync)
		return;

	codec->cache_only = 0;

	/* Sync back cached values if they're different from the
	 * hardware default.
	 */
	for (i = 2 ; i < size ; i += step) {
		data[0] = i;
		data[1] = cache[i] >> 8;
		data[2] = cache[i];
		alc5624_hw_write(codec->control_data, data, 3);
	}

	codec->cache_sync = 0;
};

/* Reset the codec */
static int alc5624_reset(struct snd_soc_codec *codec)
{
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	u8 data[3];
	
	data[0] = ALC5624_RESET;
	data[1] = 0;
	data[2] = 0;
	return alc5624->bus_hw_write(alc5624->bus_control_data, data, 3);
}


static int alc5624_set_bias_level(struct snd_soc_codec *codec,
				      enum snd_soc_bias_level level)
{
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	
	dev_dbg(codec->dev, "%s(): level: %d\n", __FUNCTION__,level);
	
	/* If already at that level, do not try it again */
	if (codec->dapm.bias_level == level)
	{
		return 0;
	}
		
	switch (level) {
	case SND_SOC_BIAS_ON:
	
		/* If playing samples, power the amplifiers */
		if (alc5624->streams_running & ALC5624_STREAM_PLAYBACK) {
		
			/* Enable class AB power amplifier and thermal detect */
			snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD2,0xC000,0xC000); 
			
			/* Power up speaker amplifier, Headphone out,Mono out, and Stereo DAC */
			snd_soc_update_bits(codec,ALC5624_PD_CTRL_STAT,0xC200,0x0000);
		}
	
		if (alc5624->streams_running &= ALC5624_STREAM_RECORD) {
	
			/* Power up Stereo ADC */
			snd_soc_update_bits(codec,ALC5624_PD_CTRL_STAT,0x0100,0x0000);
		}

		/* If using the PLL enable it and use it as clock source */
		if (alc5624->using_pll) {
		
			/* enable PLL power */
			snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD2, 0x1000,0x1000);	
	
			/* Codec sys-clock from PLL */
			snd_soc_update_bits(codec,ALC5624_GEN_CTRL_REG1, 0x8000,0x8000);
		}
		break;
		
	case SND_SOC_BIAS_PREPARE:
	
		//LOW ON----- power on all power not controlled by DAPM - Codec sys-clock from MCLK */
		snd_soc_update_bits(codec,ALC5624_GEN_CTRL_REG1,0x8000,0x0000);
		snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD1,0xE803,0x2803);
		snd_soc_update_bits(codec,ALC5624_PWR_MANAG_ADD2,0x3400,0x2000);
		snd_soc_update_bits(codec,ALC5624_PD_CTRL_STAT,0x2F00,0x0300);

		break;
		
	case SND_SOC_BIAS_STANDBY:
		/* If resuming operation from stop ... */
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF)
		{
			/* Enable clk and regulator */
			regulator_enable(alc5624->vdd);
			clk_enable(alc5624->mclk);
	
			/* Reset the codec */
			alc5624_reset(codec);
			mdelay(1);

			/* Sync registers to cache */
			alc5624_sync_cache(codec);
		}

		/* everything off except vref/vmid - Codec sys-clock from MCLK */
		snd_soc_update_bits(codec, ALC5624_GEN_CTRL_REG1, 0x8000, 0x0000);
		snd_soc_update_bits(codec, ALC5624_PWR_MANAG_ADD1, 0xE803, 0x0003);
		snd_soc_update_bits(codec, ALC5624_PWR_MANAG_ADD2, 0x3400, 0x2000);
		snd_soc_update_bits(codec, ALC5624_PD_CTRL_STAT, 0x2F00, 0x0300);

		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF)
		{
			/* Enable fast Vref */
			snd_soc_update_bits(codec, ALC5624_MISC_CTRL, 0x8000, 0x0000);
			
			/* Let it stabilize */
			mdelay(10);
			
			/* Disable fast Vref */
			snd_soc_update_bits(codec, ALC5624_MISC_CTRL, 0x8000, 0x8000);
		}

		break;
		
	case SND_SOC_BIAS_OFF:
		/* everything off - Codec sys-clock from MCLK */
		snd_soc_update_bits(codec, ALC5624_GEN_CTRL_REG1, 0x8000, 0x0000);
		snd_soc_update_bits(codec, ALC5624_PWR_MANAG_ADD1, 0xE803, 0x0000);
		snd_soc_update_bits(codec, ALC5624_PWR_MANAG_ADD2, 0x3400, 0x0000);
		snd_soc_update_bits(codec, ALC5624_PD_CTRL_STAT, 0x2F00, 0x2F00);

		/* Make sure all writes from now on will be resync when resuming */
		codec->cache_sync = 1;

		/* Disable clk and regulator */
		clk_disable(alc5624->mclk);
		regulator_disable(alc5624->vdd);
		break;
	}
	
	codec->dapm.bias_level = level;
	return 0;
}
 
#define ALC5624_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	SNDRV_PCM_FMTBIT_S20_3LE | \
	SNDRV_PCM_FMTBIT_S24_LE | \
	SNDRV_PCM_FMTBIT_S32_LE)
			
static struct snd_soc_dai_ops alc5624_dai_ops = {
		.startup		= alc5624_dai_startup,
		.shutdown		= alc5624_dai_shutdown,
		.hw_params 		= alc5624_pcm_hw_params,
		.digital_mute 	= alc5624_digital_mute,
		.set_fmt 		= alc5624_set_dai_fmt,
		.set_sysclk 	= alc5624_set_dai_sysclk,
		.set_pll 		= alc5624_set_dai_pll,
};

static struct snd_soc_dai_driver alc5624_dai = {
	.name = "alc5624-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rate_min =	8000,
		.rate_max =	48000,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = ALC5624_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rate_min =	8000,
		.rate_max =	48000,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = ALC5624_FORMATS,},
	.ops = &alc5624_dai_ops,
	.symmetric_rates = 1,
}; 


/* Check if a register is volatile or not to forbid or not caching its value */
static int alc5624_volatile_register(struct snd_soc_codec *codec,
	unsigned int reg)
{
	if (reg == ALC5624_PD_CTRL_STAT ||
		reg == ALC5624_GPIO_PIN_STATUS ||
		reg == ALC5624_OVER_TEMP_CURR_STATUS ||
		reg == ALC5624_INDEX_DATA ||
		reg == ALC5624_EQ_STATUS)
		return 1;
	return 0;
}

/* read alc5624 hw register */
static unsigned int alc5624_hw_read(struct snd_soc_codec *codec,
				    unsigned int reg)
{
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	
	/* If dealing with the main volume, scale it as requested */
	if (reg == ALC5624_SPK_OUT_VOL) {
		/* Read the volume from the hw regs */
		unsigned int reg2v,voll,volr;

		/* bus_hw_read expects that codec->control_data is pointing to 
		   the original control_data.That is the only field accessed. Create
		   a temporary struct with the required data */
		struct snd_soc_codec tmpcodec;
		tmpcodec.control_data = alc5624->bus_control_data;
		reg2v = alc5624->bus_hw_read(&tmpcodec,reg);

		/* Get left and right volumes */
		voll = (reg2v & 0x1F00) >> 8;
		volr = (reg2v & 0x001F);

		/* Limit values */
		if (voll > 0x1F) voll = 0x1F;
		if (volr > 0x1F) volr = 0x1F;

		/* Recreate the value */
		reg2v = (reg2v & 0xE0E0) | (volr) | (voll << 8);

		/* And return the inversely scaled volume */
		return reg2v;
	}

	/* If it is a real register, call the original bus access function */
	if (reg <= ALC5624_VENDOR_ID2) {
		/* bus_hw_read expects that codec->control_data is pointing to
		 * the original control_data.That is the only field accessed. Create
		 * a temporary struct with the required data
		 */
		struct snd_soc_codec tmpcodec;
		tmpcodec.control_data = alc5624->bus_control_data;
		return alc5624->bus_hw_read(&tmpcodec, reg);
	}

	/* If dealing with one of the virtual mixers, return 0. This fn
	 * won't be called anymore, as the cache will hold the written
	 * value, and that value will be used instead of the actual value
	 * of the nonexistant register
	 */
	if (reg == VIRTUAL_HPL_MIXER || reg == VIRTUAL_HPR_MIXER)
		return 0;

	/* Dealing with one of the indexed registers. Perform the access */
	if (reg >= VIRTUAL_IDX_BASE && reg < REGISTER_COUNT) {
		
		struct snd_soc_codec tmpcodec;		
		u8 data[3];
		int ret;

		/* Access the indexed register */
		data[0] = ALC5624_INDEX_ADDRESS;
		data[1] = (u8)((reg-VIRTUAL_IDX_BASE) >> 8);
		data[2] = (u8)(reg-VIRTUAL_IDX_BASE);
		if ((ret = alc5624->bus_hw_write(alc5624->bus_control_data,data,3)) < 0)
			return ret;

		/* bus_hw_read expects that codec->control_data is pointing to
		 * the original control_data.That is the only field accessed.
		 * Create a temporary struct with the required data
		 */
		tmpcodec.control_data = alc5624->bus_control_data;
		return alc5624->bus_hw_read(&tmpcodec,ALC5624_INDEX_DATA);
	}

	/* Register does not exist */
	return -EIO;
}

/* Fetch register values from the hw */
static void alc5624_fill_cache(struct snd_soc_codec *codec)
{
	/*struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);*/
	int i, step = codec->driver->reg_cache_step, size = codec->driver->reg_cache_size;
	u16 *cache = codec->reg_cache;
	for (i = 0 ; i < size ; i += step)
		cache[i] = alc5624_hw_read(codec, i);
}

static int alc5624_suspend(struct snd_soc_codec *codec, pm_message_t mesg)
{
	dev_dbg(codec->dev, "%s()\n", __FUNCTION__);
	
	/* Go to off */
	alc5624_set_bias_level(codec, SND_SOC_BIAS_OFF);
	
	return 0;
}

static int alc5624_resume(struct snd_soc_codec *codec)
{
	dev_dbg(codec->dev, "%s()\n", __FUNCTION__);
	
	/* Go to standby */
	alc5624_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}

static int alc5624_probe(struct snd_soc_codec *codec)
{
	struct alc5624_priv *alc5624 = snd_soc_codec_get_drvdata(codec);
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int ret,i;
	int spkbias;
	int hpbias;
	unsigned long reg;

	/* Get the default read and write functions for this bus */
	ret = snd_soc_codec_set_cache_io(codec, 8, 16, alc5624->control_type);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	/* Get the original hw R/W functions */
	alc5624->bus_hw_read = codec->hw_read;
	alc5624->bus_hw_write = codec->hw_write;
	alc5624->bus_control_data = codec->control_data;

	/* And install our own functions to be able to provide virtual registers */
	codec->hw_read = alc5624_hw_read;
	codec->hw_write = alc5624_hw_write;
	codec->control_data = alc5624;

	/* power on device  */
	alc5624_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/* Modify the default values to properly config the CODEC */
	for (i = 0; i < ARRAY_SIZE(alc5624_reg_default); i++)
		snd_soc_write(codec,alc5624_reg_default[i].reg,
			      alc5624_reg_default[i].def);

	/* Configure amplifier bias voltages based on voltage supplies */
	spkbias = alc5624->spkvdd_mv >> 1;
	hpbias = alc5624->hpvdd_mv >> 1;
	reg = 0;

	/* Headphone amplifier bias */
	if (hpbias >= 1500)
		reg |= (2 << 8); // HP = 1.5v bias
	else if (hpbias >= 1250)
		reg |= (1 << 8); // HP = 1.25v bias
	/* else 0=1v bias */

	/* Speaker Class D amplifier bias */
	if (spkbias <  1250)
		reg |= (3 << 6); // SPK class D bias: 1.0Vdd
	else if (spkbias <  1500)
		reg |= (2 << 6); // SPK class D bias: 1.25Vdd
	else if (spkbias <  1750)
		reg |= (1 << 6); // SPK class D bias: 1.5Vdd
	/* else 0=1.75v bias */
	
	/* Speaker Class AB amplifier bias */
	if (spkbias <  1250)
		reg |= (5 << 3); // SPK class AB bias: 1.0Vdd
	else if (spkbias <  1500)
		reg |= (4 << 3); // SPK class AB bias: 1.25Vdd
	else if (spkbias <  1750)
		reg |= (3 << 3); // SPK class AB bias: 1.5Vdd
	else if (spkbias <  2000)
		reg |= (2 << 3); // SPK class AB bias: 1.75Vdd
	else if (spkbias <  2250)
		reg |= (1 << 3); // SPK class AB bias: 2.0Vdd
	/* else 0=2.25v bias */

	/* Set the amplifier biases */
	snd_soc_update_bits(codec,ALC5624_GEN_CTRL_REG1, 0x03F8, reg);

	/* Enable strong amp if using spkbias >= 1.5v */
	snd_soc_update_bits(codec, ALC5624_MISC_CTRL, 0x4000,
			    (spkbias >= 1500) ? 0x0000 : 0x4000);

	snd_soc_add_controls(codec, alc5624_snd_controls,
				    ARRAY_SIZE(alc5624_snd_controls));

	snd_soc_dapm_new_controls(dapm, alc5624_dapm_widgets,
					ARRAY_SIZE(alc5624_dapm_widgets));

	/* set up audio path interconnects */
	snd_soc_dapm_add_routes(dapm, audio_map, ARRAY_SIZE(audio_map));

	/* make sure to register the dapm widgets */
	snd_soc_dapm_new_widgets(dapm);

	return ret;
}

/* power down chip */
static int alc5624_remove(struct snd_soc_codec *codec)
{
	alc5624_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_device_alc5624 = {
	.probe			= alc5624_probe,
	.remove			= alc5624_remove,
	.suspend		= alc5624_suspend,
	.resume			= alc5624_resume,
	.volatile_register	= alc5624_volatile_register,
	.set_bias_level		= alc5624_set_bias_level,
	.reg_cache_size		= REGISTER_COUNT,
	.reg_word_size		= sizeof(u16),
	.reg_cache_step		= 2,
};

static __devinit int alc5624_i2c_probe(struct i2c_client *client,
				       const struct i2c_device_id *id)
{
	struct alc5624_platform_data *pdata;
	struct alc5624_priv *alc5624;
	
	int ret, vid1, vid2, ver;

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->dev, "Missing platform data\n");	
		return -ENODEV;
	}

	alc5624 = devm_kzalloc(&client->dev,
			       sizeof(struct alc5624_priv), GFP_KERNEL);
	if (alc5624 == NULL) {
		dev_err(&client->dev, "no memory for context\n");
		return -ENOMEM;
	}

	alc5624->vdd = regulator_get(&client->dev, "vdd");
	if (IS_ERR(alc5624->vdd)) {
		dev_err(&client->dev, "Unable to get regulator\n");
		return -ENODEV;
	}

	/* Get the MCLK */
	alc5624->mclk = clk_get(NULL, pdata->mclk);
	if (IS_ERR(alc5624->mclk)) {
		dev_err(&client->dev, "Unable to get MCLK\n");
		goto err_mclk;
	} 

	ret = regulator_enable(alc5624->vdd);
	if (ret) {
		dev_err(&client->dev, "Unable to enable regulator\n");
		goto err_reg;
	}
	
	/* Enable it to be able to access codec registers */
	clk_enable(alc5624->mclk);
	
	/* Read chip ids */
	vid1 = i2c_smbus_read_word_data(client, ALC5624_VENDOR_ID1);
	if (vid1 < 0) {
		dev_err(&client->dev, "failed to read I2C\n");
		ret = vid1;
		goto err_i2c;
	}
	vid1 = ((vid1 & 0xff) << 8) | (vid1 >> 8);

	vid2 = i2c_smbus_read_word_data(client, ALC5624_VENDOR_ID2);
	if (vid2 < 0) {
		dev_err(&client->dev, "failed to read I2C\n");
		ret = vid2;
		goto err_i2c;
	}
	
	/* Disable clock and regulator */
	clk_disable(alc5624->mclk);
	regulator_disable(alc5624->vdd);

	ver  = (vid2 >> 8) & 0xff;	/* Version */
	vid2 = (vid2 & 0xff); 		/* Device */

	if ((vid1 != 0x10ec) || (vid2 != id->driver_data)) {
		dev_err(&client->dev, "unknown or wrong codec\n");
		dev_err(&client->dev, "Expected %x:%lx, got %x:%x\n",
				0x10ec, id->driver_data,
				vid1, vid2);
		ret = -ENODEV;
		goto err_reg;
	}

	dev_dbg(&client->dev, "Found codec id : alc5624, Version: %d\n", ver);

	alc5624->id = vid2;
	
	/* Store the supply voltages used for amplifiers */
	alc5624->spkvdd_mv = pdata->spkvdd_mv;	/* Speaker Vdd in millivolts */
	alc5624->hpvdd_mv  = pdata->hpvdd_mv;	/* Headphone Vdd in millivolts */

	i2c_set_clientdata(client, alc5624);
	alc5624->control_data = client;
	alc5624->control_type = SND_SOC_I2C;

	/* linux 2.6.38 setup is very streamlined :) */
	ret = snd_soc_register_codec(&client->dev,
		&soc_codec_device_alc5624, &alc5624_dai, 1);
	if (ret != 0) {
		dev_err(&client->dev, "Failed to register codec: %d\n", ret);
		goto err_reg;
	}

	return ret;

err_i2c:
	clk_disable(alc5624->mclk);
	regulator_disable(alc5624->vdd);
err_reg:
	clk_put(alc5624->mclk);
err_mclk:
	regulator_put(alc5624->vdd);

	return ret;
}

static __devexit int alc5624_i2c_remove(struct i2c_client *client)
{
	struct alc5624_priv *alc5624 = i2c_get_clientdata(client);

	snd_soc_unregister_codec(&client->dev);

	/* Release clock and regulator */
	clk_put(alc5624->mclk);
	regulator_put(alc5624->vdd);

	return 0;
}

static const struct i2c_device_id alc5624_i2c_table[] = {
	{"alc5624", 0x20},
	{}
};
MODULE_DEVICE_TABLE(i2c, alc5624_i2c_table);

/*  i2c codec control layer */
static struct i2c_driver alc5624_i2c_driver = {
	.driver = {
		.name = "alc5624-codec",
		.owner = THIS_MODULE,
	},
	.probe = alc5624_i2c_probe,
	.remove =  __devexit_p(alc5624_i2c_remove),
	.id_table = alc5624_i2c_table,
};

static int __init alc5624_modinit(void)
{
	int ret;

	ret = i2c_add_driver(&alc5624_i2c_driver);
	if (ret != 0) {
		pr_err("%s: can't add i2c driver", __func__);
		return ret;
	}

	return ret;
}
module_init(alc5624_modinit);

static void __exit alc5624_modexit(void)
{
	i2c_del_driver(&alc5624_i2c_driver);
}
module_exit(alc5624_modexit);

MODULE_DESCRIPTION("ASoC ALC5624 driver");
MODULE_AUTHOR("Eduardo José Tagle <ejtagle@tutopia.com>");
MODULE_LICENSE("GPL");  

