/**
 * @file task_logger.c
 * @brief tLogger: saca por USART1 (DMA) lo que las demás tareas encolan en qLog.
 *
 * Flujo: osMessageQueueGet(qLog) bloqueada → copia a un buffer estático →
 * HAL_UART_Transmit_DMA → se bloquea en un thread flag que pone
 * HAL_UART_TxCpltCallback (ISR). No hay espera activa: mientras el DMA envía,
 * la CPU queda para las demás tareas.
 */
#include "task_logger.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"           /* uxTaskGetStackHighWaterMark */
#include "usart.h"          /* huart1 (generado por CubeMX, par .c/.h) */

/* Handles creados por CubeMX en freertos.c a partir de los nombres del .ioc. */
extern osMessageQueueId_t qLogHandle;
extern osThreadId_t       tLoggerHandle;

#define LOG_FLAG_TX_DONE    0x0001U     /* lo pone el callback de fin de DMA (o de error) */
#define LOG_EOL             "\r\n"
#define LOG_EOL_LEN         2U

/* Buffer que lee el DMA: estático (no en pila) y vivo hasta el fin de la transferencia. */
static char s_tx_buf[LOG_MSG_LEN + LOG_EOL_LEN];

/* Mensajes descartados porque qLog estaba llena (lo incrementan varias tareas). */
static volatile uint32_t s_dropped;

void log_printf(const char *fmt, ...)
{
    log_msg_t msg;
    va_list args;
    int used;

    used = snprintf(msg.text, sizeof(msg.text), "%lu ",
                    (unsigned long)osKernelGetTickCount());
    if ((used < 0) || ((size_t)used >= sizeof(msg.text))) {
        used = 0;
    }
    va_start(args, fmt);
    (void)vsnprintf(&msg.text[used], sizeof(msg.text) - (size_t)used, fmt, args);
    va_end(args);

    /* Timeout 0: quien registra nunca se bloquea esperando a la UART. */
    if (osMessageQueuePut(qLogHandle, &msg, 0U, 0U) != osOK) {
        (void)__atomic_fetch_add(&s_dropped, 1U, __ATOMIC_RELAXED);
    }
}

/**
 * @brief Envía len bytes de s_tx_buf por DMA y espera (bloqueada) el fin.
 * @return true si el envío terminó a tiempo.
 */
static bool logger_send(size_t len)
{
    uint32_t flags;

    (void)osThreadFlagsClear(LOG_FLAG_TX_DONE);
    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)s_tx_buf, (uint16_t)len) != HAL_OK) {
        return false;
    }
    flags = osThreadFlagsWait(LOG_FLAG_TX_DONE, osFlagsWaitAny,
                              APP_MS_TO_TICKS(LOG_TX_TIMEOUT_MS));
    if ((flags & osFlagsError) != 0U) {
        (void)HAL_UART_AbortTransmit(&huart1);     /* no quedar con el DMA colgado */
        return false;
    }
    return true;
}

/* Copia un texto a s_tx_buf, añade "\r\n" y lo envía. */
static bool logger_send_text(const char *text)
{
    size_t len = strnlen(text, LOG_MSG_LEN - 1U);

    memcpy(s_tx_buf, text, len);
    memcpy(&s_tx_buf[len], LOG_EOL, LOG_EOL_LEN);
    return logger_send(len + LOG_EOL_LEN);
}

void StartLoggerTask(void *argument)
{
    (void)argument;

    static log_msg_t msg;           /* static: 64 B fuera de la pila */
    static char notice[LOG_MSG_LEN];
    uint32_t dropped_reported = 0;
    uint32_t tx_errors = 0;
    uint32_t last_stack_report = osKernelGetTickCount();

    (void)logger_send_text("");
    (void)logger_send_text("== Brote: mini-invernadero STM32F407 + FreeRTOS ==");

    for (;;) {
        /* Bloqueada hasta que haya algo que enviar: no consume CPU. */
        if (osMessageQueueGet(qLogHandle, &msg, NULL, osWaitForever) != osOK) {
            continue;
        }
        msg.text[LOG_MSG_LEN - 1U] = '\0';
        if (!logger_send_text(msg.text)) {
            tx_errors++;
        }

        /* Avisos propios: se envían directo (tLogger no se encola a sí mismo). */
        uint32_t dropped = s_dropped;
        if (dropped != dropped_reported) {
            (void)snprintf(notice, sizeof(notice), "%lu LOG perdidos=%lu errTx=%lu",
                           (unsigned long)osKernelGetTickCount(),
                           (unsigned long)dropped, (unsigned long)tx_errors);
            (void)logger_send_text(notice);
            dropped_reported = dropped;
        }
        if ((osKernelGetTickCount() - last_stack_report) >= APP_MS_TO_TICKS(STACK_REPORT_MS)) {
            last_stack_report = osKernelGetTickCount();
            (void)snprintf(notice, sizeof(notice), "%lu LOG pila libre=%u palabras",
                           (unsigned long)last_stack_report,
                           (unsigned)uxTaskGetStackHighWaterMark(NULL));
            (void)logger_send_text(notice);
        }
    }
}

/* ---- Callbacks de HAL (contexto de ISR): solo API ISR-safe. ---- */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        (void)osThreadFlagsSet(tLoggerHandle, LOG_FLAG_TX_DONE);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1) {
        /* Despierta a tLogger para que no espere hasta el timeout. */
        (void)osThreadFlagsSet(tLoggerHandle, LOG_FLAG_TX_DONE);
    }
}
