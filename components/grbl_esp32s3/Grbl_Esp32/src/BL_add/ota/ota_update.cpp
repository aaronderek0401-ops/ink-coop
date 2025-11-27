#include "ota_update.h"

static const char *TAG = "OTA_update";

static char appVersion[16];

esp_err_t OTA_update_init(esp_ota_handle_t *update_handle, const esp_partition_t **update_partition) {
    esp_err_t err;
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08"PRIx32", but running from offset 0x%08"PRIx32,
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition name:[%s] type %d subtype %d (offset 0x%08"PRIx32")",
             running->label, running->type, running->subtype, running->address);
    *update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition name:[%s] subtype %d at offset 0x%"PRIx32,
             (*update_partition)->label, (*update_partition)->subtype, (*update_partition)->address);

    err = esp_ota_begin(*update_partition, OTA_WITH_SEQUENTIAL_WRITES, update_handle);
    return err;
}

void OTA_update_end(esp_ota_handle_t update_handle, const esp_partition_t *update_partition) {
    esp_err_t err;
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_EARLY_LOGE(TAG, "Image validation failed, image is corrupted");
        } else {
            ESP_EARLY_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        }
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_EARLY_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
    }
}

esp_err_t OTA_set_reboot_partition(char *label) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!strcmp(running->label, label)) {
        ESP_LOGE(TAG, "Running partition name:[%s], Same running partition", running->label);
        return ESP_ERR_INVALID_ARG;
    }

    if (*label == '\0') {
        printf("current running partition is [%s]\n", running->label);
        return ESP_OK;
    }

    const esp_partition_t *find_parition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label);
    if (find_parition == NULL) {
        ESP_LOGE(TAG, "partition label:%s not found", label);
        return ESP_ERR_NOT_FOUND;
    }

    if (esp_ota_set_boot_partition(find_parition) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed");
        return ESP_FAIL;
    }

    return ESP_OK;    
}

static bool verifyOta() {
    // 示例验证逻辑，实际应用中应实现具体的验证
    // 可以检查是否过重启，如果程序有问题重启回溯版本。只检查升级之后的第一次
    return true;  // 假设验证通过，返回 true；验证失败返回 false
}

void check_ota_update() {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    ESP_LOGI(TAG, "Running partition name:[%s] type %d subtype %d (offset 0x%08"PRIx32")",
             running->label, running->type, running->subtype, running->address);
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Next partition name:[%s] type %d subtype %d at offset 0x%"PRIx32,
             update_partition->label, update_partition->type, update_partition->subtype, update_partition->address);

    esp_err_t err = esp_ota_get_state_partition(running, &ota_state);
    if (err != ESP_OK) {
        ESP_LOGE("OTA", "Failed to get OTA partition state: %s", esp_err_to_name(err));
        return;
    }
    switch (ota_state) {
        case ESP_OTA_IMG_PENDING_VERIFY:
            // 如果是待验证状态，则执行验证逻辑
            if (verifyOta()) {
                // 验证通过，标记应用有效并取消回滚
                esp_ota_mark_app_valid_cancel_rollback();
                ESP_LOGI(TAG, "OTA verification succeeded, marked app as valid.");
            } else {
                // 验证失败，标记应用无效并回滚
                ESP_LOGE(TAG, "OTA verification failed! Starting rollback to the previous version ...");
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
            break;
        case ESP_OTA_IMG_VALID:
            ESP_LOGI(TAG, "Current app is already verified and valid.");
            break;
        case ESP_OTA_IMG_INVALID:
            ESP_LOGE(TAG, "Current app is invalid.");
            break;
        case ESP_OTA_IMG_ABORTED:
            ESP_LOGE(TAG, "Current app verification was aborted.");
            break;
        case ESP_OTA_IMG_NEW:
        case ESP_OTA_IMG_UNDEFINED:
        default:
            ESP_LOGW(TAG, "Current app is in an undefined state.");
            break;
    }

    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        strcpy(appVersion, running_app_info.version);
        printf("Running firmware version: %s\n", running_app_info.version);
    } else {
        printf("Failed to get running firmware version\n");
    }
}

char *getCurrentVersion() {
    return appVersion;
}