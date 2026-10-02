#include "badge_reminder.h"
#include "badge_network.h"

#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BADGE_NICKNAME "USER-01"
#define SCREEN_TIMEOUT_MS 30000U
#define INK 0xA2FF69
#define DIM 0x57994D
#define BG 0x06120A
#define PANEL 0x122817

extern const lv_font_t badge_font_16;
extern const lv_font_t badge_font_28;
extern const lv_image_dsc_t badge_default_avatar;
extern const lv_image_dsc_t badge_icon_water;
extern const lv_image_dsc_t badge_icon_system;

typedef enum { PAGE_HOME, PAGE_ID, PAGE_WATER, PAGE_SYSTEM } page_t;
typedef struct { bsp_btn_t button; bsp_btn_ev_t event; } key_message_t;

static const char *TAG = "hydration_badge";
#define MENU_COUNT 3
static const char *const MENU[MENU_COUNT] = { "IDENTITY", "WATER", "SYSTEM" };
static QueueHandle_t s_keys;
static badge_reminder_t s_reminder;
static page_t s_page = PAGE_HOME;
static int s_menu_index;
static lv_obj_t *s_screen;
static lv_obj_t *s_title;
static lv_obj_t *s_body;
static lv_obj_t *s_footer;
static lv_obj_t *s_battery;
static lv_obj_t *s_kicker;
static bool s_screen_lit = true;
static uint64_t s_last_input_ms;
static bool s_audio_ready;
static esp_err_t s_network_error;
static uint64_t uptime_ms(void);

static void save_settings(void)
{
    nvs_handle_t handle;
    if (nvs_open("badge", NVS_READWRITE, &handle) != ESP_OK) return;
    (void)nvs_set_u8(handle, "enabled", s_reminder.reminders_enabled ? 1 : 0);
    (void)nvs_commit(handle);
    nvs_close(handle);
}

static void load_settings(void)
{
    nvs_handle_t handle;
    if (nvs_open("badge", NVS_READONLY, &handle) != ESP_OK) return;
    uint8_t enabled = 0;
    if (nvs_get_u8(handle, "enabled", &enabled) == ESP_OK && enabled <= 1)
        badge_reminder_set_enabled(&s_reminder, enabled == 1);
    nvs_close(handle);
}

static uint64_t uptime_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static void key_callback(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (event != BSP_BTN_CLICK && event != BSP_BTN_LONG) return;
    key_message_t message = { button, event };
    if (s_keys) (void)xQueueSend(s_keys, &message, 0);
}

static void label_style(lv_obj_t *label, uint32_t color, const lv_font_t *font)
{
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(label, font, 0);
}

static void add_line(lv_obj_t *parent, int x, int y, int width, int height, uint32_t color)
{
    lv_obj_t *line = lv_obj_create(parent);
    lv_obj_set_pos(line, x, y);
    lv_obj_set_size(line, width, height);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_bg_color(line, lv_color_hex(color), 0);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
}

static void draw_shell(void)
{
    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, 240, 320);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(BG), 0);
    lv_obj_set_style_border_width(s_screen, 0, 0);
    lv_obj_set_style_pad_all(s_screen, 0, 0);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    add_line(s_screen, 12, 38, 216, 2, INK);
    add_line(s_screen, 12, 272, 216, 2, INK);
    add_line(s_screen, 12, 40, 2, 232, DIM);
    add_line(s_screen, 226, 40, 2, 232, DIM);
    add_line(s_screen, 20, 47, 14, 2, DIM);
    add_line(s_screen, 206, 47, 14, 2, DIM);
    add_line(s_screen, 20, 263, 14, 2, DIM);
    add_line(s_screen, 206, 263, 14, 2, DIM);
    s_title = lv_label_create(s_screen);
    label_style(s_title, INK, &badge_font_16);
    lv_obj_set_pos(s_title, 16, 12);
    s_battery = lv_label_create(s_screen);
    label_style(s_battery, INK, &badge_font_16);
    lv_obj_align(s_battery, LV_ALIGN_TOP_RIGHT, -16, 12);
    s_kicker = lv_label_create(s_screen);
    label_style(s_kicker, DIM, &badge_font_16);
    lv_obj_set_pos(s_kicker, 27, 48);
    s_body = lv_label_create(s_screen);
    label_style(s_body, INK, &badge_font_16);
    lv_obj_set_pos(s_body, 27, 77);
    lv_obj_set_width(s_body, 188);
    s_footer = lv_label_create(s_screen);
    label_style(s_footer, DIM, &badge_font_16);
    lv_obj_set_pos(s_footer, 18, 283);
    lv_obj_set_width(s_footer, 207);
    lv_screen_load(s_screen);
}

static void draw_icon_page(const lv_image_dsc_t *icon, const char *heading)
{
    lv_obj_t *image = lv_image_create(s_screen);
    lv_image_set_src(image, icon);
    lv_obj_set_pos(image, 80, 67);
    lv_obj_t *hero = lv_label_create(s_screen);
    label_style(hero, INK, &badge_font_28);
    lv_obj_set_pos(hero, 27, 151);
    lv_label_set_text(hero, heading);
    add_line(s_screen, 27, 184, 185, 1, DIM);
    lv_obj_set_y(s_body, 192);
}

static bool clock_now(struct tm *local)
{
    time_t now = time(NULL);
    // A plausible RTC value may be left by earlier firmware or a reset.
    // Only an SNTP callback in this boot authorizes time-based reminders.
    if (!badge_network_clock_synced() || now < 1735689600) return false;
    return localtime_r(&now, local) != NULL;
}

static void refresh_screen(void)
{
    if (!bsp_lvgl_lock(500)) return;
    lv_obj_t *old_screen = s_screen;
    draw_shell();
    if (old_screen) lv_obj_delete(old_screen);
    int soc = bsp_battery_soc();
    if (soc >= 0) lv_label_set_text_fmt(s_battery, "%d%%", soc);
    else lv_label_set_text(s_battery, "--%");

    switch (s_page) {
    case PAGE_HOME:
        lv_label_set_text(s_title, "STATUS / 00");
        lv_label_set_text(s_kicker, "PERSONAL TERMINAL");
        lv_label_set_text_fmt(s_body,
            "%s\n\n%s %s\n%s %s\n%s %s\n\n> %s",
            BADGE_NICKNAME,
            s_menu_index == 0 ? ">" : " ", MENU[0],
            s_menu_index == 1 ? ">" : " ", MENU[1],
            s_menu_index == 2 ? ">" : " ", MENU[2],
            s_reminder.pending ? "WATER ALERT" :
                (s_reminder.reminders_enabled ? "READY" : "REMINDERS OFF"));
        add_line(s_screen, 27, 100, 185, 1, DIM);
        lv_label_set_text(s_footer, "UP/DOWN SELECT  OK OPEN");
        break;
    case PAGE_ID:
        lv_label_set_text(s_title, "IDENTITY / 01");
        lv_label_set_text(s_kicker, "PERSONNEL RECORD");
        lv_obj_t *avatar = lv_image_create(s_screen);
        lv_image_set_src(avatar, &badge_default_avatar);
        lv_obj_set_pos(avatar, 64, 66);
        add_line(s_screen, 25, 208, 190, 1, DIM);
        lv_obj_set_y(s_body, 212);
        lv_obj_set_style_text_font(s_body, &badge_font_28, 0);
        lv_label_set_text(s_body, "USER-01");
        lv_label_set_text(s_footer, "HOLD OK  BACK");
        break;
    case PAGE_WATER: {
        lv_label_set_text(s_title, "HYDRATION / 02");
        lv_label_set_text(s_kicker, "DAILY REMINDER");
        draw_icon_page(&badge_icon_water, "HYDRATION");
        lv_label_set_text_fmt(s_body,
            "ALL ALERTS %s\nEVERY 30 MIN / 09-21\n\n%s",
            s_reminder.reminders_enabled ? "ON" : "OFF",
            s_reminder.pending ? "DRINK NOW / OK DONE" :
                (s_reminder.reminders_enabled ? "AWAITING NEXT CYCLE" : "ALERTS PAUSED"));
        lv_label_set_text(s_footer, s_reminder.pending ?
            "UP OFF  OK DONE  HOLD BACK" : "OK ALERTS  HOLD BACK");
        break;
    }
    case PAGE_SYSTEM: {
        lv_label_set_text(s_title, "SYSTEM / 03");
        lv_label_set_text(s_kicker, "NETWORK & CLOCK");
        draw_icon_page(&badge_icon_system, "SYSTEM");
        struct tm local = {0};
        if (clock_now(&local))
            lv_label_set_text_fmt(s_body,
                "TIME  %02d:%02d\nWI-FI %s\n%s\nSCREEN OFF 30S",
                local.tm_hour, local.tm_min,
                badge_network_has_ip() ? "ONLINE" : "OFFLINE",
                badge_network_provisioning() ? "BLUFI ACTIVE" : "OK: START BLUFI");
        else lv_label_set_text_fmt(s_body,
            "CLOCK NOT SYNCED\nWI-FI %s\n%s\n%s",
            badge_network_has_ip() ? "ONLINE" : "OFFLINE",
            badge_network_provisioning() ? "BLUFI ACTIVE" : "OK: START BLUFI",
            s_network_error == ESP_OK ? "WAIT FOR NETWORK" : "NETWORK ERROR");
        lv_label_set_text(s_footer, "DOWN CLEAR WIFI  HOLD BACK");
        break;
    }
    }
    bsp_lvgl_unlock();
}

static void alert_tone(void)
{
    esp_err_t err;
    if (!s_audio_ready) {
        err = bsp_audio_init();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Alert audio init failed: %s", esp_err_to_name(err));
            return;
        }
        s_audio_ready = true;
    }
    err = bsp_audio_wake();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Alert audio wake failed: %s", esp_err_to_name(err));
        return;
    }
    bsp_audio_set_volume(80);
    err = bsp_audio_set_format(16000, 16, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Alert audio format failed: %s", esp_err_to_name(err));
        return;
    }
    int16_t pcm[320];
    // Four 500 ms beeps with 200 ms gaps; each PCM frame is 20 ms at 16 kHz.
    for (int beep = 0; beep < 4 && err == ESP_OK; beep++) {
        for (size_t i = 0; i < 320; i++) pcm[i] = (i % 16 < 8) ? 7000 : -7000;
        for (int frame = 0; frame < 25 && err == ESP_OK; frame++) {
            err = bsp_audio_write(pcm, sizeof(pcm));
        }
        if (beep < 3 && err == ESP_OK) {
            memset(pcm, 0, sizeof(pcm));
            for (int frame = 0; frame < 10 && err == ESP_OK; frame++) {
                err = bsp_audio_write(pcm, sizeof(pcm));
            }
        }
    }
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Alert audio write failed: %s", esp_err_to_name(err));
    if (err == ESP_OK) vTaskDelay(pdMS_TO_TICKS(120)); // Drain queued I2S samples.
    err = bsp_audio_sleep();
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Alert audio sleep failed: %s", esp_err_to_name(err));
}

static void handle_key(key_message_t key)
{
    s_last_input_ms = uptime_ms();
    if (!s_screen_lit) {
        s_screen_lit = true;
        bsp_display_backlight(65);
        return;
    }
    if (key.event == BSP_BTN_LONG && key.button == BSP_BTN_OK) {
        if (s_page == PAGE_SYSTEM && badge_network_provisioning())
            (void)badge_network_provision_stop();
        s_page = PAGE_HOME;
    } else if (key.event == BSP_BTN_CLICK) {
        if (s_page == PAGE_HOME) {
            if (key.button == BSP_BTN_UP) s_menu_index = (s_menu_index + MENU_COUNT - 1) % MENU_COUNT;
            if (key.button == BSP_BTN_DOWN) s_menu_index = (s_menu_index + 1) % MENU_COUNT;
            if (key.button == BSP_BTN_OK) s_page = (page_t)(s_menu_index + 1);
        } else if (s_page == PAGE_WATER) {
            if (key.button == BSP_BTN_OK) {
                if (s_reminder.pending) badge_reminder_confirm(&s_reminder, uptime_ms());
                else {
                    badge_reminder_set_enabled(&s_reminder, !s_reminder.reminders_enabled);
                    save_settings();
                }
            } else if (key.button == BSP_BTN_UP && s_reminder.pending) {
                badge_reminder_set_enabled(&s_reminder, false);
                save_settings();
            }
        } else if (s_page == PAGE_SYSTEM) {
            if (key.button == BSP_BTN_OK)
                s_network_error = badge_network_provision_start();
            else if (key.button == BSP_BTN_DOWN)
                s_network_error = badge_network_clear_credentials();
        }
    }
    refresh_screen();
}

static void badge_task(void *arg)
{
    (void)arg;
    key_message_t key;
    unsigned seconds = 0;
    for (;;) {
        if (xQueueReceive(s_keys, &key, pdMS_TO_TICKS(1000)) == pdTRUE) handle_key(key);
        struct tm local = {0};
        bool valid = clock_now(&local);
        bool due = badge_reminder_tick(&s_reminder, valid,
            valid ? (local.tm_year * 1000 + local.tm_yday) : -1,
            local.tm_hour, local.tm_min, uptime_ms());
        if (due) {
            s_page = PAGE_WATER;
            s_screen_lit = true;
            s_last_input_ms = uptime_ms();
            bsp_display_backlight(65);
            refresh_screen();
            alert_tone();
        }
        if (++seconds % 10 == 0 && s_page == PAGE_SYSTEM && s_screen_lit)
            refresh_screen();
        badge_network_maintain();
        if (s_screen_lit && uptime_ms() - s_last_input_ms >= SCREEN_TIMEOUT_MS) {
            bsp_display_backlight(0);
            s_screen_lit = false;
        }
    }
}

void app_main(void)
{
    badge_reminder_init(&s_reminder, true);
    setenv("TZ", "CST-8", 1);
    tzset();
    s_last_input_ms = uptime_ms();
    esp_err_t nvs_error = nvs_flash_init();
    if (nvs_error == ESP_OK) load_settings();
    if (bsp_i2c_init() != ESP_OK || bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "Display initialization failed");
        return;
    }
    (void)bsp_battery_init();
    s_keys = xQueueCreate(12, sizeof(key_message_t));
    if (!s_keys || bsp_button_init(key_callback, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "Button initialization failed");
        return;
    }
    bsp_display_backlight(65);
    refresh_screen();
    s_network_error = nvs_error == ESP_OK ? badge_network_start() : nvs_error;
    if (s_network_error != ESP_OK)
        ESP_LOGE(TAG, "Network initialization failed: %s", esp_err_to_name(s_network_error));
    if (xTaskCreate(badge_task, "badge", 4096, NULL, 5, NULL) != pdPASS)
        ESP_LOGE(TAG, "Badge task creation failed");
}
