#include <bmp280.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BMP280";

static float bmp280_calculate_temperature(uint32_t raw_temp, uint16_t *dig_T, int32_t *t_fine)
{
    int32_t var1, var2;
    var1 = ((((raw_temp >> 3) - ((int32_t)dig_T[0] << 1))) * ((int32_t)dig_T[1])) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)dig_T[0])) * ((raw_temp >> 4) - ((int32_t)dig_T[0]))) >> 12) * ((int32_t)dig_T[2])) >> 14;
    *t_fine = var1 + var2;
    return ((*t_fine * 5 + 128) >> 8) / 100.0; // Temperature in °C
}

static float bmp280_calculate_pressure(uint32_t raw_pressure, int16_t *dig_P, int32_t t_fine)
{
    int64_t var1, var2, p;
    uint16_t dig_P1 = (uint16_t)dig_P[0]; // Convert P1 to unsigned

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dig_P[5];
    var2 = var2 + ((var1 * (int64_t)dig_P[4]) << 17);
    var2 = var2 + (((int64_t)dig_P[3]) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P[2]) >> 8) + ((var1 * (int64_t)dig_P[1]) << 12);
    var1 = (((int64_t)1 << 47) + var1) * ((int64_t)dig_P1) >> 33; // Use unsigned dig_P1

    if (var1 == 0)
    {
        return 0; // Avoid division by zero
    }

    p = 1048576 - raw_pressure;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P[8]) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P[7]) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P[6]) << 4);

    return (float)p / 256.0; // Pressure in hPa
}

static esp_err_t bmp280_read_calibration(int i2c_num, uint8_t addr, bmp280_calib_t *calib)
{
    if (calib == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t calib_data[24];  // Buffer to store calibration data
    uint8_t reg_addr = 0x88; // Start address of calibration data

    // Use proper repeated start sequence for I2C read
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd); // Repeated start
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, calib_data, 24, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading calibration data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Parse calibration data
    calib->dig_T[0] = (calib_data[1] << 8) | calib_data[0];            // T1 (unsigned)
    calib->dig_T[1] = (int16_t)((calib_data[3] << 8) | calib_data[2]); // T2 (signed)
    calib->dig_T[2] = (int16_t)((calib_data[5] << 8) | calib_data[4]); // T3 (signed)

    calib->dig_P[0] = (calib_data[7] << 8) | calib_data[6];              // P1 (unsigned)
    calib->dig_P[1] = (int16_t)((calib_data[9] << 8) | calib_data[8]);   // P2 (signed)
    calib->dig_P[2] = (int16_t)((calib_data[11] << 8) | calib_data[10]); // P3 (signed)
    calib->dig_P[3] = (int16_t)((calib_data[13] << 8) | calib_data[12]); // P4 (signed)
    calib->dig_P[4] = (int16_t)((calib_data[15] << 8) | calib_data[14]); // P5 (signed)
    calib->dig_P[5] = (int16_t)((calib_data[17] << 8) | calib_data[16]); // P6 (signed)
    calib->dig_P[6] = (int16_t)((calib_data[19] << 8) | calib_data[18]); // P7 (signed)
    calib->dig_P[7] = (int16_t)((calib_data[21] << 8) | calib_data[20]); // P8 (signed)
    calib->dig_P[8] = (int16_t)((calib_data[23] << 8) | calib_data[22]); // P9 (signed)

    ESP_LOGI(TAG, "BMP280 calibration data read successfully");
    return ESP_OK;
}

esp_err_t bmp280_init(int i2c_num, uint8_t *addr, bmp280_calib_t *calib)
{
    if (addr == NULL || calib == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Try both possible BMP280 addresses
    uint8_t addresses[2] = {BMP280_ADDR_PRIM, BMP280_ADDR_SEC};
    bool sensor_found = false;
    uint8_t chip_id = 0;

    for (int i = 0; i < 2 && !sensor_found; i++)
    {
        // Read chip ID register (0xD0)
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addresses[i] << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, 0xD0, true); // Chip ID register
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (ret != ESP_OK)
        {
            ESP_LOGW(TAG, "Failed to access BMP280 at address 0x%02X: %s", addresses[i], esp_err_to_name(ret));
            continue;
        }

        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addresses[i] << 1) | I2C_MASTER_READ, true);
        i2c_master_read_byte(cmd, &chip_id, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);
        ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK && chip_id == 0x58)
        { // BMP280 chip ID is 0x58
            ESP_LOGI(TAG, "BMP280 found at address 0x%02X with chip ID 0x%02X", addresses[i], chip_id);
            *addr = addresses[i];
            sensor_found = true;
        }
    }

    if (!sensor_found)
    {
        ESP_LOGE(TAG, "BMP280 sensor not found! Check connections and address.");
        return ESP_ERR_NOT_FOUND;
    }

    // Initialize BMP280 using the detected address
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (*addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0xF4, true); // Control register
    i2c_master_write_byte(cmd, 0x27, true); // Normal mode, pressure and temperature oversampling x1
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error initializing BMP280: %s", esp_err_to_name(ret));
        return ret;
    }

    // Read calibration data
    return bmp280_read_calibration(i2c_num, *addr, calib);
}

esp_err_t bmp280_read(int i2c_num, uint8_t addr, bmp280_calib_t *calib, bmp280_data_t *data)
{
    if (calib == NULL || data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Read BMP280 temperature and pressure data
    uint8_t buffer[6];
    uint8_t reg_addr = 0xF7; // Start register for pressure data

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd); // Repeated start
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buffer, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error reading BMP280 data: %s", esp_err_to_name(ret));
        return ret;
    }

    // Process BMP280 data (temperature and pressure)
    uint32_t pressure_raw = ((buffer[0] << 16) | (buffer[1] << 8) | buffer[2]) >> 4;
    uint32_t temperature_raw = ((buffer[3] << 16) | (buffer[4] << 8) | buffer[5]) >> 4;

    int32_t t_fine;
    data->temperature = bmp280_calculate_temperature(temperature_raw, calib->dig_T, &t_fine);
    data->pressure = bmp280_calculate_pressure(pressure_raw, calib->dig_P, t_fine);

    return ESP_OK;
}