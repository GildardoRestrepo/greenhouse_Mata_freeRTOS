/**
 * @file app_shared.h
 * @brief Objetos RTOS compartidos y acceso protegido a g_snapshot.
 *
 * Archivo COMPARTIDO. Los handles los crea CubeMX (freertos.c) a partir de los
 * nombres del .ioc: cola "qEvents", mutex "mtxI2C" y "mtxData".
 */
#ifndef APP_SHARED_H
#define APP_SHARED_H

#include <stdbool.h>

#include "app_types.h"
#include "cmsis_os2.h"

extern osMessageQueueId_t qEventsHandle;    /* 8 × app_event_t → tControl */
extern osMutexId_t        mtxI2CHandle;     /* bus I2C1 (HTU21 + OLED) */
extern osMutexId_t        mtxDataHandle;    /* g_snapshot */

/**
 * @brief Copia g_snapshot a *out con mtxData tomado.
 * @return false si no se obtuvo el mutex (out no se modifica).
 */
bool shared_get(snapshot_t *out);

/**
 * @brief Reemplaza g_snapshot por *in con mtxData tomado. Solo la llama tControl.
 * @return false si no se obtuvo el mutex (g_snapshot no cambia).
 */
bool shared_set(const snapshot_t *in);

#endif /* APP_SHARED_H */
