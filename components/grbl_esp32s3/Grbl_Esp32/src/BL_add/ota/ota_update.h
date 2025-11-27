#include "esp_ota_ops.h"
#include "esp_log.h"
#include "esp_check.h"
#include "string.h"

esp_err_t OTA_update_init(esp_ota_handle_t *update_handle, const esp_partition_t **update_partition);
void OTA_update_end(esp_ota_handle_t update_handle, const esp_partition_t *update_partition);
esp_err_t OTA_set_reboot_partition(char *label);
char *getCurrentVersion();
void check_ota_update();