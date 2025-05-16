#include <stdint.h>
#include "util.h"

static uint16_t* const vmem = (uint16_t*)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;
static const int WIDTH = 80;
static const int HEIGHT = 25;

void clear_screen(void) {
    for (int i = 0; i < WIDTH*HEIGHT; i++) {
        vmem[i] = (7 << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

void putc(char c, uint8_t col) {
    if (c == '\b') {
        // backspace
        if (cursor_x > 0) {
            cursor_x--;
            vmem[cursor_y*WIDTH + cursor_x] = (col << 8) | ' ';
        }
        return;
    }
    if (c == '\r') {
        // carriage return
        cursor_x = 0;
        return;
    }
    if (c == '\n') {
        // newline
        cursor_x = 0;
        cursor_y++;
    } else if ((uint8_t)c >= 32 && (uint8_t)c <= 126) {
        // printable
        vmem[cursor_y*WIDTH + cursor_x] = (col << 8) | c;
        cursor_x++;
    } else {
        // ignore everything else
        return;
    }

    // wrap or scroll
    if (cursor_x >= WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= HEIGHT) {
        // simple scroll up by one line
        for (int y = 1; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                vmem[(y-1)*WIDTH + x] = vmem[y*WIDTH + x];
            }
        }
        // clear bottom line
        for (int x = 0; x < WIDTH; x++) {
            vmem[(HEIGHT-1)*WIDTH + x] = (col << 8) | ' ';
        }
        cursor_y = HEIGHT - 1;
    }
}

void puts(const char* s){ while(*s) putc(*s++, 7); }

void memset(void* d, uint8_t v, uint32_t n){ uint8_t* p=d; while(n--) *p++=v; }

/* very small helpers */
uint8_t inb(uint16_t p){ uint8_t r; asm volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r;}
void outb(uint16_t p, uint8_t v){ asm volatile("outb %0,%1"::"a"(v),"Nd"(p)); }

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        ++a; ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

void *memcpy(void *dest, const void *src, uint32_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

int atoi(const char *s) {
    int sign = 1, res = 0;
    if (*s == '-') { sign = -1; s++; }
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return sign * res;
}

void itoa(int value, char *buf, int base) {
    char tmp[33];
    int i = 0, sign = 0;
    if (value < 0) { sign = 1; value = -value; }
    do {
        tmp[i++] = "0123456789ABCDEF"[value % base];
        value /= base;
    } while (value);
    if (sign) tmp[i++] = '-';
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j] = '\0';
}

int strncmp(const char *a, const char *b, int n) {
    for (int i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb || ca == '\0' || cb == '\0')
            return ca - cb;
    }
    return 0;
}

uint16_t inw(uint16_t p) {
    uint16_t r;
    asm volatile("inw %1,%0" : "=a"(r) : "Nd"(p));
    return r;
}

int memcmp(const void *a, const void *b, uint32_t n) {
    const uint8_t* p=a; const uint8_t* q=b;
    for(uint32_t i=0;i<n;i++) if(p[i]!=q[i]) return p[i]-q[i];
    return 0;
}

static uint32_t rand_seed = 1;
uint32_t rand32(void) {
    rand_seed = rand_seed * 1103515245 + 12345;
    return rand_seed >> 1; /* 31-bit */
}
