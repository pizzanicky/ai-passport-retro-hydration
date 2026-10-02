#include "badge_network.h"
#include "badge_blufi_security.h"

#include "esp_blufi.h"
#include "esp_blufi_api.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "host/ble_hs.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

#include <string.h>

#define DEVICE_NAME "BLUFI_Hydration"
#define AP_COUNT 12

static const char *TAG = "badge_network";
static esp_netif_t *s_netif;
static esp_event_handler_instance_t s_wifi_handler;
static esp_event_handler_instance_t s_ip_handler;
static wifi_config_t s_config;
static volatile bool s_ip_ready;
static volatile bool s_ble_connected;
static bool s_wifi_ready;
static bool s_provisioning;
static bool s_profile_ready;
static bool s_gatt_ready;
static bool s_btc_ready;
static SemaphoreHandle_t s_host_stopped;
static bool s_sntp_started;
static volatile bool s_clock_synced;
static bool s_wifi_started;
static unsigned s_retry_count;

bool badge_network_has_ip(void) { return s_ip_ready; }
bool badge_network_provisioning(void) { return s_provisioning; }
bool badge_network_clock_synced(void) { return s_clock_synced; }

static void send_wifi_report(esp_blufi_sta_conn_state_t state)
{
    esp_blufi_extra_info_t info = {0};
    size_t length = strnlen((const char *)s_config.sta.ssid, sizeof(s_config.sta.ssid));
    if (length) {
        info.sta_ssid = s_config.sta.ssid;
        info.sta_ssid_len = length;
    }
    esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, state, 0, &info);
}

static void send_wifi_list(void)
{
    uint16_t count = AP_COUNT;
    wifi_ap_record_t records[AP_COUNT] = {0};
    esp_blufi_ap_record_t list[AP_COUNT] = {0};
    if (esp_wifi_scan_get_ap_records(&count, records) != ESP_OK) {
        esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        return;
    }
    for (uint16_t i = 0; i < count; i++) {
        list[i].rssi = records[i].rssi;
        memcpy(list[i].ssid, records[i].ssid, sizeof(list[i].ssid));
    }
    if (s_ble_connected) esp_blufi_send_wifi_list(count, list);
}

static void wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)data;
    if (id == WIFI_EVENT_STA_START) {
        if (esp_wifi_get_config(WIFI_IF_STA, &s_config) == ESP_OK &&
            s_config.sta.ssid[0]) (void)esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        s_ip_ready = false;
        if (s_ble_connected) send_wifi_report(ESP_BLUFI_STA_CONN_FAIL);
        if (s_wifi_started && s_config.sta.ssid[0] && s_retry_count++ < 5)
            (void)esp_wifi_connect();
    } else if (id == WIFI_EVENT_SCAN_DONE) {
        send_wifi_list();
    }
}

static void time_synced(struct timeval *tv)
{
    (void)tv;
    s_clock_synced = true;
}

static void ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)id; (void)data;
    s_ip_ready = true;
    s_retry_count = 0;
    if (s_ble_connected) send_wifi_report(ESP_BLUFI_STA_CONN_SUCCESS);
    if (!s_sntp_started) {
        esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        esp_sntp_set_time_sync_notification_cb(time_synced);
        esp_sntp_init();
        s_sntp_started = true;
    }
}

esp_err_t badge_network_start(void)
{
    if (s_wifi_ready) return ESP_OK;
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) return err;
    err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    s_netif = esp_netif_create_default_wifi_sta();
    if (!s_netif) return ESP_ERR_NO_MEM;
    wifi_init_config_t wifi = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wifi);
    if (err != ESP_OK) goto fail_netif;
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                              wifi_event, NULL, &s_wifi_handler);
    if (err != ESP_OK) goto fail_wifi;
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                              ip_event, NULL, &s_ip_handler);
    if (err != ESP_OK) goto fail_wifi_handler;
    err = esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    if (err == ESP_OK) err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_OK) err = esp_wifi_start();
    if (err == ESP_OK) {
        s_wifi_ready = true;
        s_wifi_started = true;
        (void)esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    }
    else goto fail_ip_handler;
    return err;

fail_ip_handler:
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, s_ip_handler);
fail_wifi_handler:
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, s_wifi_handler);
fail_wifi:
    esp_wifi_deinit();
fail_netif:
    esp_netif_destroy_default_wifi(s_netif);
    s_netif = NULL;
    return err;
}

void badge_network_maintain(void)
{
    // RTC keeps local time after SNTP. Release the radio between explicit setups.
    if (s_clock_synced && s_wifi_started && !s_provisioning) {
        s_wifi_started = false;
        if (esp_wifi_stop() == ESP_OK) {
            s_ip_ready = false;
            esp_sntp_stop();
            s_sntp_started = false;
        } else {
            s_wifi_started = true;
        }
    }
}

static void blufi_reset(int reason)
{
    ESP_LOGE(TAG, "NimBLE reset: %d", reason);
}

static void blufi_sync(void)
{
    if (esp_blufi_profile_init() == 0) s_profile_ready = true;
}

static void host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    if (s_host_stopped) xSemaphoreGive(s_host_stopped);
    nimble_port_freertos_deinit();
}

static void blufi_event(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    switch (event) {
    case ESP_BLUFI_EVENT_INIT_FINISH:
        esp_blufi_adv_start_with_name(DEVICE_NAME);
        break;
    case ESP_BLUFI_EVENT_BLE_CONNECT:
        s_ble_connected = true;
        esp_blufi_adv_stop();
        if (demo_blufi_security_init() != 0)
            esp_blufi_send_error_info(ESP_BLUFI_INIT_SECURITY_ERROR);
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        s_ble_connected = false;
        demo_blufi_security_deinit();
        esp_blufi_adv_start_with_name(DEVICE_NAME);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_SSID:
        if (param->sta_ssid.ssid_len >= sizeof(s_config.sta.ssid)) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            break;
        }
        memset(&s_config, 0, sizeof(s_config));
        memcpy(s_config.sta.ssid, param->sta_ssid.ssid, param->sta_ssid.ssid_len);
        (void)esp_wifi_set_config(WIFI_IF_STA, &s_config);
        break;
    case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        if (param->sta_passwd.passwd_len >= sizeof(s_config.sta.password)) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            break;
        }
        memset(s_config.sta.password, 0, sizeof(s_config.sta.password));
        memcpy(s_config.sta.password, param->sta_passwd.passwd,
               param->sta_passwd.passwd_len);
        (void)esp_wifi_set_config(WIFI_IF_STA, &s_config);
        break;
    case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
        (void)esp_wifi_disconnect();
        (void)esp_wifi_connect();
        break;
    case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
        (void)esp_wifi_disconnect();
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
        send_wifi_report(s_ip_ready ? ESP_BLUFI_STA_CONN_SUCCESS :
                         ESP_BLUFI_STA_CONN_FAIL);
        break;
    case ESP_BLUFI_EVENT_GET_WIFI_LIST: {
        wifi_scan_config_t scan = {0};
        if (esp_wifi_scan_start(&scan, false) != ESP_OK)
            esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        break;
    }
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        esp_blufi_disconnect();
        break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
        esp_blufi_send_error_info(param->report_error.state);
        break;
    default:
        break;
    }
}

static esp_blufi_callbacks_t s_callbacks = {
    .event_cb = blufi_event,
    .negotiate_data_handler = demo_blufi_negotiate,
    .encrypt_func = demo_blufi_encrypt,
    .decrypt_func = demo_blufi_decrypt,
    .checksum_func = demo_blufi_checksum,
};

esp_err_t badge_network_provision_start(void)
{
    if (s_provisioning) return ESP_OK;
    esp_err_t err = badge_network_start();
    if (err != ESP_OK) return err;
    if (!s_wifi_started) {
        err = esp_wifi_start();
        if (err != ESP_OK) return err;
        s_wifi_started = true;
        (void)esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    }
    err = esp_blufi_register_callbacks(&s_callbacks);
    if (err != ESP_OK) return err;
    err = nimble_port_init();
    if (err != ESP_OK) return err;
    s_host_stopped = xSemaphoreCreateBinary();
    if (!s_host_stopped) return ESP_ERR_NO_MEM;
    ble_hs_cfg.reset_cb = blufi_reset;
    ble_hs_cfg.sync_cb = blufi_sync;
    ble_hs_cfg.gatts_register_cb = esp_blufi_gatt_svr_register_cb;
    if (esp_blufi_gatt_svr_init() != 0) return ESP_FAIL;
    s_gatt_ready = true;
    if (ble_svc_gap_device_name_set(DEVICE_NAME) != 0) return ESP_FAIL;
    esp_blufi_btc_init();
    s_btc_ready = true;
    err = esp_nimble_enable(host_task);
    if (err == ESP_OK) s_provisioning = true;
    return err;
}

esp_err_t badge_network_provision_stop(void)
{
    if (!s_provisioning) return ESP_OK;
    if (s_ble_connected) esp_blufi_disconnect();
    demo_blufi_security_deinit();
    if (s_profile_ready) esp_blufi_adv_stop();
    if (s_gatt_ready) {
        esp_blufi_gatt_svr_deinit();
        s_gatt_ready = false;
    }
    int rc = nimble_port_stop();
    if (rc != 0 || !s_host_stopped ||
        xSemaphoreTake(s_host_stopped, pdMS_TO_TICKS(3000)) != pdTRUE) {
        ESP_LOGE(TAG, "BLUFI stop failed: %d", rc);
        return ESP_ERR_TIMEOUT;
    }
    nimble_port_deinit();
    if (s_profile_ready) {
        esp_blufi_profile_deinit();
        s_profile_ready = false;
    }
    if (s_btc_ready) {
        esp_blufi_btc_deinit();
        s_btc_ready = false;
    }
    vSemaphoreDelete(s_host_stopped);
    s_host_stopped = NULL;
    s_provisioning = false;
    s_ble_connected = false;
    return ESP_OK;
}

esp_err_t badge_network_clear_credentials(void)
{
    if (!s_wifi_ready) return ESP_ERR_INVALID_STATE;
    wifi_config_t empty = {0};
    memset(&s_config, 0, sizeof(s_config));
    s_ip_ready = false;
    s_retry_count = 0;
    (void)esp_wifi_disconnect();
    return esp_wifi_set_config(WIFI_IF_STA, &empty);
}
