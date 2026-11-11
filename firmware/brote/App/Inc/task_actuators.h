/**
 * @file task_actuators.h
 * @brief Tarea tActuators: bomba (LED de riego), buzzer y LED RGB (arquitectura.md §2).
 *
 * Recibe órdenes de tControl como thread flags (notificación de tarea); al
 * terminar un pulso de riego envía EV_PUMP_DONE a qEvents.
 */
#ifndef TASK_ACTUATORS_H
#define TASK_ACTUATORS_H

#include <stdint.h>

/* Órdenes (bits; se pueden combinar en una sola llamada a act_send). */
#define CMD_PUMP_PULSE      0x0001U     /* un pulso de PUMP_PULSE_MS como máximo */
#define CMD_PUMP_STOP       0x0002U     /* cortar la bomba ya */
#define CMD_BEEP            0x0004U     /* un bip corto */
#define CMD_LED_STATE       0x0008U     /* cambió el estado: releer g_snapshot */
#define CMD_ALL             (CMD_PUMP_PULSE | CMD_PUMP_STOP | CMD_BEEP | CMD_LED_STATE)

/** @brief Envía órdenes a tActuators (no bloquea). Solo la llama tControl. */
void act_send(uint32_t cmds);

/**
 * @brief Reacción a un desborde de pila: bomba y buzzer apagados, LED rojo fijo.
 *        No retorna. Se llama desde vApplicationStackOverflowHook (freertos.c).
 */
void act_fatal_stack_overflow(void);

/**
 * @brief Cuerpo de tActuators. Definida "As weak" en CubeMX (freertos.c); esta es
 *        la implementación real. Prioridad osPriorityHigh.
 */
void StartActuatorsTask(void *argument);

#endif /* TASK_ACTUATORS_H */
