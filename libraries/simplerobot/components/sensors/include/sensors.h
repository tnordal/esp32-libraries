#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include "esp_err.h"
#include <aht20.h>
#include <bmp280.h>

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 21
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 22
#endif
#ifndef I2C_FREQ_HZ
#define I2C_FREQ_HZ 100000
#endif

/**
 * @brief Initialize I2C for sensor communication
 *
 * @param sda_gpio GPIO number for SDA
 * @param scl_gpio GPIO number for SCL
 * @param freq_hz I2C frequency in Hz
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensors_i2c_init(int sda_gpio, int scl_gpio, uint32_t freq_hz);

/**
 * @brief Scan the I2C bus for devices
 *
 * @return int Number of devices found
 */
int sensors_i2c_scan(void);

/**
 * @brief Structure to hold all sensor data
 */
typedef struct
{
    aht20_data_t aht20;
    bmp280_data_t bmp280;
} sensors_data_t;

/**
 * @brief Global variable to access sensor data
 */
extern sensors_data_t sensors;

/**
 * @brief Initialize all sensors (I2C, AHT20, BMP280)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensors_init(void);
esp_err_t sensors_init_custom(int sda_gpio, int scl_gpio, uint32_t freq_hz);

/**
 * @brief Elegant single API for default or custom I2C config
 */
esp_err_t sensors_init_impl(int sda_gpio, int scl_gpio, uint32_t freq_hz);
static inline esp_err_t sensors_init_default(void) { return sensors_init_impl(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ); }
#define SENSORS_INIT_MACRO_CHOOSER(_1, _2, _3, NAME, ...) NAME
#define sensors_init(...) SENSORS_INIT_MACRO_CHOOSER(__VA_ARGS__, sensors_init_impl, sensors_init_default)(__VA_ARGS__)

/**
 * @brief Update all sensor readings and store in global struct
 * @return esp_err_t ESP_OK on success
 */
esp_err_t sensors_update(void);

#endif /* SENSORS_H */