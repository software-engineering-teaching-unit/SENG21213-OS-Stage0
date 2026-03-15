#ifndef KERNEL_H
#define KERNEL_H

/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: Kernel header
 *
 * Declares kernel_main() — the C entry point called by boot.asm
 * after the CPU is in 32-bit Protected Mode.
 *
 * Lecture reference: L07 §4, L08 §2
 */

/* Entry point — called from boot.asm after Protected Mode switch */
void kernel_main(void);

/* Kernel halt — disables interrupts and stops the CPU */
void kernel_halt(void);

/* Kernel version string */
#define KERNEL_VERSION  "0.1.0-stage0"
#define KERNEL_NAME     "SENG21213 Kernel"

#endif /* KERNEL_H */
