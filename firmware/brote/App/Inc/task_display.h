/**
 * @file task_display.h
 * @brief Tarea tDisplay: dibuja las pantallas en la OLED (arquitectura.md §2).
 */
#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

/**
 * @brief Cuerpo de tDisplay. Definida "As weak" en CubeMX (freertos.c); esta es
 *        la implementación real. Prioridad osPriorityBelowNormal, periodo 250 ms.
 */
void StartDisplayTask(void *argument);

#endif /* TASK_DISPLAY_H */
