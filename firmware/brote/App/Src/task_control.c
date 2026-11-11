/**
 * @file task_control.c
 * @brief tControl: traduce qEvents a la máquina de estados y ejecuta sus acciones.
 *
 * Bloqueada en osMessageQueueGet(qEvents) con timeout CONTROL_EVENT_TIMEOUT_MS:
 * el timeout no es un periodo de trabajo, solo sirve para detectar que tSensors
 * dejó de reportar (SENSOR_TIMEOUT_MS) aunque no llegue ningún evento.
 *
 * Los botones llegan por EXTI: HAL_GPIO_EXTI_Callback (abajo) aplica el
 * antirrebote por tiempo y encola el evento con la API ISR-safe.
 */
#include "task_control.h"

#include "app_config.h"
#include "app_shared.h"
#include "app_types.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "main.h"           /* etiquetas de pines de los botones */
#include "state_machine.h"
#include "task.h"           /* uxTaskGetStackHighWaterMark */
#include "task_actuators.h"
#include "task_logger.h"

/* Si en el .ioc no se pusieron las etiquetas sugeridas: pines de la placa. */
#ifndef BTN_KUP_Pin
#define BTN_KUP_Pin     GPIO_PIN_0      /* PA0, activo en alto */
#endif
#ifndef BTN_K1_Pin
#define BTN_K1_Pin      GPIO_PIN_3      /* PE3, activo en bajo */
#endif
#ifndef BTN_K0_Pin
#define BTN_K0_Pin      GPIO_PIN_4      /* PE4, activo en bajo */
#endif

static const char *control_fault_name(fault_code_t f)
{
    switch (f) {
    case FAULT_NONE:     return "-";
    case FAULT_SOIL:     return "suelo";
    case FAULT_AIR:      return "aire";
    case FAULT_ADC:      return "ADC";
    case FAULT_TIMEOUT:  return "timeout";
    case FAULT_NO_WATER: return "sin agua";
    case FAULT_OVERTEMP: return "temperatura";
    default:             return "?";
    }
}

static const char *control_event_name(event_type_t t)
{
    switch (t) {
    case EV_READING:      return "EV_READING";
    case EV_SENSOR_FAULT: return "EV_SENSOR_FAULT";
    case EV_PUMP_DONE:    return "EV_PUMP_DONE";
    case EV_BTN_K0:       return "EV_BTN_K0";
    case EV_BTN_K1:       return "EV_BTN_K1";
    case EV_BTN_KUP:      return "EV_BTN_KUP";
    case EV_BTN_KUP_LONG: return "EV_BTN_KUP_LONG";
    default:              return "EV_?";
    }
}

/* Publica el estado para tDisplay/tLogger. Si mtxData no se obtuvo, se reintenta
   en el próximo evento. */
static void control_publish(const sm_ctx_t *sm, snapshot_t *snap, uint32_t now)
{
    snap->state = sm->state;
    snap->r = sm->last;
    snap->fault_code = (uint8_t)sm->fault;
    snap->tick = now;
    if (!shared_set(snap)) {
        log_printf("CTL mtxData ocupado, snapshot no actualizado");
    }
}

/* Ejecuta lo que pidió la máquina de estados. */
static void control_apply(uint8_t act, sys_state_t prev, const sm_ctx_t *sm,
                          snapshot_t *snap, uint32_t now)
{
    uint32_t cmds = 0;

    if ((act & SM_ACT_STATE_CHG) != 0U) {
        control_publish(sm, snap, now);     /* antes de avisar a tActuators: lee el snapshot */
        cmds |= CMD_LED_STATE;
        if (sm->state == ST_ALARM) {
            log_printf("CTL %s->%s causa=%s", sm_state_name(prev), sm_state_name(sm->state),
                       control_fault_name(sm->fault));
        } else {
            log_printf("CTL %s->%s", sm_state_name(prev), sm_state_name(sm->state));
        }
    }
    if ((act & SM_ACT_PUMP_STOP) != 0U) {
        cmds |= CMD_PUMP_STOP;
    }
    if ((act & SM_ACT_PUMP_PULSE) != 0U) {
        cmds |= CMD_PUMP_PULSE;
        log_printf("CTL pulso de riego (suelo=%d%%)", (int)sm->last.soil_pct);
    }
    if ((act & SM_ACT_BEEP) != 0U) {
        cmds |= CMD_BEEP;
    }
    if (cmds != 0U) {
        act_send(cmds);
    }
}

void StartControlTask(void *argument)
{
    (void)argument;

    static sm_ctx_t sm;
    static snapshot_t snap;
    app_event_t ev;
    uint32_t now = osKernelGetTickCount();
    uint32_t last_stack_report = now;

    sm_init(&sm, now);
    snap.screen = 0;
    control_publish(&sm, &snap, now);
    act_send(CMD_LED_STATE | CMD_BEEP);     /* INIT: LED blanco + 1 bip */
    log_printf("CTL inicio, estado %s", sm_state_name(sm.state));

    for (;;) {
        osStatus_t st = osMessageQueueGet(qEventsHandle, &ev, NULL,
                                          APP_MS_TO_TICKS(CONTROL_EVENT_TIMEOUT_MS));
        now = osKernelGetTickCount();

        sys_state_t prev = sm.state;
        uint8_t act = SM_ACT_NONE;
        bool publish = false;
        sm_input_t in = { .now = now };

        if (st == osOK) {
            switch (ev.type) {
            case EV_READING:
                in.ev = SM_EV_READING;
                in.r = ev.data.r;
                act = sm_step(&sm, &in);
                publish = true;     /* lecturas nuevas para la pantalla */
                break;
            case EV_SENSOR_FAULT:
                log_printf("CTL fallo de sensor: %s",
                           control_fault_name((fault_code_t)ev.data.fault_code));
                in.ev = SM_EV_FAULT;
                in.fault = (fault_code_t)ev.data.fault_code;
                act = sm_step(&sm, &in);
                break;
            case EV_PUMP_DONE:
                in.ev = SM_EV_PUMP_DONE;
                act = sm_step(&sm, &in);
                break;
            case EV_BTN_K0:
                snap.screen++;      /* cambio de pantalla: no es un cambio de estado */
                publish = true;
                break;
            case EV_BTN_K1:
                in.ev = SM_EV_BTN_K1;
                act = sm_step(&sm, &in);
                break;
            case EV_BTN_KUP:
                in.ev = SM_EV_BTN_KUP;
                act = sm_step(&sm, &in);
                break;
            case EV_BTN_KUP_LONG:
            default:
                break;
            }
            if (ev.type >= EV_BTN_K0) {
                /* Marca de tiempo ISR → tControl: evidencia de la preempción (§5). */
                log_printf("CTL %s t_isr=%lu t=%lu", control_event_name(ev.type),
                           (unsigned long)ev.tick, (unsigned long)now);
            }
        }

        /* tSensors muda: se revisa con o sin eventos (el timeout de la cola asegura que
           esta revisión ocurra aunque no llegue nada). */
        if ((sm.state != ST_ALARM) && ((now - sm.last_reading_tick) > SENSOR_TIMEOUT_MS)) {
            in.ev = SM_EV_SENSOR_TIMEOUT;
            act |= sm_step(&sm, &in);
        }

        control_apply(act, prev, &sm, &snap, now);
        if (publish && ((act & SM_ACT_STATE_CHG) == 0U)) {
            control_publish(&sm, &snap, now);
        }

        if ((now - last_stack_report) >= APP_MS_TO_TICKS(STACK_REPORT_MS)) {
            last_stack_report = now;
            log_printf("CTL pila libre=%u palabras", (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
    }
}

/* ---- Botones: callback de EXTI (contexto de ISR, solo API ISR-safe). ---- */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t last_k0;
    static uint32_t last_k1;
    static uint32_t last_kup;
    uint32_t now = HAL_GetTick();
    uint32_t *last;
    app_event_t ev = {0};

    if (GPIO_Pin == BTN_K0_Pin) {
        last = &last_k0;
        ev.type = EV_BTN_K0;
    } else if (GPIO_Pin == BTN_K1_Pin) {
        last = &last_k1;
        ev.type = EV_BTN_K1;
    } else if (GPIO_Pin == BTN_KUP_Pin) {
        last = &last_kup;
        ev.type = EV_BTN_KUP;
    } else {
        return;
    }

    /* Antirrebote por tiempo: se ignoran flancos a menos de BTN_DEBOUNCE_MS del anterior. */
    if ((now - *last) < BTN_DEBOUNCE_MS) {
        return;
    }
    *last = now;

    ev.tick = osKernelGetTickCount();
    /* Timeout 0 (obligatorio en ISR). CMSIS-RTOS2 hace portYIELD_FROM_ISR: si tControl
       es más prioritaria que la tarea interrumpida, corre apenas termina la ISR. */
    (void)osMessageQueuePut(qEventsHandle, &ev, 0U, 0U);
}
