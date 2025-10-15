#include "wifi_setting.h"


const uint32_t FLASH_TARGET_OFFSET = 0x1F0000;


bool wifisetting_read(wifi_setting_t *setting) {
    const uint8_t *data = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(setting, data, sizeof(wifi_setting_t));
    return memcmp(setting->magic, WIFI_SETTING_MAGIC, sizeof(setting->magic)) == 0;
}

static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

void call_flash_range_program(void *param) {
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}

void wifisetting_write(wifi_setting_t *setting) {
    uint8_t buffer[FLASH_PAGE_SIZE];

    printf("wifisetting_write(\"%s\", \"%s\", \"%s\")\n", setting->ssid, setting->password, setting->http_server);
    memcpy(setting->magic, WIFI_SETTING_MAGIC, sizeof(setting->magic));
    memcpy(buffer, setting, sizeof(wifi_setting_t));

    printf("Calling flash_range_erase() returned with: %d \n", flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX));
    
    uintptr_t params2[] = { FLASH_TARGET_OFFSET, (uintptr_t)buffer};
    printf("Calling flash_range_program() returned with %d \n",flash_safe_execute(call_flash_range_program, params2, UINT32_MAX));

}

bool wifisetting_parse(wifi_setting_t *setting, const uint8_t *buffer, size_t buffer_len) {
    int n = sscanf(buffer,
                   "ssid=%32[^\n]\npassword=%63[^\n]\nhttp_server=%63[^\n]\n",
                   setting->ssid,
                   setting->password,
                    setting->http_server);
    return n == 2;
}

void wifisetting_encode(uint8_t *buffer, wifi_setting_t *setting) {
    sprintf(buffer,
            "ssid=%s\npassword=%s\nhttp_server=%s\n",
            setting->ssid,
            setting->password,
            setting->http_server);
}
