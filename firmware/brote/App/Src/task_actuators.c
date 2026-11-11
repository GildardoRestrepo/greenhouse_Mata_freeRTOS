/**
 * @file task_actuators.c
 * @brief tActuators: pulso de riego con tiempo máximo, buzzer y LED RGB.
 *
 * Es la tarea de mayor prioridad. Se bloquea en osThreadFlagsWait con un timeout
 * que es el menor entre el fin del pulso en curso y el próximo paso de la
 * animación del LED (ACT_ANIM_PERIOD_MS). Así el pulso se corta por su propio
 * tiempo aunque tControl se cuelgue: la bomba nunca queda encendida sin límite.
 */
#include "task_actuators.h"

#include <stdbool.h>

#include "app_config.h"
#include "app_shared.h"     /* qEventsHandle, shared_get */
#include "app_types.h"
#include "cmsis_os2.h"
#include "drv_outputs.h"
#include "FreeRTOS.h"
#include "main.h"           /* __disable_irq (CMSIS) */
#include "task.h"           /* uxTaskGetStackHighWaterMark */
#include "task_logger.h"

/* Handle creado por CubeMX en freertos.c a partir del nombre del .ioc. */
extern osThreadId_t tActuatorsHandle;

void act_send(uint32_t cmds)
{
    (void)osThreadFlagsSet(tActuatorsHandle, cmds & CMD_ALL);
}

void act_fatal_stack_overflow(void)
{
    __disable_irq();
    out_pump(false);
    out_buzzer(false);
    out_rgb(1000U, 0U, 0U);     /* rojo fijo = fallo fatal (arquitectura.md §2) */
    for (;;) {
        /* Sistema detenido a propósito: el depurador muestra qué tarea desbordó. */
    }
}

/* Onda triangular 0..level..0 con periodo period_ms (el "respirar" del azul). */
static uint16_t act_breath(uint32_t now, uint16_t level, uint32_t period_ms)
{
    uint32_t half = period_ms / 2U;
    uint32_t phase = now % period_ms;
    uint32_t pos = (phase < half) ? phase : (period_ms - phase);

    return (uint16_t)((pos * level) / half);
}

/* Color del LED RGB según el estado (tabla de arquitectura.md §4). */
static void act_update_rgb(sys_state_t state, uint32_t now)
{
    const uint16_t lvl = RGB_LEVEL_PERMIL;

    switch (state) {
    case ST_INIT:
        out_rgb(lvl, lvl, lvl);                                     /* blanco */
        break;
    case ST_MONITOR:
        out_rgb(0U, lvl, 0U);                                       /* verde */
        break;
    case ST_IRRIGATE:
        out_rgb(0U, 0U, act_breath(now, lvl, RGB_BREATH_PERIOD_MS)); /* azul que respira */
        break;
    case ST_MANUAL:
        out_rgb(lvl, (uint16_t)(lvl * 3U / 10U), 0U);               /* ámbar */
        break;
    case ST_ALARM:
    default:
        out_rgb((((now / ALARM_BLINK_MS) % 2U) == 0U) ? lvl : 0U, 0U, 0U); /* rojo parpadeando */
        break;
    }
}

static void act_pump_done(void)
{
    app_event_t ev = {0};

    ev.type = EV_PUMP_DONE;
    ev.tick = osKernelGetTickCount();
    if (osMessageQueuePut(qEventsHandle, &ev, 0U, 0U) != osOK) {
        log_printf("ACT qEvents llena, EV_PUMP_DONE perdido");
    }
}

void StartActuatorsTask(void *argument)
{
    (void)argument;

    sys_state_t state = ST_INIT;
    bool pump_on = false;
    uint32_t pump_off_tick = 0;     /* cuándo se corta el pulso en curso */
    uint32_t beep_until = 0;        /* fin del bip pedido con CMD_BEEP */
    uint32_t alarm_since = 0;       /* para el bip cada ALARM_BEEP_PERIOD_MS */
    uint32_t last_stack_report = osKernelGetTickCount();
    snapshot_t snap;

    if (!out_init()) {
        log_printf("ACT error al arrancar el PWM del LED RGB");
    }

    for (;;) {
        uint32_t now = osKernelGetTickCount();
        uint32_t wait = ACT_ANIM_PERIOD_MS;

        if (pump_on) {
            uint32_t left = ((int32_t)(pump_off_tick - now) > 0) ? (pump_off_tick - now) : 0U;
            if (left < wait) {
                wait = left;
            }
        }

        /* Bloqueada hasta una orden o hasta el próximo vencimiento: no consume CPU. */
        uint32_t flags = osThreadFlagsWait(CMD_ALL, osFlagsWaitAny, APP_MS_TO_TICKS(wait));
        now = osKernelGetTickCount();

        if ((flags & osFlagsError) == 0U) {
            if ((flags & CMD_LED_STATE) != 0U) {
                if (shared_get(&snap)) {
                    if ((snap.state == ST_ALARM) && (state != ST_ALARM)) {
                        alarm_since = now;
                    }
                    state = snap.state;
                }
            }
            if (((flags & CMD_PUMP_STOP) != 0U) && pump_on) {
                pump_on = false;
                out_pump(false);
                log_printf("ACT bomba OFF (orden)");
                act_pump_done();
            }
            /* Defensa: en ALARMA no se riega aunque llegue la orden. */
            if (((flags & CMD_PUMP_PULSE) != 0U) && !pump_on && (state != ST_ALARM)) {
                pump_on = true;
                pump_off_tick = now + APP_MS_TO_TICKS(PUMP_PULSE_MS);
                out_pump(true);
                log_printf("ACT bomba ON t=%lu", (unsigned long)now);
            }
            if ((flags & CMD_BEEP) != 0U) {
                beep_until = now + APP_MS_TO_TICKS(ACT_BEEP_MS);
            }
        }

        /* Tiempo máximo del pulso: se corta aquí pase lo que pase en tControl. */
        if (pump_on && ((int32_t)(now - pump_off_tick) >= 0)) {
            pump_on = false;
            out_pump(false);
            log_printf("ACT bomba OFF t=%lu", (unsigned long)now);
            act_pump_done();
        }
        if (state == ST_ALARM) {
            out_pump(false);    /* "off forzado" en ALARMA */
        }

        bool beep = ((int32_t)(beep_until - now) > 0)
                    || ((state == ST_ALARM)
                        && (((now - alarm_since) % ALARM_BEEP_PERIOD_MS) < ACT_BEEP_MS));
        out_buzzer(beep);
        act_update_rgb(state, now);

        if ((now - last_stack_report) >= APP_MS_TO_TICKS(STACK_REPORT_MS)) {
            last_stack_report = now;
            log_printf("ACT pila libre=%u palabras", (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
    }
}
