---
type: bitacora
created: 2026-11-08
---
# Bitácora del equipo

Entradas nuevas **arriba**. Formato: fecha · quién · hecho · pendiente · decisiones · problemas.

---

## 2026-11-10 · MARCO (+ Claude) — el sensor de aire pasa de SHT30 a HTU21 (GY-21), rama `feat/htu21`
- **Motivo:** no había SHT30 en la tienda; se compró el **HTU21D en módulo GY-21**.
- **INTERFAZ (cambian el archivo y las funciones del driver):** `drv_sht30.*` → **`drv_htu21.*`**, prefijo `sht30_` → **`htu21_`**. Ya no existe `sht30_read(&t, &h)`: ahora son `htu21_read_temp(&temp_cC)` y `htu21_read_hum(&hum_cPct)` por separado, más `htu21_init()`. Estados `HTU21_OK / ERR_MUTEX / ERR_BUS / ERR_CRC`. Gildardo: si `task_control.c`, el autotest de INIT o algún documento tuyo nombran `sht30_*` o la dirección 0x44, hay que cambiarlos.
- **INTERFAZ (`app_config.h`):** `SHT30_EVERY_N_CYCLES` → `HTU21_EVERY_N_CYCLES`; `SHT30_MEAS_DELAY_MS` → `HTU21_TEMP_DELAY_MS` (55) y `HTU21_HUM_DELAY_MS` (20); `SHT30_I2C_TIMEOUT_MS` → `HTU21_I2C_TIMEOUT_MS`. Se añadió `#include <stdint.h>` (nota de Gildardo en el PR #1).
- **No cambia:** bus I2C1 (PB6/PB7) y `mtxI2C`, `readings_t` (`temp_cC`, `hum_cPct`), `FAULT_AIR`, el contrato de eventos de tSensors y el periodo de 2 s del aire.
- **Hecho:**
  - `drv_htu21`: dirección **0x40**; comandos no-hold `0xF3` (temperatura) y `0xF5` (humedad); cada medida hace comando → soltar `mtxI2C` → `osDelay` → tomar → leer 3 bytes; CRC-8 poli 0x31 con **init 0x00**; máscara `raw & 0xFFFC`; `temp_cC = -4685 + 17572*raw/65536`, `hum_cPct = -600 + 12500*raw/65536`.
  - `task_sensors.c`: dos lecturas seguidas (≈ 75 ms en total, bloqueada sin espera activa); el dato de aire solo vale si las dos salen bien.
  - `propuesta.md`, `arquitectura.md` (§1, §2, §4, §6), `bom.md`, `pruebas.md` (P02, P08, P13, P14, P20, P24), `README.md` y `CLAUDE.md` al día.
- **Decisiones:**
  - Esperas de **55 y 20 ms** en vez de los 50 y 16 ms nominales: `osDelay(n)` puede durar hasta un tick menos que `n`, y leer antes de tiempo da NACK.
  - La humedad se **satura a 0..100 %** en el driver: la fórmula da de −6 % a 119 % y, sin recortar, un invernadero cerrado cerca de la saturación daría lecturas "fuera de rango" y falsas `FAULT_AIR`.
- **Verificado (solo lógica, en Python):** CRC de los ejemplos de la hoja de datos (`0xDC`→`0x79`, `0x683A`→`0x7C`, `0x4E85`→`0x6B`) y conversiones (`0x683A`→24,68 °C, `0x4E85`→32,33 %). **Sigue sin compilar**: no hay proyecto base.
- **Pendiente:** probar P13/P14/P20/P24 con el módulo real. La compra completa costó **$59.160** (anotado en [[bom]]; no se desglosó el precio del GY-21).
- **Problemas:** el SHT30 no se consiguió (sirve para "problema técnico resuelto": el cambio quedó aislado en el driver y dos constantes, sin tocar tipos ni tareas vecinas).

## 2026-11-10 · MARCO (+ Claude) — BOM reducido: sensor resistivo y bomba simulada con LED
- **Motivo:** presupuesto ajustado. La lista pasa de ≈ $120.000 a ≈ $47.150 ([[bom]]).
- **INTERFAZ (cableado; afecta `task_actuators` / `drv_outputs` de Gildardo):** la **bomba real se reemplaza por un LED indicador en PC6** (LED rojo + 220 Ω a GND, activo en alto). Gildardo **ya aprobó** este cambio. Salen el TIP122, el 1N4007, el condensador, la manguera y la fuente de 5 V. El firmware no cambia: mismo pin, mismos nombres (`CMD_PUMP_PULSE`, `PUMP_PULSE_MS`, `EV_PUMP_DONE`) y mismo pulso con tiempo máximo.
- **INTERFAZ (sensor de suelo; afecta `drv_analog` de Marco):** el capacitivo se reemplaza por el **resistivo YL-69 + módulo YL-38**, salida **AO** a PC0, alimentado a 3,3 V. Lee al revés (seco ≈ máximo del ADC, mojado = bajo); `analog_raw_to_pct()` ya admite ese sentido, pero `SOIL_RAW_DRY`, `SOIL_RAW_WET` y `SOIL_RAW_MIN` de `app_config.h` siguen con valores pensados para el capacitivo y hay que medirlos (P16).
- **Otros cambios:** LDR de 12 mm → 5 mm; protoboard fuera de la lista (ya la tenemos).
- **Consecuencias para la demo:**
  - Sin bomba el suelo no se moja solo: el ciclo de riego se cierra mojando la sonda a mano. Si nadie la moja, tras 3 pulsos hay ALARMA "sin agua" (P06 se reescribió así).
  - Si la sonda YL-69 se suelta del módulo YL-38, la lectura no se distingue de "suelo seco"; solo se detecta el cable AO suelto (pull-down de 100 kΩ). Queda en la tabla de errores de [[arquitectura]] §4.
- **Hecho:** `bom.md` reescrito; `propuesta.md` §1–§4, `arquitectura.md` (nota inicial, diagrama, §2, §4, §6) y `pruebas.md` (P04, P06, P16) puestos al día. `CLAUDE.md` en commit aparte (regla de cargas).
- **Pendiente:** actualizar los comentarios de `app_config.h` y `drv_analog.h` que todavía dicen "capacitivo" (código, va en otra rama); calibrar con el YL-69.
- **Problemas:** —

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
6. **(2026-11-10) Resistencias de 220 Ω.** Hacen falta cuatro (tres del LED RGB y una del LED de riego) y la lista reducida compra dos: confirmar que ya hay otras dos.
7. ~~**(2026-11-10) Precio de los jumpers.**~~ Resuelto: la compra completa costó $59.160.
