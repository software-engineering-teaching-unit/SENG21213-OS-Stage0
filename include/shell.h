#ifndef SHELL_H
#define SHELL_H

/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: Shell
 *
 * A simple line-editing shell that supports a small set of built-in
 * commands.  Students are expected to extend this as part of the
 * OS assignment.
 *
 * Built-in commands (Stage 0):
 *   help    — list available commands
 *   clear   — clear the screen
 *   echo    — print arguments back to the screen
 *   version — print kernel version
 *   halt    — stop the CPU
 *
 * Extension ideas:
 *   - Add command history (up/down arrow) — see keyboard_readline()
 *   - Add tab-completion scanning the command table
 *   - Add colour command: colour <fg> <bg>
 *   - After Stage 1: add  ps, kill, nice commands
 *   - After Stage 4: add  ls, touch, cat, write, rm commands
 */

void shell_run(void);

#endif /* SHELL_H */
