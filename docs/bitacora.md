---
type: bitacora
created: 2026-11-08
---
# Bitácora del equipo

Entradas nuevas **arriba**. Formato: fecha · quién · hecho · pendiente · decisiones · problemas.

---

## 2026-11-09 · MARCO (+ Claude) — driver SHT30 adelantado (rama `feat/sht30`)
- **Hecho:** `App/Inc/drv_sht30.h` y `App/Src/drv_sht30.c` según [[arquitectura]] §6: `sht30_init()` (ACK en 0x44, para el autotest) y `sht30_read(&temp_cC, &hum_cPct)` (0x2400 → soltar `mtxI2C` → `osDelay(20)` → leer 6 bytes → CRC-8 → punto fijo). Devuelve `SHT30_OK / ERR_MUTEX / ERR_BUS / ERR_CRC`; el conteo de "3 fallos seguidos" y la validación de rango quedan en `tSensors`.
- **INTERFAZ:** se crea `App/Inc/app_config.h` (compartido) solo con `I2C_MUTEX_TIMEOUT_MS`, `SHT30_MEAS_DELAY_MS` y `SHT30_I2C_TIMEOUT_MS`. Estevan: añade ahí tus parámetros; si ya tienes tu versión, se fusionan sin problema.
- **Supuestos sobre el proyecto base (Estevan, confirmar al crear el `.ioc`):** el mutex se llama `mtxI2C` en CubeMX → handle `mtxI2CHandle`; periféricos generados como par `.c/.h` → `i2c.h` exporta `hi2c1`; tick de 1 ms (`configTICK_RATE_HZ = 1000`).
- **Pendiente:** **sin compilar** (no hay proyecto base todavía); probar P02/P08/P08b con el hardware. Siguen `drv_ssd1306`, `drv_analog`, `task_sensors`, `task_display`.
- **Problemas:** —

## 2026-11-08 · GILBERT (+ Claude) — BOM y cambio de sensor
- **INTERFAZ:** el AHT20 se reemplaza por un **SHT30** (I²C 0x44, CRC-8 por medida, mejor en ambiente húmedo). Driver `drv_sht30.c`, prefijo `sht30_` (antes `drv_aht20.c` / `aht20_`). Mismo bus I2C1 y mismo `mtxI2C`. Procedimiento de lectura en [[arquitectura]] §6.
- **INTERFAZ:** el buzzer es un **módulo con PNP integrado, activo en bajo**, alimentado a 3V3 y conectado directo a PC7 (sin 2N2222). En `drv_outputs.c`: sonar = `GPIO_PIN_RESET`, PC7 inicia en alto.
- **Hecho:** `bom.md` con links y precios (Ardobot), revisado. TIP120 agotado → **TIP122** (mismo circuito).
- **Pendiente:** confirmar que el SHT30 de Ardobot es módulo de 4 pines con pull-ups (la página no lo dice); conseguir manguera de silicona de 6 mm interior (no la vende Ardobot: acuario o ferretería).

## 2026-11-08 · GILBERT (+ Claude)
- **Hecho:** lectura del enunciado y del esquemático de la placa (STM32_F4VE). Propuesta (`propuesta.md`), arquitectura (`arquitectura.md`), `CLAUDE.md` y `.claude/settings.json`.
- **Decisiones:** sensor de suelo capacitivo (ADC), AHT20 + OLED compartiendo I2C1 (justifica `mtxI2C`), bomba con TIP120 y fuente 5 V aparte, 5 tareas / 5 prioridades, UART con dueño único (tLogger), tareas CubeMX "As weak" con implementación en `App/`.
- **Planta:** árbol de jade pequeño → umbrales de suculenta (seco 15 %, húmedo 35 %, pulsos de 1 s).
- **Pendiente:** Marco revisa la propuesta; comprar componentes (lunes); crear `.ioc` y proyecto base.
- **Problemas:** —
