/**
 * @file drv_htu21.c
 * @brief Driver del HTU21D (GY-21): medidas no-hold, CRC-8 y punto fijo.
 *
 * Regla del bus: I2C1 solo se usa con mtxI2C tomado, y nunca se duerme con el
 * mutex tomado. Por eso cada medida son dos transacciones cortas separadas por
 * un osDelay() con el mutex suelto: la OLED puede usar el bus mientras el
 * sensor convierte. En modo no-hold el sensor no estira el reloj; si se le
 * lee antes de terminar responde NACK (se reporta como HTU21_ERR_BUS).
 */
#include "drv_htu21.h"

#include "app_config.h"
#include "app_shared.h"     /* mtxI2CHandle */
#include "cmsis_os2.h"
#include "i2c.h"            /* hi2c1 (generado por CubeMX, par .c/.h) */

/* Constantes del protocolo (hoja de datos del HTU21D); no son ajustables. */
#define HTU21_I2C_ADDR          (0x40U << 1)    /* dirección fija; HAL usa 8 bits */
#define HTU21_CMD_TEMP_NOHOLD   0xF3U           /* disparar temperatura, no-hold master */
#define HTU21_CMD_HUM_NOHOLD    0xF5U           /* disparar humedad, no-hold master */
#define HTU21_FRAME_LEN         3U              /* MSB, LSB, CRC */
#define HTU21_CRC_POLY          0x31U
#define HTU21_CRC_INIT          0x00U
#define HTU21_DATA_MASK         0xFFFCU         /* los 2 bits bajos son de estado, no dato */

static htu21_status_t htu21_lock(void)
{
    if (osMutexAcquire(mtxI2CHandle, APP_MS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) != osOK) {
        return HTU21_ERR_MUTEX;
    }
    return HTU21_OK;
}

static void htu21_unlock(void)
{
    (void)osMutexRelease(mtxI2CHandle);
}

uint8_t htu21_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = HTU21_CRC_INIT;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; bit++) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ HTU21_CRC_POLY);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

int16_t htu21_raw_to_temp_cC(uint16_t raw_t)
{
    int32_t raw = (int32_t)(raw_t & HTU21_DATA_MASK);

    /* 17572 * 65532 = 1 151 528 304 < 2^31: cabe en int32_t. Rango -4685..12885. */
    return (int16_t)(-4685 + (17572 * raw) / 65536);
}

int16_t htu21_raw_to_hum_cPct(uint16_t raw_h)
{
    int32_t raw = (int32_t)(raw_h & HTU21_DATA_MASK);
    int32_t hum = -600 + (12500 * raw) / 65536;     /* -600..11899 */

    if (hum < 0) {
        hum = 0;
    } else if (hum > 10000) {
        hum = 10000;
    }
    return (int16_t)hum;
}

/**
 * @brief Un ciclo completo de medida: comando → esperar con el bus libre → leer y validar.
 * @param cmd       HTU21_CMD_TEMP_NOHOLD o HTU21_CMD_HUM_NOHOLD
 * @param delay_ms  tiempo de conversión de esa magnitud
 * @param[out] raw  crudo de 16 bits tal como llega (con los bits de estado)
 */
static htu21_status_t htu21_measure(uint8_t cmd, uint32_t delay_ms, uint16_t *raw)
{
    uint8_t frame[HTU21_FRAME_LEN];
    HAL_StatusTypeDef hal;
    htu21_status_t st;

    /* 1) Lanzar la medida. */
    st = htu21_lock();
    if (st != HTU21_OK) {
        return st;
    }
    hal = HAL_I2C_Master_Transmit(&hi2c1, HTU21_I2C_ADDR, &cmd, 1U, HTU21_I2C_TIMEOUT_MS);
    htu21_unlock();
    if (hal != HAL_OK) {
        return HTU21_ERR_BUS;
    }

    /* 2) Esperar la conversión bloqueados y con el bus libre para la OLED. */
    osDelay(APP_MS_TO_TICKS(delay_ms));

    /* 3) Leer el resultado. */
    st = htu21_lock();
    if (st != HTU21_OK) {
        return st;
    }
    hal = HAL_I2C_Master_Receive(&hi2c1, HTU21_I2C_ADDR, frame, sizeof(frame),
                                 HTU21_I2C_TIMEOUT_MS);
    htu21_unlock();
    if (hal != HAL_OK) {
        return HTU21_ERR_BUS;
    }

    /* 4) Validar con el CRC antes de usar el dato. */
    if (htu21_crc8(frame, 2U) != frame[2]) {
        return HTU21_ERR_CRC;
    }

    *raw = (uint16_t)(((uint16_t)frame[0] << 8) | frame[1]);
    return HTU21_OK;
}

htu21_status_t htu21_init(void)
{
    htu21_status_t st = htu21_lock();
    if (st != HTU21_OK) {
        return st;
    }

    if (HAL_I2C_IsDeviceReady(&hi2c1, HTU21_I2C_ADDR, 2U, HTU21_I2C_TIMEOUT_MS) != HAL_OK) {
        st = HTU21_ERR_BUS;
    }

    htu21_unlock();
    return st;
}

htu21_status_t htu21_read_temp(int16_t *temp_cC)
{
    uint16_t raw = 0;
    htu21_status_t st = htu21_measure(HTU21_CMD_TEMP_NOHOLD, HTU21_TEMP_DELAY_MS, &raw);

    if (st != HTU21_OK) {
        return st;
    }
    *temp_cC = htu21_raw_to_temp_cC(raw);
    return HTU21_OK;
}

htu21_status_t htu21_read_hum(int16_t *hum_cPct)
{
    uint16_t raw = 0;
    htu21_status_t st = htu21_measure(HTU21_CMD_HUM_NOHOLD, HTU21_HUM_DELAY_MS, &raw);

    if (st != HTU21_OK) {
        return st;
    }
    *hum_cPct = htu21_raw_to_hum_cPct(raw);
    return HTU21_OK;
}
