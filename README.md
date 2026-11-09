---
type: readme
created: 2026-10-21
---

# Reto 4 — Programación en alto nivel

> [!resumen] Resumen
> **Brote**: mini-invernadero con un pequeño árbol de jade (*Crassula ovata*). Mide humedad del suelo, luz y temperatura/humedad del aire; riega sola con una mini-bomba, muestra todo en una OLED, indica el estado con un LED RGB y avisa fallos con un buzzer. STM32F407VET6 + HAL + FreeRTOS (CMSIS-RTOS v2).

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
  pruebas.md         plan y resultados de pruebas
  bitacora.md        historial de trabajo y decisiones
  datasheets/        esquemático de la placa y RM0090
firmware/brote/      proyecto STM32CubeIDE (.ioc, Core/, App/)
assets/              fotos, capturas, video
```

---

## Reparto de trabajo

| Estevan | Marco |
|---|---|
| `.ioc` y proyecto base, máquina de estados, actuadores, logger | drivers AHT20/OLED/ADC, tareas de sensores y pantalla, montaje y calibración |

Detalle en [[CLAUDE]] y [[propuesta]].

---

## Arquitectura y guía

5 tareas FreeRTOS: `tActuators` (High) · `tControl` (AboveNormal) · `tSensors` (Normal, 500 ms) · `tDisplay` (BelowNormal, 250 ms) · `tLogger` (Low). Ver [[arquitectura]].

---

## Hardware y conexiones

Suelo → PC0 (ADC) · LDR → PC1 (ADC) · AHT20 + OLED → I2C1 PB6/PB7 · Bomba → PC6 (TIP120) · Buzzer → PC7 · LED RGB → PD12–14 (TIM4 PWM) · UART → PA9/PA10. Tabla completa en [[propuesta]].

---

```

```

---

## Enlaces
- [[reto_4_os]]

