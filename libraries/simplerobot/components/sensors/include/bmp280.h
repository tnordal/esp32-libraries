#ifndef BMP280_H
#define BMP280_H

#include <stdbool.h>
#include "esp_err.h"

// Default I2C addresses for BMP280
#define BMP280_ADDR_PRIM 0x76
#define BMP280_ADDR_SEC 0x77

/**
 * @brief BMP280 sensor data structure
 */
typedef struct
{
    float temperature; // Temperature in °C
    float pressure;    // Pressure in hPa
} bmp280_data_t;

/**
 * @brief BMP280 calibration coefficients
 */
typedef struct
{
    uint16_t dig_T[3];
    int16_t dig_P[9];
} bmp280_calib_t;

/**
 * @brief Initialize the BMP280 sensor and read calibration data
 *
 * @param i2c_num I2C port number
 * @param addr BMP280 I2C address (BMP280_ADDR_PRIM or BMP280_ADDR_SEC)
 * @param calib Pointer to calibration data structure
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bmp280_init(int i2c_num, uint8_t *addr, bmp280_calib_t *calib);

/**
 * @brief Read temperature and pressure from BMP280
 *
 * @param i2c_num I2C port number
 * @param addr BMP280 I2C address
 * @param calib Pointer to calibration data structure
 * @param data Pointer to data structure for results
 * @return esp_err_t ESP_OK on success
 */
esp_err_t bmp280_read(int i2c_num, uint8_t addr, bmp280_calib_t *calib, bmp280_data_t *data);

#endif /* BMP280_H */