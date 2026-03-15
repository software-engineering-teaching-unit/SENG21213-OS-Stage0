# SENG 21213 — OS Assignment Stage 0: Boot & Shell

**University of Kelaniya · Department of Software Engineering**  
**Eng. Dr. Tiroshan Madushanka**

---

## Overview

Stage 0 is the foundation for all subsequent OS assignment stages.  
Starting from a **512-byte NASM MBR bootloader**, the system:

1. Loads a 32 KB kernel from disk via BIOS `INT 0x13` extended read  
2. Builds a minimal 3-entry **GDT** (null / code / data)  
3. Switches the CPU from 16-bit **Real Mode** to 32-bit **Protected Mode** (CR0 bit 0)  
4. Executes a **FAR JUMP** to flush the instruction pipeline  
5. Initialises a **VGA text-mode driver** (writes directly to `0xB8000`)  
6. Initialises a **PS/2 keyboard driver** (polling mode)  
7. Launches an **interactive shell** with built-in commands

> **Lecture alignment:** L07 §4–5, L08 §1–3  
> **Stallings reference:** Ch. 2 (Computer Organisation), Ch. 11 (I/O)

---

## Directory Structure

```
stage0/
├── boot/
│   └── boot.asm          MBR bootloader (512 bytes, NASM)
├── kernel/
│   ├── kernel.c          kernel_main() — C entry point
│   ├── shell.c           Interactive shell + command table
│   └── string.c          Minimal libc replacement (memset, strlen, etc.)
├── drivers/
│   ├── vga/
│   │   └── vga.c         VGA text-mode driver (0xB8000)
│   └── keyboard/
│       └── keyboard.c    PS/2 keyboard driver (polling)
├── include/
│   ├── kernel.h
│   ├── vga.h
│   ├── keyboard.h
│   ├── io.h              Inline x86 IN/OUT port helpers
│   ├── shell.h
│   └── string.h
├── linker.ld             Linker script (loads kernel at 0x10000)
├── Makefile              Full build system
└── run.sh                Convenience QEMU launch script
```

---

## Prerequisites

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install nasm gcc qemu-system-i386 make gdb
```

For the recommended cross-compiler (avoids `-m32` host issues):

```bash
# Download and build i686-elf-gcc — see https://wiki.osdev.org/GCC_Cross-Compiler
# Or use the prebuilt packages:
sudo apt install gcc-multilib   # Enables -m32 on host gcc
```

### macOS (Homebrew)

```bash
brew install nasm qemu gdb
# For the cross-compiler:
brew install i686-elf-gcc
```

---

## Build & Run

```bash
# Build the disk image
make

# Run in QEMU
make run
# or:
./run.sh

# Clean and rebuild
make clean && make
```

---

## Debugging with GDB

Open **two terminals**:

**Terminal 1 — start QEMU paused:**
```bash
make debug
# or:
./run.sh debug
```

**Terminal 2 — attach GDB:**
```bash
make gdb
```

This will:
1. Load the kernel ELF (with debug symbols)
2. Connect to QEMU on `localhost:1234`
3. Set a breakpoint at `kernel_main`
4. Continue execution (QEMU unpauses)

Useful GDB commands inside the kernel:
```
(gdb) break vga_puts           # Break before printing
(gdb) break shell_run          # Break when shell starts
(gdb) info registers           # Inspect CPU registers
(gdb) x/10xw 0xB8000           # Inspect VGA buffer
(gdb) x/10xw 0x90000           # Inspect kernel stack
(gdb) stepi                    # Single-step one instruction
(gdb) layout asm               # Show disassembly pane
```

---

## Shell Commands (Stage 0)

| Command | Description |
|---|---|
| `help` | List all available commands |
| `clear` | Clear the screen |
| `echo <text>` | Print text to screen |
| `version` | Show kernel name and version |
| `colour <fg> <bg>` | Change text colour (0–15) |
| `halt` | Disable interrupts and stop CPU |

---

## Extending for Stages 1–4

Each subsequent stage adds new commands and subsystems.  
Add new commands to the `commands[]` table in `kernel/shell.c`.

### Stage 1 — Scheduler
- Add `ps` command (list processes)
- Files to create: `kernel/process.c`, `kernel/scheduler.c`, `boot/switch.asm`
- See commented stubs in `kernel/kernel.c` and `include/shell.h`

### Stage 2 — Threads & Mutex
- Add thread creation from the shell
- Files to create: `kernel/thread.c`, `kernel/mutex.c`, `kernel/semaphore.c`

### Stage 3 — Memory Manager
- Add `meminfo` command
- Files to create: `kernel/pmm.c`

### Stage 4 — File System
- Add `ls`, `touch`, `cat`, `write`, `rm` commands
- Files to create: `kernel/ramdisk.c`, `kernel/fs.c`

---

## Key Concepts Demonstrated

| Concept | File | Lecture |
|---|---|---|
| 512-byte MBR structure | `boot/boot.asm` | L07 §4 |
| INT 0x13 extended read (DAP) | `boot/boot.asm` | L07 §4.1 |
| GDT descriptor format | `boot/boot.asm` | L07 §4.2 |
| CR0 PE bit — Protected Mode | `boot/boot.asm` | L07 §4.3 |
| FAR JUMP pipeline flush | `boot/boot.asm` | L07 §4.3 |
| VGA text buffer at 0xB8000 | `drivers/vga/vga.c` | L07 §5.1 |
| CRT controller cursor I/O ports | `drivers/vga/vga.c` | L02 §3 |
| PS/2 port 0x60 / 0x64 | `drivers/keyboard/keyboard.c` | L08 §3 |
| Polling vs interrupt-driven I/O | `drivers/keyboard/keyboard.c` | L02 §3 |
| IN/OUT instructions (inline asm) | `include/io.h` | L07 §2 |
| Freestanding C (no libc) | `kernel/string.c` | L07 §5 |

---

## Submission

Tag your Stage 0 release in GitLab:

```bash
git add .
git commit -m "Stage 0: Boot, VGA, keyboard, shell"
git tag v0.1-stage0
git push origin main --tags
```

Marking criteria:
- [ ] Boots in QEMU without crashing
- [ ] VGA driver displays text correctly
- [ ] Keyboard input is correctly echoed
- [ ] `help`, `clear`, `echo`, `version`, `halt` all work
- [ ] Code is well-commented with Stallings lecture references
- [ ] At least one extension from the list in the Stage 0 blog article

---

## References

- Stallings, W. *Computer Organization and Architecture*, 10th Ed.
- OSDev Wiki: https://wiki.osdev.org
- Intel® 64 and IA-32 Architectures SDM: https://www.intel.com/sdm
- NASM Manual: https://nasm.us/doc/
- QEMU documentation: https://www.qemu.org/docs/

---

*© 2025 Tiroshan Madushanka · SENG 21213 · University of Kelaniya*
