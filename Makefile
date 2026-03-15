# =============================================================================
# SENG 21213 — Computer Architecture & Operating Systems
# Stage 0 Makefile
#
# Targets:
#   make          — build the disk image  (seng21213.img)
#   make run      — build and launch in QEMU
#   make debug    — build and launch QEMU paused, waiting for GDB on :1234
#   make gdb      — attach GDB to a running QEMU debug session
#   make clean    — remove all build artefacts
#   make info     — show binary sizes and section layout
#
# Toolchain requirements:
#   nasm             NASM assembler
#   i686-elf-gcc     Cross-compiler (32-bit ELF target, no host libraries)
#   i686-elf-ld      Cross-linker
#   i686-elf-objcopy Object file converter (ELF → flat binary)
#   qemu-system-i386 Emulator
#   gdb              GNU debugger (for 'make debug' / 'make gdb')
#
# If you don't have i686-elf-gcc, you can use the host gcc with extra flags:
#   CC = gcc
#   CFLAGS += -m32
#   LD = ld
#   LDFLAGS += -m elf_i386
#   OBJCOPY = objcopy
# =============================================================================

# ── Toolchain ─────────────────────────────────────────────────────────────────
AS       := nasm
CC       := i686-elf-gcc
LD       := i686-elf-ld
OBJCOPY  := i686-elf-objcopy
QEMU     := qemu-system-i386
GDB      := gdb

# Fallback to host toolchain if cross-compiler is not available
ifeq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC      := gcc
    LD      := ld
    OBJCOPY := objcopy
    CFLAGS_EXTRA := -m32
    LDFLAGS_EXTRA := -m elf_i386
endif

# ── Flags ─────────────────────────────────────────────────────────────────────
ASFLAGS  := -f elf32
CFLAGS   := -std=c99 -ffreestanding -fno-stack-protector -fno-builtin  \
            -Wall -Wextra -Wno-unused-parameter                         \
            -O2 -g                                                       \
            -Iinclude                                                    \
            $(CFLAGS_EXTRA)
LDFLAGS  := -T linker.ld --oformat binary -nostdlib $(LDFLAGS_EXTRA)

# ── Directories ───────────────────────────────────────────────────────────────
BUILD    := build
BOOT_DIR := boot
KERN_DIR := kernel
VGA_DIR  := drivers/vga
KB_DIR   := drivers/keyboard

# ── Source and object files ───────────────────────────────────────────────────
BOOT_SRC := $(BOOT_DIR)/boot.asm

KERN_SRCS := \
    $(KERN_DIR)/kernel.c  \
    $(KERN_DIR)/shell.c   \
    $(KERN_DIR)/string.c

VGA_SRCS  := $(VGA_DIR)/vga.c
KB_SRCS   := $(KB_DIR)/keyboard.c

ALL_C_SRCS := $(KERN_SRCS) $(VGA_SRCS) $(KB_SRCS)
ALL_C_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(ALL_C_SRCS))

BOOT_BIN  := $(BUILD)/boot.bin
KERNEL_ELF:= $(BUILD)/kernel.elf
KERNEL_BIN:= $(BUILD)/kernel.bin
DISK_IMG  := seng21213.img

# ── Default target ────────────────────────────────────────────────────────────
.PHONY: all run debug gdb clean info
all: $(DISK_IMG)

# ── Create build subdirectories ───────────────────────────────────────────────
$(BUILD):
	@mkdir -p $(BUILD)/$(BOOT_DIR) $(BUILD)/$(KERN_DIR) \
	          $(BUILD)/$(VGA_DIR) $(BUILD)/$(KB_DIR)

# ── Assemble bootloader ───────────────────────────────────────────────────────
$(BOOT_BIN): $(BOOT_SRC) | $(BUILD)
	@echo "  AS    $<"
	@$(AS) -f bin $< -o $@
	@ls -l $@ | awk '{print "  Boot sector: " $$5 " bytes (must be 512)"}'

# ── Compile C sources ─────────────────────────────────────────────────────────
$(BUILD)/%.o: %.c | $(BUILD)
	@mkdir -p $(dir $@)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# ── Link kernel ELF (for GDB symbol debugging) ────────────────────────────────
$(KERNEL_ELF): $(ALL_C_OBJS) linker.ld | $(BUILD)
	@echo "  LD    $@"
	@$(LD) -T linker.ld -nostdlib $(LDFLAGS_EXTRA) $(ALL_C_OBJS) -o $@

# ── Extract flat binary from ELF ──────────────────────────────────────────────
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@

# ── Assemble disk image: boot sector (512 B) + kernel ────────────────────────
$(DISK_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "  IMG   $@"
	@cat $(BOOT_BIN) $(KERNEL_BIN) > $@
	@ls -lh $@ | awk '{print "  Disk image: " $$5}'

# ── QEMU launch ───────────────────────────────────────────────────────────────
QEMU_FLAGS := -drive format=raw,file=$(DISK_IMG),if=ide  \
              -m 32M                                      \
              -no-reboot                                  \
              -serial stdio

run: $(DISK_IMG)
	@echo "  QEMU  $(DISK_IMG)"
	@$(QEMU) $(QEMU_FLAGS)

# ── QEMU debug — pauses and waits for GDB on port 1234 ───────────────────────
debug: $(DISK_IMG)
	@echo "  QEMU  [debug mode] — waiting for GDB on :1234"
	@$(QEMU) $(QEMU_FLAGS) -s -S

# ── GDB — connects to the QEMU debug session ──────────────────────────────────
# Run 'make debug' in one terminal, then 'make gdb' in another.
gdb: $(KERNEL_ELF)
	@$(GDB) -ex "file $(KERNEL_ELF)"          \
	        -ex "target remote localhost:1234" \
	        -ex "break kernel_main"            \
	        -ex "continue"

# ── Show binary sizes ─────────────────────────────────────────────────────────
info: $(KERNEL_ELF)
	@echo ""
	@echo "Section layout:"
	@i686-elf-objdump -h $(KERNEL_ELF) 2>/dev/null || objdump -h $(KERNEL_ELF)
	@echo ""
	@echo "Symbol table (top 20):"
	@i686-elf-nm $(KERNEL_ELF) 2>/dev/null | sort | head -20 || nm $(KERNEL_ELF) | sort | head -20

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD) $(DISK_IMG)
