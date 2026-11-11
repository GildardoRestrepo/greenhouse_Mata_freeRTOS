/**
 * @file task_logger.h
 * @brief Tarea tLogger: único dueño de USART1 (arquitectura.md §2 y §3).
 *
 * Cualquier tarea imprime con log_printf(): el texto se formatea en la pila de
 * quien llama, se encola en qLog sin bloquear y tLogger lo envía por DMA.
 * Nadie más llama HAL_UART_Transmit ni printf.
 */
#ifndef TASK_LOGGER_H
#define TASK_LOGGER_H

#include "app_config.h"

/* Elemento de la cola qLog (Item Size en CubeMX = log_msg_t). */
typedef struct {
    char text[LOG_MSG_LEN];     /* "<tick> <mensaje>" terminado en '\0', sin "\r\n" */
} log_msg_t;

/**
 * @brief Formatea y encola un mensaje para la UART. Nunca bloquea: si qLog está
 *        llena el mensaje se descarta y se cuenta (tLogger informa los perdidos).
 *
 * Solo desde tareas (no desde ISR). Sin %f: magnitudes en punto fijo.
 * Los mensajes más largos que LOG_MSG_LEN se truncan.
 */
void log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Cuerpo de tLogger. Definida "As weak" en CubeMX (freertos.c); esta es
 *        la implementación real. Prioridad osPriorityLow, activada por qLog.
 */
void StartLoggerTask(void *argument);

#endif /* TASK_LOGGER_H */
