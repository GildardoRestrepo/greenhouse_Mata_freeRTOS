/**
 * @file drv_sht30.c
 * @brief Driver del SHT30: medida única sin clock stretching, CRC-8 y punto fijo.
 *
 * Regla del bus: I2C1 solo se usa con mtxI2C tomado, y nunca se duerme con el
 * mutex tomado. Por eso la lectura son dos transacciones cortas separadas por
 * un osDelay() con el mutex suelto: la OLED puede usar el bus mientras el
 * sensor convierte.
 */
#include "drv_sht30.h"

#include "app_config.h"
#include "app_shared.h"       /* mtxI2CHandle */
#include "cmsis_os2.h"
#include "i2c.h"            /* hi2c1 (generado por CubeMX, par .c/.h) */

/* Constantes del protocolo (hoja de datos del SHT3x-DIS); no son ajustables. */
#define SHT30_I2C_ADDR          (0x44U << 1)    /* ADDR a GND; HAL usa 8 bits */
#define SHT30_CMD_SINGLE_HIGH   0x2400U         /* medida única, alta repet., sin stretching */
#define SHT30_FRAME_LEN         6U              /* T_msb T_lsb CRC H_msb H_lsb CRC */
#define SHT30_CRC_POLY          0x31U
#define SHT30_CRC_INIT          0xFFU

static sht30_status_t sht30_lock(void)
{
    if (osMutexAcquire(mtxI2CHandle, APP_MS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) != osOK) {
        return SHT30_ERR_MUTEX;
    }
    return SHT30_OK;
}

static void sht30_unlock(void)
{
    (void)osMutexRelease(mtxI2CHandle);
}

uint8_t sht30_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = SHT30_CRC_INIT;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; bit++) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ SHT30_CRC_POLY);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}

int16_t sht30_raw_to_temp_cC(uint16_t raw_t)
{
    /* 17500 * 65535 = 1 146 862 500 < 2^31: cabe en int32_t. Rango -4500..13000. */
    return (int16_t)(-4500 + (17500 * (int32_t)raw_t) / 65535);
}

int16_t sht30_raw_to_hum_cPct(uint16_t raw_h)
{
    /* Rango 0..10000. */
    return (int16_t)((10000 * (int32_t)raw_h) / 65535);
}

sht30_status_t sht30_init(void)
{
    sht30_status_t st = sht30_lock();
    if (st != SHT30_OK) {
        return st;
    }

    if (HAL_I2C_IsDeviceReady(&hi2c1, SHT30_I2C_ADDR, 2U, SHT30_I2C_TIMEOUT_MS) != HAL_OK) {
        st = SHT30_ERR_BUS;
    }

    sht30_unlock();
    return st;
}

sht30_status_t sht30_read(int16_t *temp_cC, int16_t *hum_cPct)
{
    static const uint8_t cmd[2] = {
        (uint8_t)(SHT30_CMD_SINGLE_HIGH >> 8),
        (uint8_t)(SHT30_CMD_SINGLE_HIGH & 0xFFU)
    };
    uint8_t frame[SHT30_FRAME_LEN];
    HAL_StatusTypeDef hal;
    sht30_status_t st;

    /* 1) Lanzar la medida. */
    st = sht30_lock();
    if (st != SHT30_OK) {
        return st;
    }
    hal = HAL_I2C_Master_Transmit(&hi2c1, SHT30_I2C_ADDR, (uint8_t *)cmd, sizeof(cmd),
                                  SHT30_I2C_TIMEOUT_MS);
    sht30_unlock();
    if (hal != HAL_OK) {
        return SHT30_ERR_BUS;
    }

    /* 2) Esperar la conversión bloqueados y con el bus libre para la OLED. */
    osDelay(APP_MS_TO_TICKS(SHT30_MEAS_DELAY_MS));

    /* 3) Leer el resultado. Si aún no hubiera terminado, el sensor responde NACK. */
    st = sht30_lock();
    if (st != SHT30_OK) {
        return st;
    }
    hal = HAL_I2C_Master_Receive(&hi2c1, SHT30_I2C_ADDR, frame, sizeof(frame),
                                 SHT30_I2C_TIMEOUT_MS);
    sht30_unlock();
    if (hal != HAL_OK) {
        return SHT30_ERR_BUS;
    }

    /* 4) Validar cada palabra con su CRC antes de usarla. */
    if ((sht30_crc8(&frame[0], 2U) != frame[2]) || (sht30_crc8(&frame[3], 2U) != frame[5])) {
        return SHT30_ERR_CRC;
    }

    /* 5) Convertir a punto fijo. */
    *temp_cC = sht30_raw_to_temp_cC((uint16_t)(((uint16_t)frame[0] << 8) | frame[1]));
    *hum_cPct = sht30_raw_to_hum_cPct((uint16_t)(((uint16_t)frame[3] << 8) | frame[4]));
    return SHT30_OK;
}
