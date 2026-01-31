/*
 * Copyright 2023 Morse Micro
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define NVS_KEY_HALOW_SSID   "h_ssid"
#define NVS_KEY_HALOW_PASS   "h_pass"
#define NVS_KEY_AP_SSID      "ap_ssid"
#define NVS_KEY_AP_PASS      "ap_pass"
#define NVS_KEY_COUNTRY      "country"
#define NVS_KEY_BUFFER       "buffer"
#define NVS_KEY_LANG         "lang"

#define DEFAULT_HALOW_SSID   "Halow1"
#define DEFAULT_HALOW_PASS   "letmein111"
#define DEFAULT_AP_SSID      "XIAO_S3_HALOW"
#define DEFAULT_AP_PASS      "letmein111"
#define DEFAULT_COUNTRY      "US"
#define DEFAULT_BUFFER_PRESET BUFFER_PRESET_LARGE
#define DEFAULT_LANG          LANG_EN

static void strncpy_default(char *dst, const char *default_val, size_t n)
{
    if (n == 0) return;
    strncpy(dst, default_val, n - 1);
    dst[n - 1] = '\0';
}

void settings_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

void settings_load(bridge_settings_t *out)
{
    strncpy_default(out->halow_ssid, DEFAULT_HALOW_SSID, SETTINGS_MAX_SSID);
    strncpy_default(out->halow_pass, DEFAULT_HALOW_PASS, SETTINGS_MAX_PASS);
    strncpy_default(out->ap_ssid, DEFAULT_AP_SSID, SETTINGS_MAX_SSID);
    strncpy_default(out->ap_pass, DEFAULT_AP_PASS, SETTINGS_MAX_PASS);
    strncpy_default(out->country, DEFAULT_COUNTRY, SETTINGS_MAX_COUNTRY);
    out->buffer_preset = DEFAULT_BUFFER_PRESET;
    out->lang = DEFAULT_LANG;

    nvs_handle_t h;
    if (nvs_open(SETTINGS_NS, NVS_READONLY, &h) != ESP_OK) {
        return;
    }

    size_t len = SETTINGS_MAX_SSID;
    nvs_get_str(h, NVS_KEY_HALOW_SSID, out->halow_ssid, &len);
    len = SETTINGS_MAX_PASS;
    nvs_get_str(h, NVS_KEY_HALOW_PASS, out->halow_pass, &len);
    len = SETTINGS_MAX_SSID;
    nvs_get_str(h, NVS_KEY_AP_SSID, out->ap_ssid, &len);
    len = SETTINGS_MAX_PASS;
    nvs_get_str(h, NVS_KEY_AP_PASS, out->ap_pass, &len);
    len = SETTINGS_MAX_COUNTRY;
    nvs_get_str(h, NVS_KEY_COUNTRY, out->country, &len);
    uint8_t bp;
    if (nvs_get_u8(h, NVS_KEY_BUFFER, &bp) == ESP_OK && bp <= BUFFER_PRESET_3) {
        out->buffer_preset = bp;
    }
    uint8_t lang;
    if (nvs_get_u8(h, NVS_KEY_LANG, &lang) == ESP_OK && lang <= LANG_ZH) {
        out->lang = lang;
    }

    nvs_close(h);
}

bool settings_save(const bridge_settings_t *s)
{
    nvs_handle_t h;
    if (nvs_open(SETTINGS_NS, NVS_READWRITE, &h) != ESP_OK) {
        return false;
    }
    nvs_set_str(h, NVS_KEY_HALOW_SSID, s->halow_ssid);
    nvs_set_str(h, NVS_KEY_HALOW_PASS, s->halow_pass);
    nvs_set_str(h, NVS_KEY_AP_SSID, s->ap_ssid);
    nvs_set_str(h, NVS_KEY_AP_PASS, s->ap_pass);
    nvs_set_str(h, NVS_KEY_COUNTRY, s->country);
    nvs_set_u8(h, NVS_KEY_BUFFER, s->buffer_preset <= BUFFER_PRESET_3 ? s->buffer_preset : DEFAULT_BUFFER_PRESET);
    nvs_set_u8(h, NVS_KEY_LANG, s->lang <= LANG_ZH ? s->lang : DEFAULT_LANG);
    esp_err_t err = nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}
