/*
 * Copyright 2023 Morse Micro
 * SPDX-License-Identifier: Apache-2.0
 *
 * NVS-backed settings for HaLow STA and 2.4GHz AP. Used by loadconfig, nat_router, and web UI.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#define SETTINGS_NS           "bridge"
#define SETTINGS_MAX_SSID     32
#define SETTINGS_MAX_PASS     64
#define SETTINGS_MAX_COUNTRY  4

/** TCP buffer preset: 0=small, 1=medium, 2=large. Stored in NVS; apply via matching sdkconfig build. */
#define BUFFER_PRESET_SMALL   0   /* 16 KB */
#define BUFFER_PRESET_MEDIUM  1   /* 32 KB */
#define BUFFER_PRESET_LARGE   2   /* 50 KB */
#define BUFFER_PRESET_1      BUFFER_PRESET_SMALL
#define BUFFER_PRESET_2      BUFFER_PRESET_MEDIUM
#define BUFFER_PRESET_3      BUFFER_PRESET_LARGE

/** Web UI language: 0=English, 1=Spanish, 2=Chinese. */
#define LANG_EN              0
#define LANG_ES              1
#define LANG_ZH              2

typedef struct {
    char halow_ssid[SETTINGS_MAX_SSID];
    char halow_pass[SETTINGS_MAX_PASS];
    char ap_ssid[SETTINGS_MAX_SSID];
    char ap_pass[SETTINGS_MAX_PASS];
    char country[SETTINGS_MAX_COUNTRY];
    uint8_t buffer_preset;   /* BUFFER_PRESET_* */
    uint8_t lang;            /* LANG_* */
} bridge_settings_t;

/**
 * Load settings from NVS. If a key is missing, the corresponding field is filled with the default.
 * Defaults: Halow1 / letmein111, XIAO_S3_HALOW / letmein111, US.
 */
void settings_load(bridge_settings_t *out);

/**
 * Save settings to NVS. Returns true on success.
 */
bool settings_save(const bridge_settings_t *s);

/**
 * Initialize NVS partition for settings (call once at boot before any load/save).
 */
void settings_init(void);

#endif /* SETTINGS_H */
