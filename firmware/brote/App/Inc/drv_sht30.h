/**
 * @file drv_sht30.h
 * @brief Driver del sensor de temperatura/humedad SHT30 (I2C1, dirección 0x44).
 *
 * Capa drv_*: usa HAL y el mutex del bus (mtxI2C). Lo llama tSensors.
 * Procedimiento de lectura descrito en docs/arquitectura.md §6.
 */
#ifndef DRV_SHT30_H
#define DRV_SHT30_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SHT30_OK = 0,
    SHT30_ERR_MUTEX,    /* no se obtuvo mtxI2C dentro del timeout */
    SHT30_ERR_BUS,      /* HAL_I2C devolvió algo distinto de HAL_OK (NACK, timeout...) */
    SHT30_ERR_CRC       /* el CRC-8 de temperatura o de humedad no coincide */
} sht30_status_t;

/**
 * @brief Comprueba que el sensor responde en el bus (para el autotest de INIT).
 * @return SHT30_OK si hay ACK en 0x44.
 */
sht30_status_t sht30_init(void);

/**
 * @brief Medida única de temperatura y humedad (bloquea ~20 ms, sin espera activa).
 *
 * Envía 0x2400 (alta repetibilidad, sin clock stretching), suelta mtxI2C durante
 * la conversión y lo vuelve a tomar para leer los 6 bytes. Solo debe llamarse
 * desde una tarea, con el scheduler corriendo.
 *
 * @param[out] temp_cC   centésimas de °C (p. ej. 2350 = 23,50 °C)
 * @param[out] hum_cPct  centésimas de %HR (p. ej. 6520 = 65,20 %)
 * @return SHT30_OK y salidas válidas; en error las salidas no se modifican.
 */
sht30_status_t sht30_read(int16_t *temp_cC, int16_t *hum_cPct);

/* Funciones puras (sin HAL ni RTOS), expuestas para poder probarlas aisladas. */

/** CRC-8 de Sensirion: polinomio 0x31, init 0xFF, sin reflexión ni XOR final. */
uint8_t sht30_crc8(const uint8_t *data, size_t len);

/** temp_cC = -4500 + 17500 * raw / 65535 (aritmética de 32 bits). */
int16_t sht30_raw_to_temp_cC(uint16_t raw_t);

/** hum_cPct = 10000 * raw / 65535 (aritmética de 32 bits). */
int16_t sht30_raw_to_hum_cPct(uint16_t raw_h);

#endif /* DRV_SHT30_H */
