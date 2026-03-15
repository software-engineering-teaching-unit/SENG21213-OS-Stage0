/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: Linker Script — linker.ld
 *
 * Links all kernel object files into a flat binary loaded at 0x10000.
 * The bootloader loads the kernel at physical address 0x10000
 * (KERNEL_LOAD_SEG = 0x1000, offset = 0x0000 → 0x1000 * 0x10 = 0x10000).
 *
 * Sections:
 *   .text   — executable code    (placed first, must be at 0x10000)
 *   .rodata — read-only data      (string literals, const arrays)
 *   .data   — initialised data    (global variables with initial values)
 *   .bss    — uninitialised data  (zero-filled by the kernel at startup)
 *
 * The ENTRY symbol kernel_main is used by the linker to determine
 * where execution begins (no effect on flat binary, but good practice).
 */

ENTRY(kernel_main)

SECTIONS {
    . = 0x10000;        /* Load address — must match KERNEL_LOAD_SEG * 0x10 */

    .text : {
        *(.text)
        *(.text.*)
    }

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    }

    .data : {
        *(.data)
        *(.data.*)
    }

    .bss : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
