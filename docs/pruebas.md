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
