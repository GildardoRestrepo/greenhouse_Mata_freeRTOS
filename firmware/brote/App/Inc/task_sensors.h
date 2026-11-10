/**
 * @file task_sensors.h
 * @brief Tarea tSensors: lectura periódica de suelo, luz y aire (arquitectura.md §2).
 */
#ifndef TASK_SENSORS_H
#define TASK_SENSORS_H

/**
 * @brief Cuerpo de tSensors. Definida "As weak" en CubeMX (freertos.c); esta es
 *        la implementación real. Prioridad osPriorityNormal, periodo 500 ms.
 */
void StartSensorsTask(void *argument);

#endif /* TASK_SENSORS_H */
