/**
 * @file drv_ssd1306.h
 * @brief Driver de la OLED SSD1306 128×64 por I2C1 (dirección 0x3C).
 *
 * Las funciones de dibujo solo modifican un framebuffer estático de 1 KB (no
 * tocan el bus ni el RTOS); ssd1306_update() lo envía a la pantalla. El
 * framebuffer no está protegido: su único dueño es tDisplay.
 */
#ifndef DRV_SSD1306_H
#define DRV_SSD1306_H

#include <stdint.h>

#define SSD1306_WIDTH       128
#define SSD1306_HEIGHT      64

typedef enum {
    SSD1306_OK = 0,
    SSD1306_ERR_MUTEX,      /* no se obtuvo mtxI2C dentro del timeout */
    SSD1306_ERR_BUS         /* HAL_I2C devolvió algo distinto de HAL_OK */
} ssd1306_status_t;

typedef enum {
    SSD1306_BLACK = 0,      /* píxel apagado */
    SSD1306_WHITE,          /* píxel encendido */
    SSD1306_INVERT          /* invierte lo que haya (texto en negativo, parpadeos) */
} ssd1306_color_t;

/* ---- Acceso al bus (toman mtxI2C; llamar solo desde una tarea) ---- */

/** Envía la secuencia de inicialización y enciende el panel (pantalla en negro). */
ssd1306_status_t ssd1306_init(void);

/**
 * @brief Envía el framebuffer completo (≈ 24 ms a 400 kHz).
 *
 * Se envía página por página (8 páginas de 128 bytes) y mtxI2C se toma y se
 * suelta en cada una: tSensors nunca espera el bus más de ≈ 3 ms.
 */
ssd1306_status_t ssd1306_update(void);

/* ---- Dibujo en el framebuffer (con recorte: se puede dibujar fuera sin riesgo) ---- */

void ssd1306_clear(void);
void ssd1306_draw_pixel(int16_t x, int16_t y, ssd1306_color_t color);
void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color);
void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color);

/**
 * @brief Texto con la fuente 6×8, en blanco sobre lo que haya.
 * @param scale factor entero de ampliación (1 = 6×8, 2 = 12×16, 3 = 18×24...).
 */
void ssd1306_draw_text(int16_t x, int16_t y, const char *text, uint8_t scale);

/** Ancho en píxeles que ocupará un texto (para centrar o alinear a la derecha). */
int16_t ssd1306_text_width(const char *text, uint8_t scale);

/**
 * @brief Barra horizontal: marco de w×h y relleno proporcional a pct.
 * @param pct 0..100 (se satura); valores negativos dibujan solo el marco.
 */
void ssd1306_draw_bar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t pct);

#endif /* DRV_SSD1306_H */
