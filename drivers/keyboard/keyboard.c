/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: PS/2 Keyboard Driver — keyboard.c
 *
 * Reads scancodes from the PS/2 controller (Intel 8042) via port 0x60.
 * This implementation uses polling (not interrupts) so no IDT is required
 * at Stage 0.  Interrupt-driven input is introduced at Stage 1.
 *
 * Scancode Set 1 (default US layout):
 *   Make code  = key down
 *   Break code = make code | 0x80 (key up)
 *
 * Lecture reference: L08 §3 (interrupt-driven I/O technique)
 *
 * ── Extension opportunities (for students) ──────────────────────────────────
 *   1. Add full ISO layout (AltGr, non-US keys)
 *   2. Implement interrupt-driven input (IDT + IRQ1 handler)
 *   3. Add command-history ring buffer (up/down arrow)
 *   4. Support function keys F1–F12 for shell hotkeys
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "../include/keyboard.h"
#include "../include/vga.h"
#include "../include/io.h"
#include "../include/string.h"

/* ─────────────────────────────────────────────────────────────────────────
 * US Scancode Set 1 — printable character table
 *
 * Index = make scancode (0x01 – 0x58)
 * Value = ASCII character (0 for non-printable)
 * ───────────────────────────────────────────────────────────────────────── */
static const char sc_to_ascii_lower[] = {
/*00*/  0,
/*01*/  0,      /* Escape       */
/*02*/ '1',
/*03*/ '2',
/*04*/ '3',
/*05*/ '4',
/*06*/ '5',
/*07*/ '6',
/*08*/ '7',
/*09*/ '8',
/*0A*/ '9',
/*0B*/ '0',
/*0C*/ '-',
/*0D*/ '=',
/*0E*/  '\b',   /* Backspace    */
/*0F*/  '\t',   /* Tab          */
/*10*/ 'q',
/*11*/ 'w',
/*12*/ 'e',
/*13*/ 'r',
/*14*/ 't',
/*15*/ 'y',
/*16*/ 'u',
/*17*/ 'i',
/*18*/ 'o',
/*19*/ 'p',
/*1A*/ '[',
/*1B*/ ']',
/*1C*/  '\n',   /* Enter        */
/*1D*/  0,      /* L Ctrl       */
/*1E*/ 'a',
/*1F*/ 's',
/*20*/ 'd',
/*21*/ 'f',
/*22*/ 'g',
/*23*/ 'h',
/*24*/ 'j',
/*25*/ 'k',
/*26*/ 'l',
/*27*/ ';',
/*28*/ '\'',
/*29*/ '`',
/*2A*/  0,      /* L Shift      */
/*2B*/ '\\',
/*2C*/ 'z',
/*2D*/ 'x',
/*2E*/ 'c',
/*2F*/ 'v',
/*30*/ 'b',
/*31*/ 'n',
/*32*/ 'm',
/*33*/ ',',
/*34*/ '.',
/*35*/ '/',
/*36*/  0,      /* R Shift      */
/*37*/ '*',     /* Numpad *     */
/*38*/  0,      /* L Alt        */
/*39*/ ' ',
/*3A*/  0,      /* Caps Lock    */
/*3B*/  0,      /* F1           */
/*3C*/  0,      /* F2           */
/*3D*/  0,      /* F3           */
/*3E*/  0,      /* F4           */
/*3F*/  0,      /* F5           */
/*40*/  0,      /* F6           */
/*41*/  0,      /* F7           */
/*42*/  0,      /* F8           */
/*43*/  0,      /* F9           */
/*44*/  0,      /* F10          */
};

static const char sc_to_ascii_upper[] = {
/*00*/  0,
/*01*/  0,
/*02*/ '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
/*0C*/ '_', '+',
/*0E*/  '\b',
/*0F*/  '\t',
/*10*/ 'Q','W','E','R','T','Y','U','I','O','P',
/*1A*/ '{', '}',
/*1C*/  '\n',
/*1D*/  0,
/*1E*/ 'A','S','D','F','G','H','J','K','L',
/*27*/ ':', '"',
/*29*/ '~',
/*2A*/  0,
/*2B*/ '|',
/*2C*/ 'Z','X','C','V','B','N','M',
/*33*/ '<', '>', '?',
/*36*/  0,
/*37*/ '*',
/*38*/  0,
/*39*/ ' ',
/*3A*/  0,
};

/* ── Driver state ───────────────────────────────────────────────────────── */
static int shift_held  = 0;
static int caps_lock   = 0;     /* Extension: implement caps lock toggle */

/* ─────────────────────────────────────────────────────────────────────────
 * keyboard_init — flush the PS/2 output buffer
 * ───────────────────────────────────────────────────────────────────────── */
void keyboard_init(void)
{
    /* Drain any stale bytes in the controller output buffer */
    while (inb(KB_STATUS_PORT) & KB_STATUS_OUT_FULL)
        inb(KB_DATA_PORT);
}

/* ─────────────────────────────────────────────────────────────────────────
 * keyboard_read_scancode — non-blocking read
 * Returns 0 if no data is ready.
 * ───────────────────────────────────────────────────────────────────────── */
uint8_t keyboard_read_scancode(void)
{
    if (!(inb(KB_STATUS_PORT) & KB_STATUS_OUT_FULL))
        return 0;
    return inb(KB_DATA_PORT);
}

/* ─────────────────────────────────────────────────────────────────────────
 * keyboard_scancode_to_ascii — translate Set-1 make code to ASCII
 * Returns 0 for non-printable / modifier keys.
 * ───────────────────────────────────────────────────────────────────────── */
char keyboard_scancode_to_ascii(uint8_t sc)
{
    /* Ignore break codes (key release) */
    if (sc & KB_SC_BREAK)
        return 0;

    /* Track Shift state */
    if (sc == KB_SC_LSHIFT || sc == KB_SC_RSHIFT) {
        shift_held = 1;
        return 0;
    }

    /* Caps Lock toggle */
    if (sc == KB_SC_CAPS) {
        caps_lock = !caps_lock;
        return 0;
    }

    if (sc >= sizeof(sc_to_ascii_lower))
        return 0;

    int use_upper = shift_held ^ caps_lock;
    char c = use_upper ? sc_to_ascii_upper[sc] : sc_to_ascii_lower[sc];
    return c;
}

/* ─────────────────────────────────────────────────────────────────────────
 * keyboard_getchar — blocking poll until a printable character arrives
 * ───────────────────────────────────────────────────────────────────────── */
char keyboard_getchar(void)
{
    for (;;) {
        /* Wait for PS/2 output buffer to have data */
        while (!(inb(KB_STATUS_PORT) & KB_STATUS_OUT_FULL))
            ;

        uint8_t sc = inb(KB_DATA_PORT);

        /* Handle Shift release */
        if ((sc & KB_SC_BREAK) &&
            ((sc & ~KB_SC_BREAK) == KB_SC_LSHIFT ||
             (sc & ~KB_SC_BREAK) == KB_SC_RSHIFT)) {
            shift_held = 0;
            continue;
        }

        char c = keyboard_scancode_to_ascii(sc);
        if (c)
            return c;
    }
}

/* ─────────────────────────────────────────────────────────────────────────
 * keyboard_readline — read one line, echo to VGA, handle Backspace/Enter
 *
 * Returns number of characters in buf (not including NUL).
 *
 * Extension ideas:
 *   - Record each completed line into a ring buffer of 20 entries
 *   - On UP/DOWN arrow scancode (extended 0xE0 prefix), recall history
 * ───────────────────────────────────────────────────────────────────────── */
int keyboard_readline(char *buf, int len)
{
    int pos = 0;
    buf[0]  = '\0';

    for (;;) {
        char c = keyboard_getchar();

        if (c == '\n') {
            vga_putchar('\n');
            buf[pos] = '\0';
            return pos;
        }

        if (c == '\b') {
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                vga_putchar('\b');
            }
            continue;
        }

        if (pos < len - 1) {
            buf[pos++] = c;
            buf[pos]   = '\0';
            vga_putchar(c);
        }
    }
}
