#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensors.h"
#include "esp_log.h"

static const char *TAG = "SENSORS_EXAMPLE";

void sensors_display_task(void *pvParameter)
{
    while (1)
    {
        if (sensors_update() == ESP_OK)
        {
            printf("AHT20 - Temperature: %.2f°C, Humidity: %.2f%%\n",
                   sensors.aht20.temperature, sensors.aht20.humidity);
            printf("BMP280 - Temperature: %.2f°C, Pressure: %.2f hPa\n",
                   sensors.bmp280.temperature, sensors.bmp280.pressure);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to update sensor data");
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); // Update every 5 seconds
    }
}

void app_main(void)
{
    // Initialize all sensors (I2C, AHT20, BMP280)
    esp_err_t ret = sensors_init_default();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize sensors: %s", esp_err_to_name(ret));
        return;
    }

    // Create one task for displaying all sensor data
    xTaskCreate(sensors_display_task, "Sensors Display Task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Application started");
}