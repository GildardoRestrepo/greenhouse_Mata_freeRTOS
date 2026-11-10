/**
 * @file drv_analog.c
 * @brief Lectura promediada de ADC1 (suelo y LDR) y conversión a porcentaje.
 *
 * ADC1 está configurado en CubeMX como conversión simple por software, así que
 * el canal se selecciona antes de cada ráfaga de muestras.
 *
 * Sobre la espera: una conversión dura (480 + 12) ciclos de ADCCLK ≈ 25 µs,
 * cuarenta veces menos que un tick del RTOS (1 ms). Bloquear la tarea por algo
 * tan corto no es posible con osDelay y costaría más en cambios de contexto que
 * la propia conversión, por eso se usa HAL_ADC_PollForConversion con timeout
 * acotado. Una lectura completa (8 muestras) ocupa ≈ 0,2 ms cada 500 ms.
 */
#include "drv_analog.h"

#include <stddef.h>

#include "adc.h"            /* hadc1 (generado por CubeMX, par .c/.h) */
#include "app_config.h"

#define ANALOG_CH_SOIL      ADC_CHANNEL_10      /* PC0 */
#define ANALOG_CH_LIGHT     ADC_CHANNEL_11      /* PC1 */

/* Promedio de ANALOG_NUM_SAMPLES conversiones del canal indicado. */
static analog_status_t analog_read_avg(uint32_t channel, uint16_t *avg)
{
    ADC_ChannelConfTypeDef cfg = {0};
    uint32_t sum = 0;

    cfg.Channel = channel;
    cfg.Rank = 1;
    cfg.SamplingTime = ADC_SAMPLETIME_480CYCLES;    /* fuentes de alta impedancia */
    if (HAL_ADC_ConfigChannel(&hadc1, &cfg) != HAL_OK) {
        return ANALOG_ERR_ADC;
    }

    for (uint32_t i = 0; i < ANALOG_NUM_SAMPLES; i++) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) {
            return ANALOG_ERR_ADC;
        }
        if (HAL_ADC_PollForConversion(&hadc1, ANALOG_CONV_TIMEOUT_MS) != HAL_OK) {
            (void)HAL_ADC_Stop(&hadc1);
            return ANALOG_ERR_ADC;
        }
        sum += HAL_ADC_GetValue(&hadc1);
    }
    (void)HAL_ADC_Stop(&hadc1);

    *avg = (uint16_t)(sum / ANALOG_NUM_SAMPLES);
    return ANALOG_OK;
}

int16_t analog_raw_to_pct(uint16_t raw, uint16_t raw_0, uint16_t raw_100)
{
    int32_t span = (int32_t)raw_100 - (int32_t)raw_0;
    int32_t pct;

    if (span == 0) {
        return 0;       /* calibración degenerada: evita dividir por cero */
    }
    pct = (((int32_t)raw - (int32_t)raw_0) * 100) / span;
    if (pct < 0) {
        pct = 0;
    } else if (pct > 100) {
        pct = 100;
    }
    return (int16_t)pct;
}

analog_status_t analog_read_soil(int16_t *soil_pct, uint16_t *raw)
{
    uint16_t avg = 0;
    analog_status_t st = analog_read_avg(ANALOG_CH_SOIL, &avg);

    if (raw != NULL) {
        *raw = avg;
    }
    if ((st == ANALOG_OK) && (avg < SOIL_RAW_MIN)) {
        st = ANALOG_ERR_DISCONNECTED;
    }
    *soil_pct = (st == ANALOG_OK) ? analog_raw_to_pct(avg, SOIL_RAW_DRY, SOIL_RAW_WET) : -1;
    return st;
}

analog_status_t analog_read_light(int16_t *light_pct, uint16_t *raw)
{
    uint16_t avg = 0;
    analog_status_t st = analog_read_avg(ANALOG_CH_LIGHT, &avg);

    if (raw != NULL) {
        *raw = avg;
    }
    *light_pct = (st == ANALOG_OK) ? analog_raw_to_pct(avg, LIGHT_RAW_DARK, LIGHT_RAW_BRIGHT)
                                   : -1;
    return st;
}
