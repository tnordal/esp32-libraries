# ESP-IDF Sensors Library

A library for interacting with AHT20 (temperature/humidity) and BMP280 (temperature/pressure) sensors using ESP-IDF.

## Features

- Easy I2C initialization and device scanning
- AHT20 temperature and humidity sensing
- BMP280 temperature and pressure sensing
- Automatic detection of BMP280 I2C address
- Global sensor state handling for simplified API
- FreeRTOS task example for periodic sensor readings

## Installation

1. Copy the `components/sensors` directory to your ESP-IDF project's `components` directory
2. Include the required header in your project: `#include "sensors.h"`

## Usage Example

```c
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
}void app_main(void)
{
    // Initialize all sensors (I2C, AHT20, BMP280) with default configuration
    esp_err_t ret = sensors_init_default();
    // Alternatively, you can specify custom I2C pins and frequency:
    // esp_err_t ret = sensors_init(21, 22, 100000);
    
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize sensors: %s", esp_err_to_name(ret));
        return;
    }

    // Create one task for displaying all sensor data
    xTaskCreate(sensors_display_task, "Sensors Display Task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Application started");
}
```

## Simplified API

This library provides a simplified API for working with sensors:

1. `sensors_init_default()` - Initialize with default I2C pins and frequency
2. `sensors_init(sda, scl, freq)` - Initialize with custom I2C configuration
3. `sensors_update()` - Read all sensor data at once
4. `sensors` - Global structure containing all sensor readings:
   - `sensors.aht20.temperature` - AHT20 temperature in °C
   - `sensors.aht20.humidity` - AHT20 humidity in %
   - `sensors.bmp280.temperature` - BMP280 temperature in °C
   - `sensors.bmp280.pressure` - BMP280 pressure in hPa
