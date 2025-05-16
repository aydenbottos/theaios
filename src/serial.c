#include "serial.h"
#include "util.h"

#define COM1_PORT   0x3F8

void serial_init(void) {
    outb(COM1_PORT+1, 0x00);    // Disable all interrupts
    outb(COM1_PORT+3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT+0, 0x01);    // Divisor = 1 (lo byte) 115200 baud
    outb(COM1_PORT+1, 0x00);    //            (hi byte)
    outb(COM1_PORT+3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT+2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT+4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_putc(char c) {
    while (!(inb(COM1_PORT+5) & 0x20)); // wait for THR empty
    outb(COM1_PORT, c);
}

void serial_puts(const char *s) {
    while (*s) serial_putc(*s++);
}
