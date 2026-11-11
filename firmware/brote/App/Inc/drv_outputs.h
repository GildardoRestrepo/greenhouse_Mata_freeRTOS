/**
 * @file drv_outputs.h
 * @brief Salidas físicas: LED de riego (bomba simulada, PC6), buzzer (PC7) y
 *        LED RGB (TIM4 CH1..CH3 en PD12..PD14).
 *
 * Capa drv_*: usa HAL. Solo la llama tActuators (CLAUDE.md: es la única que
 * toca bomba, buzzer y LED RGB).
 */
#ifndef DRV_OUTPUTS_H
#define DRV_OUTPUTS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Deja todo apagado y arranca el PWM del RGB.
 * @return false si HAL_TIM_PWM_Start falló en algún canal.
 */
bool out_init(void);

/** @brief LED de riego (ocupa el lugar de la bomba): true = "regando". */
void out_pump(bool on);

/** @brief Buzzer del módulo (activo en bajo): true = suena. */
void out_buzzer(bool on);

/** @brief LED RGB, cada canal en milésimas de brillo (0..1000). */
void out_rgb(uint16_t r, uint16_t g, uint16_t b);

#endif /* DRV_OUTPUTS_H */
