#include <sensors.h>
#include <aht20.h>
#include <bmp280.h>
#include "driver/i2c.h"
#include "esp_log.h"

static const char *TAG = "SENSORS";
static bool i2c_initialized = false;
static int i2c_master_port = 0;

sensors_data_t sensors; // Global sensor data

static uint8_t aht20_addr = 0x38;
static uint8_t bmp280_addr = 0;
static bmp280_calib_t bmp280_calib;

esp_err_t sensors_i2c_init(int sda_gpio, int scl_gpio, uint32_t freq_hz)
{
    if (i2c_initialized)
    {
        return ESP_OK; // Already initialized
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_gpio,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_gpio,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = freq_hz,
    };

    esp_err_t ret = i2c_param_config(i2c_master_port, &conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C parameter configuration error: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C driver installation error: %s", esp_err_to_name(ret));
        return ret;
    }

    i2c_initialized = true;
    return ESP_OK;
}

int sensors_i2c_scan(void)
{
    if (!i2c_initialized)
    {
        ESP_LOGE(TAG, "I2C not initialized");
        return -1;
    }

    ESP_LOGI(TAG, "Scanning I2C bus...");

    uint8_t count = 0;
    for (uint8_t i = 1; i < 127; i++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(i2c_master_port, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK)
        {
            ESP_LOGI(TAG, "Device found at address 0x%02X", i);
            count++;
        }
    }

    if (count == 0)
    {
        ESP_LOGI(TAG, "No devices found on I2C bus");
    }
    else
    {
        ESP_LOGI(TAG, "Found %d devices on I2C bus", count);
    }

    return count;
}

esp_err_t sensors_init_impl(int sda_gpio, int scl_gpio, uint32_t freq_hz)
{
    esp_err_t ret;
    // Init I2C with given pins/frequency
    ret = sensors_i2c_init(sda_gpio, scl_gpio, freq_hz);
    if (ret != ESP_OK)
        return ret;
    // Init AHT20
    ret = aht20_init(0, aht20_addr);
    if (ret != ESP_OK)
        return ret;
    // Init BMP280
    ret = bmp280_init(0, &bmp280_addr, &bmp280_calib);
    if (ret != ESP_OK)
        return ret;
    return ESP_OK;
}

esp_err_t sensors_update(void)
{
    esp_err_t ret = ESP_OK;
    // Read AHT20
    if (aht20_read(0, aht20_addr, &sensors.aht20) != ESP_OK)
        ret = ESP_FAIL;
    // Read BMP280
    if (bmp280_read(0, bmp280_addr, &bmp280_calib, &sensors.bmp280) != ESP_OK)
        ret = ESP_FAIL;
    return ret;
}