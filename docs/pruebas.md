---
type: pruebas
created: 2026-11-08
---
# Pruebas y evidencias

| ID | Qué se prueba | Procedimiento | Resultado esperado | Resultado observado | Evidencia |
|---|---|---|---|---|---|
| P01 | Arranque y UART | Reset, abrir monitor serie 115200 | Banner + `INIT→MONITOREO` | | |
| P02 | Bus I²C | Escaneo al arranque | 0x44 (SHT30) y 0x3C (OLED) | | |
| P03 | Calibración suelo | Sensor en aire / en agua | crudo seco ≈ ?, mojado ≈ ? | | |
| P04 | Riego automático | Sacar sensor de la tierra | RIEGO, pulsos de 1 s cada 15 s | | |
| P05 | Fin de riego | Meter sensor en agua | vuelve a MONITOREO | | |
| P06 | Tanque vacío | Sacar la bomba del agua, suelo seco | 3 pulsos → ALARMA "sin agua" | | |
| P07 | Sensor desconectado | Desconectar AOUT del sensor de suelo | ALARMA, bomba bloqueada | | |
| P08 | SHT30 desconectado / CRC inválido | Quitar el módulo | ALARMA "aire" tras 3 fallos | | |
| P09 | Botones / modos | K_UP, K1, K0 | MANUAL ⇄ MONITOREO, riego manual, cambio de pantalla | | |
| P10 | Preempción | K1 durante refresco de OLED, analizador en PE7–PE11 | Control y Actuators cortan a Display | | |
| P11 | Pilas | High-water mark tras 5 min | ≥ 25 % libre en cada tarea | | |
| P12 | Estabilidad | 5+ min continuos con ciclos de riego | sin reinicios ni bloqueos | | |

## Pruebas por módulo (drivers y tareas de Marco)

Las líneas `SEN …` salen por UART cada 2 s con `SENSORS_LOG_READINGS = 1` (`app_config.h`):
`SEN s=<crudo suelo>/<%> l=<crudo luz>/<%> T=<centésimas °C> H=<centésimas %HR> m=<valid_mask>`.

| ID | Qué se prueba | Procedimiento | Resultado esperado | Resultado observado | Evidencia |
|---|---|---|---|---|---|
| P13 | `drv_sht30`: lectura, CRC y punto fijo | Mirar `T=` y `H=` en el log; echar aliento al sensor; tocarlo con el dedo | `T` plausible (p. ej. `2350` = 23,50 °C), `H` sube varios puntos con el aliento y `T` con el dedo; `m=7`; ningún fallo de CRC | | |
| P14 | `drv_sht30`: funciones puras | Llamar una vez en el arranque (o en el depurador): `sht30_crc8({0xBE,0xEF},2)`, `sht30_raw_to_temp_cC(0x6666)`, `sht30_raw_to_hum_cPct(0x6666)` | `0x92` (valor de la hoja de datos), `2500`, `4000` | Lógica replicada en Python el 2026-11-09: `0x92`, `2500`, `4000`. Falta en la placa | |
| P15 | `drv_ssd1306`: init, texto y barras | Encender con la OLED conectada | Sin "nieve" al arrancar; encabezado `BROTE` + estado, línea bajo el encabezado, texto nítido y sin espejo; las barras de Suelo y Luz se llenan en proporción al % | | foto |
| P16 | `drv_analog`: calibración del suelo | Sensor al aire, luego en un vaso de agua (hasta la línea); anotar el crudo `s=` y ponerlo en `SOIL_RAW_DRY` / `SOIL_RAW_WET` | Tras calibrar: aire ≈ 0 %, agua ≈ 100 %, estable (±2 %) gracias al promedio | | |
| P17 | `drv_analog`: LDR | Tapar la LDR con el dedo; alumbrarla con la linterna del celular; anotar `l=` en `LIGHT_RAW_DARK` / `LIGHT_RAW_BRIGHT` | Tapada ≈ 0 %, linterna ≈ 100 % | | |
| P18 | `drv_analog`: sensor de suelo desconectado | Quitar el cable AOUT (PC0 queda con el pull-down de 100 kΩ) | `s=` < 200, `/-1`, `m` pierde el bit 0 (p. ej. `m=6`); la OLED muestra `--` en Suelo | | |
| P19 | `tSensors`: periodo y eventos | Log de tControl con el tick de cada `EV_READING` (o pin de traza de tSensors en el analizador) | Un `EV_READING` cada 500 ms ± 1 ms sin deriva; línea `SEN` cada 2 s | | captura |
| P20 | `tSensors`: fallo del aire | Desconectar SDA del SHT30 con el sistema andando; reconectar | Tras 3 lecturas fallidas (≈ 6 s) **un solo** `EV_SENSOR_FAULT` (`FAULT_AIR`), `m` pierde el bit 2 y la OLED muestra `--`; suelo y luz siguen midiendo. Al reconectar, `m=7` de nuevo | | |
| P21 | `tDisplay`: pantallas | Pulsar K0 varias veces | Rota: resumen → suelo en grande con umbrales → diagnóstico (tiempo activo, edad del dato < 1000 ms, `Sensores SLA`) | | fotos |
| P22 | `tDisplay`: alarma | Provocar una alarma (P07) | Rótulo `ALARMA` parpadeando en negativo cada 0,5 s + causa (`Sensor de suelo`) + `K1: reconocer`, en cualquier pantalla | | video |
| P23 | `tDisplay`: OLED desconectada | Quitar la OLED con el sistema andando; reconectarla | Log `DSP OLED perdida` tras ≈ 1 s; **el resto del sistema sigue** (lecturas, riego). Al reconectar, `DSP OLED lista` en ≤ 2 s y la pantalla vuelve sola | | |
| P24 | `mtxI2C`: bus compartido | 5 min con OLED refrescando y SHT30 midiendo | Ningún error de CRC ni de bus, pantalla sin corrupción; `m=7` todo el tiempo | | |
| P25 | Pilas de tSensors y tDisplay | Líneas `SEN pila libre=` y `DSP pila libre=` (cada 10 s) tras 5 min | ≥ 64 de 256 palabras (tSensors) y ≥ 96 de 384 (tDisplay), es decir ≥ 25 % libre | | |
