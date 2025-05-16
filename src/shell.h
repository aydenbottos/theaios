#ifndef SHELL_H
#define SHELL_H

/* Feed one character of keyboard input to the shell’s line editor */
void shell_feed(char c);

/* Initialise internal buffers and display the first prompt */
void shell_init(void);

#endif /* SHELL_H */
