/**
 * @file app_config.h
 * @brief Constantes ajustables de la aplicación (umbrales, periodos, calibración).
 *
 * Archivo COMPARTIDO: cambios pequeños y avisados (ver CLAUDE.md).
 * Los valores son los de la columna "Demo" de docs/arquitectura.md §4.
 * Ningún número mágico dentro de tareas o drivers: todo lo ajustable va aquí.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

/* Los timeouts de CMSIS-RTOS van en ticks; el proyecto usa tick de 1 ms. */
#define APP_MS_TO_TICKS(ms)         ((uint32_t)(ms))

/* ======================================================================== */
/* Tareas (arquitectura.md §2). Prioridades y pilas se configuran en el     */
/* .ioc; aquí quedan como referencia y DEBEN coincidir con CubeMX.          */
/* ======================================================================== */
#define TASK_ACTUATORS_PRIO         osPriorityHigh
#define TASK_CONTROL_PRIO           osPriorityAboveNormal
#define TASK_SENSORS_PRIO           osPriorityNormal
#define TASK_DISPLAY_PRIO           osPriorityBelowNormal
#define TASK_LOGGER_PRIO            osPriorityLow

#define TASK_ACTUATORS_STACK_WORDS  256U   /* 256: log_printf usa vsnprintf en la pila */
#define TASK_CONTROL_STACK_WORDS    256U
#define TASK_SENSORS_STACK_WORDS    256U
#define TASK_DISPLAY_STACK_WORDS    384U
#define TASK_LOGGER_STACK_WORDS     384U

#define SENSORS_PERIOD_MS           500U    /* periodo de tSensors (osDelayUntil) */
#define HTU21_EVERY_N_CYCLES        4U      /* HTU21 cada 4 ciclos = 2 s */
#define DISPLAY_PERIOD_MS           250U    /* periodo de tDisplay */
#define CONTROL_EVENT_TIMEOUT_MS    1000U   /* timeout de tControl esperando qEvents */
#define STACK_REPORT_MS             10000U  /* cada cuánto reporta cada tarea su pila libre */

/* ======================================================================== */
/* Colas y mutex (arquitectura.md §3)                                       */
/* ======================================================================== */
#define QEVENTS_LEN                 8U      /* elementos app_event_t */
#define QLOG_LEN                    16U     /* elementos log_msg_t */
#define LOG_MSG_LEN                 64U     /* bytes por mensaje de log */

#define I2C_MUTEX_TIMEOUT_MS        100U    /* espera máx. por mtxI2C (un cuadro OLED ≈ 25 ms) */
#define DATA_MUTEX_TIMEOUT_MS       50U     /* espera máx. por mtxData */

/* ======================================================================== */
/* Control: umbrales y tiempos (arquitectura.md §4, valores de demo)        */
/* ======================================================================== */
#define SOIL_DRY_PCT                15      /* % : por debajo entra a RIEGO */
#define SOIL_WET_PCT                35      /* % : por encima sale de RIEGO (histéresis) */
#define PUMP_PULSE_MS               1000U   /* tiempo MÁXIMO de bomba por pulso (≈30 ml) */
#define SOAK_MS                     15000U  /* espera de absorción (real: 30 min) */
#define MAX_PULSES_NO_RISE          3U      /* pulsos sin subir la humedad → alarma "sin agua" */
#define SENSOR_TIMEOUT_MS           3000U   /* sin lecturas → alarma "timeout" */
#define TEMP_MAX_cC                 3500    /* centésimas de °C: sobrecalentamiento */
#define TEMP_HYST_cC                200     /* la alarma de temperatura se reconoce bajo MAX - HYST */
#define SOIL_RISE_MIN_PCT           3       /* subida mínima tras un pulso para contar como "sirvió" */

#define BTN_DEBOUNCE_MS             50U     /* antirrebote en la ISR */
#define BTN_LONG_MS                 1000U   /* pulsación larga de K_UP */

/* ======================================================================== */
/* tActuators: buzzer y LED RGB                                             */
/* ======================================================================== */
#define ACT_ANIM_PERIOD_MS          20U     /* paso de animación del RGB (timeout de la espera) */
#define ACT_BEEP_MS                 120U    /* duración de un bip */
#define ALARM_BEEP_PERIOD_MS        2000U   /* en ALARMA: un bip cada 2 s */
#define ALARM_BLINK_MS              500U    /* en ALARMA: rojo 0,5 s encendido / 0,5 s apagado */
#define RGB_BREATH_PERIOD_MS        2000U   /* en RIEGO: periodo del azul que "respira" */
#define RGB_LEVEL_PERMIL            600U    /* brillo máximo del RGB (milésimas) */

/* ======================================================================== */
/* tLogger                                                                  */
/* ======================================================================== */
#define LOG_TX_TIMEOUT_MS           50U     /* máx. por mensaje: 66 B a 115200 ≈ 6 ms */

/* ======================================================================== */
/* tSensors: validación y fallos                                            */
/* ======================================================================== */
#define TEMP_VALID_MIN_cC           (-1000) /* fuera de -10..60 °C la lectura se descarta */
#define TEMP_VALID_MAX_cC           6000
#define HUM_VALID_MAX_cPct          10000   /* HR > 100 % se descarta */
#define AIR_FAULT_COUNT             3U      /* lecturas de aire fallidas seguidas → FAULT_AIR */
#define SOIL_FAULT_COUNT            1U      /* ciclos con suelo desconectado → FAULT_SOIL */
#define ADC_FAULT_COUNT             3U      /* ciclos con error de ADC → FAULT_ADC */
#define SENSORS_LOG_READINGS        1       /* 1 = log de crudos cada 2 s (calibración, P03) */

/* ======================================================================== */
/* HTU21 (GY-21)                                                            */
/* ======================================================================== */
/* Conversión: máx. 50 ms (T, 14 bits) y 16 ms (HR, 12 bits). osDelay(n) puede durar hasta
   1 tick menos que n, por eso se deja margen: leer antes de tiempo da NACK. */
#define HTU21_TEMP_DELAY_MS         55U     /* espera tras disparar temperatura */
#define HTU21_HUM_DELAY_MS          20U     /* espera tras disparar humedad */
#define HTU21_I2C_TIMEOUT_MS        10U     /* timeout HAL por transacción (duran < 1 ms) */

/* ======================================================================== */
/* Analógicos (ADC1 de 12 bits: PC0 = suelo IN10, PC1 = LDR IN11)           */
/* ======================================================================== */
#define ANALOG_NUM_SAMPLES          8U      /* muestras promediadas por lectura */
#define ANALOG_CONV_TIMEOUT_MS      2U      /* timeout HAL por conversión (dura ≈ 25 µs) */

/* Calibración de la sonda resistiva YL-69 (seco = crudo alto). Medido en P16 (2026-11-11). */
#define SOIL_RAW_DRY                3604U   /* crudo en aire  → 0 %  */
#define SOIL_RAW_WET                1187U   /* crudo en agua  → 100 % */
#define SOIL_RAW_MIN                200U    /* por debajo: desconectado (pull-down 100 kΩ) */

/* LDR arriba (a 3V3) y 10 kΩ a GND: más luz = más voltaje. Medido en P17 (2026-11-11). */
#define LIGHT_RAW_DARK              987U    /* crudo a oscuras → 0 %  */
#define LIGHT_RAW_BRIGHT            3531U   /* crudo con luz fuerte → 100 % */

/* ======================================================================== */
/* OLED SSD1306 y tDisplay                                                  */
/* ======================================================================== */
#define SSD1306_I2C_TIMEOUT_MS      20U     /* timeout HAL por transacción (una página ≈ 3 ms) */
#define SSD1306_CONTRAST            0xCFU   /* 0x00..0xFF */
#define SSD1306_ROTATE_180          0       /* 1 = pantalla montada al revés */
#define SSD1306_COL_OFFSET          0U      /* 2 si el módulo resulta ser SH1106 (1.3") */

#define DISPLAY_BOOT_DELAY_MS       100U    /* espera tras encender antes de inicializar la OLED */
#define DISPLAY_NUM_SCREENS         3U      /* pantallas que rota K0 */
#define DISPLAY_BLINK_FRAMES        2U      /* cuadros por fase del parpadeo de ALARMA (0,5 s) */
#define DISPLAY_FAIL_REINIT         4U      /* cuadros fallidos seguidos → OLED "perdida" */
#define DISPLAY_REINIT_FRAMES       8U      /* cada cuántos cuadros se reintenta el init (2 s) */
#define DISPLAY_TRACE_FRAMES        0       /* 1 = log "DSP frame start/end" (demo de preempción) */

#endif /* APP_CONFIG_H */
