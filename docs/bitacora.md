---
type: bitacora
created: 2026-11-08
---
# Bitácora del equipo

Entradas nuevas **arriba**. Formato: fecha · quién · hecho · pendiente · decisiones · problemas.

---

## 2026-11-09 · MARCO (+ Claude) — drivers y tareas de Marco adelantados (rama `feat/drivers-sensores`)
Todo está en `firmware/brote/App/{Inc,Src}`. **Nada se ha compilado**: no existe todavía el proyecto base ni hay toolchain en el PC de Marco. Solo se verificó en Python la lógica del CRC, las conversiones del SHT30 y el dibujo de la fuente.

- **Hecho:**
  - `drv_sht30`: `sht30_init()` (ACK en 0x44) y `sht30_read(&temp_cC, &hum_cPct)` (0x2400 → soltar `mtxI2C` → `osDelay(20)` → leer 6 bytes → CRC-8 → punto fijo).
  - `drv_ssd1306` + `font_6x8`: framebuffer `static` de 1 KB, `init`, `clear`, `update`, píxel, rectángulos, texto con escala y barras.
  - `drv_analog`: suelo (PC0/IN10) y LDR (PC1/IN11), promedio de 8 muestras, calibración lineal desde `app_config.h`, suelo desconectado si crudo < `SOIL_RAW_MIN`.
  - `task_sensors` (`StartSensorsTask`): 500 ms con `osDelayUntil`, SHT30 cada 4 ciclos, validación de rango físico, `EV_READING` / `EV_SENSOR_FAULT` a `qEvents`.
  - `task_display` (`StartDisplayTask`): 250 ms, `shared_get()` + 3 pantallas (resumen, suelo en grande, diagnóstico), pantalla de INIT y de ALARMA parpadeante, reconexión en caliente de la OLED.
  - `docs/pruebas.md`: P13–P25 (una o más por módulo). `docs/arquitectura.md` §3 y §6 actualizados con lo de abajo.
- **INTERFAZ (archivos compartidos creados por Marco; Estevan, revisa):**
  - `app_types.h`: lo de [[arquitectura]] §3 tal cual, **más** `fault_code_t` (`FAULT_NONE/SOIL/AIR/ADC/TIMEOUT/NO_WATER/OVERTEMP`), los bits `READ_VALID_SOIL/LIGHT/AIR` y `snapshot_t { state, r, fault_code, screen, tick }`.
  - `app_config.h`: valores "Demo" de §4, periodos, tamaños de colas, prioridades/pilas (solo como referencia: mandan las del `.ioc`) y los parámetros de mis drivers.
  - `app_shared.h/.c`: `g_snapshot` + `shared_get()/shared_set()` con `mtxData`, y los `extern` de `qEventsHandle`, `mtxI2CHandle`, `mtxDataHandle`.
  - **Contrato de tSensors → tControl:** `EV_READING` en cada ciclo de 500 ms con `valid_mask`; `EV_SENSOR_FAULT` **una sola vez** al confirmarse el fallo. Para "causa resuelta" tControl mira el `valid_mask` de las lecturas siguientes.
  - **Lo que tDisplay espera de tControl:** que `shared_set()` mantenga `state`, `r`, `tick`, `fault_code` (causa de la alarma) y que incremente `screen` con K0.
- **Supuestos sobre el proyecto base (Estevan, confirmar al crear el `.ioc`; cada uno es un cambio de una línea si no coincide):**
  - Nombres en CubeMX: cola `qEvents`, mutex `mtxI2C` y `mtxData` → handles `qEventsHandle`, `mtxI2CHandle`, `mtxDataHandle`. Funciones de entrada `StartSensorsTask` y `StartDisplayTask`, *As weak*.
  - Periféricos generados como par `.c/.h`: `i2c.h` exporta `hi2c1`, `adc.h` exporta `hadc1`. ADC1 con una sola conversión por software (el driver cambia de canal).
  - `task_logger.h` declara `void log_printf(const char *fmt, ...);`.
  - Tick de 1 ms (`configTICK_RATE_HZ = 1000`).
- **Decisiones:**
  - La OLED se envía **página por página** soltando `mtxI2C` entre una y otra: tSensors espera el bus ≈ 3 ms como máximo, no los ≈ 24 ms del cuadro.
  - Si la OLED falla, tDisplay solo lo registra en el log y reintenta cada 2 s; **no** genera alarma (una pantalla dañada no debe parar el riego).
  - tDisplay arma los textos con un formateador entero propio (sin `snprintf`) para cuidar la pila.
  - El ADC se lee con `HAL_ADC_PollForConversion`: la conversión dura ≈ 25 µs, mucho menos que un tick, así que no hay forma útil de bloquearse (justificación en `drv_analog.c`).
- **Pendiente:** compilar e integrar cuando llegue el proyecto base; calibrar (`SOIL_RAW_*` y `LIGHT_RAW_*` son provisionales); ejecutar P13–P25; medir pilas (P25). Para la captura de preempción, poner `DISPLAY_TRACE_FRAMES = 1`; para la demo, `SENSORS_LOG_READINGS = 0` si el log estorba.
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

---

## Pendiente de Marco
Decisiones que el agente no tomó solo (2026-11-09). Nada de esto bloquea la integración.

1. **Fallo del SHT30 → ¿alarma o solo aviso?** [[arquitectura]] §4 lo deja en "decidir". tSensors ya envía `FAULT_AIR`; falta acordar con Estevan si tControl pasa a ALARMA o sigue regando sin dato de aire.
2. **Lectura del ADC por sondeo.** Son ≈ 0,2 ms de espera activa por lectura cada 500 ms. Si prefieres que no haya ni una sola espera activa que explicar ante la rúbrica, la alternativa es ADC en modo scan + DMA con notificación desde el callback, pero exige cambios en el `.ioc` (Estevan).
3. **Antirrebote del sensor de suelo.** `SOIL_FAULT_COUNT = 1` (alarma al primer ciclo con el sensor desconectado, como dice §4). Subirlo a 2–3 si en las pruebas aparecen falsas alarmas.
4. **Pantallas.** Revisa en la OLED real si te gustan las 3 pantallas y los textos (`task_display.c`); la orientación se cambia con `SSD1306_ROTATE_180`, y si el módulo resulta ser SH1106, `SSD1306_COL_OFFSET = 2`.
5. **Compilador en este PC.** El agente intentó instalar uno en una carpeta temporal para revisar la sintaxis y el permiso fue denegado. Si instalas STM32CubeIDE aquí, puede compilar en cuanto llegue el proyecto base.
