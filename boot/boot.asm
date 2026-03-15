; =============================================================================
; SENG 21213 — Computer Architecture & Operating Systems
; Stage 0 Bootloader — boot.asm
;
; A minimal 512-byte Master Boot Record (MBR) that:
;   1. Sets up a stack in Real Mode (16-bit)
;   2. Loads the kernel from disk via INT 0x13 extended read
;   3. Builds a minimal 3-entry Global Descriptor Table (GDT)
;   4. Switches the CPU from 16-bit Real Mode to 32-bit Protected Mode
;      by setting CR0 bit 0
;   5. Executes a FAR JUMP to flush the instruction pipeline
;   6. Jumps to kernel_main() in 32-bit C code
;
; Lecture reference: L07 §4, L08 §2  (Stallings Ch. 11, 2)
;
; Build:  nasm -f bin boot.asm -o boot.bin
; =============================================================================

BITS 16
ORG 0x7C00                      ; BIOS loads MBR here

KERNEL_LOAD_SEG  EQU 0x1000     ; Segment where kernel will be loaded
KERNEL_LOAD_OFF  EQU 0x0000     ; Offset within that segment
KERNEL_SECTORS   EQU 64         ; Number of 512-byte sectors to load (~32 KB)
KERNEL_LBA       EQU 1          ; Start LBA (sector right after MBR)

; -----------------------------------------------------------------------------
; Entry point — BIOS jumps here after POST
; -----------------------------------------------------------------------------
start:
    cli                         ; Disable interrupts during setup
    xor ax, ax
    mov ds, ax                  ; Clear segment registers
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; Stack grows downward from 0x7C00

    ; Save boot drive number (BIOS puts it in DL)
    mov [boot_drive], dl

    sti                         ; Re-enable interrupts

    ; ── Print boot message ───────────────────────────────────────────────────
    mov si, msg_loading
    call print_string

    ; ── Load kernel from disk ────────────────────────────────────────────────
    ; Use INT 0x13 AH=0x42 (Extended Read) — avoids 8 GB CHS limit
    mov ax, KERNEL_LOAD_SEG
    mov es, ax                  ; Destination segment
    xor bx, bx                  ; Destination offset = 0

    mov si, dap                 ; DS:SI -> Disk Address Packet
    mov dl, [boot_drive]
    mov ah, 0x42                ; Extended Read
    int 0x13
    jc disk_error               ; CF set on error

    mov si, msg_ok
    call print_string

    ; ── Disable interrupts before GDT / mode switch ──────────────────────────
    cli

    ; ── Load the GDT ─────────────────────────────────────────────────────────
    lgdt [gdt_descriptor]

    ; ── Switch to Protected Mode: set CR0 bit 0 ──────────────────────────────
    ; Lecture note: "Setting CR0 PE bit enables Protected Mode.
    ; The CPU still executes 16-bit code until the pipeline is flushed
    ; with a FAR JUMP."  (L07 §4.3)
    mov eax, cr0
    or  eax, 0x1                ; Set Protection Enable bit
    mov cr0, eax

    ; ── FAR JUMP — flushes the prefetch queue / pipeline ─────────────────────
    ; Selector 0x08 = GDT entry 1 (code segment, base=0, limit=4 GB, ring 0)
    jmp 0x08:protected_mode_entry

; -----------------------------------------------------------------------------
; print_string — prints a NUL-terminated string via BIOS INT 0x10 (Real Mode)
; Input: SI = pointer to string
; -----------------------------------------------------------------------------
print_string:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

; -----------------------------------------------------------------------------
; disk_error — called if INT 0x13 fails
; -----------------------------------------------------------------------------
disk_error:
    mov si, msg_error
    call print_string
.halt:
    hlt
    jmp .halt

; =============================================================================
; 32-BIT PROTECTED MODE CODE
; After the FAR JUMP above, the CPU is in 32-bit mode.
; =============================================================================
BITS 32
protected_mode_entry:
    ; Set all data segment registers to selector 0x10
    ; (GDT entry 2 — data segment, base=0, limit=4 GB, ring 0)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000            ; Set up a 32-bit stack below the kernel

    ; Jump to kernel entry point
    ; Kernel was loaded at physical address KERNEL_LOAD_SEG << 4 = 0x10000
    jmp KERNEL_LOAD_SEG * 0x10 + KERNEL_LOAD_OFF

; =============================================================================
; DATA
; =============================================================================
BITS 16

boot_drive: db 0

msg_loading: db 'SENG21213 Stage 0 — Loading kernel...', 0x0D, 0x0A, 0
msg_ok:      db 'Kernel loaded. Switching to Protected Mode.', 0x0D, 0x0A, 0
msg_error:   db 'DISK ERROR — halting.', 0x0D, 0x0A, 0

; ── Disk Address Packet (DAP) for INT 0x13 AH=0x42 ─────────────────────────
; Struct layout (Lecture L07 §4.1):
;   Offset  Size  Field
;   0       1     Packet size (always 0x10)
;   1       1     Reserved (0)
;   2       2     Number of sectors to transfer
;   4       4     Buffer address (offset:segment)
;   8       8     Starting LBA sector number
dap:
    db 0x10                     ; Packet size
    db 0x00                     ; Reserved
    dw KERNEL_SECTORS           ; Sectors to read
    dw KERNEL_LOAD_OFF          ; Buffer offset
    dw KERNEL_LOAD_SEG          ; Buffer segment
    dd KERNEL_LBA               ; LBA low 32 bits
    dd 0                        ; LBA high 32 bits

; =============================================================================
; GDT — Global Descriptor Table
;
; Entry 0: Null descriptor (required by x86 architecture)
; Entry 1: Code segment — base=0, limit=0xFFFFF (4 GB), ring 0, 32-bit
; Entry 2: Data segment — base=0, limit=0xFFFFF (4 GB), ring 0, 32-bit
;
; Descriptor format (8 bytes):
;   Bits 15:0   — Limit bits 15:0
;   Bits 31:16  — Base bits 15:0
;   Bits 39:32  — Base bits 23:16
;   Bits 47:40  — Access byte (P DPL S Type)
;   Bits 51:48  — Limit bits 19:16
;   Bits 55:52  — Flags (G DB L AVL)
;   Bits 63:56  — Base bits 31:24
;
; Lecture reference: L07 §4.2, L08 §2  (Stallings Ch. 2)
; =============================================================================
gdt_start:

; Entry 0 — Null descriptor
gdt_null:
    dd 0x00000000
    dd 0x00000000

; Entry 1 — Code segment (selector 0x08)
;   Access: 0x9A = Present, Ring 0, Code, Readable, Non-conforming
;   Flags:  0xCF = 4 KB granularity, 32-bit, limit = 4 GB
gdt_code:
    dw 0xFFFF                   ; Limit bits 15:0
    dw 0x0000                   ; Base bits 15:0
    db 0x00                     ; Base bits 23:16
    db 0x9A                     ; Access: P=1 DPL=0 S=1 Type=1010 (code, r/x)
    db 0xCF                     ; Flags + Limit bits 19:16
    db 0x00                     ; Base bits 31:24

; Entry 2 — Data segment (selector 0x10)
;   Access: 0x92 = Present, Ring 0, Data, Writable
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92                     ; Access: P=1 DPL=0 S=1 Type=0010 (data, r/w)
    db 0xCF
    db 0x00

gdt_end:

; GDT Descriptor — loaded with LGDT
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; GDT size minus 1
    dd gdt_start                ; Linear address of GDT

; =============================================================================
; Boot sector signature — BIOS checks bytes 510–511 == 0xAA55
; =============================================================================
TIMES 510 - ($ - $$) db 0       ; Pad to 510 bytes
dw 0xAA55                       ; Magic boot signature
