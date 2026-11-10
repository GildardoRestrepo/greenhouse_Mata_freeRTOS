/**
 * @file font_6x8.h
 * @brief Fuente ASCII 5x7 en celda de 6x8 píxeles (21 columnas × 8 filas en 128×64).
 */
#ifndef FONT_6X8_H
#define FONT_6X8_H

#include <stdint.h>

#define FONT_FIRST_CHAR     0x20U
#define FONT_LAST_CHAR      0x7FU
#define FONT_GLYPH_COLS     5U      /* columnas con dibujo */
#define FONT_CELL_W         6U      /* ancho de celda (glifo + 1 columna de espacio) */
#define FONT_CELL_H         8U      /* alto de celda (7 filas + 1 de espacio) */
#define FONT_CHAR_DEGREE    '\x7F'  /* símbolo de grado, para "23.5 °C" */

/**
 * @brief Devuelve las FONT_GLYPH_COLS columnas del carácter (bit 0 = fila de arriba).
 *        Los caracteres fuera de rango se dibujan como '?'.
 */
const uint8_t *font_6x8_glyph(char c);

#endif /* FONT_6X8_H */
