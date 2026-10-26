
#ifndef TTY_H
#define TTY_H

/* Reads one line from the keyboard into buf (at most max - 1 characters).
 * Echoes what is typed, handles backspace, returns when Enter is pressed.
 * The line is terminated by '\0'. Returns its length. */
int tty_readline(char *buf, int max);

#endif /* TTY_H */
