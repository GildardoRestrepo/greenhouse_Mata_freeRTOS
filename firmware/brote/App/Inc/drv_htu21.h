/**
 * @file drv_htu21.h
 * @brief Driver del sensor de temperatura/humedad HTU21D, módulo GY-21 (I2C1, dirección 0x40).
 *
 * Capa drv_*: usa HAL y el mutex del bus (mtxI2C). Lo llama tSensors.
 * A diferencia del SHT30, el HTU21 mide temperatura y humedad con comandos
 * separados: cada magnitud tiene su propia función y su propio ciclo
 * "comando → soltar mutex → esperar → tomar mutex → leer".
 * Procedimiento descrito en docs/arquitectura.md §6.
 */
#ifndef DRV_HTU21_H
#define DRV_HTU21_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HTU21_OK = 0,
    HTU21_ERR_MUTEX,    /* no se obtuvo mtxI2C dentro del timeout */
    HTU21_ERR_BUS,      /* HAL_I2C devolvió algo distinto de HAL_OK (NACK, timeout...) */
    HTU21_ERR_CRC       /* el CRC-8 de la medida no coincide */
} htu21_status_t;

/**
 * @brief Comprueba que el sensor responde en el bus (para el autotest de INIT).
 * @return HTU21_OK si hay ACK en 0x40.
 */
htu21_status_t htu21_init(void);

/**
 * @brief Mide la temperatura (bloquea ≈ 55 ms, sin espera activa).
 *
 * Envía 0xF3 (no-hold master), suelta mtxI2C durante la conversión y lo vuelve
 * a tomar para leer 3 bytes (MSB, LSB, CRC). Solo debe llamarse desde una
 * tarea, con el scheduler corriendo.
 *
 * @param[out] temp_cC centésimas de °C (p. ej. 2350 = 23,50 °C)
 * @return HTU21_OK y salida válida; en error la salida no se modifica.
 */
htu21_status_t htu21_read_temp(int16_t *temp_cC);

/**
 * @brief Mide la humedad relativa (bloquea ≈ 20 ms, sin espera activa).
 *
 * Igual que htu21_read_temp() pero con el comando 0xF5.
 *
 * @param[out] hum_cPct centésimas de %HR, saturadas a 0..10000 (6520 = 65,20 %)
 * @return HTU21_OK y salida válida; en error la salida no se modifica.
 */
htu21_status_t htu21_read_hum(int16_t *hum_cPct);

/* Funciones puras (sin HAL ni RTOS), expuestas para poder probarlas aisladas. */

/** CRC-8 del HTU21: polinomio 0x31, init 0x00, sin reflexión ni XOR final. */
uint8_t htu21_crc8(const uint8_t *data, size_t len);

/**
 * temp_cC = -4685 + 17572 * raw / 65536 (aritmética de 32 bits).
 * Los 2 bits de estado del crudo se enmascaran aquí (raw & 0xFFFC).
 */
int16_t htu21_raw_to_temp_cC(uint16_t raw_t);

/**
 * hum_cPct = -600 + 12500 * raw / 65536 (aritmética de 32 bits), saturada a
 * 0..10000: la fórmula da de -6 % a 119 % y la hoja de datos indica recortar.
 * Los 2 bits de estado del crudo se enmascaran aquí (raw & 0xFFFC).
 */
int16_t htu21_raw_to_hum_cPct(uint16_t raw_h);

#endif /* DRV_HTU21_H */
