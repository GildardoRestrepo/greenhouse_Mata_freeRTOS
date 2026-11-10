/**
 * @file task_sensors.c
 * @brief tSensors: mide, valida e informa a tControl. No decide nada.
 *
 * Cada SENSORS_PERIOD_MS (osDelayUntil, tick absoluto → periodo sin deriva):
 *   1. lee suelo y luz por ADC (promediados);
 *   2. cada HTU21_EVERY_N_CYCLES ciclos lee el HTU21 (temperatura y humedad, dos
 *      medidas separadas) y valida CRC y rango físico;
 *   3. envía EV_READING a qEvents con las lecturas y su valid_mask;
 *   4. si un fallo se confirma, envía además EV_SENSOR_FAULT con su causa
 *      (una vez por fallo, no en cada ciclo).
 *
 * La tarea solo se bloquea en osDelayUntil, en mtxI2C y en el osDelay de la
 * conversión del HTU21 (dentro del driver, con el mutex suelto).
 */
#include "task_sensors.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"           /* uxTaskGetStackHighWaterMark */

#include "app_config.h"
#include "app_shared.h"
#include "app_types.h"
#include "cmsis_os2.h"
#include "drv_analog.h"
#include "drv_htu21.h"
#include "task_logger.h"    /* log_printf */

#define SENSORS_STACK_REPORT_CYCLES     (STACK_REPORT_MS / SENSORS_PERIOD_MS)

/* Seguimiento de un fallo: se confirma tras varios ciclos malos seguidos. */
typedef struct {
    uint8_t count;      /* ciclos fallidos consecutivos */
    bool    active;     /* fallo confirmado y ya informado a tControl */
} fault_tracker_t;

/* Última lectura buena del aire: se reutiliza en los ciclos en que no se mide. */
typedef struct {
    int16_t temp_cC;
    int16_t hum_cPct;
    bool    valid;
} air_cache_t;

/**
 * @brief Actualiza el contador de un fallo.
 * @return true justo en el ciclo en que el fallo se confirma (hay que informarlo).
 */
static bool sensors_track(fault_tracker_t *t, bool failed, uint8_t threshold)
{
    if (!failed) {
        t->count = 0;
        t->active = false;
        return false;
    }
    if (t->count < UINT8_MAX) {
        t->count++;
    }
    if (!t->active && (t->count >= threshold)) {
        t->active = true;
        return true;
    }
    return false;
}

/* Envía sin bloquear: si tControl no da abasto, el periodo de medida no se altera. */
static bool sensors_send(const app_event_t *ev)
{
    if (osMessageQueuePut(qEventsHandle, ev, 0U, 0U) != osOK) {
        log_printf("SEN qEvents llena, evento %d perdido", (int)ev->type);
        return false;
    }
    return true;
}

static void sensors_send_fault(fault_tracker_t *t, fault_code_t code)
{
    app_event_t ev = {0};

    ev.type = EV_SENSOR_FAULT;
    ev.tick = osKernelGetTickCount();
    ev.data.fault_code = (uint8_t)code;
    if (!sensors_send(&ev)) {
        t->active = false;      /* no llegó: se reintenta en el próximo ciclo */
    }
}

/*
 * Lee temperatura y humedad del HTU21 (dos medidas, ≈ 75 ms en total, bloqueada sin
 * espera activa) y descarta lo que esté fuera de rango físico. Solo es buena si las
 * dos medidas lo son: así el par que se publica siempre es del mismo instante.
 */
static bool sensors_read_air(air_cache_t *air)
{
    int16_t temp_cC = 0;
    int16_t hum_cPct = 0;

    if (htu21_read_temp(&temp_cC) != HTU21_OK) {
        return false;
    }
    if (htu21_read_hum(&hum_cPct) != HTU21_OK) {
        return false;
    }
    if ((temp_cC < TEMP_VALID_MIN_cC) || (temp_cC > TEMP_VALID_MAX_cC)
            || (hum_cPct > HUM_VALID_MAX_cPct)) {
        return false;
    }
    air->temp_cC = temp_cC;
    air->hum_cPct = hum_cPct;
    air->valid = true;
    return true;
}

void StartSensorsTask(void *argument)
{
    (void)argument;

    fault_tracker_t soil_fault = {0};
    fault_tracker_t air_fault = {0};
    fault_tracker_t adc_fault = {0};
    air_cache_t air = {0};
    uint32_t cycle = 0;

    if (htu21_init() != HTU21_OK) {
        log_printf("SEN HTU21 no responde en el arranque");
    }

    uint32_t next_wake = osKernelGetTickCount();

    for (;;) {
        app_event_t ev = {0};
        readings_t *r = &ev.data.r;
        uint16_t soil_raw = 0;
        uint16_t light_raw = 0;

        /* 1) Analógicos: en cada ciclo. */
        analog_status_t soil_st = analog_read_soil(&r->soil_pct, &soil_raw);
        analog_status_t light_st = analog_read_light(&r->light_pct, &light_raw);

        if (soil_st == ANALOG_OK) {
            r->valid_mask |= READ_VALID_SOIL;
        }
        if (light_st == ANALOG_OK) {
            r->valid_mask |= READ_VALID_LIGHT;
        }

        /* 2) Aire: cada HTU21_EVERY_N_CYCLES ciclos; el resto usa la última lectura buena. */
        bool air_cycle = ((cycle % HTU21_EVERY_N_CYCLES) == 0U);
        bool air_fault_new = false;

        if (air_cycle) {
            bool air_ok = sensors_read_air(&air);

            air_fault_new = sensors_track(&air_fault, !air_ok, AIR_FAULT_COUNT);
            if (air_fault.active) {
                air.valid = false;      /* fallo confirmado: el dato viejo ya no sirve */
            }
        }
        if (air.valid) {
            r->temp_cC = air.temp_cC;
            r->hum_cPct = air.hum_cPct;
            r->valid_mask |= READ_VALID_AIR;
        }

        /* 3) Lecturas a tControl (siempre: también le sirven de "sigo viva"). */
        ev.type = EV_READING;
        ev.tick = osKernelGetTickCount();
        (void)sensors_send(&ev);

        /* 4) Fallos recién confirmados. */
        if (sensors_track(&soil_fault, soil_st == ANALOG_ERR_DISCONNECTED, SOIL_FAULT_COUNT)) {
            sensors_send_fault(&soil_fault, FAULT_SOIL);
        }
        if (sensors_track(&adc_fault,
                          (soil_st == ANALOG_ERR_ADC) || (light_st == ANALOG_ERR_ADC),
                          ADC_FAULT_COUNT)) {
            sensors_send_fault(&adc_fault, FAULT_ADC);
        }
        if (air_fault_new) {
            sensors_send_fault(&air_fault, FAULT_AIR);
        }

#if SENSORS_LOG_READINGS
        if (air_cycle) {
            log_printf("SEN s=%u/%d l=%u/%d T=%d H=%d m=%u",
                       (unsigned)soil_raw, (int)r->soil_pct,
                       (unsigned)light_raw, (int)r->light_pct,
                       (int)r->temp_cC, (int)r->hum_cPct, (unsigned)r->valid_mask);
        }
#endif

        cycle++;
        if ((cycle % SENSORS_STACK_REPORT_CYCLES) == 0U) {
            log_printf("SEN pila libre=%u palabras",
                       (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }

        next_wake += APP_MS_TO_TICKS(SENSORS_PERIOD_MS);
        (void)osDelayUntil(next_wake);
    }
}
