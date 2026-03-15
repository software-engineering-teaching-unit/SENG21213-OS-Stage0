/*
 * SENG 21213 — Computer Architecture & Operating Systems
 * Stage 0: Shell — shell.c
 *
 * A minimal interactive shell.  Reads a line from the keyboard,
 * tokenises on whitespace, looks up the first token in a command
 * table, and dispatches.
 *
 * ── Extension opportunities ──────────────────────────────────────────────────
 *   1. Add command history (see keyboard.h)
 *   2. Add tab-completion
 *   3. After Stage 1: ps, kill, nice
 *   4. After Stage 4: ls, touch, cat, write, rm
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "../include/shell.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/string.h"
#include "../include/kernel.h"

/* ── Command table ──────────────────────────────────────────────────────── */
#define MAX_ARGS   16
#define MAX_CMDS   32

typedef void (*cmd_fn_t)(int argc, char *argv[]);

typedef struct {
    const char *name;
    const char *help;
    cmd_fn_t    fn;
} command_t;

/* ── Forward declarations ───────────────────────────────────────────────── */
static void cmd_help   (int argc, char *argv[]);
static void cmd_clear  (int argc, char *argv[]);
static void cmd_echo   (int argc, char *argv[]);
static void cmd_version(int argc, char *argv[]);
static void cmd_halt   (int argc, char *argv[]);
static void cmd_colour (int argc, char *argv[]);

/* ── Command table ──────────────────────────────────────────────────────── */
static const command_t commands[] = {
    { "help",    "List available commands",         cmd_help    },
    { "clear",   "Clear the screen",                cmd_clear   },
    { "echo",    "Print arguments to screen",       cmd_echo    },
    { "version", "Show kernel version",             cmd_version },
    { "colour",  "Set text colour: colour <fg> <bg>", cmd_colour },
    { "halt",    "Halt the CPU",                    cmd_halt    },
    /*
     * ── Add your Stage 1–4 commands below this line ──────────────────────
     * { "ps",    "List processes",                  cmd_ps    },
     * { "kill",  "Kill process by PID",             cmd_kill  },
     * { "ls",    "List files",                      cmd_ls    },
     * { "touch", "Create empty file",               cmd_touch },
     * { "cat",   "Print file contents",             cmd_cat   },
     * { "write", "Write text to file",              cmd_write },
     * { "rm",    "Remove file",                     cmd_rm    },
     * { "meminfo","Show memory usage",              cmd_meminfo},
     * ─────────────────────────────────────────────────────────────────────
     */
    { 0, 0, 0 }
};

/* ── Shell banner ───────────────────────────────────────────────────────── */
static void print_banner(void)
{
    vga_set_colour(VGA_CYAN, VGA_BLACK);
    vga_puts("+=================================================+\n");
    vga_puts("|   SENG 21213 - Stage 0 Kernel Shell            |\n");
    vga_puts("|   University of Kelaniya                        |\n");
    vga_puts("|   Type 'help' for available commands            |\n");
    vga_puts("+=================================================+\n");
    vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
    vga_putchar('\n');
}

/* ── Prompt ─────────────────────────────────────────────────────────────── */
static void print_prompt(void)
{
    vga_set_colour(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("kernel");
    vga_set_colour(VGA_WHITE, VGA_BLACK);
    vga_puts("> ");
    vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
}

/* ── Tokeniser ──────────────────────────────────────────────────────────── */
static int tokenise(char *line, char *argv[], int max_args)
{
    int argc = 0;
    char *p  = line;

    while (*p && argc < max_args) {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        argv[argc++] = p;

        /* Advance to end of token */
        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) *p++ = '\0';
    }
    argv[argc] = 0;
    return argc;
}

/* ─────────────────────────────────────────────────────────────────────────
 * shell_run — main loop
 * ───────────────────────────────────────────────────────────────────────── */
void shell_run(void)
{
    static char  line[KB_LINE_BUF_SIZE];
    static char *argv[MAX_ARGS];

    print_banner();

    for (;;) {
        print_prompt();

        keyboard_readline(line, sizeof(line));

        if (line[0] == '\0')
            continue;

        int argc = tokenise(line, argv, MAX_ARGS);
        if (argc == 0)
            continue;

        /* Look up command */
        int found = 0;
        for (int i = 0; commands[i].name; i++) {
            if (strcmp(argv[0], commands[i].name) == 0) {
                commands[i].fn(argc, argv);
                found = 1;
                break;
            }
        }

        if (!found) {
            vga_set_colour(VGA_LIGHT_RED, VGA_BLACK);
            vga_printf("Unknown command: %s\n", argv[0]);
            vga_puts("Type 'help' for available commands.\n");
            vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
        }
    }
}

/* ── Built-in command implementations ──────────────────────────────────── */

static void cmd_help(int argc, char *argv[])
{
    (void)argc; (void)argv;
    vga_set_colour(VGA_YELLOW, VGA_BLACK);
    vga_puts("\nAvailable commands:\n");
    vga_puts("-------------------\n");
    vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);

    for (int i = 0; commands[i].name; i++) {
        vga_set_colour(VGA_WHITE, VGA_BLACK);
        vga_printf("  %-12s", commands[i].name);
        vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
        vga_printf("  %s\n", commands[i].help);
    }
    vga_putchar('\n');
}

static void cmd_clear(int argc, char *argv[])
{
    (void)argc; (void)argv;
    vga_clear();
}

static void cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        vga_puts(argv[i]);
        if (i < argc - 1) vga_putchar(' ');
    }
    vga_putchar('\n');
}

static void cmd_version(int argc, char *argv[])
{
    (void)argc; (void)argv;
    vga_set_colour(VGA_CYAN, VGA_BLACK);
    vga_printf("%s v%s\n", KERNEL_NAME, KERNEL_VERSION);
    vga_puts("Built for SENG 21213 — University of Kelaniya\n");
    vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
}

static void cmd_halt(int argc, char *argv[])
{
    (void)argc; (void)argv;
    vga_set_colour(VGA_LIGHT_RED, VGA_BLACK);
    vga_puts("\nHalting CPU. Power off the machine.\n");
    vga_set_colour(VGA_LIGHT_GREY, VGA_BLACK);
    kernel_halt();
}

static void cmd_colour(int argc, char *argv[])
{
    if (argc < 3) {
        vga_puts("Usage: colour <fg 0-15> <bg 0-15>\n");
        vga_puts("Colours: 0=Black 1=Blue 2=Green 3=Cyan 4=Red 5=Magenta\n");
        vga_puts("         6=Brown 7=LGrey 8=DGrey 9=LBlue 10=LGreen\n");
        vga_puts("         11=LCyan 12=LRed 13=LMagenta 14=Yellow 15=White\n");
        return;
    }
    /* Simple atoi */
    int fg = 0, bg = 0;
    for (const char *p = argv[1]; *p >= '0' && *p <= '9'; p++)
        fg = fg * 10 + (*p - '0');
    for (const char *p = argv[2]; *p >= '0' && *p <= '9'; p++)
        bg = bg * 10 + (*p - '0');

    if (fg > 15) fg = 15;
    if (bg > 15) bg = 15;
    vga_set_colour((vga_colour_t)fg, (vga_colour_t)bg);
    vga_printf("Colour set to fg=%d bg=%d\n", fg, bg);
}
