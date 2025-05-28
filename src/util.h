#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

void *memcpy(void *dest, const void *src, uint32_t n);
uint32_t strlen(const char *s);
char   *strcpy(char *dest, const char *src);
char   *strncpy(char *dest, const char *src, uint32_t n);
int     atoi(const char *s);
void    itoa(int value, char *buf, int base);
int strncmp(const char *a, const char *b, int n);

/* ── basic VGA text‑mode helpers ──────────────────────────────────── */
void  putc  (char c, uint8_t colour);   /* write a single coloured char */
void  puts  (const char *s);            /* write a null‑terminated string */
void  memset(void *dst, uint8_t val, uint32_t len);
void clear_screen(void);

/* ── tiny port‑I/O wrappers ────────────────────────────────────────── */
uint8_t inb (uint16_t port);
void    outb(uint16_t port, uint8_t value);

/* ── minimal libc string stubs used by the shell ───────────────────── */
int     strcmp (const char *a, const char *b);
char   *strcat(char *dest, const char *src);
char   *strrchr(const char *s, int c);
void    utohex(uint32_t val, char *buf);

uint16_t inw(uint16_t port);

int memcmp(const void *a, const void *b, uint32_t n);

/* Tiny linear-congruential PRNG — returns 0..2^31-1 */
uint32_t rand32(void);

#endif /* UTIL_H */
