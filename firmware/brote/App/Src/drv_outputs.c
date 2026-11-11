/**
 * @file drv_outputs.c
 * @brief LED de riego, buzzer y LED RGB por PWM (TIM4, ARR = 999 → 1 kHz).
 */
#include "drv_outputs.h"

#include "main.h"           /* etiquetas de pines generadas por CubeMX */
#include "tim.h"            /* htim4 (generado por CubeMX, par .c/.h) */

/* Si en el .ioc no se pusieron las etiquetas sugeridas, se usan los pines del mapa
   (docs/propuesta.md §3). */
#ifndef PUMP_LED_Pin
#define PUMP_LED_Pin        GPIO_PIN_6
#define PUMP_LED_GPIO_Port  GPIOC
#endif
#ifndef BUZZER_Pin
#define BUZZER_Pin          GPIO_PIN_7
#define BUZZER_GPIO_Port    GPIOC
#endif

#define OUT_PWM_FULL        1000U   /* CCR = ARR + 1 → 100 % */

static uint32_t out_scale(uint16_t permil)
{
    return (permil > OUT_PWM_FULL) ? OUT_PWM_FULL : permil;
}

bool out_init(void)
{
    bool ok = true;

    out_pump(false);
    out_buzzer(false);
    out_rgb(0U, 0U, 0U);

    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1) != HAL_OK) {
        ok = false;
    }
    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2) != HAL_OK) {
        ok = false;
    }
    if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3) != HAL_OK) {
        ok = false;
    }
    return ok;
}

void out_pump(bool on)
{
    HAL_GPIO_WritePin(PUMP_LED_GPIO_Port, PUMP_LED_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void out_buzzer(bool on)
{
    /* Módulo con PNP: PC7 en bajo = suena. */
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void out_rgb(uint16_t r, uint16_t g, uint16_t b)
{
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, out_scale(r));
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, out_scale(g));
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, out_scale(b));
}
