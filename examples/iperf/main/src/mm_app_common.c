/*
 * Copyright 2023 Morse Micro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include "mmosal.h"
#include "mmhal.h"
#include "mmwlan.h"
#include "mmipal.h"
#include "mm_app_common.h"
#include "mm_app_loadconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

/** Maximum number of DNS servers to attempt to retrieve from config store. */
#ifndef DNS_MAX_SERVERS
#define DNS_MAX_SERVERS                 2
#endif

/** XIAO user LED: status once every 10 s (1 blink = connected, 3 blinks = disconnected). */
#if CONFIG_IDF_TARGET_ESP32C6
#define USER_LED_GPIO                   15
#else
#define USER_LED_GPIO                   21
#endif
#define LED_STATUS_INTERVAL_MS          10000
#define LED_BLINK_STEP_MS               200

/** Delay before attempting HaLow reconnection after link down (ms). */
#define HALOW_RECONNECT_DELAY_MS        15000

/** Interval for sampling and printing HaLow RSSI (ms). */
#define RSSI_INTERVAL_MS                15000

/** Last sampled HaLow RSSI (dBm), or INT32_MIN if unknown. Updated every RSSI_INTERVAL_MS. */
static volatile int32_t last_rssi = (int32_t)INT32_MIN;

/** Binary semaphore used to start user_main() once the link comes up. */
static struct mmosal_semb *link_established = NULL;

/** True after first link-up (so we only block app_wlan_start once). */
static volatile bool initial_connect_done = false;

/** True while we are in the process of reconnecting (sta_disable + sta_enable). */
static volatile bool reconnecting = false;

/** One-shot timer to attempt HaLow reconnection after link down. */
static TimerHandle_t reconnect_timer = NULL;

/** 15 s repeating timer: sample HaLow RSSI and print to console. */
static TimerHandle_t rssi_timer = NULL;

/** 10 s repeating timer: triggers one status burst (1 or 3 blinks). */
static TimerHandle_t led_status_timer = NULL;
/** One-shot step timer for each on/off half-cycle of a blink. */
static TimerHandle_t led_step_timer = NULL;
/** Current step in burst (0..led_step_total-1). Even = LED on, odd = LED off. */
static int led_step = 0;
/** Total steps this burst: 2 = 1 blink, 6 = 3 blinks. */
static int led_step_total = 0;
/** True when HaLow link is up (1 blink); false = 3 blinks. */
static volatile bool link_up_for_led = false;

static void led_step_timer_cb(TimerHandle_t t);
static void led_status_timer_cb(TimerHandle_t t);

static void led_step_timer_cb(TimerHandle_t t)
{
    (void)t;
    gpio_set_level(USER_LED_GPIO, (led_step & 1) ? 1 : 0);
    led_step++;
    if (led_step < led_step_total && led_step_timer != NULL) {
        xTimerChangePeriod(led_step_timer, pdMS_TO_TICKS(LED_BLINK_STEP_MS), 0);
        xTimerStart(led_step_timer, 0);
    }
}

static void led_status_timer_cb(TimerHandle_t t)
{
    (void)t;
    led_step = 0;
    led_step_total = link_up_for_led ? 2 : 6;   /* 1 blink or 3 blinks */
    if (led_step_timer != NULL) {
        xTimerChangePeriod(led_step_timer, pdMS_TO_TICKS(LED_BLINK_STEP_MS), 0);
        xTimerStart(led_step_timer, 0);
    }
}

static void halow_led_set_blink(bool link_up)
{
    link_up_for_led = link_up;
}

/** Periodic timer: sample HaLow RSSI, store for web, print to console. */
static void rssi_timer_cb(TimerHandle_t t)
{
    (void)t;
    int32_t r = mmwlan_get_rssi();
    last_rssi = r;
    if (r == (int32_t)INT32_MIN) {
        printf("HaLow RSSI: (no link)\n");
    } else {
        printf("HaLow RSSI: %ld dBm\n", (long)r);
    }
}

int32_t get_halow_rssi(void)
{
    return (int32_t)last_rssi;
}

/**
 * WLAN station status callback, invoked when WLAN STA state changes.
 *
 * @param sta_state  The new STA state.
 */
static void sta_status_callback(enum mmwlan_sta_state sta_state)
{
    switch (sta_state)
    {
    case MMWLAN_STA_DISABLED:
        printf("WLAN STA disabled\n");
        halow_led_set_blink(false);
        break;

    case MMWLAN_STA_CONNECTING:
        printf("WLAN STA connecting\n");
        halow_led_set_blink(false);
        break;

    case MMWLAN_STA_CONNECTED:
        printf("WLAN STA connected\n");
        break;
    }
}

/**
 * Attempt HaLow reconnection: disable STA then re-enable with current config.
 * Called from the reconnect timer (do not call from link_status_callback).
 */
static void do_halow_reconnect(TimerHandle_t t)
{
    (void)t;
    enum mmwlan_status status;
    struct mmwlan_sta_args sta_args = MMWLAN_STA_ARGS_INIT;

    reconnecting = true;

    status = mmwlan_sta_disable();
    if (status != MMWLAN_SUCCESS)
    {
        printf("HaLow reconnect: sta_disable failed (%d), will retry later\n", (int)status);
        reconnecting = false;
        return;
    }

    load_mmwlan_sta_args(&sta_args);
    load_mmwlan_settings();

    printf("HaLow reconnect: attempting to reconnect to %s ...\n", sta_args.ssid);
    status = mmwlan_sta_enable(&sta_args, sta_status_callback);
    reconnecting = false;
    if (status != MMWLAN_SUCCESS)
    {
        printf("HaLow reconnect: sta_enable failed (%d), will retry on next timer\n", (int)status);
        return;
    }
}

/**
 * Link status callback
 *
 * @param link_status   Current link status info.
 */
static void link_status_callback(const struct mmipal_link_status *link_status)
{
    uint32_t time_ms = mmosal_get_time_ms();
    if (link_status->link_state == MMIPAL_LINK_UP)
    {
        halow_led_set_blink(true);
        if (reconnecting)
        {
            printf("HaLow reconnected. Time: %lu ms, ", time_ms);
            reconnecting = false;
        }
        else
        {
            printf("Link is up. Time: %lu ms, ", time_ms);
        }
        printf("IP: %s, ", link_status->ip_addr);
        printf("Netmask: %s, ", link_status->netmask);
        printf("Gateway: %s\n", link_status->gateway);

        if (!initial_connect_done)
        {
            initial_connect_done = true;
            mmosal_semb_give(link_established);
        }
    }
    else
    {
        halow_led_set_blink(false);
        printf("HaLow link down. Time: %lu ms", time_ms);
        if (initial_connect_done && !reconnecting && reconnect_timer != NULL)
        {
            if (xTimerStart(reconnect_timer, 0) == pdPASS)
            {
                printf(" (reconnect in %u s)", (unsigned)(HALOW_RECONNECT_DELAY_MS / 1000));
            }
            printf("\n");
        }
        else
        {
            printf("\n");
        }
    }
}

void app_wlan_init(void)
{
    enum mmwlan_status status;
    struct mmwlan_version version;

    /* Ensure we don't call twice */
    MMOSAL_ASSERT(link_established == NULL);
    link_established = mmosal_semb_create("link_established");

    /* Initialize Morse subsystems, note that they must be called in this order. */
    mmhal_init();
    mmwlan_init();

    mmwlan_set_channel_list(load_channel_list());

    /* Load IP stack settings from config store, or use defaults if no entry found in
     * config store. */
    struct mmipal_init_args mmipal_init_args = MMIPAL_INIT_ARGS_DEFAULT;
    load_mmipal_init_args(&mmipal_init_args);

    /* Initialize IP stack. */
    if (mmipal_init(&mmipal_init_args) != MMIPAL_SUCCESS)
    {
        printf("Error initializing network interface.\n");
        MMOSAL_ASSERT(false);
    }

    mmipal_set_link_status_callback(link_status_callback);

    /* User LED: status once every 10 s — 1 blink = connected, 3 blinks = disconnected */
    gpio_reset_pin(USER_LED_GPIO);
    gpio_set_direction(USER_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(USER_LED_GPIO, 1);
    led_status_timer = xTimerCreate("led_status", pdMS_TO_TICKS(LED_STATUS_INTERVAL_MS),
                                    pdTRUE, NULL, led_status_timer_cb);
    led_step_timer = xTimerCreate("led_step", pdMS_TO_TICKS(LED_BLINK_STEP_MS),
                                 pdFALSE, NULL, led_step_timer_cb);
    if (led_status_timer != NULL) {
        xTimerStart(led_status_timer, 0);
    }

    reconnect_timer = xTimerCreate("halow_reconn", pdMS_TO_TICKS(HALOW_RECONNECT_DELAY_MS),
                                   pdFALSE, NULL, do_halow_reconnect);
    if (reconnect_timer == NULL)
    {
        printf("Warning: could not create HaLow reconnect timer\n");
    }

    rssi_timer = xTimerCreate("rssi", pdMS_TO_TICKS(RSSI_INTERVAL_MS), pdTRUE, NULL, rssi_timer_cb);
    if (rssi_timer != NULL) {
        xTimerStart(rssi_timer, 0);
    }

    status = mmwlan_get_version(&version);
    MMOSAL_ASSERT(status == MMWLAN_SUCCESS);
    printf("Morse firmware version %s, morselib version %s, Morse chip ID 0x%lx\n\n",
           version.morse_fw_version, version.morselib_version, version.morse_chip_id);
}

void app_wlan_start(void)
{
    enum mmwlan_status status;

    /* Load Wi-Fi settings from config store */
    struct mmwlan_sta_args sta_args = MMWLAN_STA_ARGS_INIT;
    load_mmwlan_sta_args(&sta_args);
    load_mmwlan_settings();

    printf("Attempting to connect to %s ", sta_args.ssid);
    if (sta_args.security_type == MMWLAN_SAE)
    {
        printf("with passphrase %s", sta_args.passphrase);
    }
    printf("\n");
    printf("This may take some time (~30 seconds)\n");

    status = mmwlan_sta_enable(&sta_args, sta_status_callback);
    MMOSAL_ASSERT(status == MMWLAN_SUCCESS);

    /* Wait for link status callback.
    * Use a binary semaphore to block us until Link is up.
    */
    mmosal_semb_wait(link_established, UINT32_MAX);

    /* Wi-Fi link is now established, return to caller */
}

void app_wlan_stop(void)
{
    /* Shutdown wlan interface */
    mmwlan_shutdown();
}
