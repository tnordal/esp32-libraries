#include <aht20.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "AHT20";

esp_err_t aht20_init(int i2c_num, uint8_t addr)
{
    // Send initialization command to AHT20
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0xBE, true); // Initialization command
    i2c_master_write_byte(cmd, 0x08, true); // Calibration command
    i2c_master_write_byte(cmd, 0x00, true); // Reserved byte
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error initializing AHT20: %s", esp_err_to_name(ret));
        return ret;
    }

    // Wait for calibration to complete
    vTaskDelay(pdMS_TO_TICKS(500));
    return ESP_OK;
}

esp_err_t aht20_read(int i2c_num, uint8_t addr, aht20_data_t *data)
{
    if (data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Trigger measurement
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0xAC, true); // Trigger measurement
    i2c_master_write_byte(cmd, 0x33, true); // Data byte 1
    i2c_master_write_byte(cmd, 0x00, true); // Data byte 2
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error triggering AHT20 measurement: %s", esp_err_to_name(ret));
        return ret;
    }

    // Wait for measurement to complete
    vTaskDelay(pdMS_TO_TICKS(100));

    // Read measurement data
    uint8_t buffer[6];
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buffer, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading AHT20 data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Process AHT20 data (temperature and humidity)
    uint32_t humidity = ((buffer[1] << 16) | (buffer[2] << 8) | buffer[3]) >> 4;
    uint32_t temperature = ((buffer[3] & 0x0F) << 16) | (buffer[4] << 8) | buffer[5];

    data->humidity = (humidity * 100.0) / 1048576.0;
    data->temperature = (temperature * 200.0 / 1048576.0) - 50;

    return ESP_OK;
}