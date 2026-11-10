/**
 * @file app_types.h
 * @brief Tipos compartidos entre tareas (docs/arquitectura.md §3).
 *
 * Archivo COMPARTIDO: cambiar un tipo = avisar a la pareja (palabra INTERFAZ en
 * la bitácora y en el PR). Sin dependencias de HAL ni de RTOS.
 */
#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

/* Estado del sistema. Solo tControl lo cambia. */
typedef enum {
    ST_INIT,
    ST_MONITOR,
    ST_IRRIGATE,
    ST_MANUAL,
    ST_ALARM
} sys_state_t;

/* Eventos que llegan a tControl por qEvents. */
typedef enum {
    EV_READING,
    EV_SENSOR_FAULT,
    EV_PUMP_DONE,
    EV_BTN_K0,
    EV_BTN_K1,
    EV_BTN_KUP,
    EV_BTN_KUP_LONG
} event_type_t;

/* Bits de readings_t.valid_mask. */
#define READ_VALID_SOIL     0x01U   /* bit0: soil_pct válido */
#define READ_VALID_LIGHT    0x02U   /* bit1: light_pct válido */
#define READ_VALID_AIR      0x04U   /* bit2: temp_cC y hum_cPct válidos */

typedef struct {
    int16_t  soil_pct;      /* 0..100, -1 = inválido */
    int16_t  light_pct;     /* 0..100 */
    int16_t  temp_cC;       /* centésimas de °C (evita printf de float) */
    int16_t  hum_cPct;      /* centésimas de %HR */
    uint8_t  valid_mask;    /* bit0 suelo, bit1 luz, bit2 aire */
} readings_t;

/*
 * Causa de un fallo: va en app_event_t.data.fault_code (EV_SENSOR_FAULT) y en
 * snapshot_t.fault_code (causa de la alarma, para mostrarla en la OLED).
 * Los tres primeros los genera tSensors; el resto los decide tControl.
 */
typedef enum {
    FAULT_NONE = 0,
    FAULT_SOIL,         /* sensor de suelo desconectado (ADC < SOIL_RAW_MIN) */
    FAULT_AIR,          /* SHT30 no responde, CRC inválido o fuera de rango físico */
    FAULT_ADC,          /* el ADC no convirtió (error o timeout de HAL) */
    FAULT_TIMEOUT,      /* tSensors dejó de reportar (SENSOR_TIMEOUT_MS) */
    FAULT_NO_WATER,     /* MAX_PULSES_NO_RISE pulsos sin subir la humedad */
    FAULT_OVERTEMP      /* temperatura > TEMP_MAX_cC */
} fault_code_t;

typedef struct {
    event_type_t type;
    uint32_t     tick;      /* osKernelGetTickCount() al crear el evento */
    union {
        readings_t r;
        uint8_t    fault_code;      /* fault_code_t */
    } data;
} app_event_t;

/*
 * Contenido de g_snapshot (protegido por mtxData, ver app_shared.h).
 * Lo escribe tControl; lo leen tDisplay y tLogger.
 */
typedef struct {
    sys_state_t state;
    readings_t  r;          /* últimas lecturas recibidas por tControl */
    uint8_t     fault_code; /* fault_code_t: causa de la alarma, FAULT_NONE si no hay */
    uint8_t     screen;     /* pantalla pedida con K0; tDisplay usa screen % nº de pantallas */
    uint32_t    tick;       /* tick de la última actualización */
} snapshot_t;

#endif /* APP_TYPES_H */
