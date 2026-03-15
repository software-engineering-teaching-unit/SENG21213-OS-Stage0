#ifndef VGA_H
#define VGA_H

/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: VGA Text Mode Driver
 *
 * The VGA text-mode buffer lives at physical address 0xB8000.
 * Each character cell is 2 bytes:
 *   Byte 0 (low):  ASCII character code
 *   Byte 1 (high): Attribute byte  [ BG(3) | FG(4) | Blink(1) ]
 *
 * Screen dimensions: 80 columns × 25 rows = 2000 cells = 4000 bytes
 *
 * Lecture reference: L07 §5.1, L08 §1
 *
 *  ┌─────────────────────────────────────────────────────┐
 *  │  Attribute byte layout                              │
 *  │  Bit 7    : Blink (or bright background)            │
 *  │  Bits 6–4 : Background colour (3 bits)              │
 *  │  Bits 3–0 : Foreground colour (4 bits)              │
 *  └─────────────────────────────────────────────────────┘
 */

#include <stdint.h>

/* Physical address of VGA text buffer */
#define VGA_BUFFER    ((volatile uint16_t *)0xB8000)

/* Screen geometry */
#define VGA_COLS      80
#define VGA_ROWS      25

/* VGA colour codes (foreground / background) */
typedef enum {
    VGA_BLACK         = 0,
    VGA_BLUE          = 1,
    VGA_GREEN         = 2,
    VGA_CYAN          = 3,
    VGA_RED           = 4,
    VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,
    VGA_LIGHT_GREY    = 7,
    VGA_DARK_GREY     = 8,
    VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10,
    VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW        = 14,
    VGA_WHITE         = 15,
} vga_colour_t;

/* Build an attribute byte from foreground + background colour */
static inline uint8_t vga_attr(vga_colour_t fg, vga_colour_t bg)
{
    return (uint8_t)((bg << 4) | (fg & 0x0F));
}

/* Build a VGA cell (char + attribute packed into uint16_t) */
static inline uint16_t vga_cell(char c, uint8_t attr)
{
    return (uint16_t)((attr << 8) | (uint8_t)c);
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void vga_init(void);
void vga_clear(void);
void vga_set_colour(vga_colour_t fg, vga_colour_t bg);
void vga_putchar(char c);
void vga_puts(const char *str);
void vga_printf(const char *fmt, ...);
void vga_put_hex(uint32_t value);
void vga_put_uint(uint32_t value);
void vga_set_cursor(uint8_t row, uint8_t col);
void vga_update_hw_cursor(void);
void vga_scroll(void);

/* Extension hook — students can replace with their own colour scheme */
void vga_set_header_colour(void);
void vga_set_normal_colour(void);

#endif /* VGA_H */
