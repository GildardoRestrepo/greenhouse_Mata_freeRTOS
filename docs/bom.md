---
type: bom
created: 2026-11-08
estado: revisado 2026-11-08 (stock de Ardobot consultado ese día)
---
# Lista de materiales (BOM) — Brote

Detalle técnico de cada componente en [[propuesta]] §2 y §3.

## Imprescindible

| Componente | Link de compra | Precio (COP) |
|---|---|---|
| Sensor de humedad de suelo **capacitivo** DFRobot SEN0193 (×1) | https://ardobot.xyz/sensor-de-humedad-de-suelo-capacitivo-dfrobot-sen0193.html | $37.000 |
| Sensor de temperatura/humedad **SHT30** I²C (×1) ⚠️ | https://ardobot.xyz/sensor-de-humedad-y-temperatura-sht30.html | $20.000 |
| Pantalla OLED 0.96" SSD1306 128×64 I²C, 4 pines (×1) | https://ardobot.xyz/display-oled-i2c-128x64-0-96-inch.html | $12.500 |
| Fotorresistencia LDR (×1) | https://ardobot.xyz/fotorresistencia-ldr-12mm.html | $2.300 |
| Mini bomba sumergible brushless 2.5–6 V (×2, una de repuesto) | https://ardobot.xyz/mini-bomba-sumergible-dc-2-5v-a-6v-brushless.html | $15.000 ($7.500 c/u) |
| Manguera de silicona **6 mm interior** (1 m) | acuario o ferretería (Ardobot no la vende) | ≈ $4.000 |
| Transistor Darlington **TIP122** (×2) — reemplaza al TIP120, agotado | https://ardobot.xyz/transistor-npn-darlington-100v-5a-tip122.html | $2.600 ($1.300 c/u) |
| Diodo **1N4007** (×2) | https://ardobot.xyz/diodo-rectificador-1n4007.html | $200 ($100 c/u) |
| Condensador electrolítico 100 µF / 35 V (×2) | https://ardobot.xyz/capacitor-electrolitico-100uf-35v.html | $800 ($400 c/u) |
| Módulo buzzer activo, 3 pines (×1) | https://ardobot.xyz/modulo-buzzer-activo.html | $3.000 |
| LED RGB 5 mm cátodo común (×1) | https://ardobot.xyz/led-rgb-5mm-anodo-comun.html | $250 |
| Resistencias: 220 Ω ×3, 1 kΩ ×1, 10 kΩ ×1, 100 kΩ ×2 | — | $0 (listo) |
| Fuente 5 V ≥ 1 A | — | $0 (listo) |
| Protoboard 830 puntos (×1) | https://ardobot.xyz/protoboard-de-830-puntos-mb102.html | $7.000 |
| Jumpers macho-hembra 30 cm y macho-macho 10 cm (×10 c/u) | https://ardobot.xyz/cables-jumper-dupont-x-10-macho-hembra-30-cm.html · https://ardobot.xyz/cables-jumper-dupont-x-10-macho-macho-10-cm.html | $5.000 |
| Caja transparente con tapa | — | $0 (impresión 3D y creatividad) |
| Matera pequeña **con drenaje** + plato | — | $0 (listo) |
| Sustrato para cactus/suculentas | vivero | $10.000 |
| Frasco para tanque de agua (≥ 5 cm de profundidad) | — | $0 (listo) |
| Árbol de jade pequeño (*Crassula ovata*) | — | $0 (listo) |

**Total imprescindible:** ≈ $120.000 (sin imprevistos)

---

## Revisión

| Ítem                            | Veredicto                  | Nota                                                                                                                                                                                                                                                   |
| ------------------------------- | -------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Sensor de suelo DFRobot SEN0193 | ✅                          | Es el original: buena calidad. Salida de 0 a 3,0 V, compatible con el ADC de 3,3 V. Alimentarlo a **3,3 V**. El genérico de $14.900 estaba agotado.                                                                                                    |
| SHT30                           | ⚠️ **verificar en tienda** | La página no aclara si es **módulo** (4 pines VCC/GND/SCL/SDA con pull-ups) o el chip suelto. Solo sirve el módulo. Si es el chip suelto, pedir el "GY-SHT30" o volver al AHT20.                                                                       |
| OLED                            | ✅                          | Usa SSD1306 e I²C, de 3 a 5 V. Al cablear, revisar el orden de VCC y GND en la serigrafía: cambia entre fabricantes, y si se invierte se quema.                                                                                                        |
| LDR                             | ✅                          | Es de 12 mm, no de 5 mm. Funciona igual.                                                                                                                                                                                                               |
| Bomba                           | ✅                          | Corregí la cantidad: son 2 → $15.000. Salida de 7,5 mm exterior, por eso la manguera debe ser de 6 mm interior (la silicona se estira y sella).                                                                                                        |
| Manguera                        | ❌ faltaba                  | Es imprescindible. La manguera de acuario de 4 mm no entra: pedir **silicona de 6 mm interior**.                                                                                                                                                       |
| TIP120                          | ❌ agotado                  | Se reemplaza por el **TIP122**: misma familia, mismas patas (B-C-E) y mismo circuito, a $1.300.                                                                                                                                                        |
| 1N4007                          | ❌ link equivocado          | El link de la fila del diodo era el del 2N2222. Ya está corregido.                                                                                                                                                                                     |
| 2N2222                          | 🗑️ eliminado              | Ya no hace falta: el módulo de buzzer trae su propio transistor.                                                                                                                                                                                       |
| Condensador 100 µF / 35 V       | ✅                          | Que soporte más de 16 V no es problema.                                                                                                                                                                                                                |
| Módulo buzzer                   | ⚠️ **cambia la conexión**  | Trae un transistor **PNP 9012** y se activa **en bajo**. Hay que alimentarlo a **3V3, no a 5 V**: con 5 V, el pin a 3,3 V no lo apaga y sonaría siempre. La página es ambigua sobre si es activo o pasivo; probarlo como dice [[propuesta]] §4 paso 5. |
| LED RGB                         | ✅                          | Aunque el link dice "anodo-comun", el producto es **cátodo común**, que es el que necesitamos.                                                                                                                                                         |
| Resistencias                    | ✅                          | Ajusté las cantidades: sin 2N2222 sobran dos de 1 kΩ, y la de 10 kΩ es para el divisor de la LDR.                                                                                                                                                      |
| ST-Link y USB-TTL               | ❓                          | Se quitaron de la lista. Confirmar que ya los tienen: la placa **no** trae USB-serial y sin el adaptador no hay telemetría.                                                                                                                            |
| Analizador lógico               | ❓                          | Se quitó de la lista. La evidencia de preempción se hará entonces por UART con marcas de tiempo (y los LEDs PA6/PA7). Es suficiente para la rúbrica.                                                                                                   |

## Enlaces
- [[propuesta]] · [[arquitectura]]
