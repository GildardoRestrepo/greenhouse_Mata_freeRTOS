/**
 * @file state_machine.c
 * @brief Transiciones de Brote (arquitectura.md §4). Lógica pura: sin HAL ni RTOS.
 *
 * Resumen:
 *   INIT      --primera lectura con suelo válido-->            MONITOREO
 *   MONITOREO --suelo < SOIL_DRY_PCT-->                         RIEGO (pulso)
 *   RIEGO     --suelo >= SOIL_WET_PCT-->                        MONITOREO
 *   RIEGO     --pulso + SOAK_MS sin llegar al umbral-->         otro pulso
 *   RIEGO     --MAX_PULSES_NO_RISE pulsos sin subir-->          ALARMA (sin agua)
 *   MONITOREO/RIEGO --K_UP--> MANUAL --K_UP--> MONITOREO; en MANUAL, K1 = pulso
 *   cualquiera --fallo, timeout o temperatura > máx-->          ALARMA (bomba cortada)
 *   ALARMA    --K1 y causa resuelta-->                          MONITOREO
 */
#include "state_machine.h"

#include "app_config.h"

/* Un pulso se da por terminado aunque se pierda EV_PUMP_DONE (defensa). */
#define SM_PUMP_BUSY_MAX_MS     (2U * PUMP_PULSE_MS)

static bool sm_soil_valid(const readings_t *r)
{
    return ((r->valid_mask & READ_VALID_SOIL) != 0U) && (r->soil_pct >= 0);
}

static bool sm_air_valid(const readings_t *r)
{
    return (r->valid_mask & READ_VALID_AIR) != 0U;
}

static bool sm_overtemp(const readings_t *r)
{
    return sm_air_valid(r) && (r->temp_cC > TEMP_MAX_cC);
}

static bool sm_pump_busy(const sm_ctx_t *ctx, uint32_t now)
{
    return ctx->pump_on && ((now - ctx->pulse_tick) < SM_PUMP_BUSY_MAX_MS);
}

static uint8_t sm_goto(sm_ctx_t *ctx, sys_state_t next)
{
    if (ctx->state == next) {
        return SM_ACT_NONE;
    }
    ctx->state = next;
    return SM_ACT_STATE_CHG;
}

/* Entra a ALARMA: corta la bomba y avisa. Si ya estaba en alarma, conserva la primera causa. */
static uint8_t sm_alarm(sm_ctx_t *ctx, fault_code_t fault)
{
    if (ctx->state == ST_ALARM) {
        return SM_ACT_NONE;
    }
    ctx->fault = fault;
    ctx->pump_on = false;
    return (uint8_t)(sm_goto(ctx, ST_ALARM) | SM_ACT_PUMP_STOP | SM_ACT_BEEP);
}

static uint8_t sm_pulse(sm_ctx_t *ctx, uint32_t now)
{
    ctx->pump_on = true;
    ctx->pulse_tick = now;
    ctx->soil_at_pulse = ctx->last.soil_pct;
    return SM_ACT_PUMP_PULSE;
}

/* ¿Se resolvió la causa de la alarma? (para reconocerla con K1). */
static bool sm_cause_resolved(const sm_ctx_t *ctx, uint32_t now)
{
    const readings_t *r = &ctx->last;

    switch (ctx->fault) {
    case FAULT_SOIL:     return sm_soil_valid(r);
    case FAULT_ADC:      return sm_soil_valid(r) && ((r->valid_mask & READ_VALID_LIGHT) != 0U);
    case FAULT_AIR:      return sm_air_valid(r);
    case FAULT_TIMEOUT:  return (now - ctx->last_reading_tick) < SENSOR_TIMEOUT_MS;
    case FAULT_OVERTEMP: return sm_air_valid(r) && (r->temp_cC <= (TEMP_MAX_cC - TEMP_HYST_cC));
    case FAULT_NO_WATER: return true;   /* el usuario revisó el agua al reconocer */
    case FAULT_NONE:
    default:             return true;
    }
}

/* Decisión de riego con cada lectura en RIEGO. */
static uint8_t sm_irrigate_reading(sm_ctx_t *ctx, uint32_t now)
{
    const readings_t *r = &ctx->last;
    uint8_t act;

    if (!sm_soil_valid(r)) {
        return SM_ACT_NONE;     /* el fallo de suelo llega aparte como SM_EV_FAULT */
    }
    if (r->soil_pct >= SOIL_WET_PCT) {
        act = sm_pump_busy(ctx, now) ? SM_ACT_PUMP_STOP : SM_ACT_NONE;
        ctx->pump_on = false;
        return (uint8_t)(act | sm_goto(ctx, ST_MONITOR));
    }
    /* Pulso + absorción cumplidos y todavía seco: ¿sirvió el pulso anterior? */
    if ((now - ctx->pulse_tick) >= (PUMP_PULSE_MS + SOAK_MS)) {
        if (r->soil_pct >= (int16_t)(ctx->soil_at_pulse + SOIL_RISE_MIN_PCT)) {
            ctx->pulses_no_rise = 0;
        } else if (ctx->pulses_no_rise < UINT8_MAX) {
            ctx->pulses_no_rise++;
        }
        if (ctx->pulses_no_rise >= MAX_PULSES_NO_RISE) {
            return sm_alarm(ctx, FAULT_NO_WATER);
        }
        return sm_pulse(ctx, now);
    }
    return SM_ACT_NONE;
}

static uint8_t sm_on_reading(sm_ctx_t *ctx, const sm_input_t *in)
{
    ctx->last = in->r;
    ctx->last_reading_tick = in->now;

    if ((ctx->state != ST_ALARM) && (ctx->state != ST_INIT) && sm_overtemp(&ctx->last)) {
        return sm_alarm(ctx, FAULT_OVERTEMP);
    }

    switch (ctx->state) {
    case ST_INIT:
        /* Autotest: la primera lectura debe traer el suelo válido. */
        if (!sm_soil_valid(&ctx->last)) {
            return sm_alarm(ctx, FAULT_SOIL);
        }
        return sm_goto(ctx, ST_MONITOR);

    case ST_MONITOR:
        if (sm_soil_valid(&ctx->last) && (ctx->last.soil_pct < SOIL_DRY_PCT)) {
            ctx->pulses_no_rise = 0;
            return (uint8_t)(sm_goto(ctx, ST_IRRIGATE) | sm_pulse(ctx, in->now));
        }
        return SM_ACT_NONE;

    case ST_IRRIGATE:
        return sm_irrigate_reading(ctx, in->now);

    case ST_MANUAL:
    case ST_ALARM:
    default:
        return SM_ACT_NONE;
    }
}

void sm_init(sm_ctx_t *ctx, uint32_t now)
{
    ctx->state = ST_INIT;
    ctx->fault = FAULT_NONE;
    ctx->last.soil_pct = -1;
    ctx->last.light_pct = -1;
    ctx->last.temp_cC = 0;
    ctx->last.hum_cPct = 0;
    ctx->last.valid_mask = 0;
    ctx->last_reading_tick = now;   /* el timeout de sensores cuenta desde el arranque */
    ctx->pump_on = false;
    ctx->pulse_tick = now;
    ctx->soil_at_pulse = -1;
    ctx->pulses_no_rise = 0;
}

uint8_t sm_step(sm_ctx_t *ctx, const sm_input_t *in)
{
    switch (in->ev) {
    case SM_EV_READING:
        return sm_on_reading(ctx, in);

    case SM_EV_FAULT:
        return sm_alarm(ctx, in->fault);

    case SM_EV_SENSOR_TIMEOUT:
        return sm_alarm(ctx, FAULT_TIMEOUT);

    case SM_EV_PUMP_DONE:
        ctx->pump_on = false;
        return SM_ACT_NONE;

    case SM_EV_BTN_KUP:
        if ((ctx->state == ST_MONITOR) || (ctx->state == ST_IRRIGATE)) {
            uint8_t act = sm_pump_busy(ctx, in->now) ? SM_ACT_PUMP_STOP : SM_ACT_NONE;
            ctx->pump_on = false;
            return (uint8_t)(act | sm_goto(ctx, ST_MANUAL));
        }
        if (ctx->state == ST_MANUAL) {
            uint8_t act = sm_pump_busy(ctx, in->now) ? SM_ACT_PUMP_STOP : SM_ACT_NONE;
            ctx->pump_on = false;
            return (uint8_t)(act | sm_goto(ctx, ST_MONITOR));
        }
        return SM_ACT_NONE;

    case SM_EV_BTN_K1:
        if (ctx->state == ST_MANUAL) {
            return sm_pump_busy(ctx, in->now) ? SM_ACT_NONE : sm_pulse(ctx, in->now);
        }
        if (ctx->state == ST_ALARM) {
            if (sm_cause_resolved(ctx, in->now)) {
                ctx->fault = FAULT_NONE;
                return sm_goto(ctx, ST_MONITOR);
            }
            return SM_ACT_BEEP;     /* la causa sigue: aviso y se queda en ALARMA */
        }
        return SM_ACT_NONE;

    default:
        return SM_ACT_NONE;
    }
}

const char *sm_state_name(sys_state_t state)
{
    switch (state) {
    case ST_INIT:       return "INIT";
    case ST_MONITOR:    return "MONITOREO";
    case ST_IRRIGATE:   return "RIEGO";
    case ST_MANUAL:     return "MANUAL";
    case ST_ALARM:      return "ALARMA";
    default:            return "?";
    }
}
