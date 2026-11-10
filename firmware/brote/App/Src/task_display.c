/**
 * @file task_display.c
 * @brief tDisplay: dibuja el estado y las lecturas en la OLED. Solo muestra, no decide.
 *
 * Cada DISPLAY_PERIOD_MS (osDelayUntil):
 *   1. copia g_snapshot con mtxData (shared_get) → la pantalla nunca mezcla
 *      lecturas viejas y nuevas;
 *   2. dibuja en el framebuffer (RAM, sin bus);
 *   3. envía el cuadro por I2C con mtxI2C (ssd1306_update, ≈ 24 ms).
 *
 * Es la tarea más larga y por eso tiene prioridad baja: tControl y tActuators
 * la desalojan a mitad de cuadro (demostración de preempción, arquitectura §5).
 *
 * Los textos se arman con un formateador entero propio: sin printf de float y
 * sin el consumo de pila de snprintf.
 */
#include "task_display.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"           /* uxTaskGetStackHighWaterMark */

#include "app_config.h"
#include "app_shared.h"
#include "app_types.h"
#include "cmsis_os2.h"
#include "drv_ssd1306.h"
#include "font_6x8.h"
#include "task_logger.h"    /* log_printf */

#define DISPLAY_STACK_REPORT_FRAMES     (STACK_REPORT_MS / DISPLAY_PERIOD_MS)

/* ---- Distribución de la pantalla (píxeles) ---- */
#define DISPLAY_LINE_MAX    (SSD1306_WIDTH / FONT_CELL_W)   /* 21 caracteres por línea */
#define HEADER_RULE_Y       9       /* línea bajo el encabezado */
#define ROW1_Y              13
#define ROW2_Y              25
#define ROW3_Y              39
#define ROW4_Y              51
#define VALUE_X             36      /* columna donde empiezan barras y valores */
#define BAR_W               62
#define BAR_H               7
#define BIG_SCALE           3       /* número grande de la pantalla de suelo */
#define BIG_Y               24
#define FOOT_Y              55
#define ALARM_SCALE         2
#define ALARM_BOX_X         20
#define ALARM_BOX_Y         13
#define ALARM_BOX_W         88
#define ALARM_BOX_H         22
#define ALARM_TEXT_Y        16
#define ALARM_CAUSE_Y       40

/* ---- Constructor de líneas de texto (solo enteros) ---- */
typedef struct {
    char    buf[DISPLAY_LINE_MAX + 1];
    uint8_t len;
} line_t;

static void line_reset(line_t *l)
{
    l->len = 0;
    l->buf[0] = '\0';
}

static void line_putc(line_t *l, char c)
{
    if (l->len < DISPLAY_LINE_MAX) {
        l->buf[l->len++] = c;
        l->buf[l->len] = '\0';
    }
}

static void line_puts(line_t *l, const char *s)
{
    while (*s != '\0') {
        line_putc(l, *s++);
    }
}

static void line_put_uint(line_t *l, uint32_t v)
{
    char digits[10];    /* 2^32 - 1 tiene 10 dígitos */
    uint8_t n = 0;

    do {
        digits[n++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v != 0U);
    while (n > 0U) {
        line_putc(l, digits[--n]);
    }
}

static void line_put_int(line_t *l, int32_t v)
{
    if (v < 0) {
        line_putc(l, '-');
        v = -v;
    }
    line_put_uint(l, (uint32_t)v);
}

/* Centésimas → un decimal con redondeo: 2356 → "23.6", -50 → "-0.5". */
static void line_put_centi(line_t *l, int32_t centi)
{
    if (centi < 0) {
        line_putc(l, '-');
        centi = -centi;
    }
    uint32_t tenths = ((uint32_t)centi + 5U) / 10U;

    line_put_uint(l, tenths / 10U);
    line_putc(l, '.');
    line_putc(l, (char)('0' + (tenths % 10U)));
}

/* ---- Textos ---- */
static const char *display_state_name(sys_state_t state)
{
    switch (state) {
    case ST_INIT:       return "INICIO";
    case ST_MONITOR:    return "MONITOREO";
    case ST_IRRIGATE:   return "RIEGO";
    case ST_MANUAL:     return "MANUAL";
    case ST_ALARM:      return "ALARMA";
    default:            return "?";
    }
}

static const char *display_fault_text(uint8_t fault_code)
{
    switch ((fault_code_t)fault_code) {
    case FAULT_SOIL:        return "Sensor de suelo";
    case FAULT_AIR:         return "Sensor de aire";
    case FAULT_ADC:         return "Error de ADC";
    case FAULT_TIMEOUT:     return "Sensores mudos";
    case FAULT_NO_WATER:    return "Sin agua";
    case FAULT_OVERTEMP:    return "Temperatura alta";
    case FAULT_NONE:
    default:                return "Causa desconocida";
    }
}

/* ---- Piezas de dibujo ---- */
static void display_text_right(int16_t y, const char *text)
{
    ssd1306_draw_text((int16_t)(SSD1306_WIDTH - ssd1306_text_width(text, 1)), y, text, 1);
}

static void display_text_center(int16_t y, const char *text, uint8_t scale)
{
    ssd1306_draw_text((int16_t)((SSD1306_WIDTH - ssd1306_text_width(text, scale)) / 2), y,
                      text, scale);
}

static void display_draw_header(const snapshot_t *s)
{
    ssd1306_draw_text(0, 0, "BROTE", 1);
    display_text_right(0, display_state_name(s->state));
    ssd1306_fill_rect(0, HEADER_RULE_Y, SSD1306_WIDTH, 1, SSD1306_WHITE);
}

/* Fila "etiqueta + barra + porcentaje"; si no es válida muestra "--" y la barra vacía. */
static void display_draw_meter(int16_t y, const char *label, int16_t pct, bool valid)
{
    line_t line;

    line_reset(&line);
    if (valid) {
        line_put_int(&line, pct);
        line_putc(&line, '%');
    } else {
        line_puts(&line, "--");
    }
    ssd1306_draw_text(0, y, label, 1);
    ssd1306_draw_bar(VALUE_X, y, BAR_W, BAR_H, valid ? pct : -1);
    display_text_right(y, line.buf);
}

/* ---- Pantallas ---- */

/* Pantalla 0: resumen de las cuatro magnitudes. */
static void display_screen_main(const snapshot_t *s)
{
    bool air_ok = (s->r.valid_mask & READ_VALID_AIR) != 0U;
    line_t line;

    display_draw_meter(ROW1_Y, "Suelo", s->r.soil_pct, (s->r.valid_mask & READ_VALID_SOIL) != 0U);
    display_draw_meter(ROW2_Y, "Luz", s->r.light_pct, (s->r.valid_mask & READ_VALID_LIGHT) != 0U);

    ssd1306_draw_text(0, ROW3_Y, "Temp", 1);
    line_reset(&line);
    if (air_ok) {
        line_put_centi(&line, s->r.temp_cC);
        line_putc(&line, ' ');
        line_putc(&line, FONT_CHAR_DEGREE);
        line_putc(&line, 'C');
    } else {
        line_puts(&line, "--");
    }
    ssd1306_draw_text(VALUE_X, ROW3_Y, line.buf, 1);

    ssd1306_draw_text(0, ROW4_Y, "Hum", 1);
    line_reset(&line);
    if (air_ok) {
        line_put_centi(&line, s->r.hum_cPct);
        line_puts(&line, " %");
    } else {
        line_puts(&line, "--");
    }
    ssd1306_draw_text(VALUE_X, ROW4_Y, line.buf, 1);
}

/* Pantalla 1: humedad del suelo en grande y los umbrales de riego. */
static void display_screen_soil(const snapshot_t *s)
{
    line_t line;

    ssd1306_draw_text(0, ROW1_Y, "Humedad del suelo", 1);

    line_reset(&line);
    if ((s->r.valid_mask & READ_VALID_SOIL) != 0U) {
        line_put_int(&line, s->r.soil_pct);
        line_putc(&line, '%');
    } else {
        line_puts(&line, "--");
    }
    display_text_center(BIG_Y, line.buf, BIG_SCALE);

    line_reset(&line);
    line_puts(&line, "riega<");
    line_put_int(&line, SOIL_DRY_PCT);
    line_puts(&line, "  para>=");
    line_put_int(&line, SOIL_WET_PCT);
    display_text_center(FOOT_Y, line.buf, 1);
}

/* Pantalla 2: diagnóstico (tiempo encendido, edad del dato, sensores válidos, fallo). */
static void display_screen_system(const snapshot_t *s)
{
    uint32_t now = osKernelGetTickCount();
    line_t line;

    line_reset(&line);
    line_puts(&line, "Activo ");
    line_put_uint(&line, now / APP_MS_TO_TICKS(1000U));
    line_puts(&line, " s");
    ssd1306_draw_text(0, ROW1_Y, line.buf, 1);

    line_reset(&line);
    line_puts(&line, "Dato hace ");
    line_put_uint(&line, now - s->tick);    /* resta sin signo: correcta aunque el tick desborde */
    line_puts(&line, " ms");
    ssd1306_draw_text(0, ROW2_Y, line.buf, 1);

    line_reset(&line);
    line_puts(&line, "Sensores ");
    line_putc(&line, ((s->r.valid_mask & READ_VALID_SOIL) != 0U) ? 'S' : '-');
    line_putc(&line, ((s->r.valid_mask & READ_VALID_LIGHT) != 0U) ? 'L' : '-');
    line_putc(&line, ((s->r.valid_mask & READ_VALID_AIR) != 0U) ? 'A' : '-');
    ssd1306_draw_text(0, ROW3_Y, line.buf, 1);

    line_reset(&line);
    line_puts(&line, "Falla ");
    line_put_uint(&line, s->fault_code);
    ssd1306_draw_text(0, ROW4_Y, line.buf, 1);
}

static void display_screen_init(void)
{
    display_text_center(BIG_Y, "Brote", ALARM_SCALE);
    display_text_center(ROW4_Y, "iniciando...", 1);
}

/* ALARMA tapa cualquier pantalla: rótulo que parpadea en negativo + causa. */
static void display_screen_alarm(const snapshot_t *s, uint32_t frame)
{
    display_text_center(ALARM_TEXT_Y, "ALARMA", ALARM_SCALE);
    if (((frame / DISPLAY_BLINK_FRAMES) & 1U) != 0U) {
        ssd1306_fill_rect(ALARM_BOX_X, ALARM_BOX_Y, ALARM_BOX_W, ALARM_BOX_H, SSD1306_INVERT);
    }
    display_text_center(ALARM_CAUSE_Y, display_fault_text(s->fault_code), 1);
    display_text_center(FOOT_Y, "K1: reconocer", 1);
}

static void display_render(const snapshot_t *s, uint32_t frame)
{
    ssd1306_clear();
    display_draw_header(s);

    if (s->state == ST_INIT) {
        display_screen_init();
    } else if (s->state == ST_ALARM) {
        display_screen_alarm(s, frame);
    } else {
        switch (s->screen % DISPLAY_NUM_SCREENS) {
        case 0:
            display_screen_main(s);
            break;
        case 1:
            display_screen_soil(s);
            break;
        default:
            display_screen_system(s);
            break;
        }
    }
}

void StartDisplayTask(void *argument)
{
    (void)argument;

    /* Si shared_get falla (mtxData ocupado) se redibuja la última copia buena. */
    snapshot_t snap = { .state = ST_INIT };
    uint32_t frame = 0;
    uint8_t fails = 0;

    /* El panel necesita un momento tras el encendido antes de aceptar comandos. */
    osDelay(APP_MS_TO_TICKS(DISPLAY_BOOT_DELAY_MS));

    bool ready = (ssd1306_init() == SSD1306_OK);
    if (!ready) {
        log_printf("DSP OLED no responde en el arranque");
    }

    uint32_t next_wake = osKernelGetTickCount();

    for (;;) {
        (void)shared_get(&snap);

        if (ready) {
            display_render(&snap, frame);
#if DISPLAY_TRACE_FRAMES
            log_printf("DSP frame start t=%lu", (unsigned long)osKernelGetTickCount());
#endif
            ssd1306_status_t st = ssd1306_update();
#if DISPLAY_TRACE_FRAMES
            log_printf("DSP frame end t=%lu", (unsigned long)osKernelGetTickCount());
#endif
            if (st == SSD1306_OK) {
                fails = 0;
            } else if (++fails >= DISPLAY_FAIL_REINIT) {
                ready = false;
                log_printf("DSP OLED perdida (error %d), reintentando", (int)st);
            }
        } else if ((frame % DISPLAY_REINIT_FRAMES) == 0U) {
            /* Reconexión en caliente: se reintenta el init cada 2 s. */
            if (ssd1306_init() == SSD1306_OK) {
                ready = true;
                fails = 0;
                log_printf("DSP OLED lista");
            }
        }

        frame++;
        if ((frame % DISPLAY_STACK_REPORT_FRAMES) == 0U) {
            log_printf("DSP pila libre=%u palabras",
                       (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }

        next_wake += APP_MS_TO_TICKS(DISPLAY_PERIOD_MS);
        (void)osDelayUntil(next_wake);
    }
}
