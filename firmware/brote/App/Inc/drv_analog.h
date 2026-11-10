/**
 * @file drv_analog.h
 * @brief Entradas analógicas por ADC1: humedad de suelo (PC0, IN10) y LDR (PC1, IN11).
 *
 * Cada lectura promedia ANALOG_NUM_SAMPLES conversiones y la convierte a 0..100 %
 * con la calibración de app_config.h. ADC1 tiene un único dueño (tSensors), por
 * eso no necesita mutex.
 */
#ifndef DRV_ANALOG_H
#define DRV_ANALOG_H

#include <stdint.h>

typedef enum {
    ANALOG_OK = 0,
    ANALOG_ERR_ADC,             /* HAL_ADC devolvió error o timeout */
    ANALOG_ERR_DISCONNECTED     /* crudo < SOIL_RAW_MIN: el pull-down llevó el pin a 0 */
} analog_status_t;

/**
 * @brief Humedad del suelo.
 * @param[out] soil_pct 0..100 (0 = seco en aire, 100 = en agua); -1 si no es válida.
 * @param[out] raw      promedio crudo de 12 bits, para calibrar; puede ser NULL.
 */
analog_status_t analog_read_soil(int16_t *soil_pct, uint16_t *raw);

/**
 * @brief Nivel de luz.
 * @param[out] light_pct 0..100 (0 = oscuro); -1 si no es válida.
 * @param[out] raw       promedio crudo de 12 bits; puede ser NULL.
 */
analog_status_t analog_read_light(int16_t *light_pct, uint16_t *raw);

/**
 * @brief Interpolación lineal saturada (función pura, sin HAL).
 *
 * raw_0 es el crudo que corresponde a 0 % y raw_100 el de 100 %. Funciona en
 * ambos sentidos: el sensor capacitivo da MENOS voltaje cuanto más húmedo.
 */
int16_t analog_raw_to_pct(uint16_t raw, uint16_t raw_0, uint16_t raw_100);

#endif /* DRV_ANALOG_H */
