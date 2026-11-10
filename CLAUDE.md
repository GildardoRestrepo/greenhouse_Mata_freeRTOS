# CLAUDE.md — Brote (mini-invernadero, STM32F407VET6 + FreeRTOS)

Este archivo lo leen **los dos agentes** (el de Estevan y el de Marco). Es el contrato de trabajo compartido. Si algo aquí está mal o desactualizado, propón el cambio en un commit aparte: no lo ignores en silencio.

## Contexto en 30 segundos
- Proyecto final de Microprocesadores (UPB). Equipo: **Gildardo Estevan Restrepo** y **Marco Aurelio Guardia**. Entrega: **miércoles 2026-11-11**.
- Enunciado y rúbrica: `docs/enunciado.md`. Idea, compras y pines: `docs/propuesta.md`. **Diseño de software (fuente de verdad):** `docs/arquitectura.md`.
- Placa: **STM32F407VET6 "negra" (STM32_F4VE)**, esquemático en `docs/datasheets/stm32f407vet6_schematics.pdf`. Manual de referencia RM0090: `docs/datasheets/dm00031020.pdf` (2000 págs., consultar por secciones).
- Herramientas obligatorias: STM32CubeMX (`.ioc`), STM32CubeIDE, HAL de ST, FreeRTOS vía **CMSIS-RTOS v2**.
- Objetivo: sencillo pero vistoso y **sólido frente a la rúbrica** (20 pts arquitectura de tareas, 15 pts preempción/sincronización). Ante la duda, prioriza que funcione y que sea explicable.

## Estructura
```
CLAUDE.md                 ← este archivo
docs/                     ← enunciado, propuesta, arquitectura, pruebas, bitácora (Obsidian-friendly: frontmatter + [[enlaces]])
firmware/brote/           ← proyecto STM32CubeIDE (se crea desde CubeMX)
  brote.ioc
  Core/                   ← GENERADO. Solo editar dentro de /* USER CODE BEGIN */ … /* USER CODE END */
  App/Inc, App/Src        ← NUESTRO código: tareas, drivers, máquina de estados
  Drivers/, Middlewares/  ← HAL y FreeRTOS de ST. NUNCA editar.
assets/                   ← fotos, capturas del analizador lógico, video
```

## Reglas de CubeMX (las más importantes)
1. **El `.ioc` lo edita una sola persona a la vez.** Antes de regenerar, avisar a la pareja; después, commit inmediato que contenga *solo* `.ioc` + archivos regenerados, con mensaje `cubemx: <qué cambió>`. Los conflictos de merge en el `.ioc` o en `Core/` generados se resuelven regenerando, no a mano.
2. Código propio en `Core/` **solo** entre marcadores `USER CODE BEGIN/END`; lo de afuera se borra al regenerar. Lo ideal: en `Core/` solo llamadas de una línea hacia `App/`.
3. Las tareas están definidas en CubeMX como **"As weak"**. La implementación real va en `App/Src/task_<nombre>.c` con la misma firma (`void StartSensorsTask(void *argument)`). No escribir lógica en `freertos.c`.
4. Un agente **no puede** abrir CubeMX: si un cambio requiere tocar el `.ioc` (pin nuevo, periférico, tarea, cola), dilo explícitamente y describe los pasos exactos en CubeMX para que el humano los haga. No edites el `.ioc` como texto salvo cambios triviales y avisando.
5. `App/` debe estar en las rutas de include y de fuentes del proyecto (Properties → C/C++ General → Paths and Symbols / Source Location). Ya configurado en `.cproject`; no romperlo.

## Reglas de FreeRTOS (la rúbrica revisa esto)
- **Prohibido `HAL_Delay()` después de arrancar el scheduler.** Usar `osDelay()` / `osDelayUntil()` (o `vTaskDelay*`). Tareas periódicas: `osDelayUntil` con tick absoluto.
- **Nada de esperas activas.** Si se espera un dato/evento, bloquearse en cola/notificación/semaforo con timeout. Transferencias largas (UART) por DMA + notificación desde el callback.
- **Desde ISR solo API ISR-safe:** `osMessageQueuePut(q, &x, 0, 0)`, `osThreadFlagsSet`, o `xQueueSendFromISR`/`vTaskNotifyGiveFromISR` + `portYIELD_FROM_ISR`. Las IRQ que llamen al RTOS tienen prioridad NVIC numérica **≥ 5** (`configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`).
- **UART tiene un único dueño: `tLogger`.** Cualquier otro módulo que quiera imprimir usa `log_printf()` (encola en `qLog`). Nunca llamar `HAL_UART_Transmit` ni `printf` desde otra tarea.
- **I2C1 se usa solo con `mtxI2C` tomado** (lo comparten HTU21 y OLED). Tomar → transacción → soltar; nunca dormir con el mutex tomado.
- `g_snapshot` (estado + lecturas) solo se accede con `mtxData` o mediante `shared_get()/shared_set()` en `app_shared.c`.
- **Solo `tControl` cambia el estado del sistema.** Las demás tareas informan con eventos en `qEvents`; no deciden.
- **Solo `tActuators` toca bomba, buzzer y LED RGB.** (Desde el 2026-11-10 la "bomba" es un LED indicador en PC6; las reglas no cambian.) La bomba siempre se enciende con tiempo máximo (`PUMP_PULSE_MS`); jamás un "ON" sin timeout.
- Sin buffers grandes en pila; usar `static`. Sin `malloc` después del arranque (los objetos RTOS se crean en el init).
- Sin `printf` de float: magnitudes en enteros de punto fijo (`temp_cC` = centésimas de °C).
- Prioridades, periodos y pilas están en `docs/arquitectura.md` §2. Cambiar uno = actualizar esa tabla en el mismo commit.

## Estilo de código
- C11, HAL de ST. Identificadores en **inglés**, comentarios y documentación en **español**.
- Prefijos por módulo: `sensors_`, `control_`, `act_`, `display_`, `log_`, `htu21_`, `ssd1306_`, `analog_`, `out_`. Tipos `_t`. Constantes y macros en `MAYUSCULAS`. Globales compartidas con prefijo `g_`.
- Toda constante ajustable (umbrales, periodos, calibración) en `App/Inc/app_config.h`, nunca números mágicos dentro de las tareas.
- Toda función HAL que retorne `HAL_StatusTypeDef` se revisa; los errores se convierten en eventos (`EV_SENSOR_FAULT`) o en logs, no se ignoran.
- `state_machine.c` sin dependencias de HAL ni RTOS (entradas: estado + evento; salida: nuevo estado + acciones) para poder explicarla y probarla aislada.
- Indentación 4 espacios, llaves estilo K&R, líneas ≤ 100 columnas.

## Propiedad de módulos (para no pisarse)
| Módulo | Dueño principal |
|---|---|
| `.ioc`, `Core/`, `task_control.c`, `state_machine.c`, `task_actuators.c`, `task_logger.c`, `drv_outputs.c` | Estevan |
| `drv_htu21.c`, `drv_ssd1306.c`, `drv_analog.c`, `task_sensors.c`, `task_display.c`, fuentes | Marco |
| `app_types.h`, `app_config.h`, `app_shared.*`, `docs/arquitectura.md` | **compartidos**: cambios pequeños y avisados |

El dueño puede delegar. Si necesitas cambiar un archivo del otro, hazlo en tu rama y descríbelo en el PR para que lo revise su dueño.

## Git y coordinación entre agentes
- Ramas: `main` siempre compila. Trabajo en `feat/<tema>`, `fix/<tema>`, `docs/<tema>`. PR pequeño hacia `main`; el otro humano (o su agente) revisa.
- Commits en español, imperativo, con prefijo: `feat(sensors): leer HTU21 cada 2 s`, `fix(i2c): liberar mutex en error`, `cubemx: habilitar DMA en USART1`, `docs: tabla de pilas medidas`.
- **Antes de empezar a trabajar:** `git pull`, leer `docs/bitacora.md` (últimas entradas) y este archivo.
- **Al terminar una sesión:** añadir una entrada corta a `docs/bitacora.md`: fecha, quién, qué se hizo, qué quedó pendiente, decisiones tomadas, problemas encontrados (sirven para la sección "problema técnico resuelto" de la demo).
- Si cambias una interfaz compartida (`app_types.h`, firma de un driver, nombre de cola/mutex), decláralo en la bitácora y en el PR con la palabra **INTERFAZ**.
- No commitear `Debug/`, `Release/`, `.metadata/`, `*.launch`, ni `.claude/settings.local.json` (ya en `.gitignore`).
- El proyecto debe abrir en otro PC sin rutas absolutas: no introducir rutas `C:\Users\...` en `.cproject`/`.project`.

## Compilación y prueba
- Compilar: STM32CubeIDE → Project → Build (configuración Debug). Un agente puede intentar build headless con
  `stm32cubeidec.exe --launcher.suppressErrors -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data <workspace> -import firmware/brote -build brote/Debug`
  (ruta del ejecutable depende de la instalación de cada uno; pregúntala si no la conoces). Si no es posible, pide al humano que compile y te pase los errores.
- Telemetría: USART1 (PA9 TX / PA10 RX) 115200 8N1 por adaptador USB-TTL; el humano puede pegar la salida del monitor serie.
- Toda funcionalidad nueva debe tener una prueba observable descrita en `docs/pruebas.md` (entrada, resultado esperado, resultado observado). La rúbrica da 10 pts a pruebas y evidencias.

## Lo que el agente NO debe hacer
- Editar `Drivers/`, `Middlewares/` o código generado fuera de `USER CODE`.
- Inventar pines: el mapa está en `docs/propuesta.md` §3; respeta lo que ya usa la placa (flash SPI1 PB0/PB3-5, SD PC8-12/PD2, LEDs PA6/PA7, botones PA0/PE3/PE4).
- Conectar cargas directo a un pin en ningún diagrama o instrucción: el LED de riego de PC6 va con su resistencia de 220 Ω; si vuelve una bomba real, siempre con TIP122 + diodo y fuente aparte; el buzzer solo a través de su módulo (que trae transistor), alimentado a 3V3.
- Hacer push a `main` o forzar push sin que el humano lo pida.
