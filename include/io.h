#ifndef IO_H
#define IO_H

/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: x86 Port I/O Helpers
 *
 * x86 I/O ports are separate from memory — accessed with IN/OUT instructions.
 * GCC inline assembly wraps these as C-callable functions.
 *
 * Lecture reference: L07 §2 (IN/OUT instructions), L02 §3 (I/O techniques)
 *
 * The `volatile` keyword prevents the compiler reordering or eliminating
 * I/O instructions.  The "memory" clobber ensures the compiler flushes all
 * cached values before / after the I/O operation.
 */

#include <stdint.h>

/* ── Write byte to I/O port ─────────────────────────────────────────────── */
static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
        : "memory"
    );
}

/* ── Write word (16-bit) to I/O port ────────────────────────────────────── */
static inline void outw(uint16_t port, uint16_t value)
{
    __asm__ volatile (
        "outw %0, %1"
        :
        : "a"(value), "Nd"(port)
        : "memory"
    );
}

/* ── Read byte from I/O port ────────────────────────────────────────────── */
static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );
    return value;
}

/* ── Read word (16-bit) from I/O port ───────────────────────────────────── */
static inline uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ volatile (
        "inw %1, %0"
        : "=a"(value)
        : "Nd"(port)
        : "memory"
    );
    return value;
}

/*
 * io_wait — insert a small I/O delay by writing to port 0x80
 * (unused diagnostics port — safe on all x86 hardware).
 * Required by some older ISA devices that need time between I/O operations.
 */
static inline void io_wait(void)
{
    outb(0x80, 0x00);
}

#endif /* IO_H */
