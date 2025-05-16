// user.c
extern int write(int, const char*, unsigned int);
extern void exit(int);

void _start(void) {
    const char *msg = "Hello from user program!\n";
    write(1, msg, 26);
    exit(0);
}
