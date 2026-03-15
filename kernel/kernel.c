/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: Kernel Entry Point — kernel.c
 *
 * kernel_main() is called by boot.asm immediately after the CPU
 * has entered 32-bit Protected Mode and a stack has been set up.
 *
 * At this point:
 *   - CS = 0x08  (GDT code segment, ring 0)
 *   - DS = ES = SS = 0x10  (GDT data segment, ring 0)
 *   - ESP = 0x90000  (set by bootloader)
 *   - Interrupts are DISABLED (CLI was executed before CR0 write)
 *   - We are running in a flat 4 GB address space (base = 0)
 *
 * This function:
 *   1. Initialises the VGA text-mode driver
 *   2. Initialises the PS/2 keyboard driver
 *   3. Launches the interactive shell
 *
 * Lecture reference: L07 §4, L08 §1
 *
 * ── Stages roadmap ───────────────────────────────────────────────────────────
 *   Stage 0  (this file) — VGA + Keyboard + Shell
 *   Stage 1  — Add IDT, PIT timer, process table, Round-Robin scheduler
 *   Stage 2  — Add kernel threads, mutex, semaphore
 *   Stage 3  — Add physical memory manager (bitmap frame allocator)
 *   Stage 4  — Add RAM disk file system (inode-based)
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "../include/kernel.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/shell.h"

/* ─────────────────────────────────────────────────────────────────────────
 * kernel_main — C entry point (called from boot.asm)
 *
 * NOTE: The function signature has no parameters because boot.asm
 * calls it with a direct JMP, not a CALL — no return address is pushed.
 * kernel_main() must never return (it ends with an infinite loop).
 * ───────────────────────────────────────────────────────────────────────── */
void kernel_main(void)
{
    /* ── Step 1: Initialise VGA text-mode driver ────────────────────────── */
    vga_init();

    /* ── Step 2: Print boot status ──────────────────────────────────────── */
    vga_set_colour(VGA_GREEN, VGA_BLACK);
    vga_puts("[  OK  ] VGA driver initialised\n");

    /* ── Step 3: Initialise keyboard driver ─────────────────────────────── */
    keyboard_init();
    vga_set_colour(VGA_GREEN, VGA_BLACK);
    vga_puts("[  OK  ] PS/2 keyboard driver initialised\n");

    /*
     * ── Future initialisations (add here for Stages 1–4) ─────────────────
     *
     * Stage 1:
     *   idt_init();        // Install IDT (256 entries)
     *   pit_init(100);     // Program PIT for 100 Hz (10 ms tick)
     *   scheduler_init();  // Initialise Round-Robin process table
     *   vga_puts("[  OK  ] Scheduler initialised\n");
     *
     * Stage 2:
     *   // (threads/mutexes are created per-process — no global init needed)
     *
     * Stage 3:
     *   pmm_init();        // Parse BIOS memory map, build frame bitmap
     *   vga_printf("[  OK  ] Memory manager: %u MB available\n",
     *              pmm_free_frames() * 4 / 1024);
     *
     * Stage 4:
     *   ramdisk_init();    // Allocate 1 MB RAM disk
     *   fs_init();         // Initialise inode file system on RAM disk
     *   vga_puts("[  OK  ] File system mounted\n");
     * ─────────────────────────────────────────────────────────────────────
     */

    vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("\n");

    /* ── Step 4: Start the interactive shell ───────────────────────────── */
    shell_run();

    /* shell_run() never returns — but in case it does, halt */
    kernel_halt();
}

/* ─────────────────────────────────────────────────────────────────────────
 * kernel_halt — disable interrupts and stop the CPU
 * ───────────────────────────────────────────────────────────────────────── */
void kernel_halt(void)
{
    __asm__ volatile (
        "cli\n"     /* Clear Interrupt Flag — disable all maskable interrupts */
        "hlt\n"     /* Halt — CPU stops until next interrupt (none will come) */
        "1: jmp 1b" /* Safety loop — should never be reached */
    );
}
