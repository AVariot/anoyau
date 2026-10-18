#ifndef PRINTK_H
#define PRINTK_H

#include <stdarg.h>

void printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* PRINTK_H */
