/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UI_I18N_H_
#define UI_I18N_H_

typedef enum
{
    UI_LANGUAGE_ZH = 0,
    UI_LANGUAGE_EN,
    UI_LANGUAGE_COUNT
} ui_language_t;

typedef enum
{
    UI_TEXT_TITLE = 0,
    UI_TEXT_SAMPLE_RATE,
    UI_TEXT_WAVEFORM,
    UI_TEXT_SPECTRUM,
    UI_TEXT_ACQUIRING,
    UI_TEXT_WAITING,
    UI_TEXT_CORE_ERROR,
    UI_TEXT_INPUT_CLIPPED,
    UI_TEXT_DMA_OVERRUN,
    UI_TEXT_LANGUAGE_ENGLISH,
    UI_TEXT_LANGUAGE_CHINESE,
    UI_TEXT_MEASUREMENT_TEMPLATE,
    UI_TEXT_SAMPLE_DETAIL_TEMPLATE,
    UI_TEXT_COUNT
} ui_text_id_t;

void ui_i18n_set_language(ui_language_t language);
ui_language_t ui_i18n_get_language(void);
const char *ui_tr(ui_text_id_t text_id);

#endif /* UI_I18N_H_ */
