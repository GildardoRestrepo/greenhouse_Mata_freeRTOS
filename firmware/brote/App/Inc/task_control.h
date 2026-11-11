/**
 * @file task_control.h
 * @brief Tarea tControl: única dueña del estado del sistema (arquitectura.md §2 y §4).
 *
 * Consume qEvents (lecturas, fallos, botones, fin de riego), alimenta la
 * máquina de estados de state_machine.c, publica g_snapshot y manda órdenes a
 * tActuators. Aquí vive también el callback EXTI de los botones.
 */
#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

/**
 * @brief Cuerpo de tControl. Definida "As weak" en CubeMX (freertos.c); esta es
 *        la implementación real. Prioridad osPriorityAboveNormal.
 */
void StartControlTask(void *argument);

#endif /* TASK_CONTROL_H */
