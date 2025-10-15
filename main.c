#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "pico/cyw43_arch.h"

#include <tusb.h>
#include "wifi_setting.h"

TaskHandle_t usbHandle;
TaskHandle_t writeHandle;

wifi_setting_t global_wifi;

// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
static void vUSBTask() {

    usbHandle = xTaskGetCurrentTaskHandle();

    // RTOS forever loop
    for (;;) {
        // put this thread to waiting state until there is new events
        tud_task();

        // following code only run if tud_task() process at least 1 event
        tud_cdc_write_flush();
    }
}

void vWriteTask(){
    writeHandle = xTaskGetCurrentTaskHandle();
    for (;;){
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
        printf("Update WIFI.TXT (%s, %s, %s) \n", global_wifi.ssid, global_wifi.password, global_wifi.http_server);
        taskENTER_CRITICAL();
        wifisetting_write(&global_wifi);
        taskEXIT_CRITICAL();
    }
}

void connect_wifi(){
    printf("Connecting to Wi-Fi...\n");
    wifi_setting_t wifi_setting;
    if (!wifisetting_read(&wifi_setting)) {
        // init wifi setting
        strncpy(wifi_setting.ssid, "SET_SSID", sizeof(wifi_setting.ssid));
        strncpy(wifi_setting.password, "SET_PASSWORD", sizeof(wifi_setting.password));
        strncpy(wifi_setting.http_server, "SET_HTTP_SERVER", sizeof(wifi_setting.http_server));
        wifisetting_write(&wifi_setting);
    } else {
        int attempts = 0;
        printf("Read SSID %s with password %s \n", wifi_setting.ssid, wifi_setting.password);
        printf("Read HTTP server %s\n", wifi_setting.http_server); 
        
        //Connect to WiFi. Stops trying to reconnect after 5 failed attempts
        /* while (cyw43_arch_wifi_connect_timeout_ms(wifi_setting.ssid, wifi_setting.password, CYW43_AUTH_WPA2_AES_PSK, 5000) && attempts < 3) {
            printf("Failed to connect to Wi-Fi. Retrying...\n");
            attempts++;
        } */
    }

}


void main() {
    board_init();
    tud_init(BOARD_TUD_RHPORT);

    stdio_init_all();
    if (cyw43_arch_init()) {
        return;
    }
    cyw43_arch_enable_sta_mode();
    connect_wifi();
    int status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
    if (status == CYW43_LINK_UP){
        printf("Connected to Wi-Fi!\n");
    } else{
         printf("Failed to connect to Wi-Fi!\n");
    }

    
    xTaskCreate(vUSBTask, "USB Task", 1024, NULL, 1, NULL);
    xTaskCreate(vWriteTask, "Write Task", 1024, NULL, 1, NULL);
    vTaskStartScheduler();
}
