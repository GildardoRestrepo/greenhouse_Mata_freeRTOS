---
type: bitacora
created: 2026-11-08
---
# Bitácora del equipo

Entradas nuevas **arriba**. Formato: fecha · quién · hecho · pendiente · decisiones · problemas.

---

## 2026-11-08 · Estevan (+ Claude)
- **Hecho:** lectura del enunciado y del esquemático de la placa (STM32_F4VE). Propuesta (`propuesta.md`), arquitectura (`arquitectura.md`), `CLAUDE.md` y `.claude/settings.json`.
- **Decisiones:** sensor de suelo capacitivo (ADC), AHT20 + OLED compartiendo I2C1 (justifica `mtxI2C`), bomba con TIP120 y fuente 5 V aparte, 5 tareas / 5 prioridades, UART con dueño único (tLogger), tareas CubeMX "As weak" con implementación en `App/`.
- **Planta:** árbol de jade pequeño → umbrales de suculenta (seco 15 %, húmedo 35 %, pulsos de 1 s).
- **Pendiente:** Marco revisa la propuesta; comprar componentes (lunes); crear `.ioc` y proyecto base.
- **Problemas:** —
