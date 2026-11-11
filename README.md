---
type: readme
created: 2026-10-21
---

# Reto 4 — Programación en alto nivel

> [!resumen] Resumen
> **Brote**: mini-invernadero con un pequeño árbol de jade (*Crassula ovata*). Mide humedad del suelo, luz y temperatura/humedad del aire; "riega" sola (por presupuesto la bomba es un LED indicador en PC6), muestra todo en una OLED, indica el estado con un LED RGB y avisa fallos con un buzzer. STM32F407VET6 + HAL + FreeRTOS (CMSIS-RTOS v2).

**Curso:** Microprocesadores — Universidad Pontificia Bolivariana
**Integrantes:** Gildardo Estevan Restrepo · Marco Aurelio Guardia

---

## Estructura del repositorio

```
CLAUDE.md            reglas compartidas para los agentes (y para nosotros)
docs/
  enunciado.md       reto y rúbrica
  propuesta.md       idea, compras, pines, plan de montaje, cronograma
  arquitectura.md    tareas, prioridades, sincronización, estados, CubeMX
  bom.md             lista de materiales y compra real
  cableado.md        diagrama y lista de conexiones para armar
  pruebas.md         plan y resultados de pruebas
  bitacora.md        historial de trabajo y decisiones
  datasheets/        esquemático de la placa y RM0090
firmware/brote/      proyecto STM32CubeIDE (brote.ioc, Core/, App/)
assets/              fotos, capturas, video
```

---

## Reparto de trabajo

| Estevan | Marco |
|---|---|
| `.ioc` y proyecto base, máquina de estados, actuadores, logger | drivers HTU21/OLED/ADC, tareas de sensores y pantalla, montaje y calibración |

Detalle en [[CLAUDE]] y [[propuesta]].

---

## Arquitectura y guía

5 tareas FreeRTOS: `tActuators` (High) · `tControl` (AboveNormal) · `tSensors` (Normal, 500 ms) · `tDisplay` (BelowNormal, 250 ms) · `tLogger` (Low). Ver [[arquitectura]].

---

## Hardware y conexiones

Suelo → PC0 (ADC) · LDR → PC1 (ADC) · HTU21 + OLED → I2C1 PB6/PB7 · LED de riego (bomba simulada) → PC6 · Buzzer → PC7 (módulo, activo en alto) · LED RGB → PD12–14 (TIM4 PWM) · UART → PA9/PA10. Tabla completa en [[propuesta]].

---

## Estado (2026-11-11)

Firmware corriendo en la placa con todo conectado. Calibración de suelo y luz medida; riego automático y alarma "sin agua" probados (ver [[pruebas]]). Pendiente: adaptador USB-TTL para la telemetría por UART, nivel inicial bajo de PC7 en el `.ioc` y las pruebas de desconexión, preempción, pilas y estabilidad.

---

## Enlaces
- [[reto_4_os]]

