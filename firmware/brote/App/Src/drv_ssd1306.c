/**
 * @file drv_ssd1306.c
 * @brief OLED SSD1306 128×64 por I2C: framebuffer estático y primitivas de dibujo.
 *
 * Protocolo: tras la dirección va un byte de control (0x00 = siguen comandos,
 * 0x40 = siguen datos). HAL_I2C_Mem_Write envía justo "dirección + 1 byte +
 * buffer", así que el byte de control se pasa como "dirección de memoria" y no
 * hace falta copiar el framebuffer a otro buffer.
 *
 * Se usa direccionamiento por página (cada página = 8 filas = 128 bytes): cada
 * página es una transacción independiente que fija su propia posición, de modo
 * que un error en una no desalinea las demás.
 */
#include "drv_ssd1306.h"

#include <string.h>

#include "app_config.h"
#include "app_shared.h"     /* mtxI2CHandle */
#include "cmsis_os2.h"
#include "font_6x8.h"
#include "i2c.h"            /* hi2c1 (generado por CubeMX, par .c/.h) */

/* Constantes del protocolo (hoja de datos del SSD1306); no son ajustables. */
#define SSD1306_I2C_ADDR        (0x3CU << 1)    /* HAL usa la dirección en 8 bits */
#define SSD1306_CTRL_CMD        0x00U
#define SSD1306_CTRL_DATA       0x40U
#define SSD1306_PAGES           (SSD1306_HEIGHT / 8)
#define SSD1306_FB_SIZE         (SSD1306_WIDTH * SSD1306_PAGES)     /* 1024 bytes */

#define SSD1306_CMD_SET_PAGE    0xB0U           /* | nº de página */
#define SSD1306_CMD_SET_COL_LO  0x00U           /* | nibble bajo de la columna */
#define SSD1306_CMD_SET_COL_HI  0x10U           /* | nibble alto de la columna */

/* Framebuffer: estático (nunca en la pila de la tarea). Bit 0 = fila superior de la página. */
static uint8_t s_fb[SSD1306_FB_SIZE];

static const uint8_t SSD1306_INIT_SEQ[] = {
    0xAE,                       /* display OFF mientras se configura */
    0xD5, 0x80,                 /* divisor de reloj / frecuencia del oscilador */
    0xA8, SSD1306_HEIGHT - 1,   /* multiplex: 64 filas */
    0xD3, 0x00,                 /* sin desplazamiento vertical */
    0x40,                       /* línea de inicio 0 */
    0x8D, 0x14,                 /* bomba de carga interna ON (módulo a 3V3) */
    0x20, 0x02,                 /* modo de direccionamiento por página */
#if SSD1306_ROTATE_180
    0xA0, 0xC0,                 /* orientación girada 180° */
#else
    0xA1, 0xC8,                 /* orientación normal (pines arriba) */
#endif
    0xDA, 0x12,                 /* configuración de pines COM para 128×64 */
    0x81, SSD1306_CONTRAST,     /* contraste */
    0xD9, 0xF1,                 /* periodo de precarga */
    0xDB, 0x40,                 /* nivel VCOMH */
    0x2E,                       /* scroll desactivado */
    0xA4,                       /* mostrar el contenido de la RAM */
    0xA6,                       /* no invertido */
    0xAF                        /* display ON */
};

static ssd1306_status_t ssd1306_lock(void)
{
    if (osMutexAcquire(mtxI2CHandle, APP_MS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) != osOK) {
        return SSD1306_ERR_MUTEX;
    }
    return SSD1306_OK;
}

static void ssd1306_unlock(void)
{
    (void)osMutexRelease(mtxI2CHandle);
}

/* Escribe en el bus; el llamador debe tener mtxI2C tomado. */
static ssd1306_status_t ssd1306_write(uint8_t control, const uint8_t *data, uint16_t len)
{
    if (HAL_I2C_Mem_Write(&hi2c1, SSD1306_I2C_ADDR, control, I2C_MEMADD_SIZE_8BIT,
                          (uint8_t *)data, len, SSD1306_I2C_TIMEOUT_MS) != HAL_OK) {
        return SSD1306_ERR_BUS;
    }
    return SSD1306_OK;
}

/* Una página = una sección crítica del bus: posicionar + 128 bytes de datos. */
static ssd1306_status_t ssd1306_send_page(uint8_t page)
{
    const uint8_t pos[3] = {
        (uint8_t)(SSD1306_CMD_SET_PAGE | page),
        (uint8_t)(SSD1306_CMD_SET_COL_LO | (SSD1306_COL_OFFSET & 0x0FU)),
        (uint8_t)(SSD1306_CMD_SET_COL_HI | ((SSD1306_COL_OFFSET >> 4) & 0x0FU))
    };
    ssd1306_status_t st = ssd1306_lock();

    if (st != SSD1306_OK) {
        return st;
    }
    st = ssd1306_write(SSD1306_CTRL_CMD, pos, sizeof(pos));
    if (st == SSD1306_OK) {
        st = ssd1306_write(SSD1306_CTRL_DATA, &s_fb[(uint16_t)page * SSD1306_WIDTH],
                           SSD1306_WIDTH);
    }
    ssd1306_unlock();
    return st;
}

ssd1306_status_t ssd1306_init(void)
{
    ssd1306_status_t st = ssd1306_lock();

    if (st != SSD1306_OK) {
        return st;
    }
    st = ssd1306_write(SSD1306_CTRL_CMD, SSD1306_INIT_SEQ, sizeof(SSD1306_INIT_SEQ));
    ssd1306_unlock();
    if (st != SSD1306_OK) {
        return st;
    }

    /* La RAM del panel arranca con basura: se limpia antes de que se vea. */
    ssd1306_clear();
    return ssd1306_update();
}

ssd1306_status_t ssd1306_update(void)
{
    for (uint8_t page = 0; page < SSD1306_PAGES; page++) {
        ssd1306_status_t st = ssd1306_send_page(page);
        if (st != SSD1306_OK) {
            return st;
        }
    }
    return SSD1306_OK;
}

void ssd1306_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

void ssd1306_draw_pixel(int16_t x, int16_t y, ssd1306_color_t color)
{
    if ((x < 0) || (x >= SSD1306_WIDTH) || (y < 0) || (y >= SSD1306_HEIGHT)) {
        return;
    }

    uint8_t *cell = &s_fb[((uint16_t)y / 8U) * SSD1306_WIDTH + (uint16_t)x];
    uint8_t mask = (uint8_t)(1U << ((uint16_t)y % 8U));

    switch (color) {
    case SSD1306_WHITE:
        *cell |= mask;
        break;
    case SSD1306_BLACK:
        *cell &= (uint8_t)~mask;
        break;
    case SSD1306_INVERT:
    default:
        *cell ^= mask;
        break;
    }
}

void ssd1306_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color)
{
    for (int16_t row = y; row < (int16_t)(y + h); row++) {
        for (int16_t col = x; col < (int16_t)(x + w); col++) {
            ssd1306_draw_pixel(col, row, color);
        }
    }
}

void ssd1306_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, ssd1306_color_t color)
{
    if ((w <= 0) || (h <= 0)) {
        return;
    }
    ssd1306_fill_rect(x, y, w, 1, color);                           /* arriba */
    if (h > 1) {
        ssd1306_fill_rect(x, (int16_t)(y + h - 1), w, 1, color);    /* abajo */
    }
    if (h > 2) {
        ssd1306_fill_rect(x, (int16_t)(y + 1), 1, (int16_t)(h - 2), color);     /* izquierda */
        if (w > 1) {
            ssd1306_fill_rect((int16_t)(x + w - 1), (int16_t)(y + 1), 1, (int16_t)(h - 2),
                              color);                               /* derecha */
        }
    }
}

void ssd1306_draw_text(int16_t x, int16_t y, const char *text, uint8_t scale)
{
    if (scale == 0U) {
        scale = 1U;
    }

    for (; *text != '\0'; text++) {
        const uint8_t *glyph = font_6x8_glyph(*text);

        for (uint8_t col = 0; col < FONT_GLYPH_COLS; col++) {
            for (uint8_t row = 0; row < FONT_CELL_H; row++) {
                if ((glyph[col] & (1U << row)) != 0U) {
                    ssd1306_fill_rect((int16_t)(x + col * scale), (int16_t)(y + row * scale),
                                      scale, scale, SSD1306_WHITE);
                }
            }
        }
        x = (int16_t)(x + FONT_CELL_W * scale);
    }
}

int16_t ssd1306_text_width(const char *text, uint8_t scale)
{
    return (int16_t)(strlen(text) * FONT_CELL_W * scale);
}

void ssd1306_draw_bar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t pct)
{
    ssd1306_draw_rect(x, y, w, h, SSD1306_WHITE);
    if ((pct <= 0) || (w <= 4) || (h <= 4)) {
        return;
    }
    if (pct > 100) {
        pct = 100;
    }

    /* Relleno separado 1 px del marco para que se distinga el 100 %. */
    int16_t inner_w = (int16_t)(w - 4);
    int16_t fill_w = (int16_t)(((int32_t)inner_w * pct) / 100);

    ssd1306_fill_rect((int16_t)(x + 2), (int16_t)(y + 2), fill_w, (int16_t)(h - 4),
                      SSD1306_WHITE);
}
