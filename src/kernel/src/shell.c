
#include "shell.h"
#include "tty.h"
#include "printk.h"
#include "string.h"
#include "vga.h"
#include <stdint.h>

struct cmd_s {
    char *cmd_id;
    int (*cmd_fn)(char *);
};

static int cmd_ls(char line[64]) {
    (void)line;
    printk("./\n");
    return 0;
}

static int cmd_help(char line[64]) {
    (void)line;
    printk("hekp:\n");
    return 0;
}

static int cmd_clear(char line[64]) {
    (void)line;
    vga_clear();
    return 0;
}

static const struct cmd_s commands[] = {
    { "help", cmd_help },
    { "clear", cmd_clear },
    { "ls", cmd_ls },
};

void shell(void) {
    char line[64];
    uint8_t exe = 0;
    
    for (;;) {
        printk("> ");
        tty_readline(line, sizeof(line));
        for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
            if (strcmp(line, commands[i].cmd_id) == 0) {
                commands[i].cmd_fn(line);
                exe = 1;
                break;
            }
        }
        if (exe == 0) {
            printk("Command not found: %s\n", line);
        }
        exe = 0;
    }
}
