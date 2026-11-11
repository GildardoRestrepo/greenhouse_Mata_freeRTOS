---
type: pruebas
created: 2026-11-08
---
# Pruebas y evidencias

| ID | Qué se prueba | Procedimiento | Resultado esperado | Resultado observado | Evidencia |
|---|---|---|---|---|---|
| P01 | Arranque y UART | Reset, abrir monitor serie 115200 | Banner + `INIT→MONITOREO` | | |
| P02 | Bus I²C | Escaneo al arranque | 0x40 (HTU21) y 0x3C (OLED) | | |
| P03 | Calibración suelo | Sensor en aire / en agua | crudo seco ≈ ?, mojado ≈ ? | 2026-11-11: aire = 3604, agua = 1187 (leídos en el depurador, `soil_raw`) | |
| P04 | Riego automático | Sacar la sonda de la tierra | RIEGO, el LED de riego (PC6) da pulsos de 1 s cada ≈ 16 s (`PUMP_PULSE_MS` + `SOAK_MS`) | 2026-11-11: conforme, visto en la placa (LED PC6, RGB y OLED); sin log UART (no hay adaptador USB-TTL) | |
| P05 | Fin de riego | Meter sensor en agua | vuelve a MONITOREO | | |
| P06 | Riego sin efecto | Dejar la sonda seca durante RIEGO (no hay bomba real: nadie la moja) | 3 pulsos del LED → ALARMA "sin agua" | 2026-11-11: conforme, alarma al tercer pulso, vista en la placa; sin log UART | |
| P07 | Sensor desconectado | Desconectar AOUT del sensor de suelo | ALARMA, bomba bloqueada | | |
| P08 | HTU21 desconectado / CRC inválido | Quitar el módulo | ALARMA "aire" tras 3 fallos | | |
| P09 | Botones / modos | K_UP, K1, K0 | MANUAL ⇄ MONITOREO, riego manual, cambio de pantalla | | |
| P10 | Preempción | K1 durante refresco de OLED, con `DISPLAY_TRACE_FRAMES = 1` (log por UART; ver también P30) | Control y Actuators cortan a Display: sus líneas caen entre `DSP frame start` y `DSP frame end` | | |
| P11 | Pilas | High-water mark tras 5 min | ≥ 25 % libre en cada tarea | | |
| P12 | Estabilidad | 5+ min continuos con ciclos de riego | sin reinicios ni bloqueos | | |

## Pruebas por módulo (drivers y tareas de Marco)

Las líneas `SEN …` salen por UART cada 2 s con `SENSORS_LOG_READINGS = 1` (`app_config.h`):
`SEN s=<crudo suelo>/<%> l=<crudo luz>/<%> T=<centésimas °C> H=<centésimas %HR> m=<valid_mask>`.

| ID | Qué se prueba | Procedimiento | Resultado esperado | Resultado observado | Evidencia |
|---|---|---|---|---|---|
| P13 | `drv_htu21`: lectura, CRC y punto fijo | Mirar `T=` y `H=` en el log; echar aliento al sensor; tocarlo con el dedo | `T` plausible (p. ej. `2350` = 23,50 °C), `H` sube varios puntos con el aliento y `T` con el dedo; `m=7`; ningún fallo de CRC | | |
| P14 | `drv_htu21`: funciones puras | Llamar una vez en el arranque (o en el depurador): `htu21_crc8({0x68,0x3A},2)`, `htu21_crc8({0x4E,0x85},2)`, `htu21_raw_to_temp_cC(0x683A)`, `htu21_raw_to_hum_cPct(0x4E85)` | `0x7C` y `0x6B` (ejemplos de la hoja de datos), `2468` (24,7 °C) y `3233` (32,3 %) | Lógica replicada en Python el 2026-11-10: `0x7C`, `0x6B`, `2468`, `3233`. Falta en la placa | |
| P15 | `drv_ssd1306`: init, texto y barras | Encender con la OLED conectada | Sin "nieve" al arrancar; encabezado `BROTE` + estado, línea bajo el encabezado, texto nítido y sin espejo; las barras de Suelo y Luz se llenan en proporción al % | | foto |
| P16 | `drv_analog`: calibración del suelo | Sonda YL-69 al aire, luego en un vaso de agua; anotar el crudo `s=` y ponerlo en `SOIL_RAW_DRY` / `SOIL_RAW_WET` | Tras calibrar: aire ≈ 0 %, agua ≈ 100 %, estable (±2 %) gracias al promedio | 2026-11-11: `SOIL_RAW_DRY = 3604`, `SOIL_RAW_WET = 1187`; con esos valores aire ≈ 0 % y agua ≈ 100 % | |
| P17 | `drv_analog`: LDR | Tapar la LDR con el dedo; alumbrarla con la linterna del celular; anotar `l=` en `LIGHT_RAW_DARK` / `LIGHT_RAW_BRIGHT` | Tapada ≈ 0 %, linterna ≈ 100 % | 2026-11-11: tapada con el dedo = 987, linterna = 3531; `LIGHT_RAW_DARK = 987`, `LIGHT_RAW_BRIGHT = 3531`; porcentajes correctos tras recompilar | |
| P18 | `drv_analog`: sensor de suelo desconectado | Quitar el cable AOUT (PC0 queda con el pull-down de 100 kΩ) | `s=` < 200, `/-1`, `m` pierde el bit 0 (p. ej. `m=6`); la OLED muestra `--` en Suelo | | |
| P19 | `tSensors`: periodo y eventos | Log de tControl con el tick de cada `EV_READING` (o pin de traza de tSensors en el analizador) | Un `EV_READING` cada 500 ms ± 1 ms sin deriva, **también en los ciclos en que se mide el aire** (el HTU21 se lee después de enviar el evento); línea `SEN` cada 2 s, ≈ 75 ms después del evento de ese ciclo | | captura |
| P20 | `tSensors`: fallo del aire | Desconectar SDA del HTU21 con el sistema andando; reconectar | Tras 3 lecturas fallidas (≈ 6 s) **un solo** `EV_SENSOR_FAULT` (`FAULT_AIR`), `m` pierde el bit 2 y la OLED muestra `--`; suelo y luz siguen midiendo. Al reconectar, `m=7` de nuevo | | |
| P21 | `tDisplay`: pantallas | Pulsar K0 varias veces | Rota: resumen → suelo en grande con umbrales → diagnóstico (tiempo activo, edad del dato < 1000 ms, `Sensores SLA`) | | fotos |
| P22 | `tDisplay`: alarma | Provocar una alarma (P07) | Rótulo `ALARMA` parpadeando en negativo cada 0,5 s + causa (`Sensor de suelo`) + `K1: reconocer`, en cualquier pantalla | | video |
| P23 | `tDisplay`: OLED desconectada | Quitar la OLED con el sistema andando; reconectarla | Log `DSP OLED perdida` tras ≈ 1 s; **el resto del sistema sigue** (lecturas, riego). Al reconectar, `DSP OLED lista` en ≤ 2 s y la pantalla vuelve sola | | |
| P24 | `mtxI2C`: bus compartido | 5 min con OLED refrescando y HTU21 midiendo | Ningún error de CRC ni de bus, pantalla sin corrupción; `m=7` todo el tiempo | | |
| P25 | Pilas de tSensors y tDisplay | Líneas `SEN pila libre=` y `DSP pila libre=` (cada 10 s) tras 5 min | ≥ 64 de 256 palabras (tSensors) y ≥ 96 de 384 (tDisplay), es decir ≥ 25 % libre | | |
| P26 | `tLogger`: UART por DMA | Reset con el monitor serie abierto (115200 8N1) | Banner `== Brote ...`; cada línea empieza con el tick; nunca aparece `LOG perdidos=` en operación normal; `LOG pila libre=` cada 10 s | | |
| P27 | `tControl`: arranque | Reset con todo conectado | `CTL inicio, estado INIT` → `CTL INIT->MONITOREO` en ≤ 1 s; LED RGB blanco con un bip y luego verde | | |
| P28 | `state_machine`: riego automático | Sacar la sonda de la tierra (seco) | `CTL MONITOREO->RIEGO`, `ACT bomba ON` y `ACT bomba OFF` separados 1000 ms; el LED de PC6 se enciende 1 s; RGB azul "respirando"; nuevo pulso cada 16 s mientras siga seco | | |
| P29 | `state_machine`: riego sin efecto | Dejar la sonda seca durante RIEGO | Tras el 3.er pulso sin que suba ≥ 3 puntos: `CTL RIEGO->ALARMA causa=sin agua`; RGB rojo parpadeando, bip cada 2 s, PC6 apagado | | |
| P30 | Botones y preempción | En MONITOREO: K_UP, luego K1 dos veces, luego K_UP; K0 varias veces | `CTL MONITOREO->MANUAL`; cada K1 da un pulso (ignorado si hay uno en curso); `CTL MANUAL->MONITOREO`; K0 rota pantallas. En cada botón `t - t_isr` ≤ 1 ms aunque tDisplay esté dibujando | | |
| P31 | Reconocer alarma | En ALARMA "suelo" (quitar AO), pulsar K1; reconectar AO y pulsar K1 | Primer K1: solo un bip, sigue en ALARMA; segundo K1: `CTL ALARMA->MONITOREO` | | |
| P32 | Timeout de sensores | Pausar tSensors en el depurador (o hacer que no arranque) > 3 s | `CTL MONITOREO->ALARMA causa=timeout`; la bomba queda bloqueada | | |
| P33 | Pilas de tControl, tActuators y tLogger | Líneas `CTL/ACT/LOG pila libre=` tras 5 min | ≥ 25 % libre en cada una | | |
