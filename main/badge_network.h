#pragma once

#include "esp_err.h"
#include <stdbool.h>

esp_err_t badge_network_start(void);
esp_err_t badge_network_provision_start(void);
esp_err_t badge_network_provision_stop(void);
esp_err_t badge_network_clear_credentials(void);
bool badge_network_has_ip(void);
bool badge_network_provisioning(void);
bool badge_network_clock_synced(void);
void badge_network_maintain(void);
