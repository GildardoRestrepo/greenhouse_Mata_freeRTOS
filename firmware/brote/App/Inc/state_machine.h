/**
 * @file state_machine.h
 * @brief Máquina de estados de Brote (arquitectura.md §4), sin HAL ni RTOS.
 *
 * Entrada: contexto + evento (con el tick actual). Salida: el contexto queda con
 * el nuevo estado y se devuelven las acciones que tControl debe ejecutar.
 * Al no depender de HAL/RTOS se puede explicar y probar aislada (en el PC).
 */
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h"

/* Entradas de la máquina (tControl las traduce desde qEvents y su propio timeout). */
typedef enum {
    SM_EV_READING,          /* lecturas nuevas en sm_input_t.r */
    SM_EV_FAULT,            /* fallo confirmado por tSensors en sm_input_t.fault */
    SM_EV_PUMP_DONE,        /* tActuators terminó el pulso de riego */
    SM_EV_BTN_K1,           /* riego manual / reconocer alarma */
    SM_EV_BTN_KUP,          /* MONITOREO ⇄ MANUAL */
    SM_EV_SENSOR_TIMEOUT    /* tControl no recibe lecturas hace > SENSOR_TIMEOUT_MS */
} sm_event_t;

typedef struct {
    sm_event_t   ev;
    uint32_t     now;       /* tick actual (ms) */
    readings_t   r;         /* solo SM_EV_READING */
    fault_code_t fault;     /* solo SM_EV_FAULT */
} sm_input_t;

/* Acciones que pide la máquina (máscara de bits). */
#define SM_ACT_NONE         0x00U
#define SM_ACT_PUMP_PULSE   0x01U   /* un pulso de riego con tiempo máximo */
#define SM_ACT_PUMP_STOP    0x02U   /* cortar la bomba ya */
#define SM_ACT_BEEP         0x04U   /* un bip corto (aviso al usuario) */
#define SM_ACT_STATE_CHG    0x08U   /* cambió el estado: log, snapshot y LED */

typedef struct {
    sys_state_t  state;
    fault_code_t fault;             /* causa de la alarma; FAULT_NONE fuera de ALARMA */
    readings_t   last;              /* últimas lecturas recibidas */
    uint32_t     last_reading_tick; /* para detectar el timeout de sensores */
    bool         pump_on;           /* hay un pulso en curso (hasta SM_EV_PUMP_DONE) */
    uint32_t     pulse_tick;        /* cuándo empezó el último pulso de RIEGO */
    int16_t      soil_at_pulse;     /* humedad al dar el último pulso de RIEGO */
    uint8_t      pulses_no_rise;    /* pulsos seguidos sin que suba la humedad */
} sm_ctx_t;

/** @brief Deja la máquina en INIT; now = tick de arranque. */
void sm_init(sm_ctx_t *ctx, uint32_t now);

/**
 * @brief Procesa una entrada.
 * @return máscara SM_ACT_* con lo que tControl debe hacer.
 */
uint8_t sm_step(sm_ctx_t *ctx, const sm_input_t *in);

/** @brief Nombre corto del estado (para el log). */
const char *sm_state_name(sys_state_t state);

#endif /* STATE_MACHINE_H */
