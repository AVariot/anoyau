
#include "shell.h"
#include "tty.h"
#include "printk.h"
#include "string.h"
#include "vga.h"
#include <stdint.h>
#include "timer.h"
#include "pmm.h"

#define MAX_ARG 8

struct cmd_s {
    char *cmd_id;
    int (*cmd_fn)(char **);
};

static int cmd_ls(char **line) {
    (void)line;
    printk("./\n");
    return 0;
}

static int cmd_help(char **line) {
    (void)line;
    printk("help:\n");
    printk("    help: get all command\n");
    printk("    ls: list all element from current directory\n");
    printk("    clear: clear the screen\n");
    printk("    ticks: display the number of ticks\n");
    printk("    echo: display a string\n");
    printk("    mem: display the number of frame available\n");
    printk("---\n");
    return 0;
}

static int cmd_clear(char **line) {
    (void)line;
    vga_clear();
    return 0;
}

static int cmd_ticks(char **line) {
    (void)line;
    printk("Tick number: %u\n", (unsigned int)timer_get_ticks());
    return 0;
}

static int cmd_echo(char **line) {
    while (*line) {
        printk("%s", *line);
        line++;
        if (*line) {
            printk(" ");
        }
    }
    printk("\n");
    return 0;
}

static int cmd_mem(char **line) {
    (void)line;
    size_t libres = pmm_free_count();
    printk("free: %lu frames (%lu KiB)\n", libres, libres * FRAME_SIZE / 1024);
    return 0;
}

static const struct cmd_s commands[] = {
    { "help", cmd_help },
    { "clear", cmd_clear },
    { "ls", cmd_ls },
    { "ticks", cmd_ticks },
    { "echo", cmd_echo },
    { "mem", cmd_mem },
};

void shell(void) {
    char line[64];
    uint8_t exe = 0;
    char *argv[MAX_ARG + 1];
    int argc = 0;
    
    for (;;) {
        printk("> ");
        tty_readline(line, sizeof(line));

        argc = split_line(line, argv, MAX_ARG);
        if (argc == 0) {
            continue;
        }
        argv[argc] = NULL;
        for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
            if (strcmp(argv[0], commands[i].cmd_id) == 0) {
                commands[i].cmd_fn(&argv[1]);
                exe = 1;
                break;
            }
        }
        if (exe == 0) {
            printk("Command not found: %s\n", argv[0]);
        }
        exe = 0;
    }
}
