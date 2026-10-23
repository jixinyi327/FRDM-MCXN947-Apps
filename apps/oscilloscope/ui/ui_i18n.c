/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_i18n.h"

static ui_language_t s_language = UI_LANGUAGE_ZH;

static const char *const translations[UI_LANGUAGE_COUNT][UI_TEXT_COUNT] = {
    [UI_LANGUAGE_ZH] = {
        [UI_TEXT_TITLE] = "示波器",
        [UI_TEXT_SAMPLE_RATE] = "采样率 100 kS/s",
        [UI_TEXT_WAVEFORM] = "波形",
        [UI_TEXT_SPECTRUM] = "频谱",
        [UI_TEXT_ACQUIRING] = "采集运行中",
        [UI_TEXT_WAITING] = "等待采样数据",
        [UI_TEXT_CORE_ERROR] = "采样核心初始化失败",
        [UI_TEXT_INPUT_CLIPPED] = "输入超出量程",
        [UI_TEXT_DMA_OVERRUN] = "采样缓冲区溢出",
        [UI_TEXT_LANGUAGE_ENGLISH] = "EN",
        [UI_TEXT_LANGUAGE_CHINESE] = "中文",
        [UI_TEXT_MEASUREMENT_TEMPLATE] = "峰峰值 %u.%02u V | 峰值 %u.%02u kHz",
        [UI_TEXT_SAMPLE_DETAIL_TEMPLATE] = "1024 点 | FFT %lu 周期",
    },
    [UI_LANGUAGE_EN] = {
        [UI_TEXT_TITLE] = "Oscilloscope",
        [UI_TEXT_SAMPLE_RATE] = "Sample rate 100 kS/s",
        [UI_TEXT_WAVEFORM] = "Waveform",
        [UI_TEXT_SPECTRUM] = "Spectrum",
        [UI_TEXT_ACQUIRING] = "Acquisition running",
        [UI_TEXT_WAITING] = "Waiting for samples",
        [UI_TEXT_CORE_ERROR] = "Acquisition core failed",
        [UI_TEXT_INPUT_CLIPPED] = "Input signal clipped",
        [UI_TEXT_DMA_OVERRUN] = "DMA buffer overrun",
        [UI_TEXT_LANGUAGE_ENGLISH] = "EN",
        [UI_TEXT_LANGUAGE_CHINESE] = "中文",
        [UI_TEXT_MEASUREMENT_TEMPLATE] = "Vpp %u.%02u V | Peak %u.%02u kHz",
        [UI_TEXT_SAMPLE_DETAIL_TEMPLATE] = "1024 pts | FFT %lu cycles",
    },
};

void ui_i18n_set_language(ui_language_t language)
{
    if (language >= UI_LANGUAGE_COUNT)
    {
        language = UI_LANGUAGE_ZH;
    }
    s_language = language;
}

ui_language_t ui_i18n_get_language(void)
{
    return s_language;
}

const char *ui_tr(ui_text_id_t text_id)
{
    if (text_id >= UI_TEXT_COUNT)
    {
        return "";
    }
    return translations[s_language][text_id];
}
