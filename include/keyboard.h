#ifndef KEYBOARD_H
#define KEYBOARD_H

/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: PS/2 Keyboard Driver
 *
 * The PS/2 controller (Intel 8042) exposes two I/O ports:
 *   0x60 — Data port   (read scancode / write command)
 *   0x64 — Status port (read) / Command port (write)
 *
 * When a key is pressed the controller fires IRQ1, and the CPU invokes
 * the ISR registered at IDT vector 33 (0x21).  The ISR reads the scancode
 * from port 0x60 and translates it to ASCII using a scancode table.
 *
 * Lecture reference: L08 §3 (I/O interrupt-driven technique)
 *
 * Key scancode reference (Set 1, US layout):
 *   Make code = key pressed,  Break code = make | 0x80
 */

#include <stdint.h>

/* I/O port addresses */
#define KB_DATA_PORT    0x60
#define KB_STATUS_PORT  0x64
#define KB_CMD_PORT     0x64

/* Status register bits */
#define KB_STATUS_OUT_FULL  (1 << 0)   /* Output buffer full — data ready */
#define KB_STATUS_IN_FULL   (1 << 1)   /* Input buffer full  — wait before write */

/* Special scancode values */
#define KB_SC_LSHIFT    0x2A
#define KB_SC_RSHIFT    0x36
#define KB_SC_CAPS      0x3A
#define KB_SC_BACKSPACE 0x0E
#define KB_SC_ENTER     0x1C
#define KB_SC_ESCAPE    0x01
#define KB_SC_BREAK     0x80    /* OR'd with make code for key-release events */

/* Line buffer size for shell input */
#define KB_LINE_BUF_SIZE  256

/* ── Public API ─────────────────────────────────────────────────────────── */

void keyboard_init(void);

/*
 * keyboard_getchar — blocking read.
 * Polls the PS/2 data port until a printable ASCII character arrives.
 * Returns the character; returns 0 for non-printable keys.
 */
char keyboard_getchar(void);

/*
 * keyboard_readline — reads one line of input into buf (max len-1 chars).
 * Echoes each character to the VGA display, handles Backspace and Enter.
 * Returns the number of characters read (excluding NUL terminator).
 *
 * Extension: add command history support here (up/down arrow handling).
 */
int  keyboard_readline(char *buf, int len);

/* Raw scancode read (non-blocking — returns 0 if no data ready) */
uint8_t keyboard_read_scancode(void);

/* Translate Set-1 scancode to ASCII (0 if non-printable) */
char keyboard_scancode_to_ascii(uint8_t sc);

#endif /* KEYBOARD_H */
