/*
 * Copyright 2023 Morse Micro
 * SPDX-License-Identifier: Apache-2.0
 *
 * Web interface to view and change HaLow / 2.4GHz AP settings. Connect to the device's AP
 * and open http://192.168.4.1
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "settings.h"
#include "mm_app_common.h"
#include <stdint.h>

static const char *TAG = "web_config";

#define MAX_FORM_FIELD 64

/** UI strings per language (English, Spanish, Chinese UTF-8). */
struct lang_strings {
    const char *title;
    const char *h1;
    const char *label_lang;
    const char *saved_msg;
    const char *rssi_label;
    const char *refresh_hint;
    const char *label_halow_ssid;
    const char *label_halow_pass;
    const char *label_ap_ssid;
    const char *label_ap_pass;
    const char *label_country;
    const char *label_buffer;
    const char *buffer_opt1;
    const char *buffer_opt2;
    const char *buffer_opt3;
    const char *hint_reboot;
    const char *btn_save;
    const char *btn_reboot;
    const char *rebooting;
    const char *credit_prefix;   /* "Built by " before link to GitHub */
};

static const struct lang_strings lang_str[] = {
    [LANG_EN] = {
        .title         = "HaLow Bridge Settings",
        .h1            = "HaLow Bridge Settings",
        .label_lang    = "Language",
        .saved_msg     = "Settings saved. Reboot to apply changes.",
        .rssi_label    = "HaLow RSSI:",
        .refresh_hint  = "(refreshes every 15 s)",
        .label_halow_ssid = "HaLow AP (STA) SSID",
        .label_halow_pass = "HaLow passphrase",
        .label_ap_ssid   = "2.4 GHz AP SSID",
        .label_ap_pass   = "2.4 GHz AP password",
        .label_country   = "Country code (e.g. US)",
        .label_buffer    = "TCP buffer size",
        .buffer_opt1     = "Small (16 KB)",
        .buffer_opt2     = "Medium (32 KB)",
        .buffer_opt3     = "Large (50 KB)",
        .hint_reboot     = "Buffer size takes effect after reboot.",
        .btn_save        = "Save",
        .btn_reboot      = "Reboot device",
        .rebooting       = "Rebooting...",
        .credit_prefix   = "Built by ",
    },
    [LANG_ES] = {
        .title         = "Configuraci\xc3\xb3n Puente HaLow",
        .h1            = "Configuraci\xc3\xb3n Puente HaLow",
        .label_lang    = "Idioma",
        .saved_msg     = "Configuraci\xc3\xb3n guardada. Reinicie para aplicar cambios.",
        .rssi_label    = "RSSI HaLow:",
        .refresh_hint  = "(se actualiza cada 15 s)",
        .label_halow_ssid = "SSID AP HaLow (STA)",
        .label_halow_pass = "Contrase\xc3\xb1a HaLow",
        .label_ap_ssid   = "SSID AP 2.4 GHz",
        .label_ap_pass   = "Contrase\xc3\xb1a AP 2.4 GHz",
        .label_country   = "C\xc3\xb3digo de pa\xc3\xads (ej. US)",
        .label_buffer    = "Tama\xc3\xb1o de b\xc3\xbafer TCP",
        .buffer_opt1     = "Peque\xc3\xb1o (16 KB)",
        .buffer_opt2     = "Mediano (32 KB)",
        .buffer_opt3     = "Grande (50 KB)",
        .hint_reboot     = "El tama\xc3\xb1o del b\xc3\xbafer tiene efecto tras reiniciar.",
        .btn_save        = "Guardar",
        .btn_reboot      = "Reiniciar dispositivo",
        .rebooting       = "Reiniciando...",
        .credit_prefix   = "Creado por ",
    },
    [LANG_ZH] = {
        .title         = "HaLow \xe6\xa2\x81\xe6\x8e\xa5\xe8\xae\xbe\xe7\xbd\xae",
        .h1            = "HaLow \xe6\xa2\x81\xe6\x8e\xa5\xe8\xae\xbe\xe7\xbd\xae",
        .label_lang    = "\xe8\xaf\xad\xe8\xa8\x80",
        .saved_msg     = "\xe8\xae\xbe\xe7\xbd\xae\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98\xe3\x80\x82\xe9\x87\x8d\xe5\x90\xaf\xe5\x90\x8e\xe7\x94\x9f\xe6\x95\x88\xe3\x80\x82",
        .rssi_label    = "HaLow RSSI\xef\xbc\x9a",
        .refresh_hint  = "\xef\xbc\x88\xe6\xaf\x8f15\xe7\xa7\x92\xe5\x88\xb7\xe6\x96\xb0\xef\xbc\x89",
        .label_halow_ssid = "HaLow AP (STA) SSID",
        .label_halow_pass = "HaLow \xe5\xaf\x86\xe7\xa0\x81",
        .label_ap_ssid   = "2.4 GHz AP SSID",
        .label_ap_pass   = "2.4 GHz AP \xe5\xaf\x86\xe7\xa0\x81",
        .label_country   = "\xe5\x9b\xbd\xe5\xae\xb6\xe4\xbb\xa3\xe7\xa0\x81\xef\xbc\x88\xe5\xa6\x82 US\xef\xbc\x89",
        .label_buffer    = "TCP \xe7\xbc\x93\xe5\x86\xb2\xe5\x8c\xba\xe5\xa4\xa7\xe5\xb0\x8f",
        .buffer_opt1     = "\xe5\xb0\x8f (16 KB)",
        .buffer_opt2     = "\xe4\xb8\xad (32 KB)",
        .buffer_opt3     = "\xe5\xa4\xa7 (50 KB)",
        .hint_reboot     = "\xe7\xbc\x93\xe5\x86\xb2\xe5\x8c\xba\xe5\xa4\xa7\xe5\xb0\x8f\xe9\x87\x8d\xe5\x90\xaf\xe5\x90\x8e\xe7\x94\x9f\xe6\x95\x88\xe3\x80\x82",
        .btn_save        = "\xe4\xbf\x9d\xe5\xad\x98",
        .btn_reboot      = "\xe9\x87\x8d\xe5\x90\xaf\xe8\xae\xbe\xe5\xa4\x87",
        .rebooting       = "\xe6\xad\xa3\xe5\x9c\xa8\xe9\x87\x8d\xe5\x90\xaf...",
        .credit_prefix   = "\xe7\x94\xb1 ",
    },
};

/* Escape string for HTML attribute value (replace & " < >) */
static void html_escape(const char *in, char *out, size_t out_size)
{
    size_t n = 0;
    while (*in && n + 6 < out_size) {
        switch (*in) {
        case '&':  n += snprintf(out + n, out_size - n, "&amp;");  break;
        case '"':  n += snprintf(out + n, out_size - n, "&quot;"); break;
        case '<':  n += snprintf(out + n, out_size - n, "&lt;");   break;
        case '>':  n += snprintf(out + n, out_size - n, "&gt;");   break;
        default:   out[n++] = *in; break;
        }
        in++;
    }
    out[n] = '\0';
}

/* Parse one application/x-www-form-urlencoded field into buf, max len. Returns pointer past this field. */
static const char *parse_form_field(const char *form, const char *name, char *buf, size_t buf_len)
{
    size_t name_len = strlen(name);
    const char *p = form;
    while (*p) {
        if (strncmp(p, name, name_len) == 0 && p[name_len] == '=') {
            p += name_len + 1;
            size_t i = 0;
            while (*p && *p != '&' && i < buf_len - 1) {
                if (*p == '%' && p[1] && p[2]) {
                    int hi = p[1], lo = p[2];
                    buf[i++] = (char)((hi >= 'A' ? (hi & 0x1f) + 9 : hi - '0') * 16 +
                                   (lo >= 'A' ? (lo & 0x1f) + 9 : lo - '0'));
                    p += 3;
                } else if (*p == '+') {
                    buf[i++] = ' ';
                    p++;
                } else {
                    buf[i++] = *p++;
                }
            }
            buf[i] = '\0';
            return p;
        }
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }
    buf[0] = '\0';
    return form;
}

static esp_err_t get_handler(httpd_req_t *req)
{
    bridge_settings_t s;
    settings_load(&s);

    uint8_t lang = s.lang <= LANG_ZH ? s.lang : LANG_EN;
    const struct lang_strings *L = &lang_str[lang];

    char h_ssid[SETTINGS_MAX_SSID * 6], h_pass[SETTINGS_MAX_PASS * 6];
    char a_ssid[SETTINGS_MAX_SSID * 6], a_pass[SETTINGS_MAX_PASS * 6];
    char h_country[SETTINGS_MAX_COUNTRY * 6];
    html_escape(s.halow_ssid, h_ssid, sizeof(h_ssid));
    html_escape(s.halow_pass, h_pass, sizeof(h_pass));
    html_escape(s.ap_ssid, a_ssid, sizeof(a_ssid));
    html_escape(s.ap_pass, a_pass, sizeof(a_pass));
    html_escape(s.country, h_country, sizeof(h_country));

    char saved_buf[256];
    saved_buf[0] = '\0';
    if (httpd_req_get_url_query_str(req, NULL, 0) > 0) {
        char buf[16];
        if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK &&
            strstr(buf, "saved=1") != NULL) {
            snprintf(saved_buf, sizeof(saved_buf), "<p class=\"msg\">%s</p>", L->saved_msg);
        }
    }

    /* Buffer preset: 0/1/2 = top 3 performing configs */
    uint8_t bp = s.buffer_preset <= BUFFER_PRESET_LARGE ? s.buffer_preset : BUFFER_PRESET_2;
    const char *sel_1 = (bp == BUFFER_PRESET_1) ? " selected" : "";
    const char *sel_2 = (bp == BUFFER_PRESET_2) ? " selected" : "";
    const char *sel_3 = (bp == BUFFER_PRESET_3) ? " selected" : "";

    const char *sel_en = (lang == LANG_EN) ? " selected" : "";
    const char *sel_es = (lang == LANG_ES) ? " selected" : "";
    const char *sel_zh = (lang == LANG_ZH) ? " selected" : "";

    /* HaLow RSSI (refreshes every 15 s with page) */
    int32_t rssi = get_halow_rssi();
    char rssi_str[24];
    if (rssi == INT32_MIN) {
        snprintf(rssi_str, sizeof(rssi_str), "\xe2\x80\x94");
    } else {
        snprintf(rssi_str, sizeof(rssi_str), "%ld dBm", (long)rssi);
    }

    char *html;
    size_t html_len = 3200 + strlen(L->title) + strlen(L->h1) + strlen(L->label_lang) + strlen(L->saved_msg) + strlen(L->rssi_label) + strlen(L->refresh_hint)
        + strlen(L->label_halow_ssid) + strlen(L->label_halow_pass) + strlen(L->label_ap_ssid) + strlen(L->label_ap_pass)
        + strlen(L->label_country) + strlen(L->label_buffer) + strlen(L->buffer_opt1) + strlen(L->buffer_opt2) + strlen(L->buffer_opt3)
        + strlen(L->hint_reboot) + strlen(L->btn_save) + strlen(L->btn_reboot) + strlen(L->credit_prefix)
        + strlen(h_ssid) + strlen(h_pass) + strlen(a_ssid) + strlen(a_pass) + strlen(h_country) + strlen(rssi_str) + strlen(saved_buf);
    html = malloc(html_len);
    if (!html) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    snprintf(html, html_len,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<meta http-equiv=\"refresh\" content=\"15\">"
        "<title>%s</title>"
        "<style>"
        "body{font-family:sans-serif;max-width:420px;margin:1em auto;padding:0 1em;}"
        "h1{font-size:1.2em;}"
        "label{display:block;margin-top:0.8em;}"
        "input,select{width:100%%;box-sizing:border-box;padding:0.4em;}"
        ".msg{background:#cfc;padding:0.5em;border-radius:4px;}"
        "button{margin-top:1em;margin-right:0.5em;padding:0.5em 1em;}"
        ".hint{font-size:0.9em;color:#666;}"
        ".credit{font-size:0.85em;color:#888;margin-top:1.5em;}"
        "</style></head><body>"
        "<h1>%s</h1>"
        "<form method=\"post\" action=\"/save\" style=\"display:inline;margin-right:1em;\">"
        "<label style=\"display:inline;\">%s <select name=\"lang\" onchange=\"this.form.submit()\">"
        "<option value=\"0\"%s>English</option>"
        "<option value=\"1\"%s>Espa\xc3\xb1ol</option>"
        "<option value=\"2\"%s>\xe4\xb8\xad\xe6\x96\x87</option>"
        "</select></label></form>"
        "<p><strong>%s</strong> %s <span class=\"hint\">%s</span></p>"
        "%s"
        "<form method=\"post\" action=\"/save\">"
        "<input type=\"hidden\" name=\"lang\" value=\"%d\">"
        "<label>%s <input name=\"halow_ssid\" value=\"%s\" maxlength=\"%d\"></label>"
        "<label>%s <input type=\"password\" name=\"halow_pass\" value=\"%s\" maxlength=\"%d\"></label>"
        "<label>%s <input name=\"ap_ssid\" value=\"%s\" maxlength=\"%d\"></label>"
        "<label>%s <input type=\"password\" name=\"ap_pass\" value=\"%s\" maxlength=\"%d\"></label>"
        "<label>%s <input name=\"country\" value=\"%s\" maxlength=\"%d\" size=\"4\"></label>"
        "<label>%s <select name=\"buffer_preset\">"
        "<option value=\"0\"%s>%s</option>"
        "<option value=\"1\"%s>%s</option>"
        "<option value=\"2\"%s>%s</option>"
        "</select></label>"
        "<p class=\"hint\">%s</p>"
        "<button type=\"submit\">%s</button>"
        "</form>"
        "<a href=\"/reboot\"><button type=\"button\">%s</button></a>"
        "<p class=\"credit\">%s<a href=\"https://github.com/gtgreenw\" target=\"_blank\" rel=\"noopener\">Gavin Greenwood</a></p>"
        "</body></html>",
        L->title,
        L->h1,
        L->label_lang,
        sel_en, sel_es, sel_zh,
        L->rssi_label, rssi_str, L->refresh_hint,
        saved_buf,
        (int)lang,
        L->label_halow_ssid, h_ssid, SETTINGS_MAX_SSID - 1,
        L->label_halow_pass, h_pass, SETTINGS_MAX_PASS - 1,
        L->label_ap_ssid, a_ssid, SETTINGS_MAX_SSID - 1,
        L->label_ap_pass, a_pass, SETTINGS_MAX_PASS - 1,
        L->label_country, h_country, SETTINGS_MAX_COUNTRY - 1,
        L->label_buffer,
        sel_1, L->buffer_opt1, sel_2, L->buffer_opt2, sel_3, L->buffer_opt3,
        L->hint_reboot,
        L->btn_save,
        L->btn_reboot,
        L->credit_prefix
    );

    httpd_resp_set_hdr(req, "Content-Type", "text/html; charset=utf-8");
    httpd_resp_send(req, html, strlen(html));
    free(html);
    return ESP_OK;
}

static esp_err_t save_post_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len > 1024) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
        return ESP_FAIL;
    }
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }
    int r = httpd_req_recv(req, buf, req->content_len);
    if (r <= 0) {
        free(buf);
        return ESP_FAIL;
    }
    buf[r] = '\0';

    bridge_settings_t s;
    settings_load(&s);

    char tmp[MAX_FORM_FIELD];
    parse_form_field(buf, "halow_ssid", tmp, sizeof(tmp));
    if (tmp[0]) strncpy(s.halow_ssid, tmp, SETTINGS_MAX_SSID - 1), s.halow_ssid[SETTINGS_MAX_SSID - 1] = '\0';
    parse_form_field(buf, "halow_pass", tmp, sizeof(tmp));
    if (tmp[0]) strncpy(s.halow_pass, tmp, SETTINGS_MAX_PASS - 1), s.halow_pass[SETTINGS_MAX_PASS - 1] = '\0';
    parse_form_field(buf, "ap_ssid", tmp, sizeof(tmp));
    if (tmp[0]) strncpy(s.ap_ssid, tmp, SETTINGS_MAX_SSID - 1), s.ap_ssid[SETTINGS_MAX_SSID - 1] = '\0';
    parse_form_field(buf, "ap_pass", tmp, sizeof(tmp));
    if (tmp[0]) strncpy(s.ap_pass, tmp, SETTINGS_MAX_PASS - 1), s.ap_pass[SETTINGS_MAX_PASS - 1] = '\0';
    parse_form_field(buf, "country", tmp, sizeof(tmp));
    if (tmp[0]) strncpy(s.country, tmp, SETTINGS_MAX_COUNTRY - 1), s.country[SETTINGS_MAX_COUNTRY - 1] = '\0';
    parse_form_field(buf, "buffer_preset", tmp, sizeof(tmp));
    if (tmp[0]) {
        int v = atoi(tmp);
        if (v >= BUFFER_PRESET_SMALL && v <= BUFFER_PRESET_LARGE) {
            s.buffer_preset = (uint8_t)v;
        }
    }
    parse_form_field(buf, "lang", tmp, sizeof(tmp));
    if (tmp[0]) {
        int v = atoi(tmp);
        if (v >= LANG_EN && v <= LANG_ZH) {
            s.lang = (uint8_t)v;
        }
    }

    free(buf);

    if (!settings_save(&s)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Save failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Settings saved via web");

    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/?saved=1");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t reboot_handler(httpd_req_t *req)
{
    bridge_settings_t s;
    settings_load(&s);
    uint8_t lang = s.lang <= LANG_ZH ? s.lang : LANG_EN;
    const char *msg = lang_str[lang].rebooting;
    size_t len = strlen(msg);

    httpd_resp_set_hdr(req, "Content-Type", "text/plain; charset=utf-8");
    httpd_resp_send(req, msg, len);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

static httpd_uri_t uri_get = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = get_handler,
};

static httpd_uri_t uri_save = {
    .uri       = "/save",
    .method    = HTTP_POST,
    .handler   = save_post_handler,
};

static httpd_uri_t uri_reboot = {
    .uri       = "/reboot",
    .method    = HTTP_GET,
    .handler   = reboot_handler,
};

httpd_handle_t start_web_config_server(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_uri_handlers = 8;
    cfg.stack_size = 4096;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }
    httpd_register_uri_handler(server, &uri_get);
    httpd_register_uri_handler(server, &uri_save);
    httpd_register_uri_handler(server, &uri_reboot);

    ESP_LOGI(TAG, "Web config: http://192.168.4.1");
    return server;
}
