/**
 * @file app_shared.c
 * @brief g_snapshot (estado + últimas lecturas) y su acceso con mtxData.
 *
 * La copia completa dentro de la sección crítica garantiza que un lector nunca
 * ve mezcla de campos viejos y nuevos. La sección es una copia de pocos bytes:
 * no se duerme ni se hace E/S con el mutex tomado.
 */
#include "app_shared.h"

#include "app_config.h"

static snapshot_t g_snapshot = {
    .state = ST_INIT,
    .r = { .soil_pct = -1, .light_pct = -1, .temp_cC = 0, .hum_cPct = 0, .valid_mask = 0 },
    .fault_code = FAULT_NONE,
    .screen = 0,
    .tick = 0
};

bool shared_get(snapshot_t *out)
{
    if (osMutexAcquire(mtxDataHandle, APP_MS_TO_TICKS(DATA_MUTEX_TIMEOUT_MS)) != osOK) {
        return false;
    }
    *out = g_snapshot;
    (void)osMutexRelease(mtxDataHandle);
    return true;
}

bool shared_set(const snapshot_t *in)
{
    if (osMutexAcquire(mtxDataHandle, APP_MS_TO_TICKS(DATA_MUTEX_TIMEOUT_MS)) != osOK) {
        return false;
    }
    g_snapshot = *in;
    (void)osMutexRelease(mtxDataHandle);
    return true;
}
