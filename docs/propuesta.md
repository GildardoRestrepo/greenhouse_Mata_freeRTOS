---
type: propuesta
created: 2026-11-08
estado: borrador para revisar con Marco
---
# Propuesta — "Mata": mini-invernadero con riego automático

> [!resumen]
> Caja transparente con un pequeño árbol de jade (*Crassula ovata*), que mide humedad del suelo, temperatura/humedad del aire y luz; "riega" sola cuando el suelo está seco (por presupuesto la bomba se representa con un **LED indicador en PC6**, ver [[bom]]), muestra todo en una OLED, indica el estado con un LED RGB y avisa fallos con un buzzer. STM32F407VET6 + HAL + FreeRTOS (CMSIS-RTOS v2).


---

## 1. Definición (sección 2.1 del enunciado)

| # | Punto | Respuesta |
|---|---|---|
| 1 | Nombre y objetivo | **Brote**: mantener la humedad del suelo de una planta en un rango objetivo sin intervención humana, y mostrar el microclima del invernadero. |
| 2 | Usuario y escenario | Persona que cultiva una planta pequeña en interior (escritorio, apartamento) y se ausenta días; o un laboratorio escolar que quiere observar variables de crecimiento. |
| 3 | Entradas | Humedad de suelo (sonda resistiva YL-69 + módulo YL-38, ADC), luz con LDR (ADC), temperatura y humedad del aire (I²C), 3 botones de la placa (GPIO/EXTI). |
| 4 | Salidas | LED indicador de riego en PC6 (GPIO; ocupa el lugar de la bomba), pantalla OLED (I²C), LED RGB de estado (PWM TIM4), módulo buzzer activo (GPIO, activo en bajo), telemetría por UART al PC. |
| 5 | Estados | `INIT` → `MONITOREO` ⇄ `RIEGO`, `MANUAL`, `ALARMA` (ver [[arquitectura]]). |
| 6 | Periféricos | ADC1, I2C1, TIM4 PWM, USART1 (+DMA), GPIO/EXTI, TIM6 como timebase HAL. |
| 7 | Tareas FreeRTOS | 5 tareas, 5 niveles de prioridad, colas + notificaciones + mutex (ver [[arquitectura]]). |
| 8 | Demostración | Sacar el sensor del sustrato (o sustrato seco) → el LED de riego da pulsos cortos; mojarlo → para. Desconectar un sensor → alarma. Botones cambian de modo. Analizador lógico/UART muestra la preempción. 5+ min continuos. |

**Cumplimiento de mínimos:** entradas de 3 tipos distintos (analógica ADC, digital I²C, botones GPIO) y salidas de 3 tipos (pantalla, luz —LED RGB por PWM y LED de riego, que cuentan como un solo tipo— y sonido); el mínimo exigido es 2.

---

## 2. Lista de compras (tiendas locales Medellín: Sigma, Electronilab, Didácticas Electrónicas, etc.)

Precios **aproximados** en COP, verificar en tienda. Marquen lo que ya tienen.

### Imprescindible
Lista reducida del 2026-11-10 (presupuesto ajustado). Links y precios exactos en [[bom]].

| Componente | Ref. sugerida | Uso | ≈ COP |
|---|---|---|---|
| Sensor humedad de suelo **resistivo** | Sonda YL-69 + módulo YL-38 (salida AO) | Humedad del suelo (ADC). Más barato que el capacitivo; la sonda se corroe con el tiempo, suficiente para la demo. | 3.200 |
| Sensor temp/humedad aire I²C | **HTU21D** en módulo **GY-21** (trae pull-ups) | Microclima (I²C, CRC por medida) | ver [[bom]] |
| Pantalla OLED 0.96" I²C | SSD1306 128×64, **4 pines** (VCC GND SCL SDA) | Interfaz | 12.500 |
| Fotorresistencia + R 10 kΩ | LDR 5 mm | Luz (ADC) | 400 |
| LED rojo 5 mm + R 220 Ω | pack ×10 | **Indicador de riego**: representa la bomba en PC6 | 2.000 |
| Módulo buzzer activo 3 pines (trae transistor PNP 9012) | — | Alarma sonora | 3.000 |
| LED RGB cátodo común 5 mm + 3× R 220 Ω | — | Estado (PWM) | 250 |
| R 100 kΩ ×2 | — | Pull-down en entradas ADC (detectar cable de sensor suelto) | (con las demás resistencias, ≈ 1.000) |
| Jumpers M-H y M-M | packs ×10 | Montaje | ≈ 4.800 |
| Protoboard, caja transparente con tapa, matera pequeña, frasco | — | Invernadero | ya los tenemos |
| Planta | **Árbol de jade pequeño** (*Crassula ovata*) en matera **con drenaje** y tierra | — | ya la tenemos |

**Fuera de la lista (solo con bomba real):** mini bomba sumergible, manguera, TIP122, 1N4007, condensador de 100 µF y fuente de 5 V.

### Si no lo tienen ya
| Componente | Uso | ≈ COP |
|---|---|---|
| ST-Link V2 (clon) | Programar/depurar por SWD | 25.000 |
| Adaptador USB-TTL 3.3 V (CP2102 / CH340) | Telemetría UART al PC (la placa **no** trae USB-serial) | 12.000 |

### Muy recomendado (sube puntos en el criterio 4 de la rúbrica)
| Componente | Uso | ≈ COP |
|---|---|---|
| Analizador lógico 8 canales 24 MHz (clon Saleae, usa PulseView) | Evidencia visual de preempción con pines de traza por tarea | 35.000 |

**Total pagado:** 59.160 COP (detalle en [[bom]]), más ST-Link, USB-TTL o analizador si no los tienen.

---

## 3. Conexiones (placa negra STM32_F4VE)

Pines elegidos para **no chocar** con lo que la placa ya usa (flash SPI1 en PB0/PB3-5, SD en PC8-12/PD2, LEDs en PA6/PA7, botones en PA0/PE3/PE4, USB en PA11/12, SWD en PA13/14).

| Periférico | Señal | Pin STM32 | Recurso | Alimentación / nivel |
|---|---|---|---|---|
| Sensor suelo YL-69 + módulo YL-38 | AO (la salida DO no se usa) | **PC0** | ADC1_IN10 | **3.3 V** (alimentado a 5 V, AO puede pasar de 3.3 V en el pin). Pull-down de 100 kΩ en PC0 |
| LDR (divisor con 10 kΩ a GND) | punto medio | **PC1** | ADC1_IN11 | 3.3 V |
| HTU21 (GY-21) | SCL / SDA | **PB6 / PB7** | I2C1 400 kHz, dir. 0x40 (fija) | 3.3 V (el módulo trae pull-ups) |
| OLED SSD1306 | SCL / SDA | **PB6 / PB7** (mismo bus) | I2C1, dir. 0x3C | 3.3 V |
| LED de riego (representa la bomba) | ánodo; cátodo a GND con R 220 Ω | **PC6** | GPIO salida, activo en alto | 3.3 V, ≈ 5 mA |
| Módulo buzzer (I/O) | señal | **PC7** | GPIO salida push-pull, **activo en bajo** (inicia en alto) | **VCC del módulo a 3V3**, no a 5 V |
| LED RGB | R / G / B (R 220 Ω c/u) | **PD12 / PD13 / PD14** | TIM4_CH1/2/3 PWM | 3.3 V, ~6 mA por canal |
| Botón K0 (placa) | — | PE4 | GPIO EXTI, pull-up, flanco bajada | activo en bajo |
| Botón K1 (placa) | — | PE3 | GPIO EXTI, pull-up, flanco bajada | activo en bajo |
| Botón K_UP (placa) | — | PA0 | GPIO EXTI, pull-down, flanco subida | activo en alto |
| LEDs D2 / D3 (placa) | — | PA6 / PA7 | GPIO salida | activos en bajo |
| USB-TTL | RX / TX | **PA9 (TX) / PA10 (RX)** | USART1 115200 8N1 + DMA TX | 3.3 V, conector J6 |
| Pines de traza (analizador) | 1 por tarea | **PE7–PE11** | GPIO salida | — |
| ST-Link | SWDIO / SWCLK | PA13 / PA14 | SWD | conector JTAG 20p |

### LED de riego (ocupa el lugar de la bomba)
```
 PC6 ──[220 Ω]──▶|── GND      (LED rojo: ánodo hacia la resistencia, cátodo a GND)
```
Encendido = "bomba regando". El firmware es el mismo que con la bomba: mismo pin y mismo pulso con tiempo máximo (`PUMP_PULSE_MS`). Un LED con su resistencia (≈ 5 mA) sí puede ir directo al pin.

Si más adelante se monta una **bomba real, nunca directo al pin**: TIP122 con R 1 kΩ en la base, diodo 1N4007 en antiparalelo con la bomba, 100 µF cerca de ella y fuente de 5 V aparte con **GND común**.

**Buzzer:** el módulo ya trae su propio transistor PNP (9012), así que se conecta directo: VCC→3V3, GND→GND, I/O→PC7. Va a **3V3 y no a 5 V** porque con el emisor del PNP a 5 V, un "alto" de 3,3 V en el pin no lo apaga y el buzzer sonaría siempre. Lógica invertida: PC7 en bajo = suena.

---

## 4. Plan de montaje (por etapas, probar cada una antes de seguir)

1. **Placa sola:** CubeMX + blink de PA6 + "hola" por USART1 con el USB-TTL. Confirmar que el ST-Link programa (BOOT0 en 0).
2. **Bus I²C:** escaneo de direcciones por UART (esperado 0x40 HTU21, 0x3C OLED). Luego texto en la OLED y lectura del HTU21 con verificación de CRC.
3. **Analógicos:** leer PC0/PC1 crudos por UART. **Calibrar** el sensor de suelo: valor en aire (seco = 0 %) y en vaso de agua (100 %); anotar en `app_config.h`. Probar desconexión (con el pull-down de 100 kΩ debe leer ≈0 → error).
4. **LED de riego:** LED rojo + 220 Ω en PC6, comandado con un botón. Verificar la polaridad (pata larga = ánodo, hacia la resistencia).
5. **Salidas de estado:** LED RGB por PWM, buzzer. Verificar que el buzzer es **activo**: con VCC a 3V3, poner I/O a GND con un cable → debe sonar un tono continuo. Si solo hace un clic, es pasivo y habría que generar el tono con PWM.
6. **Integración física:** matera dentro de la caja; sonda YL-69 enterrada en la tierra y su módulo YL-38 afuera, en seco; frasco con agua a mano para mojar la sonda en la demo; LED de riego visible junto a la matera; LDR y HTU21 dentro de la caja a la altura de la planta; OLED y LED RGB en la tapa o frente. Cables por un orificio lateral.
7. **Pruebas:** ciclo completo de riego, alarmas, 5 min continuos, evidencia de preempción.

> [!warning] Cuidados
> - Como no hay bomba, el suelo **no se moja solo**: en la demo el ciclo se cierra mojando la sonda a mano (meterla en el frasco con agua). Si nadie la moja, tras `MAX_PULSES_NO_RISE` pulsos el sistema pasa a ALARMA "sin agua", que es justo el comportamiento esperado.
> - El jade es suculenta: el exceso de agua lo pudre. Para la demo se saca la sonda de la tierra y se moja aparte, sin encharcar la planta.
> - La sonda YL-69 es resistiva y se corroe si queda enterrada y energizada muchos días: montarla para las pruebas y la demo, no dejarla semanas.
> - Calibrar el sensor de suelo **en la tierra real de la matera**: cada sustrato lee distinto.
> - Electrónica **fuera** de la zona húmeda; protoboard sobre la tapa o al lado. Solo la sonda toca la tierra.

---

## 5. Cronograma (entrega miércoles 2026-11-11)

| Día | Estevan | Marco |
|---|---|---|
| Dom 8 (hoy) | Propuesta, arquitectura, CLAUDE.md, repo | Revisar propuesta, confirmar compras |
| Lun 9 | Compras. Crear `.ioc` + proyecto base con tareas vacías y subirlo | Drivers del sensor de aire (hoy HTU21) + SSD1306 (I²C) sobre el proyecto base |
| Mar 10 | Tareas Control (máquina de estados) + Actuadores + Logger | Tareas Sensores + Display; montaje físico; calibración |
| Mar 10 noche | Integración, prueba de 5 min, captura de preempción | ídem |
| Mié 11 | Documento técnico + fotos/video + ensayo de demo | ídem |

El reparto es una sugerencia: la idea es que cada uno sea dueño de módulos distintos para no pisarse (ver [[CLAUDE]]).

---

## Enlaces
- [[enunciado]] · [[arquitectura]] · [[CLAUDE]]
