---
type: propuesta
created: 2026-11-08
estado: borrador para revisar con Marco
---
# Propuesta — "Mata": mini-invernadero con riego automático

> [!resumen]
> Caja transparente con un pequeño árbol de jade (*Crassula ovata*), que mide humedad del suelo, temperatura/humedad del aire y luz; riega sola con una mini-bomba cuando el suelo está seco, muestra todo en una OLED, indica el estado con un LED RGB y avisa fallos con un buzzer. STM32F407VET6 + HAL + FreeRTOS (CMSIS-RTOS v2).


---

## 1. Definición (sección 2.1 del enunciado)

| # | Punto | Respuesta |
|---|---|---|
| 1 | Nombre y objetivo | **Brote**: mantener la humedad del suelo de una planta en un rango objetivo sin intervención humana, y mostrar el microclima del invernadero. |
| 2 | Usuario y escenario | Persona que cultiva una planta pequeña en interior (escritorio, apartamento) y se ausenta días; o un laboratorio escolar que quiere observar variables de crecimiento. |
| 3 | Entradas | Humedad de suelo (ADC), luz con LDR (ADC), temperatura y humedad del aire (I²C), 3 botones de la placa (GPIO/EXTI). *Opcional:* flotador de nivel del tanque (GPIO). |
| 4 | Salidas | Mini-bomba de agua (GPIO + transistor), pantalla OLED (I²C), LED RGB de estado (PWM TIM4), módulo buzzer activo (GPIO, activo en bajo), telemetría por UART al PC. |
| 5 | Estados | `INIT` → `MONITOREO` ⇄ `RIEGO`, `MANUAL`, `ALARMA` (ver [[arquitectura]]). |
| 6 | Periféricos | ADC1, I2C1, TIM4 PWM, USART1 (+DMA), GPIO/EXTI, TIM6 como timebase HAL. |
| 7 | Tareas FreeRTOS | 5 tareas, 5 niveles de prioridad, colas + notificaciones + mutex (ver [[arquitectura]]). |
| 8 | Demostración | Sacar el sensor del sustrato (o sustrato seco) → riega en pulsos cortos; mojarlo → para. Desconectar un sensor → alarma. Botones cambian de modo. Analizador lógico/UART muestra la preempción. 5+ min continuos. |

**Cumplimiento de mínimos:** entradas de 3 tipos distintos (analógica ADC, digital I²C, botones GPIO) y salidas de 4 tipos (actuador, pantalla, luz PWM, sonido).

---

## 2. Lista de compras (tiendas locales Medellín: Sigma, Electronilab, Didácticas Electrónicas, etc.)

Precios **aproximados** en COP, verificar en tienda. Marquen lo que ya tienen.

### Imprescindible
| Componente | Ref. sugerida | Uso | ≈ COP |
|---|---|---|---|
| Sensor humedad de suelo **capacitivo** | "Capacitive Soil Moisture Sensor v1.2" | Humedad del suelo (ADC). **No** el resistivo de horquilla: se corroe en días. | 10.000 |
| Sensor temp/humedad aire I²C | **SHT30** en módulo con pull-ups (ideal versión con carcasa de malla) | Microclima (I²C, CRC por medida) | 20.000 |
| Pantalla OLED 0.96" I²C | SSD1306 128×64, **4 pines** (VCC GND SCL SDA) | Interfaz | 18.000 |
| Fotorresistencia + R 10 kΩ | LDR 5 mm | Luz (ADC) | 1.500 |
| Mini bomba sumergible brushless 2.5–6 V | 80–120 L/h, salida 7,5 mm ext. + 1 m manguera de silicona **6 mm int.** | Riego | 7.500 |
| Transistor de potencia para la bomba | **TIP122** (TIP120 agotado; mismo circuito y patas). Alternativa: MOSFET **IRLZ44N**, *no* IRF520/IRFZ44N que no conducen con 3.3 V | Etapa de manejo | 1.300 |
| Diodo 1N4007 + R 1 kΩ + cap. 100 µF/16 V | — | Flyback + base + desacople bomba | 1.500 |
| Módulo buzzer activo 3 pines (trae transistor PNP 9012) | — | Alarma sonora | 3.000 |
| LED RGB cátodo común 5 mm + 3× R 220 Ω | — | Estado (PWM) | 1.500 |
| R 100 kΩ ×2 | — | Pull-down en entradas ADC (detectar sensor desconectado) | 200 |
| Fuente 5 V ≥1 A para la bomba | cargador de celular + cable USB cortado, o portapilas 4×AA | Alimentación de potencia | 10.000 |
| Protoboard + jumpers M-H y M-M | — | Montaje | 15.000 |
| Caja plástica transparente con tapa, matera pequeña, frasco (tanque) | — | Invernadero | 20.000 |
| Planta | **Árbol de jade pequeño** (*Crassula ovata*) en matera **con drenaje** y sustrato para cactus/suculentas | — | 15.000 |

### Si no lo tienen ya
| Componente | Uso | ≈ COP |
|---|---|---|
| ST-Link V2 (clon) | Programar/depurar por SWD | 25.000 |
| Adaptador USB-TTL 3.3 V (CP2102 / CH340) | Telemetría UART al PC (la placa **no** trae USB-serial) | 12.000 |

### Muy recomendado (sube puntos en el criterio 4 de la rúbrica)
| Componente | Uso | ≈ COP |
|---|---|---|
| Analizador lógico 8 canales 24 MHz (clon Saleae, usa PulseView) | Evidencia visual de preempción con pines de traza por tarea | 35.000 |
| Sensor de nivel tipo flotador | Detectar tanque vacío directamente | 8.000 |

**Total aproximado:** 115.000–190.000 COP según lo que ya tengan.

---

## 3. Conexiones (placa negra STM32_F4VE)

Pines elegidos para **no chocar** con lo que la placa ya usa (flash SPI1 en PB0/PB3-5, SD en PC8-12/PD2, LEDs en PA6/PA7, botones en PA0/PE3/PE4, USB en PA11/12, SWD en PA13/14).

| Periférico | Señal | Pin STM32 | Recurso | Alimentación / nivel |
|---|---|---|---|---|
| Sensor suelo capacitivo | AOUT | **PC0** | ADC1_IN10 | **3.3 V** (alimentarlo a 5 V puede pasar de 3.3 V en el pin) |
| LDR (divisor con 10 kΩ a GND) | punto medio | **PC1** | ADC1_IN11 | 3.3 V |
| SHT30 | SCL / SDA | **PB6 / PB7** | I2C1 400 kHz, dir. 0x44 | 3.3 V (el módulo trae pull-ups; ADDR a GND) |
| OLED SSD1306 | SCL / SDA | **PB6 / PB7** (mismo bus) | I2C1, dir. 0x3C | 3.3 V |
| Bomba (vía TIP122) | base (R 1 kΩ) | **PC6** | GPIO salida | Bomba a 5 V externa, **GND común** |
| Módulo buzzer (I/O) | señal | **PC7** | GPIO salida push-pull, **activo en bajo** (inicia en alto) | **VCC del módulo a 3V3**, no a 5 V |
| LED RGB | R / G / B (R 220 Ω c/u) | **PD12 / PD13 / PD14** | TIM4_CH1/2/3 PWM | 3.3 V, ~6 mA por canal |
| Botón K0 (placa) | — | PE4 | GPIO EXTI, pull-up, flanco bajada | activo en bajo |
| Botón K1 (placa) | — | PE3 | GPIO EXTI, pull-up, flanco bajada | activo en bajo |
| Botón K_UP (placa) | — | PA0 | GPIO EXTI, pull-down, flanco subida | activo en alto |
| LEDs D2 / D3 (placa) | — | PA6 / PA7 | GPIO salida | activos en bajo |
| USB-TTL | RX / TX | **PA9 (TX) / PA10 (RX)** | USART1 115200 8N1 + DMA TX | 3.3 V, conector J6 |
| Pines de traza (analizador) | 1 por tarea | **PE7–PE11** | GPIO salida | — |
| *Opcional* flotador | — | PE5 | GPIO entrada pull-up | — |
| ST-Link | SWDIO / SWCLK | PA13 / PA14 | SWD | conector JTAG 20p |

### Etapa de la bomba (no conectarla nunca directo al pin)
```
 +5V ext ──┬──────────────┐
           │              │
        1N4007 (cátodo   (+) BOMBA
         hacia +5V)      (−)
           │              │
           └──────┬───────┘
                  C  TIP122
 PC6 ──[1 kΩ]──── B
                  E
                  │
 GND ext ─────────┴──── GND placa   (GND COMÚN obligatorio)
 100 µF entre +5V ext y GND ext, cerca de la bomba.
```
**Buzzer:** el módulo ya trae su propio transistor PNP (9012), así que se conecta directo: VCC→3V3, GND→GND, I/O→PC7. Va a **3V3 y no a 5 V** porque con el emisor del PNP a 5 V, un "alto" de 3,3 V en el pin no lo apaga y el buzzer sonaría siempre. Lógica invertida: PC7 en bajo = suena.

---

## 4. Plan de montaje (por etapas, probar cada una antes de seguir)

1. **Placa sola:** CubeMX + blink de PA6 + "hola" por USART1 con el USB-TTL. Confirmar que el ST-Link programa (BOOT0 en 0).
2. **Bus I²C:** escaneo de direcciones por UART (esperado 0x44 SHT30, 0x3C OLED). Luego texto en la OLED y lectura del SHT30 con verificación de CRC.
3. **Analógicos:** leer PC0/PC1 crudos por UART. **Calibrar** el sensor de suelo: valor en aire (seco = 0 %) y en vaso de agua (100 %); anotar en `app_config.h`. Probar desconexión (con el pull-down de 100 kΩ debe leer ≈0 → error).
4. **Potencia:** bomba con fuente externa y TIP122, comandada con un botón. Verificar que la placa no se reinicia al arrancar la bomba.
5. **Salidas de estado:** LED RGB por PWM, buzzer. Verificar que el buzzer es **activo**: con VCC a 3V3, poner I/O a GND con un cable → debe sonar un tono continuo. Si solo hace un clic, es pasivo y habría que generar el tono con PWM.
6. **Integración física:** matera dentro de la caja; tanque (frasco) afuera o en una esquina; manguera de la bomba a la matera sujeta con un clip; sensor de suelo enterrado hasta la línea; LDR y SHT30 dentro de la caja a la altura de la planta; OLED y LED RGB en la tapa o frente. Cables por un orificio lateral.
7. **Pruebas:** ciclo completo de riego, alarmas, 5 min continuos, evidencia de preempción.

> [!warning] Cuidados
> - Probar el riego con poca agua y la caja sobre una bandeja: una bomba de 120 L/h mueve **~30 ml/s**, por eso los pulsos son de 1 s.
> - El jade es suculenta: el exceso de agua lo pudre. La lógica riega poco y solo con el sustrato casi seco; en la demo el ciclo se acelera sacando el sensor de la tierra, sin encharcar la planta de verdad (se puede apuntar la manguera a un vaso durante los ensayos).
> - Calibrar el sensor de suelo **en el sustrato de cactus** (arenoso): lee distinto que tierra normal.
> - Electrónica **fuera** de la zona húmeda; protoboard sobre la tapa o al lado.
> - La bomba no debe funcionar en seco mucho tiempo (por eso hay un tiempo máximo por pulso).

---

## 5. Cronograma (entrega miércoles 2026-11-11)

| Día | Estevan | Marco |
|---|---|---|
| Dom 8 (hoy) | Propuesta, arquitectura, CLAUDE.md, repo | Revisar propuesta, confirmar compras |
| Lun 9 | Compras. Crear `.ioc` + proyecto base con tareas vacías y subirlo | Drivers SHT30 + SSD1306 (I²C) sobre el proyecto base |
| Mar 10 | Tareas Control (máquina de estados) + Actuadores + Logger | Tareas Sensores + Display; montaje físico; calibración |
| Mar 10 noche | Integración, prueba de 5 min, captura de preempción | ídem |
| Mié 11 | Documento técnico + fotos/video + ensayo de demo | ídem |

El reparto es una sugerencia: la idea es que cada uno sea dueño de módulos distintos para no pisarse (ver [[CLAUDE]]).

---

## Enlaces
- [[enunciado]] · [[arquitectura]] · [[CLAUDE]]
