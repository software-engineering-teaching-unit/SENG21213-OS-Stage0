/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: VGA Text Mode Driver — vga.c
 *
 * Writes characters directly to the VGA text buffer at 0xB8000.
 * No BIOS calls are used — we are in Protected Mode.
 *
 * The VGA hardware cursor position is updated via I/O ports:
 *   0x3D4 — CRT Controller index register
 *   0x3D5 — CRT Controller data register
 *   Index 0x0E — Cursor location HIGH byte
 *   Index 0x0F — Cursor location LOW  byte
 *
 * Lecture reference: L07 §5, L08 §1
 *
 * ── Extension opportunities (for students) ──────────────────────────────────
 *   1. Add a circular scroll buffer with Page-Up / Page-Down
 *   2. Implement ANSI escape-code colour support (ESC[31m … ESC[0m)
 *   3. Add a double-buffering mode to eliminate flicker
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "../include/vga.h"
#include "../include/io.h"
#include "../include/string.h"
#include <stdarg.h>

/* ── CRT Controller I/O ports ───────────────────────────────────────────── */
#define VGA_CTRL_IDX  0x3D4
#define VGA_CTRL_DATA 0x3D5
#define VGA_CURSOR_HI 0x0E
#define VGA_CURSOR_LO 0x0F

/* ── Driver state ───────────────────────────────────────────────────────── */
static uint8_t  cur_row  = 0;
static uint8_t  cur_col  = 0;
static uint8_t  cur_attr;           /* Current attribute byte */

/* ── Forward declarations ───────────────────────────────────────────────── */
static void vga_putchar_at(char c, uint8_t attr, uint8_t row, uint8_t col);
static void print_number(uint32_t n, int base, int pad, char padch);

/* ─────────────────────────────────────────────────────────────────────────
 * vga_init — clear screen and reset cursor to (0, 0)
 * ───────────────────────────────────────────────────────────────────────── */
void vga_init(void)
{
    cur_attr = vga_attr(VGA_LIGHT_GREY, VGA_BLACK);
    cur_row  = 0;
    cur_col  = 0;
    vga_clear();
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_clear — fill entire screen with spaces using the current attribute
 * ───────────────────────────────────────────────────────────────────────── */
void vga_clear(void)
{
    uint16_t blank = vga_cell(' ', cur_attr);
    for (int i = 0; i < VGA_ROWS * VGA_COLS; i++)
        VGA_BUFFER[i] = blank;
    cur_row = 0;
    cur_col = 0;
    vga_update_hw_cursor();
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_set_colour — change the active foreground / background colours
 * ───────────────────────────────────────────────────────────────────────── */
void vga_set_colour(vga_colour_t fg, vga_colour_t bg)
{
    cur_attr = vga_attr(fg, bg);
}

void vga_set_header_colour(void)
{
    vga_set_colour(VGA_YELLOW, VGA_BLUE);
}

void vga_set_normal_colour(void)
{
    vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_scroll — scroll the screen up by one line
 *
 * Copies rows 1..VGA_ROWS-1 down to rows 0..VGA_ROWS-2,
 * then blanks the last row.
 * ───────────────────────────────────────────────────────────────────────── */
void vga_scroll(void)
{
    /* Move all rows up by one */
    for (int row = 0; row < VGA_ROWS - 1; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            VGA_BUFFER[row * VGA_COLS + col] =
                VGA_BUFFER[(row + 1) * VGA_COLS + col];
        }
    }
    /* Blank the last row */
    uint16_t blank = vga_cell(' ', cur_attr);
    for (int col = 0; col < VGA_COLS; col++)
        VGA_BUFFER[(VGA_ROWS - 1) * VGA_COLS + col] = blank;
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_update_hw_cursor — sync the hardware blinking cursor to (cur_row, cur_col)
 *
 * The CRT controller register pair 0x0E/0x0F holds the linear cursor
 * position = row * VGA_COLS + col.
 * ───────────────────────────────────────────────────────────────────────── */
void vga_update_hw_cursor(void)
{
    uint16_t pos = (uint16_t)(cur_row * VGA_COLS + cur_col);
    outb(VGA_CTRL_IDX,  VGA_CURSOR_HI);
    outb(VGA_CTRL_DATA, (uint8_t)(pos >> 8));
    outb(VGA_CTRL_IDX,  VGA_CURSOR_LO);
    outb(VGA_CTRL_DATA, (uint8_t)(pos & 0xFF));
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_set_cursor — move cursor to (row, col)
 * ───────────────────────────────────────────────────────────────────────── */
void vga_set_cursor(uint8_t row, uint8_t col)
{
    cur_row = row < VGA_ROWS ? row : VGA_ROWS - 1;
    cur_col = col < VGA_COLS ? col : VGA_COLS - 1;
    vga_update_hw_cursor();
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_putchar — write a single character at the current cursor position
 *
 * Handles:
 *   '\n' — move to start of next line (scroll if needed)
 *   '\r' — carriage return (col = 0)
 *   '\b' — backspace (move cursor left, overwrite with space)
 *   '\t' — advance to next 8-column tab stop
 *   other — write character and advance cursor
 * ───────────────────────────────────────────────────────────────────────── */
void vga_putchar(char c)
{
    if (c == '\n') {
        cur_col = 0;
        cur_row++;
    } else if (c == '\r') {
        cur_col = 0;
    } else if (c == '\b') {
        if (cur_col > 0) {
            cur_col--;
            vga_putchar_at(' ', cur_attr, cur_row, cur_col);
        }
    } else if (c == '\t') {
        cur_col = (uint8_t)((cur_col + 8) & ~7u);
        if (cur_col >= VGA_COLS) {
            cur_col = 0;
            cur_row++;
        }
    } else {
        vga_putchar_at(c, cur_attr, cur_row, cur_col);
        cur_col++;
        if (cur_col >= VGA_COLS) {
            cur_col = 0;
            cur_row++;
        }
    }

    /* Scroll if we have gone past the last row */
    if (cur_row >= VGA_ROWS) {
        vga_scroll();
        cur_row = VGA_ROWS - 1;
    }

    vga_update_hw_cursor();
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_puts — write a NUL-terminated string
 * ───────────────────────────────────────────────────────────────────────── */
void vga_puts(const char *str)
{
    while (*str)
        vga_putchar(*str++);
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_put_hex — print a 32-bit value as "0xXXXXXXXX"
 * ───────────────────────────────────────────────────────────────────────── */
void vga_put_hex(uint32_t value)
{
    vga_puts("0x");
    print_number(value, 16, 8, '0');
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_put_uint — print an unsigned 32-bit integer in decimal
 * ───────────────────────────────────────────────────────────────────────── */
void vga_put_uint(uint32_t value)
{
    print_number(value, 10, 0, ' ');
}

/* ─────────────────────────────────────────────────────────────────────────
 * vga_printf — minimal printf: supports %s %c %d %u %x %p %%
 * ───────────────────────────────────────────────────────────────────────── */
void vga_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            vga_putchar(*p);
            continue;
        }
        p++;
        switch (*p) {
        case 's': {
            const char *s = va_arg(args, const char *);
            vga_puts(s ? s : "(null)");
            break;
        }
        case 'c':
            vga_putchar((char)va_arg(args, int));
            break;
        case 'd': {
            int v = va_arg(args, int);
            if (v < 0) { vga_putchar('-'); v = -v; }
            print_number((uint32_t)v, 10, 0, ' ');
            break;
        }
        case 'u':
            print_number(va_arg(args, uint32_t), 10, 0, ' ');
            break;
        case 'x':
            print_number(va_arg(args, uint32_t), 16, 0, ' ');
            break;
        case 'p':
            vga_puts("0x");
            print_number(va_arg(args, uint32_t), 16, 8, '0');
            break;
        case '%':
            vga_putchar('%');
            break;
        default:
            vga_putchar('%');
            vga_putchar(*p);
            break;
        }
    }

    va_end(args);
}

/* ─────────────────────────────────────────────────────────────────────────
 * Internal helpers
 * ───────────────────────────────────────────────────────────────────────── */

static void vga_putchar_at(char c, uint8_t attr, uint8_t row, uint8_t col)
{
    VGA_BUFFER[(uint16_t)(row * VGA_COLS + col)] = vga_cell(c, attr);
}

/*
 * print_number — recursive digit printer
 * base: 10 for decimal, 16 for hex
 * pad:  minimum digit width (0 = no padding)
 * padch: padding character ('0' or ' ')
 */
static void print_number(uint32_t n, int base, int pad, char padch)
{
    static const char digits[] = "0123456789ABCDEF";
    char buf[32];
    int  i = 0;

    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n > 0) {
            buf[i++] = digits[n % (uint32_t)base];
            n /= (uint32_t)base;
        }
    }

    /* Pad to requested width */
    while (i < pad)
        buf[i++] = padch;

    /* Print in reverse (most-significant digit first) */
    while (i > 0)
        vga_putchar(buf[--i]);
}
