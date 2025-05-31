#ifndef AHT20_H
#define AHT20_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief AHT20 sensor data structure
 */
typedef struct
{
    float temperature; // Temperature in °C
    float humidity;    // Humidity in %
} aht20_data_t;

/**
 * @brief Initialize the AHT20 sensor
 *
 * @param i2c_num I2C port number
 * @param addr AHT20 I2C address (default 0x38)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aht20_init(int i2c_num, uint8_t addr);

/**
 * @brief Read temperature and humidity from AHT20
 *
 * @param i2c_num I2C port number
 * @param addr AHT20 I2C address
 * @param data Pointer to data structure for results
 * @return esp_err_t ESP_OK on success
 */
esp_err_t aht20_read(int i2c_num, uint8_t addr, aht20_data_t *data);

#endif /* AHT20_H */