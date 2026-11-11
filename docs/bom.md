---
type: bom
created: 2026-11-08
estado: lista reducida acordada el 2026-11-10 (presupuesto ajustado)
---
# Lista de materiales (BOM) — Brote

Detalle técnico de cada componente en [[propuesta]] §2 y §3.

> [!nota] Lista reducida (2026-11-10)
> Por presupuesto se cambió el sensor de suelo capacitivo por el **resistivo YL-38/YL-69**, la LDR de 12 mm por la de **5 mm**, y la **bomba real por un LED indicador en PC6** (con ella salen el TIP122, el 1N4007, el condensador, la manguera y la fuente de 5 V). La protoboard ya la tenemos. Motivo y consecuencias en [[bitacora]] (entrada del 2026-11-10).

## Por comprar

| Componente | Cant. | Precio unit. | Link |
|---|---|---|---|
| Sensor de humedad de suelo YL38/YL69 | 1 | $3.200 | https://ardobot.xyz/sensor-de-humedad-de-suelo-sonda-higrometro-yl38-y-yl69.html |
| HTU21D (módulo GY-21) — reemplaza al SHT30, que no había en tienda | 1 | ya comprado (incluido en el total) | — |
| OLED 0.96" SSD1306 I2C | 1 | $12.500 | https://ardobot.xyz/display-oled-i2c-128x64-0-96-inch.html |
| Fotorresistencia LDR 5mm | 1 | $400 | https://ardobot.xyz/fotoresistencia-ldr-5mm.html |
| Módulo buzzer activo | 1 | $3.000 | https://ardobot.xyz/modulo-buzzer-activo.html |
| LED RGB cátodo común | 1 | $250 | https://ardobot.xyz/led-rgb-5mm-anodo-comun.html |
| LED 5mm rojo pack x10 (indicador "bomba") | 1 pack | $2.000 | https://ardobot.xyz/led-5mm-tipo-chorro-pack-x-10-unid-rojo.html |
| Jumpers M-H 30cm x10 | 1 pack | — | https://ardobot.xyz/cables-jumper-dupont-x-10-macho-hembra-30-cm.html |
| Jumpers M-M 10cm x10 | 1 pack | — | https://ardobot.xyz/cables-jumper-dupont-x-10-macho-macho-10-cm.html |
| Resistencias 10kΩ, 100kΩ×2, 220Ω×2 | 5 | ≈$1.000 (confirmar en tienda) | — |

**Total pagado: $59.160** (compra real del 2026-11-10, con el GY-21 en lugar del SHT30). La lista original con el SHT30 se había estimado en ≈ $47.150; los precios unitarios de la tabla son los de esa lista.

**Ya tenemos:** protoboard, caja, matera, frasco, árbol de jade, tierra.

---

## Revisión

| Ítem | Veredicto | Nota |
|---|---|---|
| Sensor de suelo YL-38/YL-69 | ⚠️ **cambia el comportamiento** | Es **resistivo**: la sonda YL-69 se corroe si queda enterrada y energizada muchos días; para los días de la demo sirve. Se usa la salida **AO** del módulo YL-38 (la DO no) y se alimenta a **3,3 V**. Lee al revés que un porcentaje: seco o al aire ≈ máximo del ADC, mojado = valor bajo; la calibración de `app_config.h` (`SOIL_RAW_DRY` / `SOIL_RAW_WET`) ya admite ese sentido, hay que medirla (prueba P16). |
| HTU21 / GY-21 | ✅ comprado | Reemplaza al SHT30 (agotado). I²C en **0x40**, dirección fija; el módulo trae pull-ups y se alimenta a 3,3 V. Temperatura y humedad se piden con comandos separados (driver `drv_htu21`). |
| OLED | ✅ | Usa SSD1306 e I²C, de 3 a 5 V. Al cablear, revisar el orden de VCC y GND en la serigrafía: cambia entre fabricantes, y si se invierte se quema. |
| LDR 5 mm | ✅ | Reemplaza a la de 12 mm; mismo divisor con 10 kΩ a GND. Calibrar `LIGHT_RAW_DARK` / `LIGHT_RAW_BRIGHT` (prueba P17). |
| LED indicador "bomba" | ✅ | Un LED rojo del pack con **220 Ω en serie** entre PC6 y GND (≈ 5 mA: sí puede ir directo al pin). Reemplaza a la bomba; el firmware no cambia (mismo pin, mismo pulso con tiempo máximo). |
| Bomba, manguera, TIP122, 1N4007, condensador 100 µF, fuente 5 V, R 1 kΩ | 🗑️ eliminados | Solo hacían falta para la bomba real. Si algún día vuelve, **nunca directo al pin**: TIP122 + diodo + fuente aparte con GND común. |
| Protoboard | 🗑️ eliminada | Ya la tenemos. |
| Módulo buzzer | ⚠️ **cambia la conexión** | El módulo recibido es de **disparo en alto** (comprobado el 2026-11-11): suena con la señal en alto. Se alimenta a 3V3 y va directo a PC7; `out_buzzer()` ya usa esa lógica. |
| LED RGB | ✅ | Aunque el link dice "anodo-comun", el producto es **cátodo común**, que es el que necesitamos. |
| Resistencias | ❓ **contar las de 220 Ω** | Hacen falta **cuatro** de 220 Ω (tres del LED RGB y una del LED de riego) y la lista compra dos: confirmar que las otras dos ya están. La de 10 kΩ es el divisor de la LDR; las de 100 kΩ, los pull-down de PC0 y PC1. |
| Jumpers | ✅ comprados | Sin precio unitario anotado; están incluidos en el total pagado de $59.160. |
| ST-Link y USB-TTL | ⚠️ | El ST-Link está y programa. El USB-TTL **no se ha conseguido** (2026-11-11): sin él no hay telemetría por UART y las lecturas se toman con el depurador. |
| Analizador lógico | 🗑️ no se usa | No está en la lista. La evidencia de preempción es por UART con marcas de tiempo (necesita el USB-TTL) o, sin él, con el depurador. |

## Enlaces
- [[propuesta]] · [[arquitectura]] · [[bitacora]]
